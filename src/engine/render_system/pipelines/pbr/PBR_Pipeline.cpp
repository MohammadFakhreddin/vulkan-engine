#include "PBR_Pipeline.hpp"

#include "PBR_Essence.hpp"
#include "PBR_Variant.hpp"
#include "engine/BedrockPath.hpp"
#include "engine/asset_system/AssetShader.hpp"
#include "engine/render_system/RenderFrontend.hpp"
#include "tools/Importer.hpp"
#include "engine/resource_manager/ResourceManager.hpp"
#include "engine/job_system/JobSystem.hpp"

namespace MFA
{

    //-------------------------------------------------------------------------------------------------

    PBR_Pipeline::PBR_Pipeline(std::shared_ptr<DisplayRenderPass> renderPass, const uint32_t maxSets)
        : BasePipeline(maxSets)
        , mRenderPass(std::move(renderPass))
    {
        MFA_ASSERT(JS::IsMainThread());
        CreatePipeline();
        CreateErrorTexture();
        BasePipeline::init();
    }

    //-------------------------------------------------------------------------------------------------

    PBR_Pipeline::~PBR_Pipeline()
    {
        BasePipeline::shutdown();
        mPipeline.reset();
        mRenderPass.reset();
    }

    //-------------------------------------------------------------------------------------------------

    void PBR_Pipeline::CreatePipeline()
    {
        // Vertex shader
        RF_CREATE_SHADER("shaders/pbr_with_shadow_v2/PbrWithShadow.vert.spv", Vertex);

        // Fragment shader
        RF_CREATE_SHADER("shaders/pbr_with_shadow_v2/PbrWithShadow.frag.spv", Fragment)

        std::vector<RT::GpuShader const *> shaders{ gpuVertexShader.get(), gpuFragmentShader.get() };

        std::vector<VkVertexInputBindingDescription> bindingDescriptions{};

        bindingDescriptions.emplace_back(VkVertexInputBindingDescription{
            .binding = static_cast<uint32_t>(bindingDescriptions.size()),
            .stride = sizeof(PBR_Variant::SkinnedVertex),
            .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
        });

        bindingDescriptions.emplace_back(VkVertexInputBindingDescription{
            .binding = static_cast<uint32_t>(bindingDescriptions.size()),
            .stride = sizeof(PBR_Essence::VertexUVs),
            .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
        });

        std::vector<VkVertexInputAttributeDescription> inputAttributeDescriptions{};

        // WordPosition
        inputAttributeDescriptions.emplace_back(VkVertexInputAttributeDescription{
            .location = static_cast<uint32_t>(inputAttributeDescriptions.size()),
            .binding = 0,
            .format = VK_FORMAT_R32G32B32A32_SFLOAT,
            .offset = offsetof(PBR_Variant::SkinnedVertex, worldPosition)
        });

        // WorldNormal
        inputAttributeDescriptions.emplace_back(VkVertexInputAttributeDescription{
            .location = static_cast<uint32_t>(inputAttributeDescriptions.size()),
            .binding = 0,
            .format = VK_FORMAT_R32G32B32_SFLOAT,
            .offset = offsetof(PBR_Variant::SkinnedVertex, worldNormal),
        });

        // WorldTangent
        inputAttributeDescriptions.emplace_back(VkVertexInputAttributeDescription{
            .location = static_cast<uint32_t>(inputAttributeDescriptions.size()),
            .binding = 0,
            .format = VK_FORMAT_R32G32B32_SFLOAT,
            .offset = offsetof(PBR_Variant::SkinnedVertex, worldTangent),
        });

        // WorldBiTangent
        inputAttributeDescriptions.emplace_back(VkVertexInputAttributeDescription{
            .location = static_cast<uint32_t>(inputAttributeDescriptions.size()),
            .binding = 0,
            .format = VK_FORMAT_R32G32B32_SFLOAT,
            .offset = offsetof(PBR_Variant::SkinnedVertex, worldBiTangent),
        });

        // BaseColorUV
        inputAttributeDescriptions.emplace_back(VkVertexInputAttributeDescription{
            .location = static_cast<uint32_t>(inputAttributeDescriptions.size()),
            .binding = 1,
            .format = VK_FORMAT_R32G32_SFLOAT,
            .offset = offsetof(PBR_Essence::VertexUVs, baseColorTexCoord)
        });

        // Metallic/RoughnessUV
        inputAttributeDescriptions.emplace_back(VkVertexInputAttributeDescription{
            .location = static_cast<uint32_t>(inputAttributeDescriptions.size()),
            .binding = 1,
            .format = VK_FORMAT_R32G32_SFLOAT,
            .offset = offsetof(PBR_Essence::VertexUVs, metallicRoughnessTexCoord), // Metallic and roughness have same uv for gltf files
        });

        // NormalMapUV
        inputAttributeDescriptions.emplace_back(VkVertexInputAttributeDescription{
            .location = static_cast<uint32_t>(inputAttributeDescriptions.size()),
            .binding = 1,
            .format = VK_FORMAT_R32G32_SFLOAT,
            .offset = offsetof(PBR_Essence::VertexUVs, normalTexCoord),
        });

        // EmissionUV
        inputAttributeDescriptions.emplace_back(VkVertexInputAttributeDescription{
            .location = static_cast<uint32_t>(inputAttributeDescriptions.size()),
            .binding = 1,
            .format = VK_FORMAT_R32G32_SFLOAT,
            .offset = offsetof(PBR_Essence::VertexUVs, emissiveTexCoord),
        });

        // OcclusionUV
        inputAttributeDescriptions.emplace_back(VkVertexInputAttributeDescription{
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

        auto perFrameDescriptorLayout = CreatePerFrameDescriptorSetLayout();
        auto perEssenceDescriptorLayout = CreatePerEssenceDescriptorSetLayout();

        auto const descriptorSetLayouts = std::vector<VkDescriptorSetLayout>{
            perFrameDescriptorLayout->descriptorSetLayout,
            perEssenceDescriptorLayout->descriptorSetLayout,
        };

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

        mPipeline = RF::CreateGraphicPipeline(
            mRenderPass->GetVkRenderPass(),
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

    void PBR_Pipeline::CreateErrorTexture()
    {
        RC::AcquireGpuTexture(
            "Error",
            [this](std::shared_ptr<RT::GpuTexture> const & gpuTexture)->void
            {
                MFA_ASSERT(gpuTexture != nullptr);
                mErrorTexture = gpuTexture;
            }
        );
    }

    //-------------------------------------------------------------------------------------------------

    std::shared_ptr<RT::DescriptorSetLayoutGroup> PBR_Pipeline::CreatePerFrameDescriptorSetLayout()
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
        bindings.emplace_back(VkDescriptorSetLayoutBinding{
            .binding = static_cast<uint32_t>(bindings.size()),
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_GEOMETRY_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
        });

        // PointLightBuffer;
        bindings.emplace_back(VkDescriptorSetLayoutBinding{
            .binding = static_cast<uint32_t>(bindings.size()),
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_GEOMETRY_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
        });

        // Sampler
        bindings.emplace_back(VkDescriptorSetLayoutBinding{
            .binding = static_cast<uint32_t>(bindings.size()),
            .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
        });

        // DirectionalLightShadowMap
        bindings.emplace_back(VkDescriptorSetLayoutBinding{
            .binding = static_cast<uint32_t>(bindings.size()),
            .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
        });

        // PointLightShadowMap
        bindings.emplace_back(VkDescriptorSetLayoutBinding{
            .binding = static_cast<uint32_t>(bindings.size()),
            .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
        });

        return RF::CreateDescriptorSetLayout(
            static_cast<uint8_t>(bindings.size()),
            bindings.data()
        );
    }

    //-------------------------------------------------------------------------------------------------

    std::shared_ptr<RT::DescriptorSetLayoutGroup> PBR_Pipeline::CreatePerEssenceDescriptorSetLayout()
    {
        std::vector<VkDescriptorSetLayoutBinding> bindings{};

        /////////////////////////////////////////////////////////////////
        // Fragment shader
        /////////////////////////////////////////////////////////////////

        // Primitive
        bindings.emplace_back(VkDescriptorSetLayoutBinding{
            .binding = static_cast<uint32_t>(bindings.size()),
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
        });

        // Textures
        bindings.emplace_back(VkDescriptorSetLayoutBinding{
            .binding = static_cast<uint32_t>(bindings.size()),
            .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
            .descriptorCount = PBR_Essence::MAX_TEXTURE_COUNT,
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
        });

        return RF::CreateDescriptorSetLayout(
            static_cast<uint8_t>(bindings.size()),
            bindings.data()
        );
    }

    //-------------------------------------------------------------------------------------------------

}
