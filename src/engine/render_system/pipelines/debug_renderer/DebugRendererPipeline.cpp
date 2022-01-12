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
#include "engine/scene_manager/Scene.hpp"
#include "engine/scene_manager/SceneManager.hpp"
#include "engine/job_system/JobSystem.hpp"
#include "engine/resource_manager/ResourceManager.hpp"
#include "engine/asset_system/AssetDebugMesh.hpp"

// TODO Essences should be able to do instancing
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



#define CAST_VARIANT(variant)  static_cast<DebugVariant *>(variant.get())

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
        
        BasePipeline::Init();

        createDescriptorSetLayout();
        createPipeline();
        createDescriptorSets();

        std::vector<std::string> const modelNames {"Sphere", "Cube"};
        for (auto const & modelName : modelNames)
        {
            auto const cpuModel = RC::AcquireForCpu(modelName.c_str());
            MFA_ASSERT(cpuModel != nullptr);
            auto const gpuModel = RC::AcquireForGpu(modelName.c_str());
            MFA_ASSERT(gpuModel != nullptr);
            CreateEssenceWithoutModel(gpuModel, cpuModel->mesh->getIndexCount());
        }
    }

    //-------------------------------------------------------------------------------------------------

    void DebugRendererPipeline::Shutdown()
    {
        if (mIsInitialized == false)
        {
            MFA_ASSERT(false);
            return;
        }
        
        RF::DestroyPipelineGroup(mDrawPipeline);
        
        BasePipeline::Shutdown();
    }

    //-------------------------------------------------------------------------------------------------

    void DebugRendererPipeline::PreRender(RT::CommandRecordState & recordState, float const deltaTimeInSec)
    {
        BasePipeline::PreRender(recordState, deltaTimeInSec);

        // Temporary: Will be replaced with debug variant soon
        // Multi-thread update of variant animation
        auto const availableThreadCount = JS::GetNumberOfAvailableThreads();
        for (uint32_t threadNumber = 0; threadNumber < availableThreadCount; ++threadNumber)
        {
            JS::AssignTaskManually(threadNumber, [this, &recordState, deltaTimeInSec, threadNumber, availableThreadCount]()->void
            {
                for (uint32_t i = threadNumber; i < static_cast<uint32_t>(mAllVariantsList.size()); i += availableThreadCount)
                {
                    mAllVariantsList[i]->Update(deltaTimeInSec, recordState);
                }
            });
        }
        JS::WaitForThreadsToFinish();
    }

    //-------------------------------------------------------------------------------------------------

    void DebugRendererPipeline::Render(RT::CommandRecordState & drawPass, float deltaTime)
    {
        RF::BindPipeline(drawPass, mDrawPipeline);

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

            essence->bindVertexBuffer(drawPass);
            essence->bindIndexBuffer(drawPass);
            
            for (auto & variant : variantsList)
            {
                if (variant->IsActive())
                {
                    auto * debugVariant = CAST_VARIANT(variant);
                    MFA_ASSERT(debugVariant != nullptr);
                    debugVariant->getColor(pushConstants.baseColorFactor);
                    debugVariant->getTransform(pushConstants.model);

                    RF::PushConstants(
                        drawPass,
                        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                        0,
                        CBlobAliasOf(pushConstants)
                    );

                    CAST_VARIANT(variant)->Draw(drawPass);
                }
            }
        }
    }

    //-------------------------------------------------------------------------------------------------

    bool DebugRendererPipeline::CreateEssenceWithoutModel(
        std::shared_ptr<RT::GpuModel> const & gpuModel,
        uint32_t indicesCount
    )
    {
        return addEssence(std::make_shared<DebugEssence>(gpuModel, indicesCount));
    }

    //-------------------------------------------------------------------------------------------------

    std::shared_ptr<EssenceBase> DebugRendererPipeline::internalCreateEssence(
        std::shared_ptr<RT::GpuModel> const & gpuModel,
        std::shared_ptr<AS::MeshBase> const & cpuMesh
    )
    {
        return std::make_shared<DebugEssence>(gpuModel, cpuMesh->getIndexCount());
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

        MFA_VK_INVALID_ASSERT(mDescriptorSetLayout);
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
        {// Position
            VkVertexInputAttributeDescription attributeDescription{};
            attributeDescription.location = static_cast<uint32_t>(inputAttributeDescriptions.size());
            attributeDescription.binding = 0;
            attributeDescription.format = VK_FORMAT_R32G32B32_SFLOAT;
            attributeDescription.offset = offsetof(Vertex, position);

            inputAttributeDescriptions.emplace_back(attributeDescription);
        }
        MFA_ASSERT(mDrawPipeline.isValid() == false);

        RT::CreateGraphicPipelineOptions pipelineOptions{};
        pipelineOptions.useStaticViewportAndScissor = false;
        pipelineOptions.primitiveTopology = VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
        pipelineOptions.rasterizationSamples = RF::GetMaxSamplesCount();            // TODO Find a way to set sample count to 1. We only need MSAA for pbr-pipeline
        pipelineOptions.cullMode = VK_CULL_MODE_NONE;
        pipelineOptions.colorBlendAttachments.blendEnable = VK_FALSE;
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
            &mDescriptorSetLayout->descriptorSetLayout,
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
