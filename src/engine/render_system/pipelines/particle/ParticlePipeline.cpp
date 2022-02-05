#include "ParticlePipeline.hpp"

#include "ParticleEssence.hpp"
#include "ParticleVariant.hpp"
#include "engine/BedrockAssert.hpp"
#include "engine/render_system/RenderFrontend.hpp"
#include "engine/render_system/pipelines/DescriptorSetSchema.hpp"
#include "engine/BedrockPath.hpp"
#include "tools/Importer.hpp"
#include "engine/asset_system/AssetParticleMesh.hpp"
#include "engine/render_system/RenderTypes.hpp"
#include "engine/render_system/render_passes/display_render_pass/DisplayRenderPass.hpp"
#include "engine/job_system/JobSystem.hpp"
#include "engine/asset_system/AssetShader.hpp"
#include "engine/resource_manager/ResourceManager.hpp"
#include "engine/scene_manager/SceneManager.hpp"

#define CAST_ESSENCE(essence) static_cast<ParticleEssence *>(essence)

namespace MFA
{

    using namespace AssetSystem::Particle;

    //-------------------------------------------------------------------------------------------------

    ParticlePipeline::ParticlePipeline()
        : BasePipeline(1000) // ! per essence                // TODO How many sets do we need ?
    {}

    //-------------------------------------------------------------------------------------------------

    ParticlePipeline::~ParticlePipeline()
    {
        if (mIsInitialized)
        {
            shutdown();
        }
    }

    //-------------------------------------------------------------------------------------------------

    void ParticlePipeline::init()
    {
        BasePipeline::init();

        mSamplerGroup = RF::CreateSampler(RT::CreateSamplerParams {
            .addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
            .addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
            .addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER
        });
        MFA_ASSERT(mSamplerGroup != nullptr);
        
        mErrorTexture = RC::AcquireGpuTexture("Error");
        MFA_ASSERT(mErrorTexture != nullptr);

        createPerFrameDescriptorSetLayout();
        createPerEssenceDescriptorSetLayout();

        createPerFrameDescriptorSets();

        createPipeline();
    }

    //-------------------------------------------------------------------------------------------------

    void ParticlePipeline::shutdown()
    {
        BasePipeline::shutdown();
    }

    //-------------------------------------------------------------------------------------------------

    void ParticlePipeline::render(
        RT::CommandRecordState & recordState,
        float const deltaTimeInSec
    )
    {
        if (mAllEssencesList.empty())
        {
            return;
        }

        BasePipeline::render(recordState, deltaTimeInSec);

        for (auto & essenceAndVariants : mEssenceAndVariantsMap)
        {
            JS::AutoAssignTask([&recordState, deltaTimeInSec, &essenceAndVariants]()->void{
                auto * essence = essenceAndVariants.second.essence.get();
                auto const & variants = essenceAndVariants.second.variants;
                CAST_ESSENCE(essence)->update(recordState, deltaTimeInSec, variants);
            });
        }
        JS::WaitForThreadsToFinish();

        RF::BindPipeline(recordState, *mPipeline);

        RF::BindDescriptorSet(
            recordState,
            RF::DescriptorSetType::PerFrame,
            mPerFrameDescriptorSetGroup
        );

        for (auto * essence : mAllEssencesList)
        {
            CAST_ESSENCE(essence)->draw(recordState, deltaTimeInSec);
        }
    }

    //-------------------------------------------------------------------------------------------------
    // TODO: We have duplicate code in these 2 function
    std::shared_ptr<EssenceBase> ParticlePipeline::createEssenceWithModel(
        std::shared_ptr<AssetSystem::Model> const & cpuModel,
        std::string const & name
    )
    {
        MFA_ASSERT(cpuModel != nullptr);
        auto essence = std::make_shared<ParticleEssence>(cpuModel, name);
        createPerEssenceDescriptorSets(essence.get());
        bool const success = addEssence(essence);
        return success ? essence : nullptr;
    }

    //-------------------------------------------------------------------------------------------------

    std::shared_ptr<EssenceBase> ParticlePipeline::internalCreateEssence(
      std::shared_ptr<RT::GpuModel> const & gpuModel,
      std::shared_ptr<AssetSystem::MeshBase> const & cpuMesh
    )
    {
        MFA_ASSERT(gpuModel != nullptr);
        MFA_ASSERT(cpuMesh != nullptr);
        auto const particleMesh = static_pointer_cast<Mesh>(cpuMesh);
        auto const essence = std::make_shared<ParticleEssence>(
            gpuModel,
            particleMesh
        );
        createPerEssenceDescriptorSets(essence.get());
        return essence;
    }

    //-------------------------------------------------------------------------------------------------

    std::shared_ptr<VariantBase> ParticlePipeline::internalCreateVariant(EssenceBase * essence)
    {
        MFA_ASSERT(essence != nullptr);
        return std::make_shared<ParticleVariant>(static_cast<ParticleEssence *>(essence));
    }

    //-------------------------------------------------------------------------------------------------

    void ParticlePipeline::createPerFrameDescriptorSetLayout()
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

        // Sampler
        bindings.emplace_back(VkDescriptorSetLayoutBinding{
            .binding = static_cast<uint32_t>(bindings.size()),
            .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
        });

        mPerFrameDescriptorSetLayout = RF::CreateDescriptorSetLayout(
            static_cast<uint8_t>(bindings.size()),
            bindings.data()
        );
    }

    //-------------------------------------------------------------------------------------------------

    void ParticlePipeline::createPerEssenceDescriptorSetLayout()
    {
        std::vector<VkDescriptorSetLayoutBinding> bindings{};

        // Textures
        bindings.emplace_back(VkDescriptorSetLayoutBinding{
            .binding = static_cast<uint32_t>(bindings.size()),
            .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
            .descriptorCount = MAXIMUM_TEXTURE_PER_ESSENCE,
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
        });

        mPerEssenceDescriptorSetLayout = RF::CreateDescriptorSetLayout(
            static_cast<uint8_t>(bindings.size()),
            bindings.data()
        );
    }

    //-------------------------------------------------------------------------------------------------

    void ParticlePipeline::createPerFrameDescriptorSets()
    {
        mPerFrameDescriptorSetGroup = RF::CreateDescriptorSets(
            mDescriptorPool,
            RF::GetMaxFramesPerFlight(),
            mPerFrameDescriptorSetLayout->descriptorSetLayout
        );

        auto const & cameraBuffers = SceneManager::GetCameraBuffers();

        for (uint32_t frameIndex = 0; frameIndex < RF::GetMaxFramesPerFlight(); ++frameIndex)
        {
            auto const descriptorSet = mPerFrameDescriptorSetGroup.descriptorSets[frameIndex];
            MFA_VK_VALID_ASSERT(descriptorSet);

            DescriptorSetSchema descriptorSetSchema{ descriptorSet };

            VkDescriptorBufferInfo descriptorBufferInfo{
                .buffer = cameraBuffers.buffers[frameIndex]->buffer,
                .offset = 0,
                .range = cameraBuffers.bufferSize
            };
            descriptorSetSchema.AddUniformBuffer(&descriptorBufferInfo);

            // Sampler
            VkDescriptorImageInfo texturesSamplerInfo{
                .sampler = mSamplerGroup->sampler,
                .imageView = nullptr,
                .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
            };
            descriptorSetSchema.AddSampler(&texturesSamplerInfo);

            descriptorSetSchema.UpdateDescriptorSets();
        }

    }

    //-------------------------------------------------------------------------------------------------

    void ParticlePipeline::createPerEssenceDescriptorSets(ParticleEssence * essence) const
    {
        MFA_ASSERT(essence != nullptr);
        auto const & textures = essence->getGpuModel()->textures;

        auto const & descriptorSetGroup = essence->createDescriptorSetGroup(
            mDescriptorPool,
            RF::GetMaxFramesPerFlight(),
            *mPerEssenceDescriptorSetLayout
        );

        for (uint32_t frameIndex = 0; frameIndex < RF::GetMaxFramesPerFlight(); ++frameIndex)
        {
            auto const & descriptorSet = descriptorSetGroup.descriptorSets[frameIndex];
            MFA_VK_VALID_ASSERT(descriptorSet);

            DescriptorSetSchema descriptorSetSchema{ descriptorSet };

            // Textures
            MFA_ASSERT(textures.size() < MAXIMUM_TEXTURE_PER_ESSENCE);
            std::vector<VkDescriptorImageInfo> imageInfos{};
            for (auto const & texture : textures)
            {
                imageInfos.emplace_back(VkDescriptorImageInfo{
                    .sampler = nullptr,
                    .imageView = texture->imageView->imageView,
                    .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
                });
            }
            for (auto i = static_cast<uint32_t>(textures.size()); i < MAXIMUM_TEXTURE_PER_ESSENCE; ++i)
            {
                imageInfos.emplace_back(VkDescriptorImageInfo{
                    .sampler = nullptr,
                    .imageView = mErrorTexture->imageView->imageView,
                    .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
                });
            }
            MFA_ASSERT(imageInfos.size() == MAXIMUM_TEXTURE_PER_ESSENCE);
            descriptorSetSchema.AddImage(
                imageInfos.data(),
                static_cast<uint32_t>(imageInfos.size())
            );

            descriptorSetSchema.UpdateDescriptorSets();
        }
    }

    //-------------------------------------------------------------------------------------------------

    void ParticlePipeline::createPipeline()
    {
        RF_CREATE_SHADER("shaders/particle/Particle.vert.spv", Vertex);
        RF_CREATE_SHADER("shaders/particle/Particle.frag.spv", Fragment);

        std::vector<RT::GpuShader const *> shaders{ gpuVertexShader.get(), gpuFragmentShader.get() };

        std::vector<VkVertexInputBindingDescription> vertexInputBindingDescriptions {};
        vertexInputBindingDescriptions.emplace_back(VkVertexInputBindingDescription {
            .binding = 0,
            .stride = sizeof(Vertex),
            .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
        });
        vertexInputBindingDescriptions.emplace_back(VkVertexInputBindingDescription {
            .binding = 1,
            .stride = sizeof(PerInstanceData),
            .inputRate = VK_VERTEX_INPUT_RATE_INSTANCE
        });
        
        std::vector<VkVertexInputAttributeDescription> inputAttributeDescriptions{};

        // Position
        inputAttributeDescriptions.emplace_back(VkVertexInputAttributeDescription{
            .location = static_cast<uint32_t>(inputAttributeDescriptions.size()),
            .binding = 0,
            .format = VK_FORMAT_R32G32B32_SFLOAT,
            .offset = offsetof(Vertex, localPosition)
        });

        // Texture index
        inputAttributeDescriptions.emplace_back(VkVertexInputAttributeDescription{
            .location = static_cast<uint32_t>(inputAttributeDescriptions.size()),
            .binding = 0,
            .format = VK_FORMAT_R32_SINT,
            .offset = offsetof(Vertex, textureIndex)
        });

        //// UV
        //inputAttributeDescriptions.emplace_back(VkVertexInputAttributeDescription{
        //    .location = static_cast<uint32_t>(inputAttributeDescriptions.size()),
        //    .binding = 0,
        //    .format = VK_FORMAT_R32G32_SFLOAT,
        //    .offset = offsetof(Vertex, uv)
        //});

        // Color
        inputAttributeDescriptions.emplace_back(VkVertexInputAttributeDescription{
            .location = static_cast<uint32_t>(inputAttributeDescriptions.size()),
            .binding = 0,
            .format = VK_FORMAT_R32G32B32_SFLOAT,
            .offset = offsetof(Vertex, color)
        });

        // Alpha
        inputAttributeDescriptions.emplace_back(VkVertexInputAttributeDescription{
            .location = static_cast<uint32_t>(inputAttributeDescriptions.size()),
            .binding = 0,
            .format = VK_FORMAT_R32_SFLOAT,
            .offset = offsetof(Vertex, alpha)
        });

        // PointSize
        inputAttributeDescriptions.emplace_back(VkVertexInputAttributeDescription{
            .location = static_cast<uint32_t>(inputAttributeDescriptions.size()),
            .binding = 0,
            .format = VK_FORMAT_R32_SFLOAT,
            .offset = offsetof(Vertex, pointSize)
        });

        // Instance position
        inputAttributeDescriptions.emplace_back(VkVertexInputAttributeDescription {
            .location = static_cast<uint32_t>(inputAttributeDescriptions.size()),
            .binding = 1,
            .format =VK_FORMAT_R32G32B32A32_SFLOAT,
            .offset = offsetof(PerInstanceData, instancePosition)
        });
        
        RT::CreateGraphicPipelineOptions pipelineOptions{};
        pipelineOptions.primitiveTopology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
        pipelineOptions.rasterizationSamples = RF::GetMaxSamplesCount();
        pipelineOptions.cullMode = VK_CULL_MODE_NONE;
        pipelineOptions.colorBlendAttachments.blendEnable = VK_TRUE;
        pipelineOptions.colorBlendAttachments.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
		pipelineOptions.colorBlendAttachments.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		pipelineOptions.colorBlendAttachments.colorBlendOp = VK_BLEND_OP_ADD;
		pipelineOptions.colorBlendAttachments.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		pipelineOptions.colorBlendAttachments.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		pipelineOptions.colorBlendAttachments.alphaBlendOp = VK_BLEND_OP_ADD;
		pipelineOptions.colorBlendAttachments.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

        pipelineOptions.depthStencil.depthWriteEnable = VK_FALSE;

        std::vector<VkDescriptorSetLayout> const descriptorSetLayouts {
            mPerFrameDescriptorSetLayout->descriptorSetLayout,
            mPerEssenceDescriptorSetLayout->descriptorSetLayout,
        };

        mPipeline = RF::CreatePipeline(
            RF::GetDisplayRenderPass()->GetVkRenderPass(),
            static_cast<uint8_t>(shaders.size()),
            shaders.data(),
            static_cast<uint32_t>(descriptorSetLayouts.size()),
            descriptorSetLayouts.data(),
            static_cast<uint32_t>(vertexInputBindingDescriptions.size()),
            vertexInputBindingDescriptions.data(),
            static_cast<uint8_t>(inputAttributeDescriptions.size()),
            inputAttributeDescriptions.data(),
            pipelineOptions
        );
    }

    //-------------------------------------------------------------------------------------------------

}
