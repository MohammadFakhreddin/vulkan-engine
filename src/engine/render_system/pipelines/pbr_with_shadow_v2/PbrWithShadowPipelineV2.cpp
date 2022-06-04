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
#include "engine/asset_system/AssetDebugMesh.hpp"
#include "engine/render_system/pipelines/debug_renderer/DebugRendererPipeline.hpp"

#define CAST_ESSENCE_PURE(essence)      static_cast<PBR_Essence *>(essence)
#define CAST_ESSENCE_SHARED(essence)    CAST_ESSENCE_PURE(essence.get())
#define CAST_VARIANT_PURE(variant)      static_cast<PBR_Variant *>(variant)
#define CAST_VARIANT_SHARED(variant)    static_cast<PBR_Variant *>(variant.get())

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

    // Steps for multi-threading. Begin render pass -> submit subcommands -> execute into primary end renderPass
    // TODO: Submit in multiple threads


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

        auto * debugPipeline = SceneManager::GetPipeline<DebugRendererPipeline>();
        MFA_ASSERT(debugPipeline != nullptr);
        mOcclusionEssence = debugPipeline->GetEssence("CubeFill");

        createUniformBuffers();

        {// Graphic
            mPointLightShadowRenderPass->Init();
            mPointLightShadowResources->Init(mPointLightShadowRenderPass->GetVkRenderPass());

            mDirectionalLightShadowRenderPass->Init();
            mDirectionalLightShadowResources->Init(mDirectionalLightShadowRenderPass->GetVkRenderPass());
            
            mDepthPrePass->Init();
            mOcclusionRenderPass->Init();

            createGfxPerFrameDescriptorSetLayout();
            createGfxPerEssenceDescriptorSetLayout();
            
            auto const descriptorSetLayouts = std::vector<VkDescriptorSetLayout>{
                mGfxPerFrameDescriptorSetLayout->descriptorSetLayout,
                mGfxPerEssenceDescriptorSetLayout->descriptorSetLayout,
            };

            createDisplayPassPipeline(descriptorSetLayouts);
            createPointLightShadowPassPipeline(descriptorSetLayouts);
            createDirectionalLightShadowPassPipeline(descriptorSetLayouts);
            createDepthPassPipeline(descriptorSetLayouts);
            createOcclusionQueryPipeline(descriptorSetLayouts);

            createGfxPerFrameDescriptorSets();

            createOcclusionQueryPool();
        }

        {// Compute
            createSkinningPerEssenceDescriptorSetLayout();
            createSkinningPerVariantDescriptorSetLayout();
            auto const descriptorSetLayouts = std::vector<VkDescriptorSetLayout>{
                mSkinningPerEssenceDescriptorSetLayout->descriptorSetLayout,
                mSkinningPerVariantDescriptorSetLayout->descriptorSetLayout
            };
            createSkinningPipeline(descriptorSetLayouts);
        }
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

    void PBRWithShadowPipelineV2::compute(RT::CommandRecordState & recordState, float const deltaTime)
    {
        BasePipeline::compute(recordState, deltaTime);

        updateVariantsBuffers(recordState);

        preComputeBarrier(recordState);

        performSkinning(recordState);

        postComputeBarrier(recordState);
    }

    //-------------------------------------------------------------------------------------------------

    void PBRWithShadowPipelineV2::updateVariantsBuffers(RT::CommandRecordState const & recordState) const
    {
        // Multi-thread update of variant animation
        JS::AssignTaskPerThread([this, &recordState](uint32_t const threadNumber, uint32_t const threadCount)->void
        {
            for (uint32_t i = threadNumber; i < static_cast<uint32_t>(mAllVariantsList.size()); i += threadCount)
            {
                CAST_VARIANT_PURE(mAllVariantsList[i])->updateBuffers(recordState);
            }
        });
        JS::WaitForThreadsToFinish();
    }

    //-------------------------------------------------------------------------------------------------

    void PBRWithShadowPipelineV2::performSkinning(RT::CommandRecordState & recordState)
    {
        RF::BindPipeline(recordState, *mSkinningPipeline);

        SkinningPushConstants pushConstants{};
        
        for (auto const & essenceAndVariantList : mEssenceAndVariantsMap)
        {
            auto const * essence = CAST_ESSENCE_SHARED(essenceAndVariantList.second.essence);
            auto & variantsList = essenceAndVariantList.second.variants;

            if (variantsList.empty())
            {
                continue;
            }

            essence->bindForComputePipeline(recordState);

            for (auto & variant : variantsList)
            {
                if (variant->IsVisible())
                {
                    // Dispatches and bindings are done here
                    CAST_VARIANT_SHARED(variant)->compute(recordState,
                        [&recordState, &pushConstants](
                            AS::PBR::Primitive const & primitive,
                            PBR_Variant::Node const & node
                        )-> void
                        {
                            // Vertex push constants
                            Matrix::CopyGlmToCells(node.cachedGlobalInverseTransform, pushConstants.inverseNodeTransform);
                            Matrix::CopyGlmToCells(node.cachedModelTransform, pushConstants.model);
                            pushConstants.vertexCount = primitive.vertexCount;
                            pushConstants.skinIndex = primitive.hasSkin ? node.skin->skinStartingIndex : -1;
                            pushConstants.vertexStartingIndex = primitive.verticesStartingIndex;

                            RF::PushConstants(
                                recordState,
                                VK_SHADER_STAGE_COMPUTE_BIT,
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

    void PBRWithShadowPipelineV2::preComputeBarrier(RT::CommandRecordState const & recordState) const
    {
        if (RF::GetComputeQueueFamily() != RF::GetGraphicQueueFamily())
        {
            std::vector<VkBufferMemoryBarrier> barriers {};

            for (auto * variant : mAllVariantsList)
            {
                CAST_VARIANT_PURE(variant)->preComputeBarrier(barriers);
            }

            RF::PipelineBarrier(
                recordState,
                VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
                VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                static_cast<uint32_t>(barriers.size()),
                barriers.data()
            );
        }
    }
    
    //-------------------------------------------------------------------------------------------------

    void PBRWithShadowPipelineV2::postComputeBarrier(RT::CommandRecordState const & recordState)
    {
        if (RF::GetComputeQueueFamily() != RF::GetGraphicQueueFamily())
        {
            std::vector<VkBufferMemoryBarrier> barriers {};

            for (auto * variant : mAllVariantsList)
            {
                CAST_VARIANT_PURE(variant)->preRenderBarrier(barriers);
            }

            RF::PipelineBarrier(
                recordState,
                VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
                static_cast<uint32_t>(barriers.size()),
                barriers.data()
            );
        }
    }

    //-------------------------------------------------------------------------------------------------

    void PBRWithShadowPipelineV2::postRenderVariants(float deltaTimeInSec) const
    {
        // Multi-thread update of variant animation
        JS::AssignTaskPerThread([this, deltaTimeInSec](uint32_t const threadNumber, uint32_t const threadCount)->void
        {
            for (uint32_t i = threadNumber; i < static_cast<uint32_t>(mAllVariantsList.size()); i += threadCount)
            {
                CAST_VARIANT_PURE(mAllVariantsList[i])->postRender(deltaTimeInSec);
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
        auto * pbrEssence = CAST_ESSENCE_PURE(essence);
        pbrEssence->createGraphicDescriptorSet(
            mDescriptorPool,
            mGfxPerEssenceDescriptorSetLayout->descriptorSetLayout,
            *mErrorTexture
        );
        pbrEssence->createComputeDescriptorSet(
            mDescriptorPool,
            mSkinningPerEssenceDescriptorSetLayout->descriptorSetLayout
        );
    }

    //-------------------------------------------------------------------------------------------------

    std::shared_ptr<VariantBase> PBRWithShadowPipelineV2::internalCreateVariant(EssenceBase * essence)
    {
        auto * drawableEssence = dynamic_cast<PBR_Essence *>(essence);
        MFA_ASSERT(drawableEssence != nullptr);
        auto variant = std::make_shared<PBR_Variant>(drawableEssence);
        variant->createComputeDescriptorSet(
            mDescriptorPool,
            mSkinningPerVariantDescriptorSetLayout->descriptorSetLayout,
            *mErrorBuffer
        );
        return variant;
    }
    
    //-------------------------------------------------------------------------------------------------

    void PBRWithShadowPipelineV2::createGfxPerFrameDescriptorSets()
    {
        mGfxPerFrameDescriptorSetGroup = RF::CreateDescriptorSets(
            mDescriptorPool,
            RF::GetMaxFramesPerFlight(),
            *mGfxPerFrameDescriptorSetLayout
        );

        auto const * cameraBufferCollection = SceneManager::GetCameraBuffers();
        auto const * directionalLightBuffers = SceneManager::GetDirectionalLightBuffers();
        auto const * pointLightBuffers = SceneManager::GetPointLightsBuffers();
        
        for (uint32_t frameIndex = 0; frameIndex < RF::GetMaxFramesPerFlight(); ++frameIndex)
        {

            auto const descriptorSet = mGfxPerFrameDescriptorSetGroup.descriptorSets[frameIndex];
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
            mGfxPerFrameDescriptorSetGroup
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
                    CAST_VARIANT_SHARED(variant)->render(
                        recordState,
                        [&recordState, &pushConstants](
                            AS::PBR::Primitive const & primitive,
                            PBR_Variant::Node const & node
                        )-> void
                        {
                            // Vertex push constants
                            pushConstants.primitiveIndex = primitive.uniqueId;
                            
                            RF::PushConstants(
                                recordState,
                                VK_SHADER_STAGE_FRAGMENT_BIT,
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
        auto const lightCount = SceneManager::GetDirectionalLightCount();
        if (lightCount <= 0)
        {
            return;
        }

        RF::BindPipeline(recordState, *mDirectionalLightShadowPipeline);
        RF::AutoBindDescriptorSet(
            recordState,
            RenderFrontend::UpdateFrequency::PerFrame,
            mGfxPerFrameDescriptorSetGroup
        );

        mDirectionalLightShadowRenderPass->BeginRenderPass(recordState, *mDirectionalLightShadowResources);

        DirectionalLightPushConstants pushConstants {};

        for (int lightIndex = 0; lightIndex < static_cast<int>(lightCount); ++lightIndex)
        {
            pushConstants.lightIndex = lightIndex;

            RF::PushConstants(
                recordState,
                AssetSystem::ShaderStage::Vertex,
                0,
                CBlobAliasOf(pushConstants)
            );

            renderForDirectionalLightShadowPass(recordState, AS::AlphaMode::Opaque);
            renderForDirectionalLightShadowPass(recordState, AS::AlphaMode::Mask);
            renderForDirectionalLightShadowPass(recordState, AS::AlphaMode::Blend);
        }

        mDirectionalLightShadowRenderPass->EndRenderPass(recordState);
    }

    //-------------------------------------------------------------------------------------------------

    void PBRWithShadowPipelineV2::renderForDirectionalLightShadowPass(RT::CommandRecordState const & recordState, AS::AlphaMode const alphaMode) const
    {
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
                    CAST_VARIANT_SHARED(variant)->render(recordState, nullptr, alphaMode);
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
            mGfxPerFrameDescriptorSetGroup
        );

        mPointLightShadowRenderPass->BeginRenderPass(recordState, *mPointLightShadowResources);

        PointLightShadowPassPushConstants pushConstants {};

        auto const & pointLights = SceneManager::GetActivePointLights();
        MFA_ASSERT(pointLights.size() == pointLightCount);

        for (int lightIndex = 0; lightIndex < static_cast<int>(pointLightCount); ++lightIndex)
        {
            pushConstants.lightIndex = lightIndex;
            renderForPointLightShadowPass(recordState, AS::AlphaMode::Opaque, pushConstants, pointLights[lightIndex]);
            renderForPointLightShadowPass(recordState, AS::AlphaMode::Mask, pushConstants, pointLights[lightIndex]);
            renderForPointLightShadowPass(recordState, AS::AlphaMode::Blend, pushConstants, pointLights[lightIndex]);
        }

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
    
    void PBRWithShadowPipelineV2::renderForPointLightShadowPass(
        RT::CommandRecordState const & recordState,
        AS::AlphaMode alphaMode,
        PointLightShadowPassPushConstants & pushConstants,
        PointLightComponent const * pointLight
    ) const
    {
        
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

                auto const bvComponent = variant->GetBoundingVolume();
                if (bvComponent == nullptr)
                {
                    continue;
                }

                // We only render variants that are within pointLight's visible range
                if (pointLight->IsBoundingVolumeInRange(bvComponent.get()) == false)
                {
                    continue;
                }

                for (int faceIndex = 0; faceIndex < 6; ++faceIndex)
                {
                    pushConstants.faceIndex = faceIndex;

                    RF::PushConstants(
                        recordState,
                        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                        0,
                        CBlobAliasOf(pushConstants)
                    );

                    CAST_VARIANT_SHARED(variant)->render(recordState, nullptr, alphaMode);
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
            mGfxPerFrameDescriptorSetGroup
        );

        mOcclusionRenderPass->BeginRenderPass(recordState);
        renderForOcclusionQueryPass(recordState);
        mOcclusionRenderPass->EndRenderPass(recordState);
    }

    //-------------------------------------------------------------------------------------------------
    
    void PBRWithShadowPipelineV2::renderForOcclusionQueryPass(RT::CommandRecordState const & recordState)
    {
        auto & occlusionQueryData = mOcclusionQueryDataList[recordState.frameIndex];

        auto const occlusionEssence = mOcclusionEssence.lock();
        MFA_ASSERT(occlusionEssence != nullptr);

        occlusionEssence->bindForGraphicPipeline(recordState);

        OcclusionPassPushConstants pushConstants{};

        for (auto const & essenceAndVariantList : mEssenceAndVariantsMap)
        {
            auto & variantsList = essenceAndVariantList.second.variants;

            if (variantsList.empty())
            {
                continue;
            }
            
            for (auto & variant : variantsList)
            {
                if (variant->IsActive() && variant->IsInFrustum())
                {
                    auto bvComp = variant->GetBoundingVolume();

                    if (bvComp != nullptr && bvComp->OcclusionEnabled())
                    {
                        if (auto const transformC = bvComp->GetVolumeTransform().lock())
                        {
                            RF::BeginQuery(
                                recordState,
                                occlusionQueryData.Pool,
                                static_cast<uint32_t>(occlusionQueryData.Variants.size())
                            );

                            Matrix::CopyGlmToCells(
                                transformC->GetTransform(),
                                pushConstants.model
                            );

                            RF::PushConstants(
                                recordState,
                                AS::ShaderStage::Vertex,
                                0,
                                CBlobAliasOf(pushConstants)
                            );

                            RF::DrawIndexed(recordState, occlusionEssence->getIndicesCount());

                            RF::EndQuery(
                                recordState,
                                occlusionQueryData.Pool,
                                static_cast<uint32_t>(occlusionQueryData.Variants.size())
                            );

                            // What if the variant is destroyed in next frame?
                            occlusionQueryData.Variants.emplace_back(variant);
                        }
                    }

                }
            }
        }
    }

    //-------------------------------------------------------------------------------------------------

    void PBRWithShadowPipelineV2::performDisplayPass(RT::CommandRecordState & recordState) const
    {
        RF::BindPipeline(recordState, *mDisplayPassPipeline);
        RF::AutoBindDescriptorSet(
            recordState,
            RenderFrontend::UpdateFrequency::PerFrame,
            mGfxPerFrameDescriptorSetGroup
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
                    // TODO We can render all instances at once and have a large push constant for all of them
                    CAST_VARIANT_SHARED(variant)->render(
                        recordState,
                        [&recordState, &pushConstants](
                            AS::PBR::Primitive const & primitive,
                            PBR_Variant::Node const & node
                        )-> void
                        {
                            // Push constants
                            pushConstants.primitiveIndex = primitive.uniqueId;
                            RF::PushConstants(
                                recordState,
                                VK_SHADER_STAGE_FRAGMENT_BIT,
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

    void PBRWithShadowPipelineV2::createGfxPerFrameDescriptorSetLayout()
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

        mGfxPerFrameDescriptorSetLayout = RF::CreateDescriptorSetLayout(
            static_cast<uint8_t>(bindings.size()),
            bindings.data()
        );
    }

    //-------------------------------------------------------------------------------------------------

    void PBRWithShadowPipelineV2::createGfxPerEssenceDescriptorSetLayout()
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
            .descriptorCount = PBR_Essence::MAX_TEXTURE_COUNT,
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
        });
        
        mGfxPerEssenceDescriptorSetLayout = RF::CreateDescriptorSetLayout(
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

        std::vector<VkVertexInputBindingDescription> bindingDescriptions {};

        bindingDescriptions.emplace_back(VkVertexInputBindingDescription {
            .binding = static_cast<uint32_t>(bindingDescriptions.size()),
            .stride = sizeof(PBR_Variant::SkinnedVertex),
            .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
        });

        bindingDescriptions.emplace_back(VkVertexInputBindingDescription {
            .binding = static_cast<uint32_t>(bindingDescriptions.size()),
            .stride = sizeof(PBR_Essence::VertexUVs),
            .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
        });
        
        std::vector<VkVertexInputAttributeDescription> inputAttributeDescriptions{};
        
        // WordPosition
        inputAttributeDescriptions.emplace_back(VkVertexInputAttributeDescription {
            .location = static_cast<uint32_t>(inputAttributeDescriptions.size()),
            .binding = 0,
            .format = VK_FORMAT_R32G32B32A32_SFLOAT,
            .offset = offsetof(PBR_Variant::SkinnedVertex, worldPosition)
        });

        // WorldNormal
        inputAttributeDescriptions.emplace_back(VkVertexInputAttributeDescription {
            .location = static_cast<uint32_t>(inputAttributeDescriptions.size()),
            .binding = 0,
            .format = VK_FORMAT_R32G32B32_SFLOAT,
            .offset = offsetof(PBR_Variant::SkinnedVertex, worldNormal),
        });

        // WorldTangent
        inputAttributeDescriptions.emplace_back(VkVertexInputAttributeDescription {
            .location = static_cast<uint32_t>(inputAttributeDescriptions.size()),
            .binding = 0,
            .format = VK_FORMAT_R32G32B32_SFLOAT,
            .offset = offsetof(PBR_Variant::SkinnedVertex, worldTangent),
        });

        // WorldBiTangent
        inputAttributeDescriptions.emplace_back(VkVertexInputAttributeDescription {
            .location = static_cast<uint32_t>(inputAttributeDescriptions.size()),
            .binding = 0,
            .format = VK_FORMAT_R32G32B32_SFLOAT,
            .offset = offsetof(PBR_Variant::SkinnedVertex, worldBiTangent),
        });
        
        // BaseColorUV
        inputAttributeDescriptions.emplace_back(VkVertexInputAttributeDescription {
            .location = static_cast<uint32_t>(inputAttributeDescriptions.size()),
            .binding = 1,
            .format = VK_FORMAT_R32G32_SFLOAT,
            .offset = offsetof(PBR_Essence::VertexUVs, baseColorTexCoord)
        });

        // Metallic/RoughnessUV
        inputAttributeDescriptions.emplace_back(VkVertexInputAttributeDescription {
            .location = static_cast<uint32_t>(inputAttributeDescriptions.size()),
            .binding = 1,
            .format = VK_FORMAT_R32G32_SFLOAT,
            .offset = offsetof(PBR_Essence::VertexUVs, metallicRoughnessTexCoord), // Metallic and roughness have same uv for gltf files  
        });
        
        // NormalMapUV
        inputAttributeDescriptions.emplace_back(VkVertexInputAttributeDescription {
            .location = static_cast<uint32_t>(inputAttributeDescriptions.size()),
            .binding = 1,
            .format = VK_FORMAT_R32G32_SFLOAT,
            .offset = offsetof(PBR_Essence::VertexUVs, normalTexCoord),
        });
    
        // EmissionUV
        inputAttributeDescriptions.emplace_back(VkVertexInputAttributeDescription {
            .location = static_cast<uint32_t>(inputAttributeDescriptions.size()),
            .binding = 1,
            .format = VK_FORMAT_R32G32_SFLOAT,
            .offset = offsetof(PBR_Essence::VertexUVs, emissiveTexCoord),
        });

        // OcclusionUV
        inputAttributeDescriptions.emplace_back(VkVertexInputAttributeDescription {
            .location = static_cast<uint32_t>(inputAttributeDescriptions.size()),
            .binding = 1,
            .format = VK_FORMAT_R32G32_SFLOAT,
            .offset = offsetof(PBR_Essence::VertexUVs, occlusionTexCoord), // Metallic and roughness have same uv for gltf files  
        });
        
        std::vector<VkPushConstantRange> pushConstantRanges{};
        pushConstantRanges.emplace_back(VkPushConstantRange{
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
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
            static_cast<uint32_t>(bindingDescriptions.size()),
            bindingDescriptions.data(),
            static_cast<uint8_t>(inputAttributeDescriptions.size()),
            inputAttributeDescriptions.data(),
            options
        );
    }

    //-------------------------------------------------------------------------------------------------

    void PBRWithShadowPipelineV2::createDirectionalLightShadowPassPipeline(std::vector<VkDescriptorSetLayout> const & descriptorSetLayouts)
    {
        // Vertex shader
        RF_CREATE_SHADER("shaders/directional_light_shadow_v2/DirectionalLightShadowV2.vert.spv", Vertex)
        
        std::vector<RT::GpuShader const *> shaders{ gpuVertexShader.get()};

        VkVertexInputBindingDescription const vertexInputBindingDescription{
            .binding = 0,
            .stride = sizeof(PBR_Variant::SkinnedVertex),
            .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
        };

        std::vector<VkVertexInputAttributeDescription> inputAttributeDescriptions{};

        // WorldPosition
        inputAttributeDescriptions.emplace_back(VkVertexInputAttributeDescription {
            .location = static_cast<uint32_t>(inputAttributeDescriptions.size()),
            .binding = 0,
            .format = VK_FORMAT_R32G32B32A32_SFLOAT,
            .offset = offsetof(PBR_Variant::SkinnedVertex, worldPosition),
        });

        std::vector<VkPushConstantRange> pushConstantRanges{};
        pushConstantRanges.emplace_back(VkPushConstantRange{
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
            .offset = 0,
            .size = sizeof(DirectionalLightPushConstants),
        });

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
        RF_CREATE_SHADER("shaders/point_light_shadow_v2/PointLightShadowV2.vert.spv", Vertex)
        RF_CREATE_SHADER("shaders/point_light_shadow_v2/PointLightShadowV2.frag.spv", Fragment)

        std::vector<RT::GpuShader const *> shaders {
            gpuVertexShader.get(),
            gpuFragmentShader.get()
        };

        VkVertexInputBindingDescription const vertexInputBindingDescription{
            .binding = 0,
            .stride = sizeof(PBR_Variant::SkinnedVertex),
            .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
        };

        std::vector<VkVertexInputAttributeDescription> inputAttributeDescriptions{};

        // WorldPosition
        inputAttributeDescriptions.emplace_back(VkVertexInputAttributeDescription {
            .location = static_cast<uint32_t>(inputAttributeDescriptions.size()),
            .binding = 0,
            .format = VK_FORMAT_R32G32B32A32_SFLOAT,
            .offset = offsetof(PBR_Variant::SkinnedVertex, worldPosition),
        });

        std::vector<VkPushConstantRange> pushConstantRanges{};
        VkPushConstantRange pushConstantRange{
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
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

        std::vector<VkVertexInputBindingDescription> bindingDescriptions {};

        bindingDescriptions.emplace_back(VkVertexInputBindingDescription {
            .binding = static_cast<uint32_t>(bindingDescriptions.size()),
            .stride = sizeof(PBR_Variant::SkinnedVertex),
            .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
        });

        bindingDescriptions.emplace_back(VkVertexInputBindingDescription {
            .binding = static_cast<uint32_t>(bindingDescriptions.size()),
            .stride = sizeof(PBR_Essence::VertexUVs),
            .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
        });

        std::vector<VkVertexInputAttributeDescription> inputAttributeDescriptions{};

        // WorldPosition
        inputAttributeDescriptions.emplace_back(VkVertexInputAttributeDescription {
            .location = static_cast<uint32_t>(inputAttributeDescriptions.size()),
            .binding = 0,
            .format = VK_FORMAT_R32G32B32A32_SFLOAT,
            .offset = offsetof(PBR_Variant::SkinnedVertex, worldPosition),
        });

        // BaseColorUV
        inputAttributeDescriptions.emplace_back(VkVertexInputAttributeDescription {
            .location = static_cast<uint32_t>(inputAttributeDescriptions.size()),
            .binding = 1,
            .format = VK_FORMAT_R32G32_SFLOAT,
            .offset = offsetof(PBR_Essence::VertexUVs, baseColorTexCoord)
        });

        // Pipeline layout
        std::vector<VkPushConstantRange> pushConstantRanges{};
        // Push constants  
        VkPushConstantRange pushConstantRange{
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
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
            static_cast<uint32_t>(bindingDescriptions.size()),
            bindingDescriptions.data(),
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

    void PBRWithShadowPipelineV2::createSkinningPerEssenceDescriptorSetLayout()
    {
        std::vector<VkDescriptorSetLayoutBinding> bindings{};

        /////////////////////////////////////////////////////////////////
        // Compute shader
        /////////////////////////////////////////////////////////////////

        // RawVertices
        bindings.emplace_back(VkDescriptorSetLayoutBinding {
            .binding = static_cast<uint32_t>(bindings.size()),
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
        });
        
        mSkinningPerEssenceDescriptorSetLayout = RF::CreateDescriptorSetLayout(
            static_cast<uint8_t>(bindings.size()),
            bindings.data()
        );
    }

    //-------------------------------------------------------------------------------------------------

    void PBRWithShadowPipelineV2::createSkinningPerVariantDescriptorSetLayout()
    {
        std::vector<VkDescriptorSetLayoutBinding> bindings{};

        /////////////////////////////////////////////////////////////////
        // Compute shader
        /////////////////////////////////////////////////////////////////

        // SkinJoints
        bindings.emplace_back(VkDescriptorSetLayoutBinding {
            .binding = static_cast<uint32_t>(bindings.size()),
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
        });

        // SkinnedVertices
        bindings.emplace_back(VkDescriptorSetLayoutBinding {
            .binding = static_cast<uint32_t>(bindings.size()),
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
        });

        mSkinningPerVariantDescriptorSetLayout = RF::CreateDescriptorSetLayout(
            static_cast<uint8_t>(bindings.size()),
            bindings.data()
        );
    }

    //-------------------------------------------------------------------------------------------------

    void PBRWithShadowPipelineV2::createSkinningPipeline(std::vector<VkDescriptorSetLayout> const & descriptorSetLayouts)
    {
        RF_CREATE_SHADER("shaders/skinning/Skinning.comp.spv", Compute)

        // Push constants  
        std::vector<VkPushConstantRange> pushConstantRanges{};
        VkPushConstantRange pushConstantRange{
            .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
            .offset = 0,
            .size = sizeof(SkinningPushConstants),
        };
        pushConstantRanges.emplace_back(pushConstantRange);

        auto const pipelineLayout = RF::CreatePipelineLayout(
            static_cast<uint32_t>(descriptorSetLayouts.size()),
            descriptorSetLayouts.data(),
            static_cast<uint32_t>(pushConstantRanges.size()),
            pushConstantRanges.data()
        );

        mSkinningPipeline = RF::CreateComputePipeline(*gpuComputeShader, pipelineLayout);
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
        
        std::vector<RT::GpuShader const *> shaders{ gpuVertexShader.get() };

        std::vector<VkVertexInputBindingDescription> bindingDescriptions {};

        bindingDescriptions.emplace_back(VkVertexInputBindingDescription {
            .binding = static_cast<uint32_t>(bindingDescriptions.size()),
            .stride = sizeof(AS::Debug::Vertex),
            .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
        });

        std::vector<VkVertexInputAttributeDescription> inputAttributeDescriptions{};

        // Position
        inputAttributeDescriptions.emplace_back(VkVertexInputAttributeDescription {
            .location = static_cast<uint32_t>(inputAttributeDescriptions.size()),
            .binding = 0,
            .format = VK_FORMAT_R32G32B32_SFLOAT,
            .offset = offsetof(AS::Debug::Vertex, position),
        });

        // Pipeline layout
        std::vector<VkPushConstantRange> pushConstantRanges{};
        VkPushConstantRange pushConstantRange{
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
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
        graphicPipelineOptions.cullMode = VK_CULL_MODE_NONE;
        graphicPipelineOptions.rasterizationSamples = RF::GetMaxSamplesCount();
        graphicPipelineOptions.depthStencil.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
        graphicPipelineOptions.depthStencil.depthWriteEnable = VK_FALSE;

        mOcclusionQueryPipeline = RF::CreateGraphicPipeline(
            mOcclusionRenderPass->GetVkRenderPass(),
            static_cast<uint8_t>(shaders.size()),
            shaders.data(),
            pipelineLayout,
            static_cast<uint32_t>(bindingDescriptions.size()),
            bindingDescriptions.data(),
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
