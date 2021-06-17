#include "TextureViewerScene.hpp"

#include "engine/BedrockMemory.hpp"
#include "libs/tiny_ktx/tinyktx.h"
#include "tools/Importer.hpp"
#include "tools/ShapeGenerator.hpp"
#include "engine/BedrockPath.hpp"

namespace RF = MFA::RenderFrontend;
namespace RB = MFA::RenderBackend;
namespace Importer = MFA::Importer;
namespace AS = MFA::AssetSystem;
namespace SG = MFA::ShapeGenerator;
namespace UI = MFA::UISystem;
namespace Path = MFA::Path;

void TextureViewerScene::Init() {

    // Vertex shader
    auto cpu_vertex_shader = Importer::ImportShaderFromSPV(
        Path::Asset("shaders/texture_viewer/TextureViewer.vert.spv").c_str(), 
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
        Path::Asset("shaders/texture_viewer/TextureViewer.frag.spv").c_str(),
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
    mTotalMipCount = mGpuModel.textures[0].cpuTexture()->GetMipCount();

    {// Create sampler
        RB::CreateSamplerParams params {};
        params.min_lod = 0.0f;
        params.max_lod = static_cast<float>(mTotalMipCount);
        params.anisotropy_enabled = true;
        params.max_anisotropy = 16.0f;
        // TODO We need nearest and linear filters
        mSamplerGroup = RF::CreateSampler(params);
    }

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

    mDrawableObject->update(deltaTimeInSec);
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
    {// ViewProjectionBuffer
        VkDescriptorSetLayoutBinding layoutBinding {};
        layoutBinding.binding = static_cast<uint32_t>(bindings.size());
        layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        layoutBinding.descriptorCount = 1;
        layoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

        bindings.emplace_back(layoutBinding);
    }
    {// Texture
        VkDescriptorSetLayoutBinding layoutBinding {};
        layoutBinding.binding = static_cast<uint32_t>(bindings.size());
        layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        layoutBinding.descriptorCount = 1;
        layoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        layoutBinding.pImmutableSamplers = nullptr;

        bindings.emplace_back(layoutBinding);
    }
    {// ImageOptions
        VkDescriptorSetLayoutBinding layoutBinding {};
        layoutBinding.binding = static_cast<uint32_t>(bindings.size());
        layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        layoutBinding.descriptorCount = 1;
        layoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        bindings.emplace_back(layoutBinding);
    }
    mDescriptorSetLayout = RF::CreateDescriptorSetLayout(
        static_cast<uint8_t>(bindings.size()),
        bindings.data()
    );
}

void TextureViewerScene::createDrawPipeline(
    uint8_t const gpuShaderCount, 
    MFA::RenderBackend::GpuShader * gpuShaders
) {
    VkVertexInputBindingDescription vertexInputBindingDescription {};
    vertexInputBindingDescription.binding = 0;
    vertexInputBindingDescription.stride = sizeof(AS::MeshVertex);
    vertexInputBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    
    std::vector<VkVertexInputAttributeDescription> vkVertexInputAttributeDescriptions {};
    {// Position
        VkVertexInputAttributeDescription attributeDescription {};
        attributeDescription.location = static_cast<uint32_t>(vkVertexInputAttributeDescriptions.size());
        attributeDescription.binding = 0;
        attributeDescription.format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescription.offset = offsetof(AS::MeshVertex, position);
        vkVertexInputAttributeDescriptions.emplace_back(attributeDescription);
    }
    {// BaseColor
        VkVertexInputAttributeDescription attributeDescription {};
        attributeDescription.location = static_cast<uint32_t>(vkVertexInputAttributeDescriptions.size());
        attributeDescription.binding = 0;
        attributeDescription.format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescription.offset = offsetof(AS::MeshVertex, baseColorUV);   
        vkVertexInputAttributeDescriptions.emplace_back(attributeDescription);
    }
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
    Importer::ImportTextureOptions options {};
    options.tryToGenerateMipmaps = true;
    auto cpuTexture = Importer::ImportImage(
        Path::Asset("models/FlightHelmet/glTF/FlightHelmet_baseColor3.png").c_str(), 
        options
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
    auto descriptorSet = mDrawableObject->getDescriptorSetByPrimitiveUniqueId(primitive.uniqueId);

    std::vector<VkWriteDescriptorSet> writeInfo {};

    {// ViewProjection
        VkDescriptorBufferInfo viewProjectionBufferInfo {};
        viewProjectionBufferInfo.buffer = viewProjectionBuffer->buffers[0].buffer;
        viewProjectionBufferInfo.offset = 0;
        viewProjectionBufferInfo.range = viewProjectionBuffer->bufferSize;

        VkWriteDescriptorSet writeDescriptorSet {};
        writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        writeDescriptorSet.dstSet = descriptorSet,
        writeDescriptorSet.dstBinding = static_cast<uint32_t>(writeInfo.size()),
        writeDescriptorSet.dstArrayElement = 0,
        writeDescriptorSet.descriptorCount = 1,
        writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        writeDescriptorSet.pBufferInfo = &viewProjectionBufferInfo,

        writeInfo.emplace_back(writeDescriptorSet);
    }
    {// Texture
        VkDescriptorImageInfo baseColorImageInfo {};
        baseColorImageInfo.sampler = mSamplerGroup.sampler;          // TODO Each texture has it's own properties that may need it's own sampler (Not sure yet)
        baseColorImageInfo.imageView = mGpuModel.textures[primitive.baseColorTextureIndex].image_view();
        baseColorImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        VkWriteDescriptorSet writeDescriptorSet {};
        writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeDescriptorSet.dstSet = descriptorSet;
        writeDescriptorSet.dstBinding = static_cast<uint32_t>(writeInfo.size());
        writeDescriptorSet.dstArrayElement = 0;
        writeDescriptorSet.descriptorCount = 1;
        writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writeDescriptorSet.pImageInfo = &baseColorImageInfo;
        
        writeInfo.emplace_back(writeDescriptorSet);
    }
    {// ImageOptions
        VkDescriptorBufferInfo lightViewBufferInfo {};
        lightViewBufferInfo.buffer = imageOptionsBuffer->buffers[0].buffer;
        lightViewBufferInfo.offset = 0;
        lightViewBufferInfo.range = imageOptionsBuffer->bufferSize;
        
        VkWriteDescriptorSet writeDescriptorSet {};
        writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeDescriptorSet.dstSet = descriptorSet;
        writeDescriptorSet.dstBinding = static_cast<uint32_t>(writeInfo.size());
        writeDescriptorSet.dstArrayElement = 0;
        writeDescriptorSet.descriptorCount = 1;
        writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        writeDescriptorSet.pBufferInfo = &lightViewBufferInfo;

        writeInfo.emplace_back(writeDescriptorSet);
    }
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
