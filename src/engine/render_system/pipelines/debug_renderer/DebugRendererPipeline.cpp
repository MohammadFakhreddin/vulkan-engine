#include "DebugRendererPipeline.hpp"

#include "engine/BedrockAssert.hpp"
#include "engine/BedrockMatrix.hpp"
#include "tools/Importer.hpp"
#include "engine/BedrockPath.hpp"
#include "engine/render_system/render_passes/display_render_pass/DisplayRenderPass.hpp"
#include "engine/render_system/pipelines/DescriptorSetSchema.hpp"
#include "engine/entity_system/Entity.hpp"
#include "engine/entity_system/components/ColorComponent.hpp"
#include "engine/render_system/RenderFrontend.hpp"
#include "engine/render_system/pipelines/EssenceBase.hpp"
#include "engine/render_system/pipelines/VariantBase.hpp"
#include "engine/render_system/pipelines/pbr_with_shadow_v2/PBR_Essence.hpp"
#include "engine/render_system/pipelines/pbr_with_shadow_v2/PBR_Variant.hpp"
#include "engine/scene_manager/Scene.hpp"
#include "engine/scene_manager/SceneManager.hpp"

// TODO We need DebugVariant instead
#define CAST_VARIANT(variant)  static_cast<PBR_Variant *>(variant.get())

namespace MFA
{
    

    DebugRendererPipeline::DebugRendererPipeline()
        : BasePipeline(10)
    {}

    //-------------------------------------------------------------------------------------------------

    DebugRendererPipeline::~DebugRendererPipeline()
    {
        if (mIsInitialized)
        {
            Shutdown();
        }
    }

    //-------------------------------------------------------------------------------------------------

    void DebugRendererPipeline::Init()
    {
        if (mIsInitialized == true)
        {
            MFA_ASSERT(false);
            return;
        }
        mIsInitialized = true;

        BasePipeline::Init();

        createDescriptorSetLayout();
        createPipeline();
        createDescriptorSets();
    }

    //-------------------------------------------------------------------------------------------------

    void DebugRendererPipeline::Shutdown()
    {
        if (mIsInitialized == false)
        {
            MFA_ASSERT(false);
            return;
        }
        mIsInitialized = false;

        RF::DestroyPipelineGroup(mDrawPipeline);
        destroyDescriptorSetLayout();
       
        BasePipeline::Shutdown();
    }

    //-------------------------------------------------------------------------------------------------

    void DebugRendererPipeline::PreRender(RT::CommandRecordState & drawPass, float const deltaTime)
    {
        BasePipeline::PreRender(drawPass, deltaTime);
    }

    //-------------------------------------------------------------------------------------------------

    void DebugRendererPipeline::Render(RT::CommandRecordState & drawPass, float deltaTime)
    {
        RF::BindDrawPipeline(drawPass, mDrawPipeline);

        RF::BindDescriptorSet(
            drawPass,
            RenderFrontend::DescriptorSetType::PerFrame,
            mDescriptorSetGroup
        );

        PushConstants pushConstants{};

        for (auto const & essenceAndVariantList : mEssenceAndVariantsMap)
        {
            auto & essence = essenceAndVariantList.second.essence;
            auto & variantsList = essenceAndVariantList.second.variants;

            essence->BindVertexBuffer(drawPass);
            essence->BindIndexBuffer(drawPass);
            
            for (auto & variant : variantsList)
            {
                if (variant->IsActive())
                {
                    auto colorComponent = variant->GetEntity()->GetComponent<ColorComponent>().lock();
                    if (colorComponent != nullptr)
                    {
                        colorComponent->GetColor(pushConstants.baseColorFactor);
                    }

                    // We do not need primitive for debug stuff
                    CAST_VARIANT(variant)->Draw(
                        drawPass,
                        [&drawPass, &pushConstants](AS::PBR::Primitive const & primitive, PBR_Variant::Node const & node)-> void
                        {
                            Matrix::CopyGlmToCells(node.cachedModelTransform, pushConstants.model);

                            RF::PushConstants(
                                drawPass,
                                VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                                0,
                                CBlobAliasOf(pushConstants)
                            );
                        }
                    , AS::AlphaMode::Opaque);
                }
            }
        }
    }

    //-------------------------------------------------------------------------------------------------

    std::shared_ptr<EssenceBase> DebugRendererPipeline::internalCreateEssence(
        std::shared_ptr<RT::GpuModel> const & gpuModel,
        std::shared_ptr<AS::MeshBase> const & cpuMesh
    )
    {
        return std::make_shared<PBR_Essence>(gpuModel, cpuMesh);
    }

    //-------------------------------------------------------------------------------------------------

    std::shared_ptr<VariantBase> DebugRendererPipeline::internalCreateVariant(EssenceBase * essence)
    {
        MFA_ASSERT(essence != nullptr);
        return std::make_shared<PBR_Variant>(static_cast<PBR_Essence *>(essence));
    }

    //-------------------------------------------------------------------------------------------------

    void DebugRendererPipeline::createDescriptorSetLayout()
    {
        std::vector<VkDescriptorSetLayoutBinding> bindings{};

        // ModelViewProjection
        VkDescriptorSetLayoutBinding layoutBinding{
            .binding = static_cast<uint32_t>(bindings.size()),
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT
        };
        bindings.emplace_back(layoutBinding);

        MFA_VK_INVALID_ASSERT(mDescriptorSetLayout);
        mDescriptorSetLayout = RF::CreateDescriptorSetLayout(
            static_cast<uint8_t>(bindings.size()),
            bindings.data()
        );
    }

    //-------------------------------------------------------------------------------------------------

    void DebugRendererPipeline::destroyDescriptorSetLayout()
    {
        MFA_VK_VALID_ASSERT(mDescriptorSetLayout);
        RF::DestroyDescriptorSetLayout(mDescriptorSetLayout);
        MFA_VK_MAKE_NULL(mDescriptorSetLayout);
    }

    //-------------------------------------------------------------------------------------------------

    void DebugRendererPipeline::createPipeline()
    {
        // Vertex shader
        RF_CREATE_SHADER("shaders/debug_renderer/DebugRenderer.vert.spv", Vertex)

        // Fragment shader
        RF_CREATE_SHADER("shaders/debug_renderer/DebugRenderer.frag.spv", Fragment)

        std::vector<RT::GpuShader const *> shaders{ gpuVertexShader.get(), gpuFragmentShader.get() };

        VkVertexInputBindingDescription const bindingDescription{
            .binding = 0,
            .stride = sizeof(AS::PBR::Vertex),
            .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
        };

        std::vector<VkVertexInputAttributeDescription> inputAttributeDescriptions{};
        {// Position
            VkVertexInputAttributeDescription attributeDescription{};
            attributeDescription.location = static_cast<uint32_t>(inputAttributeDescriptions.size());
            attributeDescription.binding = 0;
            attributeDescription.format = VK_FORMAT_R32G32B32_SFLOAT;
            attributeDescription.offset = offsetof(AS::PBR::Vertex, position);

            inputAttributeDescriptions.emplace_back(attributeDescription);
        }
        MFA_ASSERT(mDrawPipeline.isValid() == false);

        RT::CreateGraphicPipelineOptions pipelineOptions{};
        pipelineOptions.useStaticViewportAndScissor = false;
        pipelineOptions.primitiveTopology = VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
        pipelineOptions.rasterizationSamples = RF::GetMaxSamplesCount();
        pipelineOptions.cullMode = VK_CULL_MODE_NONE;

        std::vector<VkPushConstantRange> pushConstantRanges{
            VkPushConstantRange {
                .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT,
                .offset = 0,
                .size = sizeof(PushConstants),
            }
        };
        pipelineOptions.pushConstantsRangeCount = static_cast<uint32_t>(pushConstantRanges.size());
        pipelineOptions.pushConstantRanges = pushConstantRanges.data();

        mDrawPipeline = RF::CreatePipeline(
            RF::GetDisplayRenderPass()->GetVkRenderPass(),
            static_cast<uint8_t>(shaders.size()),
            shaders.data(),
            1,
            &mDescriptorSetLayout,
            bindingDescription,
            static_cast<uint8_t>(inputAttributeDescriptions.size()),
            inputAttributeDescriptions.data(),
            pipelineOptions
        );
    }

    //-------------------------------------------------------------------------------------------------

    void DebugRendererPipeline::createDescriptorSets()
    {
        mDescriptorSetGroup = RF::CreateDescriptorSets(
            mDescriptorPool,
            RF::GetMaxFramesPerFlight(),
            mDescriptorSetLayout
        );

        // Idea: Maybe we can have active camera buffer inside scene manager
        auto const activeScene = SceneManager::GetActiveScene();
        MFA_ASSERT(activeScene != nullptr);

        auto const & cameraBufferCollection = activeScene->GetCameraBuffers();
        
        for (uint32_t frameIndex = 0; frameIndex < RF::GetMaxFramesPerFlight(); ++frameIndex)
        {

            auto const & descriptorSet = mDescriptorSetGroup.descriptorSets[frameIndex];
            MFA_VK_VALID_ASSERT(descriptorSet);

            DescriptorSetSchema descriptorSetSchema{ descriptorSet };

            /////////////////////////////////////////////////////////////////
            // Vertex shader
            /////////////////////////////////////////////////////////////////

            // ViewProjectionTransform
            VkDescriptorBufferInfo bufferInfo{
                .buffer = cameraBufferCollection.buffers[frameIndex]->buffer,
                .offset = 0,
                .range = cameraBufferCollection.bufferSize,
            };
            descriptorSetSchema.AddUniformBuffer(&bufferInfo);

            descriptorSetSchema.UpdateDescriptorSets();
        }
    }

    //-------------------------------------------------------------------------------------------------

}
