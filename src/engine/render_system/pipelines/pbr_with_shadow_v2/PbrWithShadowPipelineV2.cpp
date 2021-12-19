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
#include "engine/render_system/render_passes/directional_light_shadow_render_pass/DirectionalLightShadowRenderPass.hpp"
#include "engine/render_system/render_passes/occlusion_render_pass/OcclusionRenderPass.hpp"
#include "engine/render_system/render_passes/point_light_shadow_render_pass/PointLightShadowRenderPass.hpp"
#include "engine/render_system/render_resources/directional_light_shadow_resources/DirectionalLightShadowResources.hpp"
#include "engine/render_system/render_resources/point_light_shadow_resources/PointLightShadowResources.hpp"
#include "engine/scene_manager/Scene.hpp"
#include "engine/scene_manager/SceneManager.hpp"

namespace MFA
{

    //-------------------------------------------------------------------------------------------------

    PBRWithShadowPipelineV2::PBRWithShadowPipelineV2(Scene * attachedScene)
        : BasePipeline(10000)
        , mPointLightShadowRenderPass(std::make_unique<PointLightShadowRenderPass>())
        , mPointLightShadowResources(std::make_unique<PointLightShadowResources>())
        , mDirectionalLightShadowRenderPass(std::make_unique<DirectionalLightShadowRenderPass>())
        , mDirectionalLightShadowResources(std::make_unique<DirectionalLightShadowResources>())
        , mDepthPrePass(std::make_unique<DepthPrePass>())
        , mOcclusionRenderPass(std::make_unique<OcclusionRenderPass>())
        , mAttachedScene(attachedScene)
    {
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
        std::shared_ptr<RT::SamplerGroup> samplerGroup,
        std::shared_ptr<RT::GpuTexture> errorTexture
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
        mSamplerGroup = std::move(samplerGroup);
        MFA_ASSERT(errorTexture != nullptr);
        mErrorTexture = std::move(errorTexture);

        createUniformBuffers();

        mPointLightShadowRenderPass->Init();
        mPointLightShadowResources->Init(mPointLightShadowRenderPass->GetVkRenderPass());

        mDirectionalLightShadowRenderPass->Init();
        mDirectionalLightShadowResources->Init(mDirectionalLightShadowRenderPass->GetVkRenderPass());
        
        mDepthPrePass->Init();
        mOcclusionRenderPass->Init();

        createPerFrameDescriptorSetLayout();
        createPerEssenceDescriptorSetLayout();
        createPerVariantDescriptorSetLayout();

        mDescriptorSetLayouts = {
            mPerFrameDescriptorSetLayout,
            mPerEssenceDescriptorSetLayout,
            mPerVariantDescriptorSetLayout
        };

        createDisplayPassPipeline();
        createPointLightShadowPassPipeline();
        createDirectionalLightShadowPassPipeline();
        createDepthPassPipeline();
        createOcclusionQueryPipeline();

        createPerFrameDescriptorSets();

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

        mPointLightShadowResources->Shutdown();
        mPointLightShadowRenderPass->Shutdown();

        mDirectionalLightShadowResources->Shutdown();
        mDirectionalLightShadowRenderPass->Shutdown();
        
        destroyPipeline();
        destroyDescriptorSetLayout();
        
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

        performDirectionalLightShadowPass(recordState);

        performPointLightShadowPass(recordState);

        {// Sampling barriers
            std::vector<VkImageMemoryBarrier> barrier {};
            mPointLightShadowRenderPass->PrepareRenderTargetForSampling(
                recordState,
                mPointLightShadowResources.get(),
                mAttachedScene->GetPointLightCount() > 0,
                barrier
            );
            mDirectionalLightShadowRenderPass->PrepareRenderTargetForSampling(
                recordState,
                mDirectionalLightShadowResources.get(),
                mAttachedScene->GetDirectionalLightCount() > 0,
                barrier
            );
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

    void PBRWithShadowPipelineV2::Render(RT::CommandRecordState & recordState, float deltaTime)
    {
        performDisplayPass(recordState);
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

    DrawableVariant * PBRWithShadowPipelineV2::CreateDrawableVariant(RT::GpuModelId const id)
    {
        auto * variant = BasePipeline::CreateDrawableVariant(id);
        MFA_ASSERT(variant != nullptr);

        createVariantDescriptorSets(variant);

        return variant;
    }

    //-------------------------------------------------------------------------------------------------

    void PBRWithShadowPipelineV2::internalCreateDrawableEssence(DrawableEssence & essence)
    {
        createEssenceDescriptorSets(essence);
    }

    //-------------------------------------------------------------------------------------------------

    void PBRWithShadowPipelineV2::createPerFrameDescriptorSets()
    {
        mPerFrameDescriptorSetGroup = RF::CreateDescriptorSets(
            mDescriptorPool,
            RF::GetMaxFramesPerFlight(),
            mPerFrameDescriptorSetLayout
        );

        auto const & cameraBufferCollection = mAttachedScene->GetCameraBufferCollection();
        auto const & directionalLightBuffers = mAttachedScene->GetDirectionalLightBuffers();
        auto const & pointLightBuffers = mAttachedScene->GetPointLightsBuffers();
        
        for (uint32_t frameIndex = 0; frameIndex < RF::GetMaxFramesPerFlight(); ++frameIndex)
        {

            auto const descriptorSet = mPerFrameDescriptorSetGroup.descriptorSets[frameIndex];
            MFA_VK_VALID_ASSERT(descriptorSet);

            DescriptorSetSchema descriptorSetSchema{ descriptorSet };
            // Important note: Keep reference of all descriptor buffer infos until updateDescriptorSets is called
            // DisplayViewProjection
            VkDescriptorBufferInfo viewProjectionBufferInfo{
                .buffer = cameraBufferCollection.buffers[frameIndex]->buffer,
                .offset = 0,
                .range = cameraBufferCollection.bufferSize,
            };
            descriptorSetSchema.AddUniformBuffer(&viewProjectionBufferInfo);

            // DirectionalLightBuffer
            VkDescriptorBufferInfo directionalLightBufferInfo {
                .buffer = directionalLightBuffers.buffers[frameIndex]->buffer,
                .offset = 0,
                .range = directionalLightBuffers.bufferSize,
            };
            descriptorSetSchema.AddUniformBuffer(&directionalLightBufferInfo);

            // PointLightBuffer
            VkDescriptorBufferInfo pointLightBufferInfo {
                .buffer = pointLightBuffers.buffers[frameIndex]->buffer,
                .offset = 0,
                .range = pointLightBuffers.bufferSize,
            };
            descriptorSetSchema.AddUniformBuffer(&pointLightBufferInfo);

            // Sampler
            VkDescriptorImageInfo texturesSamplerInfo{
                .sampler = mSamplerGroup->sampler,
                .imageView = nullptr,
                .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
            };
            descriptorSetSchema.AddSampler(&texturesSamplerInfo);

            // DirectionalLightShadowMap
            auto directionalLightShadowMap = VkDescriptorImageInfo {
                .sampler = nullptr,
                .imageView = mDirectionalLightShadowResources->GetShadowMap(frameIndex).imageView->imageView,
                .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
            };
            descriptorSetSchema.AddImage(&directionalLightShadowMap, 1);

            // PointLightShadowMap
            auto pointLightShadowCubeMapArray = VkDescriptorImageInfo {
                .sampler = nullptr,
                .imageView = mPointLightShadowResources->GetShadowCubeMap(frameIndex).imageView->imageView,
                .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
            };
            descriptorSetSchema.AddImage(
                &pointLightShadowCubeMapArray,
                1
            );
            
            descriptorSetSchema.UpdateDescriptorSets();
        }
    }

    //-------------------------------------------------------------------------------------------------

    void PBRWithShadowPipelineV2::createEssenceDescriptorSets(DrawableEssence & essence) const
    {
        auto const & textures = essence.GetGpuModel()->textures;

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
                .buffer = primitiveBuffer.buffers[0]->buffer,
                .offset = 0,
                .range = primitiveBuffer.bufferSize,
            };
            descriptorSetSchema.AddUniformBuffer(&primitiveBufferInfo);

            // TODO Each one need their own sampler
            // Textures
            MFA_ASSERT(textures.size() <= 64);
            // We need to keep imageInfos alive
            std::vector<VkDescriptorImageInfo> imageInfos{};
            for (auto const & texture : textures)
            {
                imageInfos.emplace_back(VkDescriptorImageInfo{
                    .sampler = nullptr,
                    .imageView = texture->imageView->imageView,
                    .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
                });
            }
            for (auto i = static_cast<uint32_t>(textures.size()); i < 64; ++i)
            {
                imageInfos.emplace_back(VkDescriptorImageInfo{
                    .sampler = nullptr,
                    .imageView = mErrorTexture->imageView->imageView,
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
        auto const * storedSkinJointsBuffer = variant->GetSkinJointsBuffer();

        for (uint32_t frameIndex = 0; frameIndex < RF::GetMaxFramesPerFlight(); ++frameIndex)
        {

            auto const descriptorSet = descriptorSetGroup.descriptorSets[frameIndex];
            MFA_VK_VALID_ASSERT(descriptorSet);

            DescriptorSetSchema descriptorSetSchema{ descriptorSet };

            /////////////////////////////////////////////////////////////////
            // Vertex shader
            /////////////////////////////////////////////////////////////////

            // SkinJoints
            VkBuffer skinJointsBuffer = mErrorBuffer->buffers[0]->buffer;
            size_t skinJointsBufferSize = mErrorBuffer->bufferSize;
            if (storedSkinJointsBuffer != nullptr && storedSkinJointsBuffer->bufferSize > 0)
            {
                skinJointsBuffer = storedSkinJointsBuffer->buffers[frameIndex]->buffer;
                skinJointsBufferSize = storedSkinJointsBuffer->bufferSize;
            }
            VkDescriptorBufferInfo skinTransformBufferInfo{
                .buffer = skinJointsBuffer,
                .offset = 0,
                .range = skinJointsBufferSize,
            };
            descriptorSetSchema.AddUniformBuffer(&skinTransformBufferInfo);

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
                if (auto const variant = occlusionQueryData.Variants[i].lock())
                {
                    variant->SetIsOccluded(occlusionQueryData.Results[i] == 0);
                }
                if (occlusionQueryData.Results[i] == 0)
                {
                    occlusionCount++;
                }
            }
            MFA_LOG_INFO("Occluded objects: %d", static_cast<int>(occlusionCount));

            occlusionQueryData.Variants.clear();
        }
    }

    //-------------------------------------------------------------------------------------------------

    void PBRWithShadowPipelineV2::performDepthPrePass(RT::CommandRecordState & recordState)
    {
        DepthPrePassPushConstants pushConstants{};

        RF::BindDrawPipeline(recordState, mDepthPassPipeline);
        RF::BindDescriptorSet(
            recordState,
            RenderFrontend::DescriptorSetType::PerFrame,
            mPerFrameDescriptorSetGroup
        );

        mDepthPrePass->BeginRenderPass(recordState);
        for (auto const & essenceAndVariantList : mEssenceAndVariantsMap)
        {
            auto & essence = essenceAndVariantList.second.Essence;
            auto & variantsList = essenceAndVariantList.second.Variants;

            if (variantsList.empty())
            {
                continue;
            }

            essence->BindAllRenderRequiredData(recordState);

            for (auto & variant : variantsList)
            {
                if (variant->IsVisible())
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
        mDepthPrePass->EndRenderPass(recordState);
    }

    //-------------------------------------------------------------------------------------------------
    
    void PBRWithShadowPipelineV2::performDirectionalLightShadowPass(RT::CommandRecordState & recordState)
    {
        if (mAttachedScene->GetDirectionalLightCount() <= 0)
        {
            return;
        }

        RF::BindDrawPipeline(recordState, mDirectionalLightShadowPipeline);
        RF::BindDescriptorSet(
            recordState,
            RenderFrontend::DescriptorSetType::PerFrame,
            mPerFrameDescriptorSetGroup
        );

        // Vertex push constants
        DirectionalLightShadowPassPushConstants pushConstants {};

        mDirectionalLightShadowRenderPass->BeginRenderPass(recordState, *mDirectionalLightShadowResources);
        for (auto const & essenceAndVariantList : mEssenceAndVariantsMap)
        {
            auto & essence = essenceAndVariantList.second.Essence;
            auto & variantsList = essenceAndVariantList.second.Variants;

            if (variantsList.empty())
            {
                continue;
            }

            essence->BindAllRenderRequiredData(recordState);

            for (auto & variant : variantsList)
            {
                if (variant->IsVisible())
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
                        Matrix::CopyGlmToCells(node.cachedModelTransform, pushConstants.model);
                        Matrix::CopyGlmToCells(node.cachedGlobalInverseTransform, pushConstants.inverseNodeTransform);

                        RF::PushConstants(
                            recordState,
                            VK_SHADER_STAGE_VERTEX_BIT,
                            0,
                            CBlobAliasOf(pushConstants)
                        );
                    });
                }
            }
        }
        mDirectionalLightShadowRenderPass->EndRenderPass(recordState);
    }

    //-------------------------------------------------------------------------------------------------

    void PBRWithShadowPipelineV2::performPointLightShadowPass(RT::CommandRecordState & recordState)
    {
        if (mAttachedScene->GetPointLightCount() <= 0)
        {
            return;
        }

        RF::BindDrawPipeline(recordState, mPointLightShadowPipeline);
        RF::BindDescriptorSet(
            recordState,
            RenderFrontend::DescriptorSetType::PerFrame,
            mPerFrameDescriptorSetGroup
        );

        // Vertex push constants
        PointLightShadowPassPushConstants pushConstants {};

        mPointLightShadowRenderPass->BeginRenderPass(recordState, *mPointLightShadowResources);
        for (auto const & essenceAndVariantList : mEssenceAndVariantsMap)
        {
            auto & essence = essenceAndVariantList.second.Essence;
            auto & variantsList = essenceAndVariantList.second.Variants;

            if (variantsList.empty())
            {
                continue;
            }

            essence->BindAllRenderRequiredData(recordState);

            for (auto & variant : variantsList)
            {
                if (variant->IsVisible())
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
                            VK_SHADER_STAGE_VERTEX_BIT,
                            0,
                            CBlobAliasOf(pushConstants)
                        );
                    });
                }
            }
        }
        mPointLightShadowRenderPass->EndRenderPass(recordState);
    }

    //-------------------------------------------------------------------------------------------------
    // We have separate pass for occlusion. Because of not having order of drawing, we need the depth pre pass instead.
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

        OcclusionPassPushConstants pushConstants{};

        for (auto const & essenceAndVariantList : mEssenceAndVariantsMap)
        {
            auto & essence = essenceAndVariantList.second.Essence;
            auto & variantsList = essenceAndVariantList.second.Variants;

            if (variantsList.empty())
            {
                continue;
            }

            essence->BindAllRenderRequiredData(recordState);

            for (auto & variant : variantsList)
            {
                if (variant->IsActive() && variant->IsInFrustum())
                {

                    RF::BindDescriptorSet(
                        recordState,
                        RenderFrontend::DescriptorSetType::PerVariant,
                        variant->GetDescriptorSetGroup()
                    );

                    RF::BeginQuery(recordState, occlusionQueryData.Pool, static_cast<uint32_t>(occlusionQueryData.Variants.size()));

                    // TODO Draw a placeholder cube instead of complex geometry
                    variant->Draw(recordState, [&recordState, &pushConstants](AS::MeshPrimitive const & primitive, DrawableVariant::Node const & node)-> void
                        {
                            // Vertex push constants
                            Matrix::CopyGlmToCells(node.cachedModelTransform, pushConstants.modelTransform);
                            Matrix::CopyGlmToCells(node.cachedGlobalInverseTransform, pushConstants.inverseNodeTransform);
                            pushConstants.skinIndex = node.skin != nullptr ? node.skin->skinStartingIndex : -1;
                            pushConstants.primitiveIndex = primitive.uniqueId;

                            RF::PushConstants(
                                recordState,
                                VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                                0,
                                CBlobAliasOf(pushConstants)
                            );
                        }
                    );

                    RF::EndQuery(recordState, occlusionQueryData.Pool, static_cast<uint32_t>(occlusionQueryData.Variants.size()));

                    // What if the variant is destroyed in next frame?
                    occlusionQueryData.Variants.emplace_back(variant);
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

        DisplayPassPushConstants pushConstants{};

        for (auto const & essenceAndVariantList : mEssenceAndVariantsMap)
        {
            auto & essence = essenceAndVariantList.second.Essence;
            auto & variantsList = essenceAndVariantList.second.Variants;

            if (variantsList.empty())
            {
                continue;
            }

            essence->BindAllRenderRequiredData(recordState);

            for (auto & variant : variantsList)
            {
                if (variant->IsVisible())
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

        // CameraBuffer
        VkDescriptorSetLayoutBinding cameraBufferLayoutBinding{
            .binding = static_cast<uint32_t>(bindings.size()),
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
        };
        bindings.emplace_back(cameraBufferLayoutBinding);

        // DirectionalLightBuffer
        bindings.emplace_back(VkDescriptorSetLayoutBinding {
            .binding = static_cast<uint32_t>(bindings.size()),
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_GEOMETRY_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
        });

        // PointLightBuffer;
        bindings.emplace_back(VkDescriptorSetLayoutBinding {
            .binding = static_cast<uint32_t>(bindings.size()),
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_GEOMETRY_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
        });

        // Sampler
        bindings.emplace_back(VkDescriptorSetLayoutBinding {
            .binding = static_cast<uint32_t>(bindings.size()),
            .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
        });

        // DirectionalLightShadowMap
        bindings.emplace_back(VkDescriptorSetLayoutBinding {
            .binding = static_cast<uint32_t>(bindings.size()),
            .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
        });
        
        // PointLightShadowMap
        bindings.emplace_back(VkDescriptorSetLayoutBinding {
            .binding = static_cast<uint32_t>(bindings.size()),
            .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
        });

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

        // Primitive
        bindings.emplace_back(VkDescriptorSetLayoutBinding {
            .binding = static_cast<uint32_t>(bindings.size()),
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
        });

        // Textures
        bindings.emplace_back(VkDescriptorSetLayoutBinding {
            .binding = static_cast<uint32_t>(bindings.size()),
            .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
            .descriptorCount = 64,
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
        });
        
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

        // SkinJoints
        bindings.emplace_back(VkDescriptorSetLayoutBinding {
            .binding = static_cast<uint32_t>(bindings.size()),
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
        });
        
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
        RF_CREATE_SHADER("shaders/pbr_with_shadow_v2/PbrWithShadow.vert.spv", Vertex)

        // Fragment shader
        RF_CREATE_SHADER("shaders/pbr_with_shadow_v2/PbrWithShadow.frag.spv", Fragment)

        std::vector<RT::GpuShader const *> shaders{ gpuVertexShader.get(), gpuFragmentShader.get() };

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
            .size = sizeof(DisplayPassPushConstants),
        });

        RT::CreateGraphicPipelineOptions options{};
        options.rasterizationSamples = RF::GetMaxSamplesCount();
        options.pushConstantRanges = mPushConstantRanges.data();
        options.pushConstantsRangeCount = static_cast<uint8_t>(mPushConstantRanges.size());
        options.cullMode = VK_CULL_MODE_BACK_BIT;
        options.depthStencil.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;          // It must be less or equal because transparent and transluent objects are discarded in depth prepass and occlusion pass

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

    void PBRWithShadowPipelineV2::createDirectionalLightShadowPassPipeline()
    {
        // Vertex shader
        RF_CREATE_SHADER("shaders/directional_light_shadow/DirectionalLightShadow.vert.spv", Vertex)
        RF_CREATE_SHADER("shaders/directional_light_shadow/DirectionalLightShadow.geom.spv", Geometry)
        
        std::vector<RT::GpuShader const *> shaders{ gpuVertexShader.get(), gpuGeometryShader.get() };

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
            attributeDescription.offset = offsetof(AS::MeshVertex, hasSkin);
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
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
            .offset = 0,
            .size = sizeof(DirectionalLightShadowPassPushConstants),
        };
        pushConstantRanges.emplace_back(pushConstantRange);

        RT::CreateGraphicPipelineOptions graphicPipelineOptions{};
        graphicPipelineOptions.cullMode = VK_CULL_MODE_FRONT_BIT;                // TODO Find a good fit!
        graphicPipelineOptions.pushConstantRanges = pushConstantRanges.data();
        // TODO Probably we need to make pushConstantsRangeCount uint32_t
        graphicPipelineOptions.pushConstantsRangeCount = static_cast<uint8_t>(pushConstantRanges.size());

        MFA_ASSERT(mDirectionalLightShadowPipeline.isValid() == false);
        mDirectionalLightShadowPipeline = RF::CreatePipeline(
            mDirectionalLightShadowRenderPass->GetVkRenderPass(),
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

    void PBRWithShadowPipelineV2::createPointLightShadowPassPipeline()
    {
        // Vertex shader
        RF_CREATE_SHADER("shaders/point_light_shadow/PointLightShadow.vert.spv", Vertex)
        RF_CREATE_SHADER("shaders/point_light_shadow/PointLightShadow.geom.spv", Geometry)
        RF_CREATE_SHADER("shaders/point_light_shadow/PointLightShadow.frag.spv", Fragment)

        std::vector<RT::GpuShader const *> shaders {
            gpuVertexShader.get(),
            gpuGeometryShader.get(),
            gpuFragmentShader.get()
        };

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
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
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
        RF_CREATE_SHADER("shaders/depth_pre_pass/DepthPrePass.vert.spv", Vertex)
        RF_CREATE_SHADER("shaders/depth_pre_pass/DepthPrePass.frag.spv", Fragment)

        std::vector<RT::GpuShader const *> shaders{ gpuVertexShader.get(), gpuFragmentShader.get() };

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

        // BaseColorUV
        inputAttributeDescriptions.emplace_back(VkVertexInputAttributeDescription{
            .location = static_cast<uint32_t>(inputAttributeDescriptions.size()),
            .binding = 0,
            .format = VK_FORMAT_R32G32_SFLOAT,
            .offset = offsetof(AS::MeshVertex, baseColorUV),
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
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
            .offset = 0,
            .size = sizeof(DepthPrePassPushConstants),
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
        RF_CREATE_SHADER("shaders/occlusion_query/Occlusion.vert.spv", Vertex)
        RF_CREATE_SHADER("shaders/occlusion_query/Occlusion.frag.spv", Fragment)

        std::vector<RT::GpuShader const *> shaders{ gpuVertexShader.get(), gpuFragmentShader.get() };

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

        // BaseColorUV
        inputAttributeDescriptions.emplace_back(VkVertexInputAttributeDescription{
            .location = static_cast<uint32_t>(inputAttributeDescriptions.size()),
            .binding = 0,
            .format = VK_FORMAT_R32G32_SFLOAT,
            .offset = offsetof(AS::MeshVertex, baseColorUV),
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
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
            .offset = 0,
            .size = sizeof(OcclusionPassPushConstants),
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

        MFA_ASSERT(mDirectionalLightShadowPipeline.isValid());
        RF::DestroyPipelineGroup(mDirectionalLightShadowPipeline);

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

}
