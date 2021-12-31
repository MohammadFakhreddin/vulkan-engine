//#include "ParticlePipeline.hpp"
//
//#include "engine/BedrockAssert.hpp"
//#include "engine/render_system/RenderFrontend.hpp"
//#include "engine/render_system/pipelines/DescriptorSetSchema.hpp"
//#include "engine/scene_manager/Scene.hpp"
//#include "engine/BedrockPath.hpp"
//#include "tools/Importer.hpp"
//
////-------------------------------------------------------------------------------------------------
//
//MFA::ParticlePipeline::ParticlePipeline(Scene * attachedScene)
//    : BasePipeline(1000)                // TODO How many sets do we need ?
//    , mAttachedScene(attachedScene)
//{}
//
////-------------------------------------------------------------------------------------------------
//
//MFA::ParticlePipeline::~ParticlePipeline()
//{
//    if (mIsInitialized)
//    {
//        Shutdown();
//    }
//}
//
////-------------------------------------------------------------------------------------------------
//
//void MFA::ParticlePipeline::Init(
//    std::shared_ptr<RT::SamplerGroup> samplerGroup,
//    std::shared_ptr<RT::GpuTexture> errorTexture
//)
//{
//    if (mIsInitialized == true)
//    {
//        MFA_ASSERT(false);
//        return;
//    }
//    mIsInitialized = true;
//
//    BasePipeline::Init();
//
//    MFA_ASSERT(samplerGroup != nullptr);
//    mSamplerGroup = std::move(samplerGroup);
//    MFA_ASSERT(errorTexture != nullptr);
//    mErrorTexture = std::move(errorTexture);
//
//    createPerFrameDescriptorSetLayout();
//    createPerEssenceDescriptorSetLayout();
//
//    createPerFrameDescriptorSets();
//
//    createPipeline();
//}
//
////-------------------------------------------------------------------------------------------------
//
//void MFA::ParticlePipeline::Shutdown()
//{
//    if (mIsInitialized == false)
//    {
//        MFA_ASSERT(false);
//        return;
//    }
//    mIsInitialized = false;
//
//    BasePipeline::Shutdown();
//}
//
////-------------------------------------------------------------------------------------------------
//
//void MFA::ParticlePipeline::PreRender(RT::CommandRecordState & drawPass, float const deltaTime)
//{
//    BasePipeline::PreRender(drawPass, deltaTime);
//}
//
////-------------------------------------------------------------------------------------------------
//
//void MFA::ParticlePipeline::Render(RT::CommandRecordState & drawPass, float const deltaTime)
//{
//    BasePipeline::Render(drawPass, deltaTime);
//}
//
////-------------------------------------------------------------------------------------------------
//
//MFA::DrawableVariant * MFA::ParticlePipeline::CreateDrawableVariant(RT::GpuModelId id)
//{
//    return BasePipeline::CreateDrawableVariant(id);
//}
//
////-------------------------------------------------------------------------------------------------
//
//void MFA::ParticlePipeline::internalCreateDrawableEssence(DrawableEssence & essence)
//{
//    createPerEssenceDescriptorSets(&essence);
//}
//
////-------------------------------------------------------------------------------------------------
//
//void MFA::ParticlePipeline::createPerFrameDescriptorSetLayout()
//{
//    std::vector<VkDescriptorSetLayoutBinding> bindings{};
//
//    // CameraBuffer
//    VkDescriptorSetLayoutBinding cameraBufferLayoutBinding{
//        .binding = static_cast<uint32_t>(bindings.size()),
//        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
//        .descriptorCount = 1,
//        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
//    };
//    bindings.emplace_back(cameraBufferLayoutBinding);
//
//    // Sampler
//    bindings.emplace_back(VkDescriptorSetLayoutBinding {
//        .binding = static_cast<uint32_t>(bindings.size()),
//        .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER,
//        .descriptorCount = 1,
//        .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
//    });
//
//    mPerFrameDescriptorSetLayout = RF::CreateDescriptorSetLayout(
//        static_cast<uint8_t>(bindings.size()),
//        bindings.data()
//    );
//}
//
////-------------------------------------------------------------------------------------------------
//
//void MFA::ParticlePipeline::createPerEssenceDescriptorSetLayout()
//{
//    std::vector<VkDescriptorSetLayoutBinding> bindings {};
//
//    // Textures
//    bindings.emplace_back(VkDescriptorSetLayoutBinding {
//        .binding = static_cast<uint32_t>(bindings.size()),
//        .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
//        .descriptorCount = MAXIMUM_TEXTURE_PER_ESSENCE,
//        .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
//    });
//    
//    mPerEssenceDescriptorSetLayout = RF::CreateDescriptorSetLayout(
//        static_cast<uint8_t>(bindings.size()),
//        bindings.data()
//    );
//}
//
////-------------------------------------------------------------------------------------------------
//
//void MFA::ParticlePipeline::createPerFrameDescriptorSets()
//{
//    mPerFrameDescriptorSetGroup = RF::CreateDescriptorSets(
//        mDescriptorPool,
//        RF::GetMaxFramesPerFlight(),
//        mPerFrameDescriptorSetLayout
//    );
//
//    auto const & cameraBuffers = mAttachedScene->GetCameraBuffers();
//
//    for (uint32_t frameIndex = 0; frameIndex < RF::GetMaxFramesPerFlight(); ++frameIndex)
//    {
//        auto const descriptorSet = mPerFrameDescriptorSetGroup.descriptorSets[frameIndex];
//        MFA_VK_VALID_ASSERT(descriptorSet);
//
//        DescriptorSetSchema descriptorSetSchema {descriptorSet};
//
//        VkDescriptorBufferInfo descriptorBufferInfo {
//            .buffer = cameraBuffers.buffers[frameIndex]->buffer,
//            .offset = 0,
//            .range = cameraBuffers.bufferSize
//        };
//        descriptorSetSchema.AddUniformBuffer(&descriptorBufferInfo);
//
//        descriptorSetSchema.UpdateDescriptorSets();
//    }
//
//}
//
////-------------------------------------------------------------------------------------------------
//
//void MFA::ParticlePipeline::createPerEssenceDescriptorSets(DrawableEssence * essence) const
//{
//    MFA_ASSERT(essence != nullptr);
//    auto const & textures = essence->GetGpuModel()->textures;
//
//    auto const & descriptorSetGroup = essence->CreateDescriptorSetGroup(
//        mDescriptorPool,
//        RF::GetMaxFramesPerFlight(),
//        mPerEssenceDescriptorSetLayout
//    );
//
//    for (uint32_t frameIndex = 0; frameIndex < RF::GetMaxFramesPerFlight(); ++frameIndex)
//    {
//        auto const & descriptorSet = descriptorSetGroup.descriptorSets[frameIndex];
//        MFA_VK_VALID_ASSERT(descriptorSet);
//
//        DescriptorSetSchema descriptorSetSchema {descriptorSet};
//
//        // Sampler
//        VkDescriptorImageInfo texturesSamplerInfo{
//            .sampler = mSamplerGroup->sampler,
//            .imageView = nullptr,
//            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
//        };
//        descriptorSetSchema.AddSampler(&texturesSamplerInfo);
//
//        // Textures
//        MFA_ASSERT(textures.size() < MAXIMUM_TEXTURE_PER_ESSENCE);
//        std::vector<VkDescriptorImageInfo> imageInfos{};
//        for (auto const & texture : textures)
//        {
//            imageInfos.emplace_back(VkDescriptorImageInfo{
//                .sampler = nullptr,
//                .imageView = texture->imageView->imageView,
//                .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
//            });
//        }
//        for (auto i = static_cast<uint32_t>(textures.size()); i < MAXIMUM_TEXTURE_PER_ESSENCE; ++i)
//        {
//            imageInfos.emplace_back(VkDescriptorImageInfo{
//                .sampler = nullptr,
//                .imageView = mErrorTexture->imageView->imageView,
//                .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
//            });
//        }
//        MFA_ASSERT(imageInfos.size() == MAXIMUM_TEXTURE_PER_ESSENCE);
//        descriptorSetSchema.AddImage(
//            imageInfos.data(),
//            static_cast<uint32_t>(imageInfos.size())
//        );
//
//        descriptorSetSchema.UpdateDescriptorSets();
//    }
//}
//
////-------------------------------------------------------------------------------------------------
//
//void MFA::ParticlePipeline::createPipeline()
//{
//    RF_CREATE_SHADER("shaders/particle/Particle.vert.spv", Vertex);
//    RF_CREATE_SHADER("shaders/particle/Particle.frag.spv", Fragment);
//
//    std::vector<RT::GpuShader const *> shaders{gpuVertexShader.get(), gpuFragmentShader.get()};
//
//    VkVertexInputBindingDescription vertexInputBindingDescription{
//        .binding = 0,
//        .stride = sizeof(AS::MeshVertex),
//        .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
//    };
//
//    // TODO We need something other than AS::MeshVertex here to save bandwidth
//    std::vector<VkVertexInputAttributeDescription> inputAttributeDescriptions {};
//
//    // Position
//    inputAttributeDescriptions.emplace_back(VkVertexInputAttributeDescription {
//        .location = static_cast<uint32_t>(inputAttributeDescriptions.size()),
//        .binding = 0,
//        .format = VK_FORMAT_R32G32B32_SFLOAT
//    });
//
//
//
//}
//
////-------------------------------------------------------------------------------------------------
