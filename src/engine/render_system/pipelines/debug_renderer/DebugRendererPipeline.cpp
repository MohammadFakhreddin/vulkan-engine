#include "DebugRendererPipeline.hpp"

#include "engine/BedrockAssert.hpp"
#include "engine/BedrockMatrix.hpp"
#include "tools/Importer.hpp"
#include "engine/BedrockPath.hpp"
#include "engine/render_system/render_passes/display_render_pass/DisplayRenderPass.hpp"
#include "engine/render_system/pipelines/DescriptorSetSchema.hpp"
#include "engine/entity_system/Entity.hpp"
#include "engine/render_system/RenderFrontend.hpp"
#include "engine/render_system/pipelines/EssenceBase.hpp"
#include "engine/render_system/pipelines/VariantBase.hpp"
#include "engine/render_system/pipelines/debug_renderer/DebugEssence.hpp"
#include "engine/render_system/pipelines/debug_renderer/DebugVariant.hpp"
#include "engine/scene_manager/SceneManager.hpp"
#include "engine/resource_manager/ResourceManager.hpp"
#include "engine/asset_system/AssetDebugMesh.hpp"
#include "engine/asset_system/AssetModel.hpp"
#include "engine/asset_system/AssetShader.hpp"

// Vertex input bindings
// The instancing pipeline uses a vertex input state with two bindings
//bindingDescriptions = {
//	// Binding point 0: Mesh vertex layout description at per-vertex rate
//	vks::initializers::vertexInputBindingDescription(VERTEX_BUFFER_BIND_ID, sizeof(vkglTF::Vertex), VK_VERTEX_INPUT_RATE_VERTEX),
//	// Binding point 1: Instanced data at per-instance rate
//	vks::initializers::vertexInputBindingDescription(INSTANCE_BUFFER_BIND_ID, sizeof(InstanceData), VK_VERTEX_INPUT_RATE_INSTANCE)
//};

// Vertex attribute bindings
// Note that the shader declaration for per-vertex and per-instance attributes is the same, the different input rates are only stored in the bindings:
// instanced.vert:
//	layout (location = 0) in vec3 inPos;		Per-Vertex
//	...
//	layout (location = 4) in vec3 instancePos;	Per-Instance
//attributeDescriptions = {
//	// Per-vertex attributes
//	// These are advanced for each vertex fetched by the vertex shader
//	vks::initializers::vertexInputAttributeDescription(VERTEX_BUFFER_BIND_ID, 0, VK_FORMAT_R32G32B32_SFLOAT, 0),					// Location 0: Position
//	vks::initializers::vertexInputAttributeDescription(VERTEX_BUFFER_BIND_ID, 1, VK_FORMAT_R32G32B32_SFLOAT, sizeof(float) * 3),	// Location 1: Normal
//	vks::initializers::vertexInputAttributeDescription(VERTEX_BUFFER_BIND_ID, 2, VK_FORMAT_R32G32_SFLOAT, sizeof(float) * 6),		// Location 2: Texture coordinates
//	vks::initializers::vertexInputAttributeDescription(VERTEX_BUFFER_BIND_ID, 3, VK_FORMAT_R32G32B32_SFLOAT, sizeof(float) * 8),	// Location 3: Color
//	// Per-Instance attributes
//	// These are fetched for each instance rendered
//	vks::initializers::vertexInputAttributeDescription(INSTANCE_BUFFER_BIND_ID, 4, VK_FORMAT_R32G32B32_SFLOAT, 0),					// Location 4: Position
//	vks::initializers::vertexInputAttributeDescription(INSTANCE_BUFFER_BIND_ID, 5, VK_FORMAT_R32G32B32_SFLOAT, sizeof(float) * 3),	// Location 5: Rotation
//	vks::initializers::vertexInputAttributeDescription(INSTANCE_BUFFER_BIND_ID, 6, VK_FORMAT_R32_SFLOAT,sizeof(float) * 6),			// Location 6: Scale
//	vks::initializers::vertexInputAttributeDescription(INSTANCE_BUFFER_BIND_ID, 7, VK_FORMAT_R32_SINT, sizeof(float) * 7),			// Location 7: Texture array layer index
//};

#define CAST_VARIANT(variant)   static_cast<DebugVariant *>(variant.get())
#define CAST_ESSENCE(essence)   static_cast<DebugEssence *>(essence.get())

namespace MFA
{

    using namespace AssetSystem::Debug;

    DebugRendererPipeline::DebugRendererPipeline()
        : BasePipeline(10)
    {}

    //-------------------------------------------------------------------------------------------------

    DebugRendererPipeline::~DebugRendererPipeline()
    {
        if (mIsInitialized)
        {
            shutdown();
        }
    }

    //-------------------------------------------------------------------------------------------------

    void DebugRendererPipeline::init()
    {
        MFA_ASSERT(JS::IsMainThread());
        BasePipeline::init();
        createDescriptorSetLayout();
        createPipeline();
        createDescriptorSets();

        std::vector<std::string> const modelNames{ "Sphere", "CubeStrip", "CubeFill" };

        for (auto const & modelName : modelNames)
        {
            RC::AcquireEssence(modelName, this, [modelName](bool success)->void{
                MFA_ASSERT(success);
            });
        }
    }

    //-------------------------------------------------------------------------------------------------

    void DebugRendererPipeline::shutdown()
    {
        BasePipeline::shutdown();
    }

    //-------------------------------------------------------------------------------------------------

    void DebugRendererPipeline::render(RT::CommandRecordState & recordState, float deltaTime)
    {
        RF::BindPipeline(recordState, *mPipeline);

        RF::AutoBindDescriptorSet(
            recordState,
            RenderFrontend::UpdateFrequency::PerFrame,
            mDescriptorSetGroup
        );

        PushConstants pushConstants{};

        for (auto const & essenceAndVariantList : mEssenceAndVariantsMap)
        {
            auto const * essence = CAST_ESSENCE(essenceAndVariantList.second.essence);
            auto & variantsList = essenceAndVariantList.second.variants;

            essence->bindForGraphicPipeline(recordState);
            
            for (auto & variant : variantsList)
            {
                if (variant->IsActive())
                {
                    auto * debugVariant = CAST_VARIANT(variant);
                    MFA_ASSERT(debugVariant != nullptr);
                    debugVariant->getColor(pushConstants.baseColorFactor);
                    debugVariant->getTransform(pushConstants.model);

                    RF::PushConstants(
                        recordState,
                        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                        0,
                        CBlobAliasOf(pushConstants)
                    );

                    CAST_VARIANT(variant)->Draw(recordState);
                }
            }
        }
    }

    //-------------------------------------------------------------------------------------------------

    void DebugRendererPipeline::onResize()
    {}

    //-------------------------------------------------------------------------------------------------

    void DebugRendererPipeline::freeUnusedEssences() {}

    //-------------------------------------------------------------------------------------------------

    std::weak_ptr<DebugEssence> DebugRendererPipeline::GetEssence(std::string const & nameId)
    {
        return std::static_pointer_cast<DebugEssence>(BasePipeline::GetEssence(nameId));
    }

    //-------------------------------------------------------------------------------------------------

    void DebugRendererPipeline::internalAddEssence(EssenceBase * essence)
    {
        MFA_ASSERT(dynamic_cast<DebugEssence *>(essence) != nullptr);
    }

    //-------------------------------------------------------------------------------------------------

    std::shared_ptr<VariantBase> DebugRendererPipeline::internalCreateVariant(EssenceBase * essence)
    {
        MFA_ASSERT(essence != nullptr);
        return std::make_shared<DebugVariant>(static_cast<DebugEssence *>(essence));
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

        MFA_ASSERT(mDescriptorSetLayout == VK_NULL_HANDLE);
        mDescriptorSetLayout = RF::CreateDescriptorSetLayout(
            static_cast<uint8_t>(bindings.size()),
            bindings.data()
        );
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
            .stride = sizeof(Vertex),
            .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
        };

        std::vector<VkVertexInputAttributeDescription> inputAttributeDescriptions{};
        // Position
        inputAttributeDescriptions.emplace_back(VkVertexInputAttributeDescription{
            .location = static_cast<uint32_t>(inputAttributeDescriptions.size()),
            .binding = 0,
            .format = VK_FORMAT_R32G32B32_SFLOAT,
            .offset = offsetof(Vertex, position),
        });

        RT::CreateGraphicPipelineOptions pipelineOptions{};
        pipelineOptions.useStaticViewportAndScissor = false;
        pipelineOptions.primitiveTopology = VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
        // TODO I think we should submit each pipeline . Each one should have independent depth buffer 
        pipelineOptions.rasterizationSamples = RF::GetMaxSamplesCount();            // TODO Find a way to set sample count to 1. We only need MSAA for pbr-pipeline
        pipelineOptions.cullMode = VK_CULL_MODE_NONE;
        pipelineOptions.colorBlendAttachments.blendEnable = VK_FALSE;

        // pipeline layout
        std::vector<VkPushConstantRange> const pushConstantRanges{
            VkPushConstantRange {
                .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT,
                .offset = 0,
                .size = sizeof(PushConstants),
            }
        };
        const auto pipelineLayout = RF::CreatePipelineLayout(
            1,
            &mDescriptorSetLayout->descriptorSetLayout,
            static_cast<uint32_t>(pushConstantRanges.size()),
            pushConstantRanges.data()
        );
        mPipeline = RF::CreateGraphicPipeline(
            RF::GetDisplayRenderPass()->GetVkRenderPass(),
            static_cast<uint8_t>(shaders.size()),
            shaders.data(),
            pipelineLayout,
            1,
            &bindingDescription,
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
            *mDescriptorSetLayout
        );

        auto const * cameraBufferGroup = SceneManager::GetCameraBuffers();

        for (uint32_t frameIndex = 0; frameIndex < RF::GetMaxFramesPerFlight(); ++frameIndex)
        {

            auto const & descriptorSet = mDescriptorSetGroup.descriptorSets[frameIndex];
            MFA_ASSERT(descriptorSet != VK_NULL_HANDLE);

            DescriptorSetSchema descriptorSetSchema{ descriptorSet };

            /////////////////////////////////////////////////////////////////
            // Vertex shader
            /////////////////////////////////////////////////////////////////

            // ViewProjectionTransform
            VkDescriptorBufferInfo bufferInfo{
                .buffer = cameraBufferGroup->buffers[frameIndex]->buffer,
                .offset = 0,
                .range = cameraBufferGroup->bufferSize,
            };
            descriptorSetSchema.AddUniformBuffer(&bufferInfo);

            descriptorSetSchema.UpdateDescriptorSets();
        }
    }

    //-------------------------------------------------------------------------------------------------

}
