#include "PbrWithShadowPipelineV2.hpp"

#include "engine/BedrockAssert.hpp"
#include "engine/BedrockPath.hpp"
#include "engine/render_system/render_passes/display_render_pass/DisplayRenderPass.hpp"
#include "tools/Importer.hpp"
#include "engine/BedrockMatrix.hpp"
#include "engine/render_system/drawable_essence/DrawableEssence.hpp"
#include "engine/render_system/drawable_variant/DrawableVariant.hpp"
#include "engine/render_system/pipelines/DescriptorSetSchema.hpp"
#include "engine/render_system/RenderFrontend.hpp"
#include "engine/render_system/render_passes/depth_pre_pass/DepthPrePass.hpp"
#include "engine/render_system/render_passes/occlusion_render_pass/OcclusionRenderPass.hpp"
#include "engine/render_system/render_passes/point_light_shadow_render_pass/PointLightShadowRenderPass.hpp"
#include "engine/render_system/render_resources/point_light_shadow_resource_collection/PointLightShadowResourceCollection.hpp"
#include "engine/scene_manager/Scene.hpp"
#include "engine/scene_manager/SceneManager.hpp"

namespace MFA
{
    struct PBRWithShadowPipelineV2::PointLightShadowRenderTargets
    {

        static constexpr int Count = Scene::MAX_VISIBLE_POINT_LIGHT_COUNT;
        
        explicit PointLightShadowRenderTargets(PointLightShadowRenderPass & renderPass)
            : RenderPass(renderPass)
        {}

        void Init()
        {
            for (auto & renderTarget : RenderTargets)
            {
                renderTarget.Init(RenderPass.GetVkRenderPass());
            }
        }

        void Shutdown()
        {
            for (auto & renderTarget : RenderTargets)
            {
                renderTarget.Shutdown();
            }
        }

        PointLightShadowResourceCollection RenderTargets[Count] {};
        PointLightShadowRenderPass & RenderPass;

    };

    //-------------------------------------------------------------------------------------------------

    PBRWithShadowPipelineV2::PBRWithShadowPipelineV2()
        : BasePipeline(10000)
        , mPointLightShadowRenderPass(std::make_unique<PointLightShadowRenderPass>())
        , mDepthPrePass(std::make_unique<DepthPrePass>())
        , mOcclusionRenderPass(std::make_unique<OcclusionRenderPass>())
    {
        mPointLightShadowRenderTargets = std::make_unique<PointLightShadowRenderTargets>(*mPointLightShadowRenderPass);
    }

    //-------------------------------------------------------------------------------------------------

    PBRWithShadowPipelineV2::~PBRWithShadowPipelineV2()
    {
        if (mIsInitialized)
        {
            Shutdown();
        }
    }

    //-------------------------------------------------------------------------------------------------

    void PBRWithShadowPipelineV2::Init(
        RT::SamplerGroup * samplerGroup,
        RT::GpuTexture * errorTexture,
        float const projectionNear,
        float const projectionFar
    )
    {
        if (mIsInitialized == true)
        {
            MFA_ASSERT(false);
            return;
        }
        mIsInitialized = true;

        BasePipeline::Init();

        MFA_ASSERT(samplerGroup != nullptr);
        mSamplerGroup = samplerGroup;
        MFA_ASSERT(errorTexture != nullptr);
        mErrorTexture = errorTexture;

        mProjectionFar = projectionFar;
        mProjectionNear = projectionNear;
        mProjectionFarToNearDistance = projectionFar - projectionNear;
        MFA_ASSERT(mProjectionFarToNearDistance > 0.0f);

        createUniformBuffers();

        mPointLightShadowRenderPass->Init();
        mPointLightShadowRenderTargets->Init();
        
        mDepthPrePass->Init();
        mOcclusionRenderPass->Init();

        createPerFrameDescriptorSetLayout();
        createPerEssenceDescriptorSetLayout();
        createPerVariantDescriptorSetLayout();

        mDescriptorSetLayouts = { mPerFrameDescriptorSetLayout, mPerEssenceDescriptorSetLayout, mPerVariantDescriptorSetLayout };

        createDisplayPassPipeline();
        createShadowPassPipeline();
        createDepthPassPipeline();
        createOcclusionQueryPipeline();

        createFrameDescriptorSets();

        createOcclusionQueryPool();
    }

    //-------------------------------------------------------------------------------------------------

    void PBRWithShadowPipelineV2::Shutdown()
    {
        if (mIsInitialized == false)
        {
            MFA_ASSERT(false);
            return;
        }
        mIsInitialized = false;

        destroyOcclusionQueryPool();

        mSamplerGroup = nullptr;
        mErrorTexture = nullptr;

        mOcclusionRenderPass->Shutdown();
        mDepthPrePass->Shutdown();
        mPointLightShadowRenderTargets->Shutdown();
        mPointLightShadowRenderPass->Shutdown();
        
        destroyPipeline();
        destroyDescriptorSetLayout();
        destroyUniformBuffers();

        BasePipeline::Shutdown();

    }

    //-------------------------------------------------------------------------------------------------
    // TODO We need storage buffer per vertex to store data // Use instance id inside shader instead of creating unique id for vertices
    void PBRWithShadowPipelineV2::PreRender(RT::CommandRecordState & recordState, float const deltaTime)
    {
        // TODO I should render bounding volume for objects and geometry for occluders.
        // Some objects might need more than 1 occluder.
        retrieveOcclusionQueryResult(recordState);

        BasePipeline::PreRender(recordState, deltaTime);

        performDepthPrePass(recordState);

        performOcclusionQueryPass(recordState);

        performPointLightShadowPass(recordState);
    }

    //-------------------------------------------------------------------------------------------------

    void PBRWithShadowPipelineV2::Render(RT::CommandRecordState & drawPass, float deltaTime)
    {
        performDisplayPass(drawPass);
    }

    //-------------------------------------------------------------------------------------------------

    void PBRWithShadowPipelineV2::PostRender(RT::CommandRecordState & drawPass, float deltaTime)
    {
        BasePipeline::PostRender(drawPass, deltaTime);
    }

    //-------------------------------------------------------------------------------------------------

    void PBRWithShadowPipelineV2::OnResize()
    {
        mDepthPrePass->OnResize();
        mOcclusionRenderPass->OnResize();
    }

    //-------------------------------------------------------------------------------------------------

    std::weak_ptr<DrawableVariant> PBRWithShadowPipelineV2::CreateDrawableVariant(char const * essenceName)
    {
        auto const variant = BasePipeline::CreateDrawableVariant(essenceName).lock();
        MFA_ASSERT(variant != nullptr);

        createVariantDescriptorSets(variant.get());

        return variant;
    }

    //-------------------------------------------------------------------------------------------------

    void PBRWithShadowPipelineV2::internalCreateDrawableEssence(DrawableEssence & essence)
    {
        createEssenceDescriptorSets(essence);
    }

    //-------------------------------------------------------------------------------------------------

    void PBRWithShadowPipelineV2::createFrameDescriptorSets()
    {
        mPerFrameDescriptorSetGroup = RF::CreateDescriptorSets(
            mDescriptorPool,
            RF::GetMaxFramesPerFlight(),
            mPerFrameDescriptorSetLayout
        );

        auto const activeScene = SceneManager::GetActiveScene().lock();
        MFA_ASSERT(activeScene != nullptr);

        auto const & cameraBufferCollection = activeScene->GetCameraBufferCollection();
        auto const & lightBufferCollection = activeScene->GetPointLightsBufferCollection();
        
        for (uint32_t frameIndex = 0; frameIndex < RF::GetMaxFramesPerFlight(); ++frameIndex)
        {

            auto const descriptorSet = mPerFrameDescriptorSetGroup.descriptorSets[frameIndex];
            MFA_VK_VALID_ASSERT(descriptorSet);

            DescriptorSetSchema descriptorSetSchema{ descriptorSet };

            /////////////////////////////////////////////////////////////////
            // Vertex shader
            /////////////////////////////////////////////////////////////////

            // DisplayViewProjection
            VkDescriptorBufferInfo viewProjectionBufferInfo{
                .buffer = cameraBufferCollection.buffers[frameIndex].buffer,
                .offset = 0,
                .range = cameraBufferCollection.bufferSize,
            };
            descriptorSetSchema.AddUniformBuffer(viewProjectionBufferInfo);

            /////////////////////////////////////////////////////////////////
            // Fragment shader
            /////////////////////////////////////////////////////////////////

            // LightBuffer
            VkDescriptorBufferInfo lightViewBufferInfo{
                .buffer = lightBufferCollection.buffers[frameIndex].buffer,
                .offset = 0,
                .range = lightBufferCollection.bufferSize,
            };
            descriptorSetSchema.AddUniformBuffer(lightViewBufferInfo);

            // Sampler
            VkDescriptorImageInfo texturesSamplerInfo{
                .sampler = mSamplerGroup->sampler,
                .imageView = nullptr,
                .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            };
            descriptorSetSchema.AddSampler(texturesSamplerInfo);

            // PointLightShadowMap
            std::vector<VkDescriptorImageInfo> pointLightShadowImageInfos{};
            for (auto & renderTarget : mPointLightShadowRenderTargets->RenderTargets)
            {
                pointLightShadowImageInfos.emplace_back(VkDescriptorImageInfo{
                    .sampler = nullptr,
                    .imageView = renderTarget.GetShadowCubeMap(frameIndex).imageView,
                    .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
                });
            }
            descriptorSetSchema.AddImage(
                pointLightShadowImageInfos.data(),
                static_cast<uint32_t>(pointLightShadowImageInfos.size())
            );
            
            descriptorSetSchema.UpdateDescriptorSets();
        }
    }

    //-------------------------------------------------------------------------------------------------

    void PBRWithShadowPipelineV2::createEssenceDescriptorSets(DrawableEssence & essence) const
    {
        auto const & textures = essence.GetGpuModel().textures;

        auto const descriptorSetGroup = essence.CreateDescriptorSetGroup(
            mDescriptorPool,
            RF::GetMaxFramesPerFlight(),
            mPerEssenceDescriptorSetLayout
        );

        auto & primitiveBuffer = essence.GetPrimitivesBuffer();

        for (uint32_t frameIndex = 0; frameIndex < RF::GetMaxFramesPerFlight(); ++frameIndex)
        {

            auto const descriptorSet = descriptorSetGroup.descriptorSets[frameIndex];
            MFA_VK_VALID_ASSERT(descriptorSet);

            DescriptorSetSchema descriptorSetSchema{ descriptorSet };

            /////////////////////////////////////////////////////////////////
            // Fragment shader
            /////////////////////////////////////////////////////////////////

            // Primitives
            VkDescriptorBufferInfo primitiveBufferInfo{
                .buffer = primitiveBuffer.buffers[0].buffer,
                .offset = 0,
                .range = primitiveBuffer.bufferSize,
            };
            descriptorSetSchema.AddUniformBuffer(primitiveBufferInfo);

            // TODO Each one need their own sampler
            // Textures
            MFA_ASSERT(textures.size() <= 64);
            // We need to keep imageInfos alive
            std::vector<VkDescriptorImageInfo> imageInfos{};
            for (auto const & texture : textures)
            {
                imageInfos.emplace_back(VkDescriptorImageInfo{
                    .sampler = nullptr,
                    .imageView = texture.imageView(),
                    .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
                });
            }
            for (auto i = static_cast<uint32_t>(textures.size()); i < 64; ++i)
            {
                imageInfos.emplace_back(VkDescriptorImageInfo{
                    .sampler = nullptr,
                    .imageView = mErrorTexture->imageView(),
                    .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
                });
            }
            MFA_ASSERT(imageInfos.size() == 64);
            descriptorSetSchema.AddImage(
                imageInfos.data(),
                static_cast<uint32_t>(imageInfos.size())
            );

            descriptorSetSchema.UpdateDescriptorSets();
        }
    }

    //-------------------------------------------------------------------------------------------------

    void PBRWithShadowPipelineV2::createVariantDescriptorSets(DrawableVariant * variant)
    {
        MFA_ASSERT(variant != nullptr);

        auto const descriptorSetGroup = variant->CreateDescriptorSetGroup(
            mDescriptorPool,
            RF::GetMaxFramesPerFlight(),
            mPerVariantDescriptorSetLayout
        );
        auto const & actualSkinJointsBuffer = variant->GetSkinJointsBuffer();

        for (uint32_t frameIndex = 0; frameIndex < RF::GetMaxFramesPerFlight(); ++frameIndex)
        {

            auto const descriptorSet = descriptorSetGroup.descriptorSets[frameIndex];
            MFA_VK_VALID_ASSERT(descriptorSet);

            DescriptorSetSchema descriptorSetSchema{ descriptorSet };

            /////////////////////////////////////////////////////////////////
            // Vertex shader
            /////////////////////////////////////////////////////////////////

            // SkinJoints
            VkBuffer skinJointsBuffer = mErrorBuffer.buffers[0].buffer;
            size_t skinJointsBufferSize = mErrorBuffer.bufferSize;
            if (actualSkinJointsBuffer.bufferSize > 0)
            {
                skinJointsBuffer = actualSkinJointsBuffer.buffers[frameIndex].buffer;
                skinJointsBufferSize = actualSkinJointsBuffer.bufferSize;
            }
            VkDescriptorBufferInfo skinTransformBufferInfo{
                .buffer = skinJointsBuffer,
                .offset = 0,
                .range = skinJointsBufferSize,
            };
            descriptorSetSchema.AddUniformBuffer(skinTransformBufferInfo);

            descriptorSetSchema.UpdateDescriptorSets();
        }

    }
    
    //-------------------------------------------------------------------------------------------------

    void PBRWithShadowPipelineV2::retrieveOcclusionQueryResult(RT::CommandRecordState const & recordState)
    {
        // Retrieving previous frame results
        // I was thinking to retrieve previous frame result but it is impossible
        auto & occlusionQueryData = mOcclusionQueryDataList[recordState.frameIndex];
        if (occlusionQueryData.Variants.empty() == false)
        {// Trying to get previous frame results

            occlusionQueryData.Results.resize(occlusionQueryData.Variants.size());

            RF::GetQueryPoolResult(
                occlusionQueryData.Pool,
                static_cast<uint32_t>(occlusionQueryData.Results.size()),
                occlusionQueryData.Results.data()
            );

            uint32_t occlusionCount = 0;
            for (uint32_t i = 0; i < static_cast<uint32_t>(occlusionQueryData.Results.size()); ++i)
            {
                occlusionQueryData.Variants[i]->SetIsOccluded(occlusionQueryData.Results[i] == 0);
                if (occlusionQueryData.Results[i] == 0)
                {
                    occlusionCount++;
                }
            }
            //MFA_LOG_INFO("Occluded objects: %d", static_cast<int>(occlusionCount));

            occlusionQueryData.Variants.clear();
        }
    }

    //-------------------------------------------------------------------------------------------------

    void PBRWithShadowPipelineV2::performDepthPrePass(RT::CommandRecordState & recordState)
    {
        DepthPrePassVertexStagePushConstants pushConstants{};

        RF::BindDrawPipeline(recordState, mDepthPassPipeline);
        RF::BindDescriptorSet(
            recordState,
            RenderFrontend::DescriptorSetType::PerFrame,
            mPerFrameDescriptorSetGroup
        );

        mDepthPrePass->BeginRenderPass(recordState);
        for (auto const & essenceAndVariantList : mEssenceAndVariantsMap)
        {
            auto & essence = essenceAndVariantList.second->essence;
            auto & variantsList = essenceAndVariantList.second->variants;

            RF::BindVertexBuffer(recordState, essence.GetGpuModel().meshBuffers.verticesBuffer);
            RF::BindIndexBuffer(recordState, essence.GetGpuModel().meshBuffers.indicesBuffer);

            RF::BindDescriptorSet(
                recordState,
                RenderFrontend::DescriptorSetType::PerEssence,
                essence.GetDescriptorSetGroup()
            );

            for (auto const & variant : variantsList)
            {
                if (variant->IsActive() && variant->IsInFrustum() && variant->IsOccluded() == false)
                {

                    RF::BindDescriptorSet(
                        recordState,
                        RenderFrontend::DescriptorSetType::PerVariant,
                        variant->GetDescriptorSetGroup()
                    );

                    variant->Draw(recordState, [&recordState, &pushConstants](AS::MeshPrimitive const & primitive, DrawableVariant::Node const & node)-> void
                        {
                            // Vertex push constants
                            pushConstants.skinIndex = node.skin != nullptr ? node.skin->skinStartingIndex : -1;
                            Matrix::CopyGlmToCells(node.cachedModelTransform, pushConstants.modelTransform);
                            Matrix::CopyGlmToCells(node.cachedGlobalInverseTransform, pushConstants.inverseNodeTransform);

                            RF::PushConstants(
                                recordState,
                                AS::ShaderStage::Vertex,
                                0,
                                CBlobAliasOf(pushConstants)
                            );
                        }
                    );

                }
            }
        }
        mDepthPrePass->EndRenderPass(recordState);
    }

    //-------------------------------------------------------------------------------------------------

    void PBRWithShadowPipelineV2::performPointLightShadowPass(RT::CommandRecordState & recordState)
    {
        RF::BindDrawPipeline(recordState, mPointLightShadowPipeline);
        RF::BindDescriptorSet(
            recordState,
            RenderFrontend::DescriptorSetType::PerFrame,
            mPerFrameDescriptorSetGroup
        );

        // Vertex push constants
        PointLightShadowPassPushConstants pushConstants {};

        auto const activeScene = SceneManager::GetActiveScene().lock();          // Each pipeline must be bound to a scene
        if (activeScene == nullptr)
        {
            return;
        }

        //{// Shading barrier
        //    std::vector<VkImageMemoryBarrier> barrier {};
        //    for (uint32_t lightIndex = 0; lightIndex < activeScene->GetPointLightCount(); ++lightIndex)
        //    {
        //        mPointLightShadowRenderPass->PrepareRenderTargetForShading(
        //            recordState,
        //            &mPointLightShadowRenderTargets->RenderTargets[lightIndex],
        //            barrier
        //        );
        //    }
        //    RF::PipelineBarrier(
        //        RF::GetGraphicCommandBuffer(recordState),
        //        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
        //        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
        //        static_cast<uint32_t>(barrier.size()),
        //        barrier.data()
        //    );
        //}
        // TODO We can do everything in 1 pass no matter the light count by having layers on frame buffer
        // Geometry shader has so many advantages for shadow like preventing skinning for multiple times
        for (uint32_t lightIndex = 0; lightIndex < activeScene->GetPointLightCount(); ++lightIndex)
        {
            pushConstants.lightIndex = lightIndex;

            auto & renderTarget = mPointLightShadowRenderTargets->RenderTargets[lightIndex];

            mPointLightShadowRenderPass->BeginRenderPass(recordState, renderTarget);
            for (auto const & essenceAndVariantList : mEssenceAndVariantsMap)
            {
                auto & essence = essenceAndVariantList.second->essence;
                auto & variantsList = essenceAndVariantList.second->variants;
                RF::BindVertexBuffer(recordState, essence.GetGpuModel().meshBuffers.verticesBuffer);
                RF::BindIndexBuffer(recordState, essence.GetGpuModel().meshBuffers.indicesBuffer);

                RF::BindDescriptorSet(
                    recordState,
                    RenderFrontend::DescriptorSetType::PerEssence,
                    essence.GetDescriptorSetGroup()
                );

                for (auto const & variant : variantsList)
                {
                    // TODO We can have a isVisible class
                    if (variant->IsActive() && variant->IsInFrustum() && variant->IsOccluded() == false)
                    {
                        RF::BindDescriptorSet(
                            recordState,
                            RenderFrontend::DescriptorSetType::PerVariant,
                            variant->GetDescriptorSetGroup()
                        );

                        variant->Draw(recordState, [&recordState, &pushConstants](AS::MeshPrimitive const & primitive, DrawableVariant::Node const & node)-> void
                        {
                            // Vertex push constants
                            pushConstants.skinIndex = node.skin != nullptr ? node.skin->skinStartingIndex : -1;
                            Matrix::CopyGlmToCells(node.cachedModelTransform, pushConstants.modelTransform);
                            Matrix::CopyGlmToCells(node.cachedGlobalInverseTransform, pushConstants.inverseNodeTransform);

                            RF::PushConstants(
                                recordState,
                                VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_GEOMETRY_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                                0,
                                CBlobAliasOf(pushConstants)
                            );
                        });
                    }
                }
            }
            mPointLightShadowRenderPass->EndRenderPass(recordState);
        }
        //{// Copy barriers
        //    std::vector<VkImageMemoryBarrier> barrier {};
        //    for (uint32_t lightIndex = 0; lightIndex < activeScene->GetPointLightCount(); ++lightIndex)
        //    {
        //        mPointLightShadowRenderPass->PrepareRenderTargetForImageTransfer(
        //            recordState,
        //            &mPointLightShadowRenderTargets->RenderTargets[lightIndex],
        //            barrier
        //        );
        //    }
        //    RF::PipelineBarrier(
        //        RF::GetGraphicCommandBuffer(recordState),
        //        VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT,
        //        VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT,
        //        static_cast<uint32_t>(barrier.size()),
        //        barrier.data()
        //    );
        //}
        //{// Copying into cubemap faces (Will be replaced with geometry shader
        //    for (uint32_t lightIndex = 0; lightIndex < activeScene->GetPointLightCount(); ++lightIndex)
        //    {
        //        mPointLightShadowRenderPass->CopyColorAttachmentIntoCubeMap(
        //            recordState,
        //            &mPointLightShadowRenderTargets->RenderTargets[lightIndex]
        //        );
        //    }
        //}
        {// Sampling barriers
            std::vector<VkImageMemoryBarrier> barrier {};
            for (uint32_t lightIndex = 0; lightIndex < activeScene->GetPointLightCount(); ++lightIndex)
            {
                mPointLightShadowRenderPass->PrepareUsedRenderTargetForSampling(
                    recordState,
                    &mPointLightShadowRenderTargets->RenderTargets[lightIndex],
                    barrier
                );
            }
            for (uint32_t lightIndex = activeScene->GetPointLightCount(); lightIndex < Scene::MAX_VISIBLE_POINT_LIGHT_COUNT; ++lightIndex)
            {
                mPointLightShadowRenderPass->PrepareUnUsedRenderTargetForSampling(
                    recordState,
                    &mPointLightShadowRenderTargets->RenderTargets[lightIndex],
                    barrier
                );
            }
            RF::PipelineBarrier(
                RF::GetGraphicCommandBuffer(recordState),
                VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                static_cast<uint32_t>(barrier.size()),
                barrier.data()
            );
        }
    }

    //-------------------------------------------------------------------------------------------------

    void PBRWithShadowPipelineV2::performOcclusionQueryPass(RT::CommandRecordState & recordState)
    {
        auto & occlusionQueryData = mOcclusionQueryDataList[recordState.frameIndex];

        RF::ResetQueryPool(
            recordState,
            occlusionQueryData.Pool,
            10000
        );

        RF::BindDrawPipeline(recordState, mOcclusionQueryPipeline);
        RF::BindDescriptorSet(
            recordState,
            RenderFrontend::DescriptorSetType::PerFrame,
            mPerFrameDescriptorSetGroup
        );

        mOcclusionRenderPass->BeginRenderPass(recordState);

        OcclusionPassVertexStagePushConstants pushConstants{};

        for (auto const & essenceAndVariantList : mEssenceAndVariantsMap)
        {
            auto & essence = essenceAndVariantList.second->essence;
            auto & variantsList = essenceAndVariantList.second->variants;

            RF::BindVertexBuffer(recordState, essence.GetGpuModel().meshBuffers.verticesBuffer);
            RF::BindIndexBuffer(recordState, essence.GetGpuModel().meshBuffers.indicesBuffer);

            RF::BindDescriptorSet(
                recordState,
                RenderFrontend::DescriptorSetType::PerEssence,
                essence.GetDescriptorSetGroup()
            );

            for (auto const & variant : variantsList)
            {
                if (variant->IsActive() && variant->IsInFrustum())
                {

                    RF::BindDescriptorSet(
                        recordState,
                        RenderFrontend::DescriptorSetType::PerVariant,
                        variant->GetDescriptorSetGroup()
                    );

                    RF::BeginQuery(recordState, occlusionQueryData.Pool, static_cast<uint32_t>(occlusionQueryData.Variants.size()));

                    variant->Draw(recordState, [&recordState, &pushConstants](AS::MeshPrimitive const & primitive, DrawableVariant::Node const & node)-> void
                        {
                            // Vertex push constants
                            Matrix::CopyGlmToCells(node.cachedModelTransform, pushConstants.modelTransform);
                            Matrix::CopyGlmToCells(node.cachedGlobalInverseTransform, pushConstants.inverseNodeTransform);
                            pushConstants.skinIndex = node.skin != nullptr ? node.skin->skinStartingIndex : -1;

                            RF::PushConstants(
                                recordState,
                                AS::ShaderStage::Vertex,
                                0,
                                CBlobAliasOf(pushConstants)
                            );
                        }
                    );

                    RF::EndQuery(recordState, occlusionQueryData.Pool, static_cast<uint32_t>(occlusionQueryData.Variants.size()));

                    occlusionQueryData.Variants.emplace_back(variant.get());
                }
            }
        }

        mOcclusionRenderPass->EndRenderPass(recordState);
    }

    //-------------------------------------------------------------------------------------------------

    void PBRWithShadowPipelineV2::performDisplayPass(RT::CommandRecordState & recordState)
    {
        RF::BindDrawPipeline(recordState, mDisplayPassPipeline);
        RF::BindDescriptorSet(
            recordState,
            RenderFrontend::DescriptorSetType::PerFrame,
            mPerFrameDescriptorSetGroup
        );

        DisplayPassAllStagesPushConstants pushConstants{};

        for (auto const & essenceAndVariantList : mEssenceAndVariantsMap)
        {
            auto & essence = essenceAndVariantList.second->essence;
            auto & variantsList = essenceAndVariantList.second->variants;

            RF::BindVertexBuffer(recordState, essence.GetGpuModel().meshBuffers.verticesBuffer);
            RF::BindIndexBuffer(recordState, essence.GetGpuModel().meshBuffers.indicesBuffer);

            RF::BindDescriptorSet(
                recordState,
                RenderFrontend::DescriptorSetType::PerEssence,
                essence.GetDescriptorSetGroup()
            );


            for (auto const & variant : variantsList)
            {
                if (variant->IsActive() && variant->IsInFrustum() && variant->IsOccluded() == false)
                {

                    RF::BindDescriptorSet(
                        recordState,
                        RenderFrontend::DescriptorSetType::PerVariant,
                        variant->GetDescriptorSetGroup()
                    );

                    variant->Draw(recordState, [&recordState, &pushConstants](AS::MeshPrimitive const & primitive, DrawableVariant::Node const & node)-> void
                        {
                            // Push constants
                            pushConstants.skinIndex = node.skin != nullptr ? node.skin->skinStartingIndex : -1;
                            pushConstants.primitiveIndex = primitive.uniqueId;
                            Matrix::CopyGlmToCells(node.cachedModelTransform, pushConstants.modelTransform);
                            Matrix::CopyGlmToCells(node.cachedGlobalInverseTransform, pushConstants.inverseNodeTransform);
                            RF::PushConstants(
                                recordState,
                                VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                                0,
                                CBlobAliasOf(pushConstants)
                            );
                        }
                    );
                }
            }
        }
    }

    //-------------------------------------------------------------------------------------------------
    void PBRWithShadowPipelineV2::createPerFrameDescriptorSetLayout()
    {
        std::vector<VkDescriptorSetLayoutBinding> bindings{};

        /////////////////////////////////////////////////////////////////
        // Vertex shader
        /////////////////////////////////////////////////////////////////

        // CameraBuffer
        VkDescriptorSetLayoutBinding cameraBufferLayoutBinding{
            .binding = static_cast<uint32_t>(bindings.size()),
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
        };
        bindings.emplace_back(cameraBufferLayoutBinding);

        /////////////////////////////////////////////////////////////////
        // Fragment shader
        /////////////////////////////////////////////////////////////////

        // LightBuffer
        VkDescriptorSetLayoutBinding lightViewLayoutBinding{
            .binding = static_cast<uint32_t>(bindings.size()),
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_GEOMETRY_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
        };
        bindings.emplace_back(lightViewLayoutBinding);

        // Sampler
        VkDescriptorSetLayoutBinding samplerLayoutBinding{
            .binding = static_cast<uint32_t>(bindings.size()),
            .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
        };
        bindings.emplace_back(samplerLayoutBinding);
        
        // PointLightShadowMap
        VkDescriptorSetLayoutBinding pointLightShadowLayoutBinding{
            .binding = static_cast<uint32_t>(bindings.size()),
            .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
            .descriptorCount = Scene::MAX_VISIBLE_POINT_LIGHT_COUNT,
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
        };
        bindings.emplace_back(pointLightShadowLayoutBinding);

        mPerFrameDescriptorSetLayout = RF::CreateDescriptorSetLayout(
            static_cast<uint8_t>(bindings.size()),
            bindings.data()
        );
    }

    //-------------------------------------------------------------------------------------------------

    void PBRWithShadowPipelineV2::createPerEssenceDescriptorSetLayout()
    {
        std::vector<VkDescriptorSetLayoutBinding> bindings{};

        /////////////////////////////////////////////////////////////////
        // Fragment shader
        /////////////////////////////////////////////////////////////////
        {// Primitive
            VkDescriptorSetLayoutBinding layoutBinding{
                .binding = static_cast<uint32_t>(bindings.size()),
                .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .descriptorCount = 1,
                .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
            };
            bindings.emplace_back(layoutBinding);
        }
        {// Textures
            VkDescriptorSetLayoutBinding layoutBinding{
                .binding = static_cast<uint32_t>(bindings.size()),
                .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
                .descriptorCount = 64,
                .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
            };
            bindings.emplace_back(layoutBinding);
        }

        mPerEssenceDescriptorSetLayout = RF::CreateDescriptorSetLayout(
            static_cast<uint8_t>(bindings.size()),
            bindings.data()
        );
    }

    //-------------------------------------------------------------------------------------------------

    void PBRWithShadowPipelineV2::createPerVariantDescriptorSetLayout()
    {
        std::vector<VkDescriptorSetLayoutBinding> bindings{};

        /////////////////////////////////////////////////////////////////
        // Vertex shader
        /////////////////////////////////////////////////////////////////

        {// SkinJoints
            VkDescriptorSetLayoutBinding layoutBinding{};
            layoutBinding.binding = static_cast<uint32_t>(bindings.size());
            layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            layoutBinding.descriptorCount = 1;
            layoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
            bindings.emplace_back(layoutBinding);
        }

        mPerVariantDescriptorSetLayout = RF::CreateDescriptorSetLayout(
            static_cast<uint8_t>(bindings.size()),
            bindings.data()
        );
    }

    //-------------------------------------------------------------------------------------------------

    void PBRWithShadowPipelineV2::destroyDescriptorSetLayout() const
    {
        MFA_VK_VALID_ASSERT(mPerFrameDescriptorSetLayout);
        RF::DestroyDescriptorSetLayout(mPerFrameDescriptorSetLayout);

        MFA_VK_VALID_ASSERT(mPerEssenceDescriptorSetLayout);
        RF::DestroyDescriptorSetLayout(mPerEssenceDescriptorSetLayout);

        MFA_VK_VALID_ASSERT(mPerVariantDescriptorSetLayout);
        RF::DestroyDescriptorSetLayout(mPerVariantDescriptorSetLayout);
    }

    //-------------------------------------------------------------------------------------------------

    void PBRWithShadowPipelineV2::createDisplayPassPipeline()
    {
        // Vertex shader
        auto cpuVertexShader = Importer::ImportShaderFromSPV(
            Path::Asset("shaders/pbr_with_shadow_v2/PbrWithShadow.vert.spv").c_str(),
            AssetSystem::Shader::Stage::Vertex,
            "main"
        );
        MFA_ASSERT(cpuVertexShader.isValid());
        auto gpuVertexShader = RF::CreateShader(cpuVertexShader);
        MFA_ASSERT(gpuVertexShader.valid());
        MFA_DEFER{
            RF::DestroyShader(gpuVertexShader);
            Importer::FreeShader(cpuVertexShader);
        };

        // Fragment shader
        auto cpuFragmentShader = Importer::ImportShaderFromSPV(
            Path::Asset("shaders/pbr_with_shadow_v2/PbrWithShadow.frag.spv").c_str(),
            AssetSystem::Shader::Stage::Fragment,
            "main"
        );
        auto gpuFragmentShader = RF::CreateShader(cpuFragmentShader);
        MFA_ASSERT(cpuFragmentShader.isValid());
        MFA_ASSERT(gpuFragmentShader.valid());
        MFA_DEFER{
            RF::DestroyShader(gpuFragmentShader);
            Importer::FreeShader(cpuFragmentShader);
        };

        std::vector<RT::GpuShader const *> shaders{ &gpuVertexShader, &gpuFragmentShader };

        VkVertexInputBindingDescription vertexInputBindingDescription{};
        vertexInputBindingDescription.binding = 0;
        vertexInputBindingDescription.stride = sizeof(AS::MeshVertex);
        vertexInputBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        std::vector<VkVertexInputAttributeDescription> inputAttributeDescriptions{};
        
        // Position
        inputAttributeDescriptions.emplace_back(VkVertexInputAttributeDescription {
            .location = static_cast<uint32_t>(inputAttributeDescriptions.size()),
            .binding = 0,
            .format = VK_FORMAT_R32G32B32_SFLOAT,
            .offset = offsetof(AS::MeshVertex, position)
        });

        // BaseColorUV
        inputAttributeDescriptions.emplace_back(VkVertexInputAttributeDescription {
            .location = static_cast<uint32_t>(inputAttributeDescriptions.size()),
            .binding = 0,
            .format = VK_FORMAT_R32G32_SFLOAT,
            .offset = offsetof(AS::MeshVertex, baseColorUV)
        });

        {// Metallic/Roughness
            VkVertexInputAttributeDescription attributeDescription{};
            attributeDescription.location = static_cast<uint32_t>(inputAttributeDescriptions.size());
            attributeDescription.binding = 0;
            attributeDescription.format = VK_FORMAT_R32G32_SFLOAT;
            attributeDescription.offset = offsetof(AS::MeshVertex, metallicUV); // Metallic and roughness have same uv for gltf files  

            inputAttributeDescriptions.emplace_back(attributeDescription);
        }
        {// NormalMapUV
            VkVertexInputAttributeDescription attributeDescription{};
            attributeDescription.location = static_cast<uint32_t>(inputAttributeDescriptions.size());
            attributeDescription.binding = 0;
            attributeDescription.format = VK_FORMAT_R32G32_SFLOAT;
            attributeDescription.offset = offsetof(AS::MeshVertex, normalMapUV);

            inputAttributeDescriptions.emplace_back(attributeDescription);
        }
        {// Tangent
            VkVertexInputAttributeDescription attributeDescription{};
            attributeDescription.location = static_cast<uint32_t>(inputAttributeDescriptions.size());
            attributeDescription.binding = 0;
            attributeDescription.format = VK_FORMAT_R32G32B32A32_SFLOAT;
            attributeDescription.offset = offsetof(AS::MeshVertex, tangentValue);

            inputAttributeDescriptions.emplace_back(attributeDescription);
        }
        {// Normal
            VkVertexInputAttributeDescription attributeDescription{};
            attributeDescription.location = static_cast<uint32_t>(inputAttributeDescriptions.size());
            attributeDescription.binding = 0;
            attributeDescription.format = VK_FORMAT_R32G32B32_SFLOAT;
            attributeDescription.offset = offsetof(AS::MeshVertex, normalValue);

            inputAttributeDescriptions.emplace_back(attributeDescription);
        }
        {// EmissionUV
            VkVertexInputAttributeDescription attributeDescription{};
            attributeDescription.location = static_cast<uint32_t>(inputAttributeDescriptions.size());
            attributeDescription.binding = 0;
            attributeDescription.format = VK_FORMAT_R32G32_SFLOAT;
            attributeDescription.offset = offsetof(AS::MeshVertex, emissionUV);
            inputAttributeDescriptions.emplace_back(attributeDescription);
        }
        {// HasSkin
            VkVertexInputAttributeDescription attributeDescription{};
            attributeDescription.location = static_cast<uint32_t>(inputAttributeDescriptions.size());
            attributeDescription.binding = 0;
            attributeDescription.format = VK_FORMAT_R32_SINT;
            attributeDescription.offset = offsetof(AS::MeshVertex, hasSkin); // TODO We should use a primitiveInfo instead
            inputAttributeDescriptions.emplace_back(attributeDescription);
        }
        {// JointIndices
            VkVertexInputAttributeDescription attributeDescription{};
            attributeDescription.location = static_cast<uint32_t>(inputAttributeDescriptions.size());
            attributeDescription.binding = 0;
            attributeDescription.format = VK_FORMAT_R32G32B32A32_SINT;
            attributeDescription.offset = offsetof(AS::MeshVertex, jointIndices);
            inputAttributeDescriptions.emplace_back(attributeDescription);
        }
        {// JointWeights
            VkVertexInputAttributeDescription attributeDescription{};
            attributeDescription.location = static_cast<uint32_t>(inputAttributeDescriptions.size());
            attributeDescription.binding = 0;
            attributeDescription.format = VK_FORMAT_R32G32B32A32_SFLOAT;
            attributeDescription.offset = offsetof(AS::MeshVertex, jointWeights);
            inputAttributeDescriptions.emplace_back(attributeDescription);
        }
        MFA_ASSERT(mDisplayPassPipeline.isValid() == false);

        std::vector<VkPushConstantRange> mPushConstantRanges{};
        mPushConstantRanges.emplace_back(VkPushConstantRange{
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
            .offset = 0,
            .size = sizeof(DisplayPassAllStagesPushConstants),
        });

        RT::CreateGraphicPipelineOptions options{};
        options.rasterizationSamples = RF::GetMaxSamplesCount();
        options.pushConstantRanges = mPushConstantRanges.data();
        options.pushConstantsRangeCount = static_cast<uint8_t>(mPushConstantRanges.size());
        options.cullMode = VK_CULL_MODE_BACK_BIT;
        options.depthStencil.depthCompareOp = VK_COMPARE_OP_EQUAL;

        mDisplayPassPipeline = RF::CreatePipeline(
            RF::GetDisplayRenderPass()->GetVkRenderPass(),
            static_cast<uint8_t>(shaders.size()),
            shaders.data(),
            static_cast<uint32_t>(mDescriptorSetLayouts.size()),
            mDescriptorSetLayouts.data(),
            vertexInputBindingDescription,
            static_cast<uint8_t>(inputAttributeDescriptions.size()),
            inputAttributeDescriptions.data(),
            options
        );
    }

    //-------------------------------------------------------------------------------------------------

    void PBRWithShadowPipelineV2::createShadowPassPipeline()
    {
        // Vertex shader
        RF_CREATE_SHADER("shaders/point_light_shadow/PointLightShadow.vert.spv", Vertex)
        RF_CREATE_SHADER("shaders/point_light_shadow/PointLightShadow.geom.spv", Geometry)
        RF_CREATE_SHADER("shaders/point_light_shadow/PointLightShadow.frag.spv", Fragment)

        std::vector<RT::GpuShader const *> shaders{ &gpuVertexShader, &gpuGeometryShader, &gpuFragmentShader };

        VkVertexInputBindingDescription vertexInputBindingDescription{};
        vertexInputBindingDescription.binding = 0;
        vertexInputBindingDescription.stride = sizeof(AS::MeshVertex);
        vertexInputBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        std::vector<VkVertexInputAttributeDescription> inputAttributeDescriptions{};

        {// Position
            VkVertexInputAttributeDescription attributeDescription{};
            attributeDescription.location = static_cast<uint32_t>(inputAttributeDescriptions.size());
            attributeDescription.binding = 0;
            attributeDescription.format = VK_FORMAT_R32G32B32_SFLOAT;
            attributeDescription.offset = offsetof(AS::MeshVertex, position);

            inputAttributeDescriptions.emplace_back(attributeDescription);
        }
        {// HasSkin
            VkVertexInputAttributeDescription attributeDescription{};
            attributeDescription.location = static_cast<uint32_t>(inputAttributeDescriptions.size());
            attributeDescription.binding = 0;
            attributeDescription.format = VK_FORMAT_R32_SINT;
            attributeDescription.offset = offsetof(AS::MeshVertex, hasSkin); // TODO We should use a primitiveInfo instead
            inputAttributeDescriptions.emplace_back(attributeDescription);
        }
        {// JointIndices
            VkVertexInputAttributeDescription attributeDescription{};
            attributeDescription.location = static_cast<uint32_t>(inputAttributeDescriptions.size());
            attributeDescription.binding = 0;
            attributeDescription.format = VK_FORMAT_R32G32B32A32_SINT;
            attributeDescription.offset = offsetof(AS::MeshVertex, jointIndices);
            inputAttributeDescriptions.emplace_back(attributeDescription);
        }
        {// JointWeights
            VkVertexInputAttributeDescription attributeDescription{};
            attributeDescription.location = static_cast<uint32_t>(inputAttributeDescriptions.size());
            attributeDescription.binding = 0;
            attributeDescription.format = VK_FORMAT_R32G32B32A32_SFLOAT;
            attributeDescription.offset = offsetof(AS::MeshVertex, jointWeights);
            inputAttributeDescriptions.emplace_back(attributeDescription);
        }

        std::vector<VkPushConstantRange> pushConstantRanges{};
        VkPushConstantRange pushConstantRange{
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_GEOMETRY_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
            .offset = 0,
            .size = sizeof(PointLightShadowPassPushConstants),
        };
        pushConstantRanges.emplace_back(pushConstantRange);

        RT::CreateGraphicPipelineOptions graphicPipelineOptions{};
        graphicPipelineOptions.cullMode = VK_CULL_MODE_BACK_BIT;
        graphicPipelineOptions.pushConstantRanges = pushConstantRanges.data();
        // TODO Probably we need to make pushConstantsRangeCount uint32_t
        graphicPipelineOptions.pushConstantsRangeCount = static_cast<uint8_t>(pushConstantRanges.size());

        MFA_ASSERT(mPointLightShadowPipeline.isValid() == false);
        mPointLightShadowPipeline = RF::CreatePipeline(
            mPointLightShadowRenderPass->GetVkRenderPass(),
            static_cast<uint8_t>(shaders.size()),
            shaders.data(),
            static_cast<uint32_t>(mDescriptorSetLayouts.size()),
            mDescriptorSetLayouts.data(),
            vertexInputBindingDescription,
            static_cast<uint8_t>(inputAttributeDescriptions.size()),
            inputAttributeDescriptions.data(),
            graphicPipelineOptions
        );
    }

    //-------------------------------------------------------------------------------------------------

    void PBRWithShadowPipelineV2::createDepthPassPipeline()
    {
        // Vertex shader
        auto cpuVertexShader = Importer::ImportShaderFromSPV(
            Path::Asset("shaders/depth_pre_pass/DepthPrePass.vert.spv").c_str(),
            AssetSystem::Shader::Stage::Vertex,
            "main"
        );
        MFA_ASSERT(cpuVertexShader.isValid());
        auto gpuVertexShader = RF::CreateShader(cpuVertexShader);
        MFA_ASSERT(gpuVertexShader.valid());
        MFA_DEFER{
            RF::DestroyShader(gpuVertexShader);
            Importer::FreeShader(cpuVertexShader);
        };

        std::vector<RT::GpuShader const *> shaders{ &gpuVertexShader};

        VkVertexInputBindingDescription vertexInputBindingDescription{};
        vertexInputBindingDescription.binding = 0;
        vertexInputBindingDescription.stride = sizeof(AS::MeshVertex);
        vertexInputBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        std::vector<VkVertexInputAttributeDescription> inputAttributeDescriptions{};

        // Position
        inputAttributeDescriptions.emplace_back(VkVertexInputAttributeDescription{
            .location = static_cast<uint32_t>(inputAttributeDescriptions.size()),
            .binding = 0,
            .format = VK_FORMAT_R32G32B32_SFLOAT,
            .offset = offsetof(AS::MeshVertex, position),
        });

        // HasSkin
        inputAttributeDescriptions.emplace_back(VkVertexInputAttributeDescription{
            .location = static_cast<uint32_t>(inputAttributeDescriptions.size()),
            .binding = 0,
            .format = VK_FORMAT_R32_SINT,
            .offset = offsetof(AS::MeshVertex, hasSkin), // TODO We should use a primitiveInfo instead
        });

        // JointIndices
        inputAttributeDescriptions.emplace_back(VkVertexInputAttributeDescription{
            .location = static_cast<uint32_t>(inputAttributeDescriptions.size()),
            .binding = 0,
            .format = VK_FORMAT_R32G32B32A32_SINT,
            .offset = offsetof(AS::MeshVertex, jointIndices),
        });

        // JointWeights
        inputAttributeDescriptions.emplace_back(VkVertexInputAttributeDescription{
            .location = static_cast<uint32_t>(inputAttributeDescriptions.size()),
            .binding = 0,
            .format = VK_FORMAT_R32G32B32A32_SFLOAT,
            .offset = offsetof(AS::MeshVertex, jointWeights),
        });

        std::vector<VkPushConstantRange> pushConstantRanges{};
        // Push constants  
        VkPushConstantRange pushConstantRange{
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
            .offset = 0,
            .size = sizeof(DepthPrePassVertexStagePushConstants),
        };
        pushConstantRanges.emplace_back(pushConstantRange);

        RT::CreateGraphicPipelineOptions graphicPipelineOptions{};
        graphicPipelineOptions.cullMode = VK_CULL_MODE_BACK_BIT;
        graphicPipelineOptions.rasterizationSamples = RF::GetMaxSamplesCount();
        graphicPipelineOptions.pushConstantRanges = pushConstantRanges.data();
        // TODO Probably we need to make pushConstantsRangeCount uint32_t
        graphicPipelineOptions.pushConstantsRangeCount = static_cast<uint8_t>(pushConstantRanges.size());
        graphicPipelineOptions.depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;

        MFA_ASSERT(mDepthPassPipeline.isValid() == false);
        mDepthPassPipeline = RF::CreatePipeline(
            mDepthPrePass->GetVkRenderPass(),
            static_cast<uint8_t>(shaders.size()),
            shaders.data(),
            static_cast<uint32_t>(mDescriptorSetLayouts.size()),
            mDescriptorSetLayouts.data(),
            vertexInputBindingDescription,
            static_cast<uint8_t>(inputAttributeDescriptions.size()),
            inputAttributeDescriptions.data(),
            graphicPipelineOptions
        );
    }

    //-------------------------------------------------------------------------------------------------

    void PBRWithShadowPipelineV2::createOcclusionQueryPool()
    {
        mOcclusionQueryDataList.resize(RF::GetMaxFramesPerFlight());
        for (auto & occlusionQueryData : mOcclusionQueryDataList)
        {
            occlusionQueryData.Pool = RF::CreateQueryPool(VkQueryPoolCreateInfo{
                .sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO,
                .queryType = VK_QUERY_TYPE_OCCLUSION,
                .queryCount = 10000,
            });
        }
    }

    //-------------------------------------------------------------------------------------------------

    void PBRWithShadowPipelineV2::destroyOcclusionQueryPool()
    {
        // Cleaning occlusion query
        for (auto const & occlusionQueryData : mOcclusionQueryDataList)
        {
            RF::DestroyQueryPool(occlusionQueryData.Pool);
        }
        mOcclusionQueryDataList.clear();
    }

    //-------------------------------------------------------------------------------------------------

    void PBRWithShadowPipelineV2::createOcclusionQueryPipeline()
    {
        // Vertex shader
        auto cpuVertexShader = Importer::ImportShaderFromSPV(
            Path::Asset("shaders/occlusion_query/Occlusion.vert.spv").c_str(),
            AssetSystem::Shader::Stage::Vertex,
            "main"
        );
        MFA_ASSERT(cpuVertexShader.isValid());
        auto gpuVertexShader = RF::CreateShader(cpuVertexShader);
        MFA_ASSERT(gpuVertexShader.valid());
        MFA_DEFER{
            RF::DestroyShader(gpuVertexShader);
            Importer::FreeShader(cpuVertexShader);
        };

        std::vector<RT::GpuShader const *> shaders{ &gpuVertexShader };

        VkVertexInputBindingDescription vertexInputBindingDescription{};
        vertexInputBindingDescription.binding = 0;
        vertexInputBindingDescription.stride = sizeof(AS::MeshVertex);
        vertexInputBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        std::vector<VkVertexInputAttributeDescription> inputAttributeDescriptions{};

        // Position
        inputAttributeDescriptions.emplace_back(VkVertexInputAttributeDescription{
            .location = static_cast<uint32_t>(inputAttributeDescriptions.size()),
            .binding = 0,
            .format = VK_FORMAT_R32G32B32_SFLOAT,
            .offset = offsetof(AS::MeshVertex, position),
        });

        // HasSkin
        inputAttributeDescriptions.emplace_back(VkVertexInputAttributeDescription{
            .location = static_cast<uint32_t>(inputAttributeDescriptions.size()),
            .binding = 0,
            .format = VK_FORMAT_R32_SINT,
            .offset = offsetof(AS::MeshVertex, hasSkin), // TODO We should use a primitiveInfo instead
        });

        // JointIndices
        inputAttributeDescriptions.emplace_back(VkVertexInputAttributeDescription{
            .location = static_cast<uint32_t>(inputAttributeDescriptions.size()),
            .binding = 0,
            .format = VK_FORMAT_R32G32B32A32_SINT,
            .offset = offsetof(AS::MeshVertex, jointIndices),
        });

        // JointWeights
        inputAttributeDescriptions.emplace_back(VkVertexInputAttributeDescription{
            .location = static_cast<uint32_t>(inputAttributeDescriptions.size()),
            .binding = 0,
            .format = VK_FORMAT_R32G32B32A32_SFLOAT,
            .offset = offsetof(AS::MeshVertex, jointWeights),
        });

        std::vector<VkPushConstantRange> pushConstantRanges{};
        // Model data  
        VkPushConstantRange pushConstantRange{
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
            .offset = 0,
            .size = sizeof(OcclusionPassVertexStagePushConstants),
        };
        pushConstantRanges.emplace_back(pushConstantRange);

        RT::CreateGraphicPipelineOptions graphicPipelineOptions{};
        graphicPipelineOptions.cullMode = VK_CULL_MODE_BACK_BIT;
        graphicPipelineOptions.rasterizationSamples = RF::GetMaxSamplesCount();
        graphicPipelineOptions.pushConstantRanges = pushConstantRanges.data();
        // TODO Probably we need to make pushConstantsRangeCount uint32_t
        graphicPipelineOptions.pushConstantsRangeCount = static_cast<uint8_t>(pushConstantRanges.size());
        graphicPipelineOptions.depthStencil.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;

        MFA_ASSERT(mOcclusionQueryPipeline.isValid() == false);
        mOcclusionQueryPipeline = RF::CreatePipeline(
            mOcclusionRenderPass->GetVkRenderPass(),
            static_cast<uint8_t>(shaders.size()),
            shaders.data(),
            static_cast<uint32_t>(mDescriptorSetLayouts.size()),
            mDescriptorSetLayouts.data(),
            vertexInputBindingDescription,
            static_cast<uint8_t>(inputAttributeDescriptions.size()),
            inputAttributeDescriptions.data(),
            graphicPipelineOptions
        );
    }

    //-------------------------------------------------------------------------------------------------

    void PBRWithShadowPipelineV2::destroyPipeline()
    {
        MFA_ASSERT(mDisplayPassPipeline.isValid());
        RF::DestroyPipelineGroup(mDisplayPassPipeline);

        MFA_ASSERT(mPointLightShadowPipeline.isValid());
        RF::DestroyPipelineGroup(mPointLightShadowPipeline);

        MFA_ASSERT(mDepthPassPipeline.isValid());
        RF::DestroyPipelineGroup(mDepthPassPipeline);

        MFA_ASSERT(mOcclusionQueryPipeline.isValid());
        RF::DestroyPipelineGroup(mOcclusionQueryPipeline);
    }

    //-------------------------------------------------------------------------------------------------

    void PBRWithShadowPipelineV2::createUniformBuffers()
    {
        mErrorBuffer = RF::CreateUniformBuffer(sizeof(DrawableVariant::JointTransformData), 1);
    }

    //-------------------------------------------------------------------------------------------------

    void PBRWithShadowPipelineV2::destroyUniformBuffers()
    {
        RF::DestroyUniformBuffer(mErrorBuffer);
    }

    //-------------------------------------------------------------------------------------------------

}
