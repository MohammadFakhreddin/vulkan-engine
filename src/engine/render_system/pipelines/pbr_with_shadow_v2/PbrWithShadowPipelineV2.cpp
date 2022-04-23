#include "PbrWithShadowPipelineV2.hpp"

#include "PBR_Essence.hpp"
#include "PBR_Variant.hpp"
#include "engine/BedrockAssert.hpp"
#include "engine/BedrockPath.hpp"
#include "engine/render_system/render_passes/display_render_pass/DisplayRenderPass.hpp"
#include "tools/Importer.hpp"
#include "engine/BedrockMatrix.hpp"
#include "engine/render_system/pipelines/DescriptorSetSchema.hpp"
#include "engine/render_system/RenderFrontend.hpp"
#include "engine/render_system/render_passes/depth_pre_pass/DepthPrePass.hpp"
#include "engine/render_system/render_passes/directional_light_shadow_render_pass/DirectionalLightShadowRenderPass.hpp"
#include "engine/render_system/render_passes/occlusion_render_pass/OcclusionRenderPass.hpp"
#include "engine/render_system/render_passes/point_light_shadow_render_pass/PointLightShadowRenderPass.hpp"
#include "engine/render_system/render_resources/directional_light_shadow_resources/DirectionalLightShadowResources.hpp"
#include "engine/render_system/render_resources/point_light_shadow_resources/PointLightShadowResources.hpp"
#include "engine/job_system/JobSystem.hpp"
#include "engine/asset_system/AssetShader.hpp"
#include "engine/entity_system/components/PointLightComponent.hpp"
#include "engine/scene_manager/SceneManager.hpp"
#include "engine/resource_manager/ResourceManager.hpp"

#define CAST_ESSENCE_PURE(essence)      static_cast<PBR_Essence *>(essence)
#define CAST_ESSENCE_SHARED(essence)    CAST_ESSENCE_PURE(essence.get())
#define CAST_VARIANT_PURE(variant)      static_cast<PBR_Variant *>(variant.get())

/*
There are four C++ style casts:

const_cast
static_cast
reinterpret_cast
dynamic_cast
As already mentioned, the first three are compile-time operations. There is no run-time penalty for using them.
They are messages to the compiler that data that has been declared one way needs to be accessed a different way.
"I said this was an int*, but let me access it as if it were a char* pointing to sizeof(int) chars" or "I said this data was read-only,
and now I need to pass it to a function that won't modify it, but doesn't take the parameter as a const reference."
*/


/*
 * https://community.arm.com/arm-community-blogs/b/graphics-gaming-and-vr-blog/posts/vulkan-mobile-best-practices-and-management
 * Multi-threaded command recording has the potential to improve CPU time significantly, but it also opens up several pitfalls.
 * In the worst case scenario, this can lead to worse performance than single threaded. 
 *
 * Our general recommendation is to use a profiler and figure out the bottleneck for your application,
 * while keeping a close eye on common pain points regarding threading in general.
 * The issues that we have encountered most often are the following: 
 *
 * Thread spawning causing a significant overhead. This could happen if you use std::async directly to spawn your threads,
 * as STL implementations usually do not pool threads in that case. We recommend using a thread pool library instead, or to implement thread pooling yourself.
 *
 * Synchronization overhead might be significant. If you are using mutexes to guard all your map accesses,
 * the code might end up running in a serialized fashion with the extra overhead for lock acquisition/release.
 * Alternative approaches could be using a read/write mutex like shared_mutex, or go lock-free by ensuring that the map is read-only while executing multi-threaded code.
 *
 * In the lock-free approach, each thread can keep a list of entries to add to the map.
 * These per-thread lists of entries are then inserted into the map after all the threads have returned. 
 * Having few meshes per thread.
 * Multi-threaded command recording has some performance overhead both on the CPU side (cost of threading) and on the GPU side (executing secondary command buffers).
 * Therefore, using the full parallelism available is not always a good choice. As a rule of thumb,
 * only go parallel if you measure that draw call recording is taking a significant portion of your frame time. 
 */
namespace MFA
{

    // Steps for multi-threading. Begin render pass -> submit subcommands -> execute into primary end renderPpass

    //-------------------------------------------------------------------------------------------------

    PBRWithShadowPipelineV2::PBRWithShadowPipelineV2()
        : BasePipeline(10000)
        , mPointLightShadowRenderPass(std::make_unique<PointLightShadowRenderPass>())
        , mPointLightShadowResources(std::make_unique<PointLightShadowResources>())
        , mDirectionalLightShadowRenderPass(std::make_unique<DirectionalLightShadowRenderPass>())
        , mDirectionalLightShadowResources(std::make_unique<DirectionalLightShadowResources>())
        , mDepthPrePass(std::make_unique<DepthPrePass>())
        , mOcclusionRenderPass(std::make_unique<OcclusionRenderPass>())
    {
    }

    //-------------------------------------------------------------------------------------------------

    PBRWithShadowPipelineV2::~PBRWithShadowPipelineV2()
    {
        if (mIsInitialized)
        {
            shutdown();
        }
    }

    //-------------------------------------------------------------------------------------------------

    void PBRWithShadowPipelineV2::init()
    {
        if (mIsInitialized == true)
        {
            MFA_ASSERT(false);
            return;
        }
        
        BasePipeline::init();

        mSamplerGroup = RF::CreateSampler(RT::CreateSamplerParams{});
        MFA_ASSERT(mSamplerGroup != nullptr);

        mErrorTexture = RC::AcquireGpuTexture("Error");
        MFA_ASSERT(mErrorTexture != nullptr);
        
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

        auto const descriptorSetLayouts = std::vector<VkDescriptorSetLayout>{
            mPerFrameDescriptorSetLayout->descriptorSetLayout,
            mPerEssenceDescriptorSetLayout->descriptorSetLayout,
            mPerVariantDescriptorSetLayout->descriptorSetLayout
        };

        createDisplayPassPipeline(descriptorSetLayouts);
        createPointLightShadowPassPipeline(descriptorSetLayouts);
        createDirectionalLightShadowPassPipeline(descriptorSetLayouts);
        createDepthPassPipeline(descriptorSetLayouts);
        createOcclusionQueryPipeline(descriptorSetLayouts);

        createPerFrameDescriptorSets();

        createOcclusionQueryPool();
    }

    //-------------------------------------------------------------------------------------------------

    void PBRWithShadowPipelineV2::shutdown()
    {
        if (mIsInitialized == false)
        {
            MFA_ASSERT(false);
            return;
        }
        
        destroyOcclusionQueryPool();

        mSamplerGroup = nullptr;
        mErrorTexture = nullptr;

        mOcclusionRenderPass->Shutdown();

        mDepthPrePass->Shutdown();

        mPointLightShadowResources->Shutdown();
        mPointLightShadowRenderPass->Shutdown();

        mDirectionalLightShadowResources->Shutdown();
        mDirectionalLightShadowRenderPass->Shutdown();
     
        BasePipeline::shutdown();

    }

    //-------------------------------------------------------------------------------------------------

    void PBRWithShadowPipelineV2::preRenderVariants(float deltaTimeInSec, RT::CommandRecordState const & recordState) const
    {
        // Multi-thread update of variant animation
        JS::AssignTaskPerThread([this, &recordState, deltaTimeInSec](uint32_t const threadNumber, uint32_t const threadCount)->void
        {
            for (uint32_t i = threadNumber; i < static_cast<uint32_t>(mAllVariantsList.size()); i += threadCount)
            {
                mAllVariantsList[i]->PreRender(deltaTimeInSec, recordState);
            }
        });
        JS::WaitForThreadsToFinish();
    }

    //-------------------------------------------------------------------------------------------------

    void PBRWithShadowPipelineV2::postRenderVariants(float deltaTimeInSec) const
    {
        // Multi-thread update of variant animation
        JS::AssignTaskPerThread([this, deltaTimeInSec](uint32_t const threadNumber, uint32_t const threadCount)->void
        {
            for (uint32_t i = threadNumber; i < static_cast<uint32_t>(mAllVariantsList.size()); i += threadCount)
            {
                mAllVariantsList[i]->PostRender(deltaTimeInSec);
            }
        });

        if (JS::IsMainThread())
        {
            JS::WaitForThreadsToFinish();
        }
    }

    //-------------------------------------------------------------------------------------------------

    void PBRWithShadowPipelineV2::preRender(RT::CommandRecordState & recordState, float const deltaTime)
    {
        // TODO I should render bounding volume for objects and geometry for occluders.
        // Some objects might need more than 1 occluder.
        retrieveOcclusionQueryResult(recordState);

        BasePipeline::preRender(recordState, deltaTime);

        preRenderVariants(deltaTime, recordState);

        performDepthPrePass(recordState);

        performOcclusionQueryPass(recordState);

        performDirectionalLightShadowPass(recordState);

        performPointLightShadowPass(recordState);

        prepareShadowMapsForSampling(recordState);

        RF::GetDisplayRenderPass()->notifyDepthImageLayoutIsSet();
    }

    //-------------------------------------------------------------------------------------------------

    void PBRWithShadowPipelineV2::render(RT::CommandRecordState & recordState, float const deltaTimeInSec)
    {
        BasePipeline::render(recordState, deltaTimeInSec);
        performDisplayPass(recordState);
    }

    //-------------------------------------------------------------------------------------------------

    void PBRWithShadowPipelineV2::update(float const deltaTimeInSec)
    {
        BasePipeline::update(deltaTimeInSec);
        postRenderVariants(deltaTimeInSec);
    }

    //-------------------------------------------------------------------------------------------------

    void PBRWithShadowPipelineV2::onResize()
    {
        mDepthPrePass->OnResize();
        mOcclusionRenderPass->OnResize();
    }

    //-------------------------------------------------------------------------------------------------

    void PBRWithShadowPipelineV2::internalAddEssence(EssenceBase * essence)
    {
        MFA_ASSERT(dynamic_cast<PBR_Essence *>(essence) != nullptr);
        CAST_ESSENCE_PURE(essence)->createGraphicDescriptorSet(
            mDescriptorPool,
            mPerEssenceDescriptorSetLayout->descriptorSetLayout,
            *mErrorTexture
        );
    }

    //-------------------------------------------------------------------------------------------------

    std::shared_ptr<VariantBase> PBRWithShadowPipelineV2::internalCreateVariant(EssenceBase * essence)
    {
        auto * drawableEssence = dynamic_cast<PBR_Essence *>(essence);
        MFA_ASSERT(drawableEssence != nullptr);
        auto variant = std::make_shared<PBR_Variant>(drawableEssence);
        createVariantDescriptorSets(*variant);
        return variant;
    }
    
    //-------------------------------------------------------------------------------------------------

    void PBRWithShadowPipelineV2::createPerFrameDescriptorSets()
    {
        mPerFrameDescriptorSetGroup = RF::CreateDescriptorSets(
            mDescriptorPool,
            RF::GetMaxFramesPerFlight(),
            *mPerFrameDescriptorSetLayout
        );

        auto const * cameraBufferCollection = SceneManager::GetCameraBuffers();
        auto const * directionalLightBuffers = SceneManager::GetDirectionalLightBuffers();
        auto const * pointLightBuffers = SceneManager::GetPointLightsBuffers();
        
        for (uint32_t frameIndex = 0; frameIndex < RF::GetMaxFramesPerFlight(); ++frameIndex)
        {

            auto const descriptorSet = mPerFrameDescriptorSetGroup.descriptorSets[frameIndex];
            MFA_VK_VALID_ASSERT(descriptorSet);

            DescriptorSetSchema descriptorSetSchema{ descriptorSet };
            // Important note: Keep reference of all descriptor buffer infos until updateDescriptorSets is called
            // DisplayViewProjection
            VkDescriptorBufferInfo viewProjectionBufferInfo{
                .buffer = cameraBufferCollection->buffers[frameIndex]->buffer,
                .offset = 0,
                .range = cameraBufferCollection->bufferSize,
            };
            descriptorSetSchema.AddUniformBuffer(&viewProjectionBufferInfo);

            // DirectionalLightBuffer
            VkDescriptorBufferInfo directionalLightBufferInfo {
                .buffer = directionalLightBuffers->buffers[frameIndex]->buffer,
                .offset = 0,
                .range = directionalLightBuffers->bufferSize,
            };
            descriptorSetSchema.AddUniformBuffer(&directionalLightBufferInfo);

            // PointLightBuffer
            VkDescriptorBufferInfo pointLightBufferInfo {
                .buffer = pointLightBuffers->buffers[frameIndex]->buffer,
                .offset = 0,
                .range = pointLightBuffers->bufferSize,
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

    
    void PBRWithShadowPipelineV2::createVariantDescriptorSets(PBR_Variant & variant) const
    {
        auto const descriptorSetGroup = variant.CreateDescriptorSetGroup(
            mDescriptorPool,
            RF::GetMaxFramesPerFlight(),
            *mPerVariantDescriptorSetLayout
        );
        auto const * storedSkinJointsBuffer = variant.GetSkinJointsBuffer();

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

            for (uint32_t i = 0; i < static_cast<uint32_t>(occlusionQueryData.Results.size()); ++i)
            {
                if (auto const variant = occlusionQueryData.Variants[i].lock())
                {
                    variant->SetIsOccluded(true);
                }
            }

            for (uint32_t i = 0; i < static_cast<uint32_t>(occlusionQueryData.Results.size()); ++i)
            {
                if (auto const variant = occlusionQueryData.Variants[i].lock())
                {
                    if (occlusionQueryData.Results[i] > 0)
                    {
                        variant->SetIsOccluded(false);
                    }
                }
            }

#ifdef MFA_DEBUG
            uint32_t occlusionCount = 0;
            for (uint32_t i = 0; i < static_cast<uint32_t>(occlusionQueryData.Results.size()); ++i)
            {
                if (auto const variant = occlusionQueryData.Variants[i].lock())
                {
                    if (variant->IsOccluded())
                    {
                        occlusionCount++;
                    }
                }
            }
            MFA_LOG_INFO("Occluded objects: %d", static_cast<int>(occlusionCount));
#endif

            occlusionQueryData.Variants.clear();
        }
    }

    //-------------------------------------------------------------------------------------------------

    void PBRWithShadowPipelineV2::performDepthPrePass(RT::CommandRecordState & recordState) const
    {
        RF::BindPipeline(recordState, *mDepthPassPipeline);
        RF::AutoBindDescriptorSet(
            recordState,
            RenderFrontend::UpdateFrequency::PerFrame,
            mPerFrameDescriptorSetGroup
        );

        mDepthPrePass->BeginRenderPass(recordState);
        renderForDepthPrePass(recordState, AS::AlphaMode::Opaque);
        mDepthPrePass->EndRenderPass(recordState);
    }

    //-------------------------------------------------------------------------------------------------

    void PBRWithShadowPipelineV2::renderForDepthPrePass(RT::CommandRecordState const & recordState, AS::AlphaMode const alphaMode) const
    {
        DepthPrePassPushConstants pushConstants{};
        // TODO: Submit in multiple threads
        for (auto const & essenceAndVariantList : mEssenceAndVariantsMap)
        {
            auto const * essence = CAST_ESSENCE_SHARED(essenceAndVariantList.second.essence);
            auto & variantsList = essenceAndVariantList.second.variants;

            if (variantsList.empty())
            {
                continue;
            }

            essence->bindForGraphicPipeline(recordState);

            for (auto & variant : variantsList)
            {
                if (variant->IsVisible())
                {

                    RF::AutoBindDescriptorSet(
                        recordState,
                        RenderFrontend::UpdateFrequency::PerVariant,
                        variant->GetDescriptorSetGroup()
                    );

                    CAST_VARIANT_PURE(variant)->Render(
                        recordState,
                        [&recordState, &pushConstants](
                            AS::PBR::Primitive const & primitive,
                            PBR_Variant::Node const & node
                        )-> void
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
                        },
                        alphaMode
                    );

                }
            }
        }
    }

    //-------------------------------------------------------------------------------------------------
    
    void PBRWithShadowPipelineV2::performDirectionalLightShadowPass(RT::CommandRecordState & recordState) const
    {
        if (SceneManager::GetDirectionalLightCount() <= 0)
        {
            return;
        }

        RF::BindPipeline(recordState, *mDirectionalLightShadowPipeline);
        RF::AutoBindDescriptorSet(
            recordState,
            RenderFrontend::UpdateFrequency::PerFrame,
            mPerFrameDescriptorSetGroup
        );

        mDirectionalLightShadowRenderPass->BeginRenderPass(recordState, *mDirectionalLightShadowResources);
        renderForDirectionalLightShadowPass(recordState, AS::AlphaMode::Opaque);
        renderForDirectionalLightShadowPass(recordState, AS::AlphaMode::Mask);
        renderForDirectionalLightShadowPass(recordState, AS::AlphaMode::Blend);
        mDirectionalLightShadowRenderPass->EndRenderPass(recordState);
    }

    //-------------------------------------------------------------------------------------------------

    void PBRWithShadowPipelineV2::renderForDirectionalLightShadowPass(
        RT::CommandRecordState const & recordState,
        AS::AlphaMode const alphaMode
    ) const
    {
        DirectionalLightShadowPassPushConstants pushConstants {};

        for (auto const & essenceAndVariantList : mEssenceAndVariantsMap)
        {
            auto const * essence = CAST_ESSENCE_SHARED(essenceAndVariantList.second.essence);
            auto & variantsList = essenceAndVariantList.second.variants;

            if (variantsList.empty())
            {
                continue;
            }

            essence->bindForGraphicPipeline(recordState);

            for (auto & variant : variantsList)
            {
                // TODO We need a wider frustum for shadows
                if (variant->IsActive())
                {
                    RF::AutoBindDescriptorSet(
                        recordState,
                        RenderFrontend::UpdateFrequency::PerVariant,
                        variant->GetDescriptorSetGroup()
                    );

                    CAST_VARIANT_PURE(variant)->Render(recordState, [&recordState, &pushConstants](AS::PBR::Primitive const & primitive, PBR_Variant::Node const & node)-> void
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
                    }, alphaMode);
                }
            }
        }
    }

    //-------------------------------------------------------------------------------------------------

    void PBRWithShadowPipelineV2::performPointLightShadowPass(RT::CommandRecordState & recordState) const
    {
        auto const pointLightCount = SceneManager::GetPointLightCount();
        if (pointLightCount <= 0)
        {
            return;
        }

        RF::BindPipeline(recordState, *mPointLightShadowPipeline);
        RF::AutoBindDescriptorSet(
            recordState,
            RenderFrontend::UpdateFrequency::PerFrame,
            mPerFrameDescriptorSetGroup
        );

        mPointLightShadowRenderPass->BeginRenderPass(recordState, *mPointLightShadowResources);
        renderForPointLightShadowPass(recordState, AS::AlphaMode::Opaque);
        renderForPointLightShadowPass(recordState, AS::AlphaMode::Mask);
        renderForPointLightShadowPass(recordState, AS::AlphaMode::Blend);
        mPointLightShadowRenderPass->EndRenderPass(recordState);
    }

    //-------------------------------------------------------------------------------------------------
    
    void PBRWithShadowPipelineV2::prepareShadowMapsForSampling(RT::CommandRecordState const & recordState) const
    {
        std::vector<VkImageMemoryBarrier> barrier {};
        mPointLightShadowRenderPass->PrepareRenderTargetForSampling(
            recordState,
            mPointLightShadowResources.get(),
            SceneManager::GetPointLightCount() > 0,
            barrier
        );
        mDirectionalLightShadowRenderPass->PrepareRenderTargetForSampling(
            recordState,
            mDirectionalLightShadowResources.get(),
            SceneManager::GetDirectionalLightCount() > 0,
            barrier
        );
        RF::PipelineBarrier(
            recordState,
            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
            static_cast<uint32_t>(barrier.size()),
            barrier.data()
        );
    }

    //-------------------------------------------------------------------------------------------------
    
    void PBRWithShadowPipelineV2::renderForPointLightShadowPass(RT::CommandRecordState const & recordState, AS::AlphaMode const alphaMode) const
    {
        auto const pointLightCount = SceneManager::GetPointLightCount();
        MFA_ASSERT(pointLightCount > 0);
        auto const & pointLights = SceneManager::GetActivePointLights();
        MFA_ASSERT(pointLights.size() == pointLightCount);

        PointLightShadowPassPushConstants pushConstants {};

        for (uint32_t lightIndex = 0; lightIndex < pointLightCount; ++lightIndex)
        {
            pushConstants.lightIndex = static_cast<int>(lightIndex);

            auto * pointLight = pointLights[lightIndex];
            MFA_ASSERT(pointLight != nullptr);

            for (auto const & essenceAndVariantList : mEssenceAndVariantsMap)
            {
                auto const * essence = CAST_ESSENCE_SHARED(essenceAndVariantList.second.essence);
                auto & variantsList = essenceAndVariantList.second.variants;

                if (variantsList.empty())
                {
                    continue;
                }

                essence->bindForGraphicPipeline(recordState);

                for (auto & variant : variantsList)
                {
                    if (variant->IsActive() == false)
                    {
                        continue;
                    }
                    auto const * bvComponent = variant->GetBoundingVolume();
                    if (bvComponent == nullptr)
                    {
                        continue;
                    }
                    // We only render variants that are within pointLight's visible range
                    if (pointLight->IsBoundingVolumeInRange(bvComponent) == false)
                    {
                        continue;
                    }
                    RF::AutoBindDescriptorSet(
                        recordState,
                        RenderFrontend::UpdateFrequency::PerVariant,
                        variant->GetDescriptorSetGroup()
                    );

                    CAST_VARIANT_PURE(variant)->Render(recordState, [&recordState, &pushConstants](AS::PBR::Primitive const & primitive, PBR_Variant::Node const & node)-> void
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
                    }, alphaMode);
                }
            }
        }
    }

    //-------------------------------------------------------------------------------------------------

    void PBRWithShadowPipelineV2::performOcclusionQueryPass(RT::CommandRecordState & recordState)
    {
        RF::ResetQueryPool(
            recordState,
            mOcclusionQueryDataList[recordState.frameIndex].Pool,
            10000
        );

        RF::BindPipeline(recordState, *mOcclusionQueryPipeline);
        RF::AutoBindDescriptorSet(
            recordState,
            RenderFrontend::UpdateFrequency::PerFrame,
            mPerFrameDescriptorSetGroup
        );

        mOcclusionRenderPass->BeginRenderPass(recordState);
        renderForOcclusionQueryPass(recordState, AS::AlphaMode::Opaque);
        renderForOcclusionQueryPass(recordState, AS::AlphaMode::Mask);
        renderForOcclusionQueryPass(recordState, AS::AlphaMode::Blend); 
        mOcclusionRenderPass->EndRenderPass(recordState);
    }

    //-------------------------------------------------------------------------------------------------
    
    void PBRWithShadowPipelineV2::renderForOcclusionQueryPass(RT::CommandRecordState const & recordState, AS::AlphaMode const alphaMode)
    {
        auto & occlusionQueryData = mOcclusionQueryDataList[recordState.frameIndex];

        OcclusionPassPushConstants pushConstants{};

        for (auto const & essenceAndVariantList : mEssenceAndVariantsMap)
        {
            auto * essence = CAST_ESSENCE_SHARED(essenceAndVariantList.second.essence);
            auto & variantsList = essenceAndVariantList.second.variants;

            if (variantsList.empty())
            {
                continue;
            }

            essence->bindForGraphicPipeline(recordState);

            for (auto & variant : variantsList)
            {
                if (variant->IsActive() && variant->IsInFrustum())
                {

                    RF::AutoBindDescriptorSet(
                        recordState,
                        RenderFrontend::UpdateFrequency::PerVariant,
                        variant->GetDescriptorSetGroup()
                    );

                    RF::BeginQuery(recordState, occlusionQueryData.Pool, static_cast<uint32_t>(occlusionQueryData.Variants.size()));

                    // TODO Draw a placeholder cube instead of complex geometry
                    CAST_VARIANT_PURE(variant)->Render(recordState, [&recordState, &pushConstants](AS::PBR::Primitive const & primitive, PBR_Variant::Node const & node)-> void
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
                    , alphaMode);

                    RF::EndQuery(recordState, occlusionQueryData.Pool, static_cast<uint32_t>(occlusionQueryData.Variants.size()));

                    // What if the variant is destroyed in next frame?
                    occlusionQueryData.Variants.emplace_back(variant);
                }
            }
        }
    }

    //-------------------------------------------------------------------------------------------------

    void PBRWithShadowPipelineV2::performDisplayPass(RT::CommandRecordState & recordState)
    {
        RF::BindPipeline(recordState, *mDisplayPassPipeline);
        RF::AutoBindDescriptorSet(
            recordState,
            RenderFrontend::UpdateFrequency::PerFrame,
            mPerFrameDescriptorSetGroup
        );
        renderForDisplayPass(recordState, AS::AlphaMode::Opaque);
        renderForDisplayPass(recordState, AS::AlphaMode::Mask);
        renderForDisplayPass(recordState, AS::AlphaMode::Blend);
    }

    //-------------------------------------------------------------------------------------------------
    
    void PBRWithShadowPipelineV2::renderForDisplayPass(RT::CommandRecordState const & recordState, AS::AlphaMode alphaMode) const
    {
        DisplayPassPushConstants pushConstants{};

        for (auto const & essenceAndVariantList : mEssenceAndVariantsMap)
        {
            auto * essence = CAST_ESSENCE_SHARED(essenceAndVariantList.second.essence);
            auto & variantsList = essenceAndVariantList.second.variants;

            if (variantsList.empty())
            {
                continue;
            }

            essence->bindForGraphicPipeline(recordState);

            for (auto & variant : variantsList)
            {
                if (variant->IsVisible())
                {

                    RF::AutoBindDescriptorSet(
                        recordState,
                        RenderFrontend::UpdateFrequency::PerVariant,
                        variant->GetDescriptorSetGroup()
                    );
                    // TODO We can render all instances at once and have a large push constant for all of them
                    CAST_VARIANT_PURE(variant)->Render(recordState, [&recordState, &pushConstants](AS::PBR::Primitive const & primitive, PBR_Variant::Node const & node)-> void
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
                    , alphaMode);
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

    void PBRWithShadowPipelineV2::createDisplayPassPipeline(std::vector<VkDescriptorSetLayout> const & descriptorSetLayouts)
    {
        // Vertex shader
        RF_CREATE_SHADER("shaders/pbr_with_shadow_v2/PbrWithShadow.vert.spv", Vertex)

        // Fragment shader
        RF_CREATE_SHADER("shaders/pbr_with_shadow_v2/PbrWithShadow.frag.spv", Fragment)

        std::vector<RT::GpuShader const *> shaders{ gpuVertexShader.get(), gpuFragmentShader.get() };

        VkVertexInputBindingDescription vertexInputBindingDescription{};
        vertexInputBindingDescription.binding = 0;
        vertexInputBindingDescription.stride = sizeof(AS::PBR::Vertex);
        vertexInputBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        std::vector<VkVertexInputAttributeDescription> inputAttributeDescriptions{};
        
        // Position
        inputAttributeDescriptions.emplace_back(VkVertexInputAttributeDescription {
            .location = static_cast<uint32_t>(inputAttributeDescriptions.size()),
            .binding = 0,
            .format = VK_FORMAT_R32G32B32_SFLOAT,
            .offset = offsetof(AS::PBR::Vertex, position)
        });

        // BaseColorUV
        inputAttributeDescriptions.emplace_back(VkVertexInputAttributeDescription {
            .location = static_cast<uint32_t>(inputAttributeDescriptions.size()),
            .binding = 0,
            .format = VK_FORMAT_R32G32_SFLOAT,
            .offset = offsetof(AS::PBR::Vertex, baseColorUV)
        });

        {// Metallic/RoughnessUV
            VkVertexInputAttributeDescription attributeDescription{};
            attributeDescription.location = static_cast<uint32_t>(inputAttributeDescriptions.size());
            attributeDescription.binding = 0;
            attributeDescription.format = VK_FORMAT_R32G32_SFLOAT;
            attributeDescription.offset = offsetof(AS::PBR::Vertex, metallicUV); // Metallic and roughness have same uv for gltf files  

            inputAttributeDescriptions.emplace_back(attributeDescription);
        }
        {// NormalMapUV
            VkVertexInputAttributeDescription attributeDescription{};
            attributeDescription.location = static_cast<uint32_t>(inputAttributeDescriptions.size());
            attributeDescription.binding = 0;
            attributeDescription.format = VK_FORMAT_R32G32_SFLOAT;
            attributeDescription.offset = offsetof(AS::PBR::Vertex, normalMapUV);

            inputAttributeDescriptions.emplace_back(attributeDescription);
        }
        {// Tangent
            VkVertexInputAttributeDescription attributeDescription{};
            attributeDescription.location = static_cast<uint32_t>(inputAttributeDescriptions.size());
            attributeDescription.binding = 0;
            attributeDescription.format = VK_FORMAT_R32G32B32A32_SFLOAT;
            attributeDescription.offset = offsetof(AS::PBR::Vertex, tangentValue);

            inputAttributeDescriptions.emplace_back(attributeDescription);
        }
        {// Normal
            VkVertexInputAttributeDescription attributeDescription{};
            attributeDescription.location = static_cast<uint32_t>(inputAttributeDescriptions.size());
            attributeDescription.binding = 0;
            attributeDescription.format = VK_FORMAT_R32G32B32_SFLOAT;
            attributeDescription.offset = offsetof(AS::PBR::Vertex, normalValue);

            inputAttributeDescriptions.emplace_back(attributeDescription);
        }
        {// EmissionUV
            VkVertexInputAttributeDescription attributeDescription{};
            attributeDescription.location = static_cast<uint32_t>(inputAttributeDescriptions.size());
            attributeDescription.binding = 0;
            attributeDescription.format = VK_FORMAT_R32G32_SFLOAT;
            attributeDescription.offset = offsetof(AS::PBR::Vertex, emissionUV);
            inputAttributeDescriptions.emplace_back(attributeDescription);
        }
        // OcclusionUV
        inputAttributeDescriptions.emplace_back(VkVertexInputAttributeDescription {
            .location = static_cast<uint32_t>(inputAttributeDescriptions.size()),
            .binding = 0,
            .format = VK_FORMAT_R32G32_SFLOAT,
            .offset = offsetof(AS::PBR::Vertex, occlusionUV), // Metallic and roughness have same uv for gltf files  
        });
        {// HasSkin
            VkVertexInputAttributeDescription attributeDescription{};
            attributeDescription.location = static_cast<uint32_t>(inputAttributeDescriptions.size());
            attributeDescription.binding = 0;
            attributeDescription.format = VK_FORMAT_R32_SINT;
            attributeDescription.offset = offsetof(AS::PBR::Vertex, hasSkin); // TODO We should use a primitiveInfo instead
            inputAttributeDescriptions.emplace_back(attributeDescription);
        }
        {// JointIndices
            VkVertexInputAttributeDescription attributeDescription{};
            attributeDescription.location = static_cast<uint32_t>(inputAttributeDescriptions.size());
            attributeDescription.binding = 0;
            attributeDescription.format = VK_FORMAT_R32G32B32A32_SINT;
            attributeDescription.offset = offsetof(AS::PBR::Vertex, jointIndices);
            inputAttributeDescriptions.emplace_back(attributeDescription);
        }
        {// JointWeights
            VkVertexInputAttributeDescription attributeDescription{};
            attributeDescription.location = static_cast<uint32_t>(inputAttributeDescriptions.size());
            attributeDescription.binding = 0;
            attributeDescription.format = VK_FORMAT_R32G32B32A32_SFLOAT;
            attributeDescription.offset = offsetof(AS::PBR::Vertex, jointWeights);
            inputAttributeDescriptions.emplace_back(attributeDescription);
        }
        
        std::vector<VkPushConstantRange> pushConstantRanges{};
        pushConstantRanges.emplace_back(VkPushConstantRange{
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
            .offset = 0,
            .size = sizeof(DisplayPassPushConstants),
        });

        const auto pipelineLayout = RF::CreatePipelineLayout(
            static_cast<uint32_t>(descriptorSetLayouts.size()),
            descriptorSetLayouts.data(),
            static_cast<uint32_t>(pushConstantRanges.size()),
            pushConstantRanges.data()
        );
        
        RT::CreateGraphicPipelineOptions options{};
        options.rasterizationSamples = RF::GetMaxSamplesCount();
        options.cullMode = VK_CULL_MODE_BACK_BIT;
        options.depthStencil.depthWriteEnable = VK_FALSE;
        options.depthStencil.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;          // It must be less or equal because transparent and transluent objects are discarded in depth prepass and occlusion pass

        mDisplayPassPipeline = RF::CreateGraphicPipeline(
            RF::GetDisplayRenderPass()->GetVkRenderPass(),
            static_cast<uint8_t>(shaders.size()),
            shaders.data(),
            pipelineLayout,
            1,
            &vertexInputBindingDescription,
            static_cast<uint8_t>(inputAttributeDescriptions.size()),
            inputAttributeDescriptions.data(),
            options
        );
    }

    //-------------------------------------------------------------------------------------------------

    void PBRWithShadowPipelineV2::createDirectionalLightShadowPassPipeline(std::vector<VkDescriptorSetLayout> const & descriptorSetLayouts)
    {
        // Vertex shader
        RF_CREATE_SHADER("shaders/directional_light_shadow/DirectionalLightShadow.vert.spv", Vertex)
        RF_CREATE_SHADER("shaders/directional_light_shadow/DirectionalLightShadow.geom.spv", Geometry)
        
        std::vector<RT::GpuShader const *> shaders{ gpuVertexShader.get(), gpuGeometryShader.get() };

        VkVertexInputBindingDescription vertexInputBindingDescription{};
        vertexInputBindingDescription.binding = 0;
        vertexInputBindingDescription.stride = sizeof(AS::PBR::Vertex);
        vertexInputBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        std::vector<VkVertexInputAttributeDescription> inputAttributeDescriptions{};

        {// Position
            VkVertexInputAttributeDescription attributeDescription{};
            attributeDescription.location = static_cast<uint32_t>(inputAttributeDescriptions.size());
            attributeDescription.binding = 0;
            attributeDescription.format = VK_FORMAT_R32G32B32_SFLOAT;
            attributeDescription.offset = offsetof(AS::PBR::Vertex, position);

            inputAttributeDescriptions.emplace_back(attributeDescription);
        }
        {// HasSkin
            VkVertexInputAttributeDescription attributeDescription{};
            attributeDescription.location = static_cast<uint32_t>(inputAttributeDescriptions.size());
            attributeDescription.binding = 0;
            attributeDescription.format = VK_FORMAT_R32_SINT;
            attributeDescription.offset = offsetof(AS::PBR::Vertex, hasSkin);
            inputAttributeDescriptions.emplace_back(attributeDescription);
        }
        {// JointIndices
            VkVertexInputAttributeDescription attributeDescription{};
            attributeDescription.location = static_cast<uint32_t>(inputAttributeDescriptions.size());
            attributeDescription.binding = 0;
            attributeDescription.format = VK_FORMAT_R32G32B32A32_SINT;
            attributeDescription.offset = offsetof(AS::PBR::Vertex, jointIndices);
            inputAttributeDescriptions.emplace_back(attributeDescription);
        }
        {// JointWeights
            VkVertexInputAttributeDescription attributeDescription{};
            attributeDescription.location = static_cast<uint32_t>(inputAttributeDescriptions.size());
            attributeDescription.binding = 0;
            attributeDescription.format = VK_FORMAT_R32G32B32A32_SFLOAT;
            attributeDescription.offset = offsetof(AS::PBR::Vertex, jointWeights);
            inputAttributeDescriptions.emplace_back(attributeDescription);
        }

        std::vector<VkPushConstantRange> pushConstantRanges{};
        VkPushConstantRange pushConstantRange{
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
            .offset = 0,
            .size = sizeof(DirectionalLightShadowPassPushConstants),
        };
        pushConstantRanges.emplace_back(pushConstantRange);

        const auto pipelineLayout = RF::CreatePipelineLayout(
            static_cast<uint32_t>(descriptorSetLayouts.size()),
            descriptorSetLayouts.data(),
            static_cast<uint32_t>(pushConstantRanges.size()),
            pushConstantRanges.data()
        );
        
        RT::CreateGraphicPipelineOptions graphicPipelineOptions{};
        graphicPipelineOptions.cullMode = VK_CULL_MODE_FRONT_BIT;                // TODO Find a good fit!

        mDirectionalLightShadowPipeline = RF::CreateGraphicPipeline(
            mDirectionalLightShadowRenderPass->GetVkRenderPass(),
            static_cast<uint8_t>(shaders.size()),
            shaders.data(),
            pipelineLayout,
            1,
            &vertexInputBindingDescription,
            static_cast<uint8_t>(inputAttributeDescriptions.size()),
            inputAttributeDescriptions.data(),
            graphicPipelineOptions
        );
    }

    //-------------------------------------------------------------------------------------------------

    void PBRWithShadowPipelineV2::createPointLightShadowPassPipeline(std::vector<VkDescriptorSetLayout> const & descriptorSetLayouts)
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
        vertexInputBindingDescription.stride = sizeof(AS::PBR::Vertex);
        vertexInputBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        std::vector<VkVertexInputAttributeDescription> inputAttributeDescriptions{};

        {// Position
            VkVertexInputAttributeDescription attributeDescription{};
            attributeDescription.location = static_cast<uint32_t>(inputAttributeDescriptions.size());
            attributeDescription.binding = 0;
            attributeDescription.format = VK_FORMAT_R32G32B32_SFLOAT;
            attributeDescription.offset = offsetof(AS::PBR::Vertex, position);

            inputAttributeDescriptions.emplace_back(attributeDescription);
        }
        {// HasSkin
            VkVertexInputAttributeDescription attributeDescription{};
            attributeDescription.location = static_cast<uint32_t>(inputAttributeDescriptions.size());
            attributeDescription.binding = 0;
            attributeDescription.format = VK_FORMAT_R32_SINT;
            attributeDescription.offset = offsetof(AS::PBR::Vertex, hasSkin); // TODO We should use a primitiveInfo instead
            inputAttributeDescriptions.emplace_back(attributeDescription);
        }
        {// JointIndices
            VkVertexInputAttributeDescription attributeDescription{};
            attributeDescription.location = static_cast<uint32_t>(inputAttributeDescriptions.size());
            attributeDescription.binding = 0;
            attributeDescription.format = VK_FORMAT_R32G32B32A32_SINT;
            attributeDescription.offset = offsetof(AS::PBR::Vertex, jointIndices);
            inputAttributeDescriptions.emplace_back(attributeDescription);
        }
        {// JointWeights
            VkVertexInputAttributeDescription attributeDescription{};
            attributeDescription.location = static_cast<uint32_t>(inputAttributeDescriptions.size());
            attributeDescription.binding = 0;
            attributeDescription.format = VK_FORMAT_R32G32B32A32_SFLOAT;
            attributeDescription.offset = offsetof(AS::PBR::Vertex, jointWeights);
            inputAttributeDescriptions.emplace_back(attributeDescription);
        }

        std::vector<VkPushConstantRange> pushConstantRanges{};
        VkPushConstantRange pushConstantRange{
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_GEOMETRY_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
            .offset = 0,
            .size = sizeof(PointLightShadowPassPushConstants),
        };
        pushConstantRanges.emplace_back(pushConstantRange);

        const auto pipelineLayout = RF::CreatePipelineLayout(
            static_cast<uint32_t>(descriptorSetLayouts.size()),
            descriptorSetLayouts.data(),
            static_cast<uint32_t>(pushConstantRanges.size()),
            pushConstantRanges.data()
        );

        RT::CreateGraphicPipelineOptions graphicPipelineOptions{};
        graphicPipelineOptions.cullMode = VK_CULL_MODE_BACK_BIT;

        mPointLightShadowPipeline = RF::CreateGraphicPipeline(
            mPointLightShadowRenderPass->GetVkRenderPass(),
            static_cast<uint8_t>(shaders.size()),
            shaders.data(),
            pipelineLayout,
            1,
            &vertexInputBindingDescription,
            static_cast<uint8_t>(inputAttributeDescriptions.size()),
            inputAttributeDescriptions.data(),
            graphicPipelineOptions
        );
    }

    //-------------------------------------------------------------------------------------------------

    void PBRWithShadowPipelineV2::createDepthPassPipeline(std::vector<VkDescriptorSetLayout> const & descriptorSetLayouts)
    {
        // Vertex shader
        RF_CREATE_SHADER("shaders/depth_pre_pass/DepthPrePass.vert.spv", Vertex)
        
        std::vector<RT::GpuShader const *> shaders{ gpuVertexShader.get() };

        VkVertexInputBindingDescription vertexInputBindingDescription{};
        vertexInputBindingDescription.binding = 0;
        vertexInputBindingDescription.stride = sizeof(AS::PBR::Vertex);
        vertexInputBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        std::vector<VkVertexInputAttributeDescription> inputAttributeDescriptions{};

        // Position
        inputAttributeDescriptions.emplace_back(VkVertexInputAttributeDescription{
            .location = static_cast<uint32_t>(inputAttributeDescriptions.size()),
            .binding = 0,
            .format = VK_FORMAT_R32G32B32_SFLOAT,
            .offset = offsetof(AS::PBR::Vertex, position),
        });

        // BaseColorUV
        inputAttributeDescriptions.emplace_back(VkVertexInputAttributeDescription{
            .location = static_cast<uint32_t>(inputAttributeDescriptions.size()),
            .binding = 0,
            .format = VK_FORMAT_R32G32_SFLOAT,
            .offset = offsetof(AS::PBR::Vertex, baseColorUV),
        });

        // HasSkin
        inputAttributeDescriptions.emplace_back(VkVertexInputAttributeDescription{
            .location = static_cast<uint32_t>(inputAttributeDescriptions.size()),
            .binding = 0,
            .format = VK_FORMAT_R32_SINT,
            .offset = offsetof(AS::PBR::Vertex, hasSkin), // TODO We should use a primitiveInfo instead
        });

        // JointIndices
        inputAttributeDescriptions.emplace_back(VkVertexInputAttributeDescription{
            .location = static_cast<uint32_t>(inputAttributeDescriptions.size()),
            .binding = 0,
            .format = VK_FORMAT_R32G32B32A32_SINT,
            .offset = offsetof(AS::PBR::Vertex, jointIndices),
        });

        // JointWeights
        inputAttributeDescriptions.emplace_back(VkVertexInputAttributeDescription{
            .location = static_cast<uint32_t>(inputAttributeDescriptions.size()),
            .binding = 0,
            .format = VK_FORMAT_R32G32B32A32_SFLOAT,
            .offset = offsetof(AS::PBR::Vertex, jointWeights),
        });

        // Pipeline layout
        std::vector<VkPushConstantRange> pushConstantRanges{};
        // Push constants  
        VkPushConstantRange pushConstantRange{
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
            .offset = 0,
            .size = sizeof(DepthPrePassPushConstants),
        };
        pushConstantRanges.emplace_back(pushConstantRange);

        const auto pipelineLayout = RF::CreatePipelineLayout(
            static_cast<uint32_t>(descriptorSetLayouts.size()),
            descriptorSetLayouts.data(),
            static_cast<uint32_t>(pushConstantRanges.size()),
            pushConstantRanges.data()
        );

        RT::CreateGraphicPipelineOptions graphicPipelineOptions{};
        graphicPipelineOptions.cullMode = VK_CULL_MODE_BACK_BIT;
        graphicPipelineOptions.rasterizationSamples = RF::GetMaxSamplesCount();
        graphicPipelineOptions.depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;

        graphicPipelineOptions.colorBlendAttachments.blendEnable = VK_FALSE;

        mDepthPassPipeline = RF::CreateGraphicPipeline(
            mDepthPrePass->GetVkRenderPass(),
            static_cast<uint8_t>(shaders.size()),
            shaders.data(),
            pipelineLayout,
            1,
            &vertexInputBindingDescription,
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

    void PBRWithShadowPipelineV2::createOcclusionQueryPipeline(std::vector<VkDescriptorSetLayout> const & descriptorSetLayouts)
    {
        RF_CREATE_SHADER("shaders/occlusion_query/Occlusion.vert.spv", Vertex)
        RF_CREATE_SHADER("shaders/occlusion_query/Occlusion.frag.spv", Fragment)

        std::vector<RT::GpuShader const *> shaders{ gpuVertexShader.get(), gpuFragmentShader.get() };

        VkVertexInputBindingDescription vertexInputBindingDescription{};
        vertexInputBindingDescription.binding = 0;
        vertexInputBindingDescription.stride = sizeof(AS::PBR::Vertex);
        vertexInputBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        std::vector<VkVertexInputAttributeDescription> inputAttributeDescriptions{};

        // Position
        inputAttributeDescriptions.emplace_back(VkVertexInputAttributeDescription{
            .location = static_cast<uint32_t>(inputAttributeDescriptions.size()),
            .binding = 0,
            .format = VK_FORMAT_R32G32B32_SFLOAT,
            .offset = offsetof(AS::PBR::Vertex, position),
        });

        // BaseColorUV
        inputAttributeDescriptions.emplace_back(VkVertexInputAttributeDescription{
            .location = static_cast<uint32_t>(inputAttributeDescriptions.size()),
            .binding = 0,
            .format = VK_FORMAT_R32G32_SFLOAT,
            .offset = offsetof(AS::PBR::Vertex, baseColorUV),
        });

        // HasSkin
        inputAttributeDescriptions.emplace_back(VkVertexInputAttributeDescription{
            .location = static_cast<uint32_t>(inputAttributeDescriptions.size()),
            .binding = 0,
            .format = VK_FORMAT_R32_SINT,
            .offset = offsetof(AS::PBR::Vertex, hasSkin), // TODO We should use a primitiveInfo instead
        });

        // JointIndices
        inputAttributeDescriptions.emplace_back(VkVertexInputAttributeDescription{
            .location = static_cast<uint32_t>(inputAttributeDescriptions.size()),
            .binding = 0,
            .format = VK_FORMAT_R32G32B32A32_SINT,
            .offset = offsetof(AS::PBR::Vertex, jointIndices),
        });

        // JointWeights
        inputAttributeDescriptions.emplace_back(VkVertexInputAttributeDescription{
            .location = static_cast<uint32_t>(inputAttributeDescriptions.size()),
            .binding = 0,
            .format = VK_FORMAT_R32G32B32A32_SFLOAT,
            .offset = offsetof(AS::PBR::Vertex, jointWeights),
        });

        // Pipeline layout
        std::vector<VkPushConstantRange> pushConstantRanges{};
        VkPushConstantRange pushConstantRange{
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
            .offset = 0,
            .size = sizeof(OcclusionPassPushConstants),
        };
        pushConstantRanges.emplace_back(pushConstantRange);

        const auto pipelineLayout = RF::CreatePipelineLayout(
            static_cast<uint32_t>(descriptorSetLayouts.size()),
            descriptorSetLayouts.data(),
            static_cast<uint32_t>(pushConstantRanges.size()),
            pushConstantRanges.data()
        );

        RT::CreateGraphicPipelineOptions graphicPipelineOptions{};
        graphicPipelineOptions.cullMode = VK_CULL_MODE_BACK_BIT;
        graphicPipelineOptions.rasterizationSamples = RF::GetMaxSamplesCount();
        graphicPipelineOptions.depthStencil.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
        graphicPipelineOptions.depthStencil.depthWriteEnable = VK_FALSE;

        mOcclusionQueryPipeline = RF::CreateGraphicPipeline(
            mOcclusionRenderPass->GetVkRenderPass(),
            static_cast<uint8_t>(shaders.size()),
            shaders.data(),
            pipelineLayout,
            1,
            &vertexInputBindingDescription,
            static_cast<uint8_t>(inputAttributeDescriptions.size()),
            inputAttributeDescriptions.data(),
            graphicPipelineOptions
        );
    }

    //-------------------------------------------------------------------------------------------------

    void PBRWithShadowPipelineV2::createUniformBuffers()
    {
        mErrorBuffer = RF::CreateLocalUniformBuffer(sizeof(PBR_Variant::JointTransformData), 1);
    }

    //-------------------------------------------------------------------------------------------------

}
