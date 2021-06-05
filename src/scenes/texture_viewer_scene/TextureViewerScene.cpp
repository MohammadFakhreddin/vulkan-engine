#include "TextureViewerScene.hpp"

#include "engine/BedrockMemory.hpp"
#include "libs/tiny_ktx/tinyktx.h"
#include "tools/Importer.hpp"
#include "tools/ShapeGenerator.hpp"

namespace RF = MFA::RenderFrontend;
namespace RB = MFA::RenderBackend;
namespace Importer = MFA::Importer;
namespace AS = MFA::AssetSystem;
namespace SG = MFA::ShapeGenerator;
namespace UI = MFA::UISystem;

void TextureViewerScene::Init() {

    // Vertex shader
    auto cpu_vertex_shader = Importer::ImportShaderFromSPV(
        "../assets/shaders/texture_viewer/TextureViewer.vert.spv", 
        MFA::AssetSystem::Shader::Stage::Vertex, 
        "main"
    );
    MFA_ASSERT(cpu_vertex_shader.isValid());
    auto gpu_vertex_shader = RF::CreateShader(cpu_vertex_shader);
    MFA_ASSERT(gpu_vertex_shader.valid());
    MFA_DEFER {
        RF::DestroyShader(gpu_vertex_shader);
        Importer::FreeShader(&cpu_vertex_shader);
    };
    
    // Fragment shader
    auto cpu_fragment_shader = Importer::ImportShaderFromSPV(
        "../assets/shaders/texture_viewer/TextureViewer.frag.spv",
        MFA::AssetSystem::Shader::Stage::Fragment,
        "main"
    );
    auto gpu_fragment_shader = RF::CreateShader(cpu_fragment_shader);
    MFA_ASSERT(cpu_fragment_shader.isValid());
    MFA_ASSERT(gpu_fragment_shader.valid());
    MFA_DEFER {
        RF::DestroyShader(gpu_fragment_shader);
        Importer::FreeShader(&cpu_fragment_shader);
    };

    std::vector<RB::GpuShader> shaders {gpu_vertex_shader, gpu_fragment_shader};

    createModel();

    MFA_ASSERT(mGpuModel.textures.size() == 1);
    mTotalMipCount = mGpuModel.textures[0].cpu_texture()->GetMipCount();

    // TODO We need nearest and linear filters
    mSamplerGroup = RF::CreateSampler(RB::CreateSamplerParams {
        .min_lod = 0.0f,
        .max_lod = static_cast<float>(mTotalMipCount),
        .anisotropy_enabled = true,
        .max_anisotropy = 16.0f
    });

    createDescriptorSetLayout();
    
    createDrawPipeline(static_cast<uint8_t>(shaders.size()), shaders.data());

    createDrawableObject();
}

void TextureViewerScene::Shutdown() {
    RF::DestroyDrawPipeline(mDrawPipeline);
    RF::DestroyDescriptorSetLayout(mDescriptorSetLayout);
    RF::DestroySampler(mSamplerGroup);
    RF::DestroyGpuModel(mGpuModel);
    Importer::FreeModel(&mGpuModel.model);
    mDrawableObject->deleteUniformBuffers();
}

void TextureViewerScene::OnDraw(
    float deltaTimeInSec, 
    RF::DrawPass & drawPass
) {
    RF::BindDrawPipeline(drawPass, mDrawPipeline);

    // ProjectionViewBuffer
    {
        // Rotation
        MFA::Matrix4X4Float rotationMat {};
        MFA::Matrix4X4Float::AssignRotation(
            rotationMat,
            mModelRotation[0],
            mModelRotation[1],
            mModelRotation[2]
        );

        // Scale
        MFA::Matrix4X4Float scaleMat {};
        MFA::Matrix4X4Float::AssignScale(scaleMat, mModelScale);

        // Position
        MFA::Matrix4X4Float translationMat {};
        MFA::Matrix4X4Float::AssignTranslation(
            translationMat,
            mModelPosition[0],
            mModelPosition[1],
            mModelPosition[2]
        );

        MFA::Matrix4X4Float transformMat {};
        MFA::Matrix4X4Float::Identity(transformMat);
        transformMat.multiply(translationMat);
        transformMat.multiply(rotationMat);
        transformMat.multiply(scaleMat);

        ::memcpy(mViewProjectionBuffer.view, transformMat.cells, sizeof(transformMat.cells));
        static_assert(sizeof(transformMat.cells) == sizeof(mViewProjectionBuffer.view));

        mDrawableObject->updateUniformBuffer(
            "viewProjection",
            MFA::CBlobAliasOf(mViewProjectionBuffer)
        );
    }
    
    // Texture options buffer
    {
        mImageOptionsBuffer.mipLevel = static_cast<float>(mMipLevel);

        mDrawableObject->updateUniformBuffer(
            "imageOptions",
            MFA::CBlobAliasOf(mImageOptionsBuffer)
        );
    }

    mDrawableObject->draw(drawPass);
}

void TextureViewerScene::OnUI(
    float deltaTimeInSec, 
    RF::DrawPass & draw_pass
) {
    static constexpr float ItemWidth = 500;
    UI::BeginWindow("Object viewer");
    UI::SetNextItemWidth(ItemWidth);
    UI::SliderInt("MipLevel", &mMipLevel, 0, mTotalMipCount - 1);
    UI::SetNextItemWidth(ItemWidth);
    UI::SliderFloat("XDegree", &mModelRotation[0], -360.0f, 360.0f);
    UI::SetNextItemWidth(ItemWidth);
    UI::SliderFloat("YDegree", &mModelRotation[1], -360.0f, 360.0f);
    UI::SetNextItemWidth(ItemWidth);
    UI::SliderFloat("ZDegree", &mModelRotation[2], -360.0f, 360.0f);
    UI::SetNextItemWidth(ItemWidth);
    UI::SliderFloat("Scale", &mModelScale, 0.0f, 1.0f);
    UI::SetNextItemWidth(ItemWidth);
    UI::SliderFloat("XDistance", &mModelPosition[0], -5.0f, 5.0f);
    UI::SetNextItemWidth(ItemWidth);
    UI::SliderFloat("YDistance", &mModelPosition[1], -5.0f, 5.0f);
    UI::SetNextItemWidth(ItemWidth);
    UI::SliderFloat("ZDistance", &mModelPosition[2], -5.0f, 5.0f);
    UI::EndWindow();
}

void TextureViewerScene::OnResize() {
    updateProjection();
}

void TextureViewerScene::createDescriptorSetLayout() {
    std::vector<VkDescriptorSetLayoutBinding> bindings {};
    // ViewProjectionBuffer 
    bindings.emplace_back(VkDescriptorSetLayoutBinding {
        .binding = static_cast<uint32_t>(bindings.size()),
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
        .pImmutableSamplers = nullptr, // Optional
    });
    // Texture
    bindings.emplace_back(VkDescriptorSetLayoutBinding {
        .binding = static_cast<uint32_t>(bindings.size()),
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
        .pImmutableSamplers = nullptr,
    });
    // ImageOptions
    bindings.emplace_back(VkDescriptorSetLayoutBinding {
        .binding = static_cast<uint32_t>(bindings.size()),
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
        .pImmutableSamplers = nullptr, // Optional
    });
    mDescriptorSetLayout = RF::CreateDescriptorSetLayout(
        static_cast<uint8_t>(bindings.size()),
        bindings.data()
    );
}

void TextureViewerScene::createDrawPipeline(
    uint8_t const gpuShaderCount, 
    MFA::RenderBackend::GpuShader * gpuShaders
) {
    VkVertexInputBindingDescription const vertexInputBindingDescription {
        .binding = 0,
        .stride = sizeof(AS::MeshVertex),
        .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
    };

    std::vector<VkVertexInputAttributeDescription> vkVertexInputAttributeDescriptions {};
    // Position
    vkVertexInputAttributeDescriptions.emplace_back(VkVertexInputAttributeDescription {
        .location = static_cast<uint32_t>(vkVertexInputAttributeDescriptions.size()),
        .binding = 0,
        .format = VK_FORMAT_R32G32B32_SFLOAT,
        .offset = offsetof(AS::MeshVertex, position),   
    });
    // BaseColor
    vkVertexInputAttributeDescriptions.emplace_back(VkVertexInputAttributeDescription {
        .location = static_cast<uint32_t>(vkVertexInputAttributeDescriptions.size()),
        .binding = 0,
        .format = VK_FORMAT_R32G32_SFLOAT,
        .offset = offsetof(AS::MeshVertex, baseColorUV),   
    });

    mDrawPipeline = RF::CreateBasicDrawPipeline(
        gpuShaderCount, 
        gpuShaders,
        1,
        &mDescriptorSetLayout,
        vertexInputBindingDescription,
        static_cast<uint8_t>(vkVertexInputAttributeDescriptions.size()),
        vkVertexInputAttributeDescriptions.data()
    );
}

void TextureViewerScene::createModel() {
    auto cpuModel = SG::Sheet();

    //auto cpuTexture = Importer::ImportImage("../assets/models/sponza/11490520546946913238.ktx");
    auto cpuTexture = Importer::ImportImage(
        "../assets/models/FlightHelmet/glTF/FlightHelmet_baseColor3.png", 
        Importer::ImportTextureOptions {
            .tryToGenerateMipmaps = true
        }
    );
    MFA_ASSERT(cpuTexture.isValid());

    cpuModel.textures.emplace_back(cpuTexture);
    MFA_ASSERT(cpuModel.mesh.getSubMeshCount() == 1);
    MFA_ASSERT(cpuModel.mesh.getSubMeshByIndex(0).primitives.size() == 1);
    cpuModel.mesh.getSubMeshByIndex(0).primitives[0].baseColorTextureIndex = 0;

    mGpuModel = MFA::RenderFrontend::CreateGpuModel(cpuModel);
}

void TextureViewerScene::createDrawableObject() {
    auto const & cpuModel = mGpuModel.model;
    mDrawableObject = std::make_unique<MFA::DrawableObject> (
        mGpuModel,
        mDescriptorSetLayout
    );

    auto const * viewProjectionBuffer = mDrawableObject->createUniformBuffer(
        "viewProjection", 
        sizeof(ViewProjectionBuffer)
    );
    MFA_ASSERT(viewProjectionBuffer != nullptr);

    auto const * imageOptionsBuffer = mDrawableObject->createUniformBuffer(
        "imageOptions", 
        sizeof(ImageOptionsBuffer)
    );
    MFA_ASSERT(imageOptionsBuffer != nullptr);

    MFA_ASSERT(cpuModel.mesh.getSubMeshCount() == 1);
    MFA_ASSERT(cpuModel.mesh.getSubMeshByIndex(0).primitives.size() == 1);
    auto const & primitive = cpuModel.mesh.getSubMeshByIndex(0).primitives[0];
    auto * descriptorSet = mDrawableObject->getDescriptorSetByPrimitiveUniqueId(primitive.uniqueId);

    std::vector<VkWriteDescriptorSet> writeInfo {};

    // ViewProjection
    VkDescriptorBufferInfo viewProjectionBufferInfo {
        .buffer = viewProjectionBuffer->buffers[0].buffer,
        .offset = 0,
        .range = viewProjectionBuffer->bufferSize
    };
    writeInfo.emplace_back(VkWriteDescriptorSet {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = descriptorSet,
        .dstBinding = static_cast<uint32_t>(writeInfo.size()),
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .pBufferInfo = &viewProjectionBufferInfo,
    });

    // Texture
    VkDescriptorImageInfo baseColorImageInfo {
        .sampler = mSamplerGroup.sampler,          // TODO Each texture has it's own properties that may need it's own sampler (Not sure yet)
        .imageView = mGpuModel.textures[primitive.baseColorTextureIndex].image_view(),
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    };
    writeInfo.emplace_back(VkWriteDescriptorSet {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = descriptorSet,
        .dstBinding = static_cast<uint32_t>(writeInfo.size()),
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .pImageInfo = &baseColorImageInfo,
    });

    // ImageOptions
    VkDescriptorBufferInfo light_view_buffer_info {
        .buffer = imageOptionsBuffer->buffers[0].buffer,
        .offset = 0,
        .range = imageOptionsBuffer->bufferSize
    };
    writeInfo.emplace_back(VkWriteDescriptorSet {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = descriptorSet,
        .dstBinding = static_cast<uint32_t>(writeInfo.size()),
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .pBufferInfo = &light_view_buffer_info,
    });

    RF::UpdateDescriptorSets(
        static_cast<uint8_t>(writeInfo.size()),
        writeInfo.data()
    );
}

void TextureViewerScene::updateProjection() {
    // Perspective
    int32_t width; int32_t height;
    RF::GetWindowSize(width, height);
    float const ratio = static_cast<float>(width) / static_cast<float>(height);
    MFA::Matrix4X4Float perspectiveMat {};
    MFA::Matrix4X4Float::PreparePerspectiveProjectionMatrix(
        perspectiveMat,
        ratio,
        40,
        Z_NEAR,
        Z_FAR
    );
    static_assert(sizeof(mViewProjectionBuffer.perspective) == sizeof(perspectiveMat.cells));
    ::memcpy(mViewProjectionBuffer.perspective, perspectiveMat.cells, sizeof(perspectiveMat.cells));
}
