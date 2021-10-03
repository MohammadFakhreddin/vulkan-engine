#include "PointLightPipeline.hpp"

#include "engine/BedrockAssert.hpp"
#include "engine/BedrockMatrix.hpp"
#include "tools/Importer.hpp"
#include "engine/BedrockPath.hpp"
#include "engine/render_system/render_passes/display_render_pass/DisplayRenderPass.hpp"
#include "engine/render_system/drawable_variant/DrawableVariant.hpp"
#include "engine/render_system/pipelines/DescriptorSetSchema.hpp"

#include <ranges>

#include "engine/entity_system/Entity.hpp"
#include "engine/entity_system/components/ColorComponent.hpp"

using namespace MFA;

//-------------------------------------------------------------------------------------------------

PointLightPipeline::~PointLightPipeline()
{
    if (mIsInitialized)
    {
        Shutdown();
    }
}

//-------------------------------------------------------------------------------------------------

void PointLightPipeline::Init()
{
    if (mIsInitialized == true)
    {
        MFA_ASSERT(false);
        return;
    }
    mIsInitialized = true;

    BasePipeline::Init();

    createUniformBuffers();
    createDescriptorSetLayout();
    createPipeline();
    createDescriptorSets();
}

//-------------------------------------------------------------------------------------------------

void PointLightPipeline::Shutdown()
{
    if (mIsInitialized == false)
    {
        MFA_ASSERT(false);
        return;
    }
    mIsInitialized = false;

    RF::DestroyPipelineGroup(mDrawPipeline);
    destroyDescriptorSetLayout();
    destroyUniformBuffers();

    BasePipeline::Shutdown();
}

//-------------------------------------------------------------------------------------------------

void PointLightPipeline::PreRender(RT::DrawPass & drawPass, float const deltaTime)
{
    BasePipeline::PreRender(drawPass, deltaTime);
    updateViewProjectionBuffer(drawPass);
}

//-------------------------------------------------------------------------------------------------

void PointLightPipeline::Render(RT::DrawPass & drawPass, float deltaTime)
{
    RF::BindDrawPipeline(drawPass, mDrawPipeline);
    RF::BindDescriptorSet(drawPass, mDescriptorSetGroup.descriptorSets[drawPass.frameIndex]);

    PushConstants pushConstants{};

    for (auto const & essenceAndVariantList : mEssenceAndVariantsMap)
    {
        auto & essence = essenceAndVariantList.second->essence;
        auto & variantsList = essenceAndVariantList.second->variants;
        RF::BindVertexBuffer(drawPass, essence.GetGpuModel().meshBuffers.verticesBuffer);
        RF::BindIndexBuffer(drawPass, essence.GetGpuModel().meshBuffers.indicesBuffer);

        for (auto const & variant : variantsList)
        {
            if (variant->IsActive() && variant->IsInFrustum())
            {
                auto * color = variant->GetEntity()->GetComponent<ColorComponent>()->GetColor();

                Copy<3>(pushConstants.baseColorFactor, color);

                variant->Draw(drawPass, [&drawPass, &pushConstants](AS::MeshPrimitive const & primitive, DrawableVariant::Node const & node)-> void
                    {
                        Matrix::CopyGlmToCells(node.cachedModelTransform, pushConstants.model);

                        RF::PushConstants(
                            drawPass,
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

void PointLightPipeline::UpdateCameraView(float cameraTransform[16])
{
    if (Matrix::IsEqual(mCameraTransform, cameraTransform) == false)
    {
        Matrix::CopyCellsToMat4(cameraTransform, mCameraTransform);
        mViewProjectionBufferDirtyCounter = RF::GetMaxFramesPerFlight();
    }
}

//-------------------------------------------------------------------------------------------------

void PointLightPipeline::UpdateCameraProjection(float cameraProjection[16])
{
    if (Matrix::IsEqual(mCameraProjection, cameraProjection) == false)
    {
        Matrix::CopyCellsToMat4(cameraProjection, mCameraProjection);
        mViewProjectionBufferDirtyCounter = RF::GetMaxFramesPerFlight();
    }
}

//-------------------------------------------------------------------------------------------------

void PointLightPipeline::updateViewProjectionBuffer(RT::DrawPass const & drawPass)
{
    if (mViewProjectionBufferDirtyCounter <= 0)
    {
        return;
    }
    if (mViewProjectionBufferDirtyCounter == RF::GetMaxFramesPerFlight())
    {
        glm::mat4 const viewProjection = mCameraProjection * mCameraTransform;
        Matrix::CopyGlmToCells(viewProjection, mViewProjectionData.viewProjection);
    }
    --mViewProjectionBufferDirtyCounter;
    MFA_ASSERT(mViewProjectionBufferDirtyCounter >= 0);

    RF::UpdateUniformBuffer(mViewProjectionBuffers.buffers[drawPass.frameIndex], CBlobAliasOf(mViewProjectionData));
}

//-------------------------------------------------------------------------------------------------

void PointLightPipeline::createUniformBuffers()
{
    mViewProjectionBuffers = RF::CreateUniformBuffer(sizeof(ViewProjectionData), RF::GetMaxFramesPerFlight());
    for (uint32_t i = 0; i < RF::GetMaxFramesPerFlight(); ++i)
    {
        RF::UpdateUniformBuffer(mViewProjectionBuffers.buffers[i], CBlobAliasOf(mViewProjectionData));
    }
}

//-------------------------------------------------------------------------------------------------

void PointLightPipeline::destroyUniformBuffers()
{
    RF::DestroyUniformBuffer(mViewProjectionBuffers);
}

//-------------------------------------------------------------------------------------------------

void PointLightPipeline::createDescriptorSetLayout()
{
    std::vector<VkDescriptorSetLayoutBinding> bindings{};
    {// ModelViewProjection
        VkDescriptorSetLayoutBinding layoutBinding{
            .binding = static_cast<uint32_t>(bindings.size()),
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT
        };
        bindings.emplace_back(layoutBinding);
    }
    MFA_VK_INVALID_ASSERT(mDescriptorSetLayout);
    mDescriptorSetLayout = RF::CreateDescriptorSetLayout(
        static_cast<uint8_t>(bindings.size()),
        bindings.data()
    );
}

//-------------------------------------------------------------------------------------------------

void PointLightPipeline::destroyDescriptorSetLayout()
{
    MFA_VK_VALID_ASSERT(mDescriptorSetLayout);
    RF::DestroyDescriptorSetLayout(mDescriptorSetLayout);
    MFA_VK_MAKE_NULL(mDescriptorSetLayout);
}

//-------------------------------------------------------------------------------------------------

void PointLightPipeline::createPipeline()
{
    // Vertex shader
    auto cpuVertexShader = Importer::ImportShaderFromSPV(
        Path::Asset("shaders/point_light/PointLight.vert.spv").c_str(),
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
        Path::Asset("shaders/point_light/PointLight.frag.spv").c_str(),
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

    VkVertexInputBindingDescription bindingDescription{};
    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(AS::MeshVertex);
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    std::vector<VkVertexInputAttributeDescription> inputAttributeDescriptions{};
    {// Position
        VkVertexInputAttributeDescription attributeDescription{};
        attributeDescription.location = static_cast<uint32_t>(inputAttributeDescriptions.size());
        attributeDescription.binding = 0;
        attributeDescription.format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescription.offset = offsetof(AS::MeshVertex, position);

        inputAttributeDescriptions.emplace_back(attributeDescription);
    }
    MFA_ASSERT(mDrawPipeline.isValid() == false);

    RT::CreateGraphicPipelineOptions pipelineOptions{};
    pipelineOptions.useStaticViewportAndScissor = false;
    pipelineOptions.primitiveTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
    pipelineOptions.rasterizationSamples = RF::GetMaxSamplesCount();
    pipelineOptions.cullMode = VK_CULL_MODE_BACK_BIT;

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

void PointLightPipeline::createDescriptorSets()
{
    mDescriptorSetGroup = RF::CreateDescriptorSets(RF::GetMaxFramesPerFlight(), mDescriptorSetLayout);

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
            .buffer = mViewProjectionBuffers.buffers[frameIndex].buffer,
            .offset = 0,
            .range = mViewProjectionBuffers.bufferSize,
        };
        descriptorSetSchema.AddUniformBuffer(bufferInfo);

        descriptorSetSchema.UpdateDescriptorSets();
    }
}

//-------------------------------------------------------------------------------------------------
