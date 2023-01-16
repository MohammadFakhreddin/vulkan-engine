#include "PBR_Material.hpp"

#include "PBR_Essence.hpp"
#include "PBR_Variant.hpp"
#include "engine/BedrockPath.hpp"
#include "engine/asset_system/AssetShader.hpp"
#include "engine/camera/CameraComponent.hpp"
#include "engine/entity_system/components/TransformComponent.hpp"
#include "engine/render_system/RenderFrontend.hpp"
#include "tools/Importer.hpp"
#include "engine/resource_manager/ResourceManager.hpp"
#include "engine/job_system/JobSystem.hpp"
#include "engine/render_system/materials/VariantBase.hpp"
#include "engine/scene_manager/SceneManager.hpp"

#define ESSENCE_PURE(essence)      static_cast<PBR_Essence *>(essence)
#define ESSENCE_SHARED(essence)    ESSENCE_PURE(essence.get())
#define VARIANT_PURE(variant)      static_cast<PBR_Variant *>(variant)
#define VARIANT_SHARED(variant)    static_cast<PBR_Variant *>(variant.get())

namespace MFA
{

    //-------------------------------------------------------------------------------------------------

    PBR_Material::PBR_Material(
        std::shared_ptr<DisplayRenderPass> renderPass,
        std::shared_ptr<RT::BufferGroup> cameraBufferGroup,
        std::shared_ptr<RT::BufferGroup> directionalLightBufferGroup,
        std::shared_ptr<RT::BufferGroup> pointLightBufferGroup,
        std::shared_ptr<DirectionalLightShadowResource> directionalLightShadowResource,
        std::shared_ptr<PointLightShadowResource> pointLightShadowResource,
        const uint32_t maxSets
    )
        : BaseMaterial(maxSets)
        , mRenderPass(std::move(renderPass))
        , mCameraBufferGroup(std::move(cameraBufferGroup))
        , mDLightBufferGroup(std::move(directionalLightBufferGroup))
        , mPLightBufferGroup(std::move(pointLightBufferGroup))
        , mDLightShadowResource(std::move(directionalLightShadowResource))
        , mPLightShadowResource(std::move(pointLightShadowResource))
    {
        MFA_ASSERT(JS::IsMainThread());

        CreatePerFrameDescriptorSetLayout();
        CreatePerEssenceDescriptorSetLayout();

        CreatePipeline();

        mSamplerGroup = RF::CreateSampler(RT::CreateSamplerParams{});
        MFA_ASSERT(mSamplerGroup != nullptr);

        CreateErrorTexture();

        CreatePerFrameDescriptorSets();

        BaseMaterial::Init();
    }

    //-------------------------------------------------------------------------------------------------

    PBR_Material::~PBR_Material()
    {
        BaseMaterial::Shutdown();
        mPipeline.reset();
        mRenderPass.reset();
    }

    //-------------------------------------------------------------------------------------------------

    void PBR_Material::CreatePipeline()
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

        auto const descriptorSetLayouts = std::vector<VkDescriptorSetLayout>{
            mPerFrameDescriptorSetLayout->descriptorSetLayout,
            mPerEssenceDescriptorSetLayout->descriptorSetLayout,
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

    void PBR_Material::CreateErrorTexture()
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

    void PBR_Material::PerformDisplayPass(RT::CommandRecordState const& recordState, AS::AlphaMode const alphaMode)
    {
        DisplayPassPushConstants pushConstants{};

        auto const activeCamera = SceneManager::GetActiveCamera().lock();
        if (activeCamera != nullptr)
        {
            // TODO: Investigate about this negative value
            Copy(pushConstants.cameraPosition, activeCamera->GetTransform()->GetWorldPosition());
            pushConstants.projectFarToNearDistance = activeCamera->GetProjectionFarToNearDistance();
        }

        auto bindFunction = [&recordState, &pushConstants](AS::PBR::Primitive const & primitive)-> void
        {
            // Push constants
            pushConstants.primitiveIndex = primitive.uniqueId;
            RF::PushConstants(
                recordState,
                VK_SHADER_STAGE_FRAGMENT_BIT,
                0,
                CBlobAliasOf(pushConstants)
            );
        };

        for (auto const & item : mEssenceAndVariantsMap)
        {
            auto const * essence = ESSENCE_SHARED(item.second.essence);
            auto & variantsList = item.second.variants;

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
                    VARIANT_SHARED(variant)->Render(recordState, bindFunction, alphaMode);
                }
            }
        }
    }

    //-------------------------------------------------------------------------------------------------

    void PBR_Material::Render(RT::CommandRecordState& recordState, float const deltaTimeInSec)
    {
        BaseMaterial::Render(recordState, deltaTimeInSec);

        RF::BindPipeline(recordState, *mPipeline);
        RF::AutoBindDescriptorSet(
            recordState,
            RenderFrontend::UpdateFrequency::PerFrame,
            *mPerFrameDescriptorSet
        );
        PerformDisplayPass(recordState, AS::AlphaMode::Opaque);
        PerformDisplayPass(recordState, AS::AlphaMode::Mask);
        PerformDisplayPass(recordState, AS::AlphaMode::Blend);
    }

    //-------------------------------------------------------------------------------------------------

    void PBR_Material::CreatePerFrameDescriptorSetLayout()
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

        mPerFrameDescriptorSetLayout = RF::CreateDescriptorSetLayout(
            static_cast<uint8_t>(bindings.size()),
            bindings.data()
        );
    }

    //-------------------------------------------------------------------------------------------------

    void PBR_Material::CreatePerEssenceDescriptorSetLayout()
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

        mPerEssenceDescriptorSetLayout = RF::CreateDescriptorSetLayout(
            static_cast<uint8_t>(bindings.size()),
            bindings.data()
        );
    }

    //-------------------------------------------------------------------------------------------------

    void PBR_Material::CreatePerFrameDescriptorSets()
    {
        mPerFrameDescriptorSet = std::make_shared<RT::DescriptorSetGroup>(RF::CreateDescriptorSets(
            mDescriptorPool,
            RF::GetMaxFramesPerFlight(),
            *mPerFrameDescriptorSetLayout
        ));

        auto const * cameraBufferCollection = SceneManager::GetCameraBuffers();
        auto const * directionalLightBuffers = SceneManager::GetDirectionalLightBuffers();
        auto const * pointLightBuffers = SceneManager::GetPointLightsBuffers();

        for (uint32_t frameIndex = 0; frameIndex < RF::GetMaxFramesPerFlight(); ++frameIndex)
        {

            auto const descriptorSet = mPerFrameDescriptorSet->descriptorSets[frameIndex];
            MFA_ASSERT(descriptorSet != VK_NULL_HANDLE);

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
            VkDescriptorBufferInfo directionalLightBufferInfo{
                .buffer = directionalLightBuffers->buffers[frameIndex]->buffer,
                .offset = 0,
                .range = directionalLightBuffers->bufferSize,
            };
            descriptorSetSchema.AddUniformBuffer(&directionalLightBufferInfo);

            // PointLightBuffer
            VkDescriptorBufferInfo pointLightBufferInfo{
                .buffer = pointLightBuffers->buffers[frameIndex]->buffer,
                .offset = 0,
                .range = pointLightBuffers->bufferSize,
            };
            descriptorSetSchema.AddUniformBuffer(&pointLightBufferInfo);

            // Sampler
            VkDescriptorImageInfo texturesSamplerInfo{
                .sampler = mSamplerGroup->sampler,
                .imageView = VK_NULL_HANDLE,
                .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
            };
            descriptorSetSchema.AddSampler(&texturesSamplerInfo);

            // DirectionalLightShadowMap
            auto directionalLightShadowMap = VkDescriptorImageInfo{
                .sampler = VK_NULL_HANDLE,
                .imageView = mDLightShadowResource->GetShadowMap(frameIndex).imageView->imageView,
                .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
            };
            descriptorSetSchema.AddImage(&directionalLightShadowMap, 1);

            // PointLightShadowMap
            auto pointLightShadowCubeMapArray = VkDescriptorImageInfo{
                .sampler = VK_NULL_HANDLE,
                .imageView = mPLightShadowResource->GetShadowCubeMap(frameIndex).imageView->imageView,
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

}
