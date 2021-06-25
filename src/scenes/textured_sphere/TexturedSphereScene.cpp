#include "TexturedSphereScene.hpp"

#include "tools/ShapeGenerator.hpp"
#include "engine/BedrockPath.hpp"

namespace UI = MFA::UISystem;
namespace Path = MFA::Path;

void TexturedSphereScene::Init() {
    // Gpu model
    createGpuModel();

    // Cpu shader
    auto cpuVertexShader = Importer::ImportShaderFromSPV(
        Path::Asset("shaders/textured_sphere/TexturedSphere.vert.spv").c_str(), 
        AS::Shader::Stage::Vertex, 
        "main"
    );
    MFA_ASSERT(cpuVertexShader.isValid());
    auto gpuVertexShader = RF::CreateShader(cpuVertexShader);
    MFA_ASSERT(gpuVertexShader.valid());
    MFA_DEFER {
        RF::DestroyShader(gpuVertexShader);
        Importer::FreeShader(&cpuVertexShader);
    };
    
    // Fragment shader
    auto cpuFragmentShader = Importer::ImportShaderFromSPV(
        Path::Asset("shaders/textured_sphere/TexturedSphere.frag.spv").c_str(),
        MFA::AssetSystem::Shader::Stage::Fragment,
        "main"
    );
    auto gpuFragmentShader = RF::CreateShader(cpuFragmentShader);
    MFA_ASSERT(cpuFragmentShader.isValid());
    MFA_ASSERT(gpuFragmentShader.valid());
    MFA_DEFER {
        RF::DestroyShader(gpuFragmentShader);
        Importer::FreeShader(&cpuFragmentShader);
    };

    std::vector<RB::GpuShader> shaders {gpuVertexShader, gpuFragmentShader};

    // TODO We should use gltf sampler info here
    mSamplerGroup = RF::CreateSampler();

    mLVBuffer = RF::CreateUniformBuffer(sizeof(LightViewBuffer), 1);

    createDescriptorSetLayout();

    createDrawPipeline(static_cast<uint8_t>(shaders.size()), shaders.data());
    
    createDrawableObject();

    updateProjectionBuffer();
}

void TexturedSphereScene::OnDraw(float deltaTimeInSec, MFA::RenderFrontend::DrawPass & drawPass) {
    RF::BindDrawPipeline(drawPass, mDrawPipeline);

    {// Updating Transform buffer
        // Rotation
        // TODO Try sending Matrices directly
        MFA::Matrix4X4Float rotationMat {};
        MFA::Matrix4X4Float::AssignRotation(
            rotationMat,
            mModelRotation[0],
            mModelRotation[1],
            mModelRotation[2]
        );
        static_assert(sizeof(mTranslateData.rotation) == sizeof(rotationMat.cells));
        ::memcpy(mTranslateData.rotation, rotationMat.cells, sizeof(rotationMat.cells));
        // Position
        MFA::Matrix4X4Float transformationMat {};
        MFA::Matrix4X4Float::AssignTranslation(
            transformationMat,
            mModelPosition[0],
            mModelPosition[1],
            mModelPosition[2]
        );
        static_assert(sizeof(mTranslateData.transformation) == sizeof(transformationMat.cells));
        ::memcpy(mTranslateData.transformation, transformationMat.cells, sizeof(transformationMat.cells));

        
        mDrawableObject->updateUniformBuffer(
            "transform", 
            MFA::CBlobAliasOf(mTranslateData)
        );
    }
    {// LightViewBuffer
        ::memcpy(mLightViewData.light_position, mLightPosition, sizeof(mLightPosition));
        static_assert(sizeof(mLightPosition) == sizeof(mLightViewData.light_position));

        ::memcpy(mLightViewData.light_color, mLightColor, sizeof(mLightColor));
        static_assert(sizeof(mLightColor) == sizeof(mLightViewData.light_color));

        RF::UpdateUniformBuffer(mLVBuffer.buffers[0], MFA::CBlobAliasOf(mLightViewData));
    }
    mDrawableObject->update(deltaTimeInSec);
    mDrawableObject->draw(drawPass);
}

void TexturedSphereScene::OnUI(float deltaTimeInSec, MFA::RenderFrontend::DrawPass & draw_pass) {
    UI::BeginWindow("Object viewer");
    UI::SetNextItemWidth(300.0f);
    UI::SliderFloat("XDegree", &mModelRotation[0], -360.0f, 360.0f);
    UI::SetNextItemWidth(300.0f);
    UI::SliderFloat("YDegree", &mModelRotation[1], -360.0f, 360.0f);
    UI::SetNextItemWidth(300.0f);
    UI::SliderFloat("ZDegree", &mModelRotation[2], -360.0f, 360.0f);
    UI::SetNextItemWidth(300.0f);
    UI::SliderFloat("XDistance", &mModelPosition[0], -100.0f, 100.0f);
    UI::SetNextItemWidth(300.0f);
    UI::SliderFloat("YDistance", &mModelPosition[1], -100.0f, 100.0f);
    UI::SetNextItemWidth(300.0f);
    UI::SliderFloat("ZDistance", &mModelPosition[2], -100.0f, 100.0f);
    UI::EndWindow();

    UI::BeginWindow("Light");
    UI::SetNextItemWidth(300.0f);
    UI::SliderFloat("PositionX", &mLightPosition[0], -200.0f, 200.0f);
    UI::SetNextItemWidth(300.0f);
    UI::SliderFloat("PositionY", &mLightPosition[1], -200.0f, 200.0f);
    UI::SetNextItemWidth(300.0f);
    UI::SliderFloat("PositionZ", &mLightPosition[2], -200.0f, 200.0f);
    UI::SetNextItemWidth(300.0f);
    UI::SliderFloat("ColorR", &mLightColor[0], 0.0f, 1.0f);
    UI::SetNextItemWidth(300.0f);
    UI::SliderFloat("ColorG", &mLightColor[1], 0.0f, 1.0f);
    UI::SetNextItemWidth(300.0f);
    UI::SliderFloat("ColorB", &mLightColor[2], 0.0f, 1.0f);
    UI::EndWindow();
}

void TexturedSphereScene::Shutdown() {
    destroyDrawableObject();
    RF::DestroyUniformBuffer(mLVBuffer);
    RF::DestroyDrawPipeline(mDrawPipeline);
    RF::DestroyDescriptorSetLayout(mDescriptorSetLayout);
    RF::DestroySampler(mSamplerGroup);
    RF::DestroyGpuModel(mGpuModel);
    Importer::FreeModel(&mGpuModel.model);
}

void TexturedSphereScene::OnResize() {
    updateProjectionBuffer();
}

void TexturedSphereScene::createDrawableObject(){

    auto const & mesh = mGpuModel.model.mesh;

     mDrawableObject = std::make_unique<MFA::DrawableObject>(
        mGpuModel,
        mDescriptorSetLayout
    );
    // TODO AO map for emission
    const auto * transformBuffer = mDrawableObject->createUniformBuffer("transform", sizeof(ModelTransformBuffer));
    MFA_ASSERT(transformBuffer != nullptr);
    
    auto const & textures = mDrawableObject->getModel()->textures;

    for (uint32_t nodeIndex = 0; nodeIndex < mesh.getNodesCount(); ++nodeIndex) {// Updating descriptor sets
        auto const & node = mesh.getNodeByIndex(nodeIndex);
        auto const & subMesh = mesh.getSubMeshByIndex(node.subMeshIndex);
        if (subMesh.primitives.empty() == false) {
            for (auto const & primitive : subMesh.primitives) {
                MFA_ASSERT(primitive.uniqueId >= 0);
                auto descriptorSet = mDrawableObject->getDescriptorSetByPrimitiveUniqueId(primitive.uniqueId);
                MFA_VK_VALID_ASSERT(descriptorSet);

                std::vector<VkWriteDescriptorSet> writeInfo {};

                {// Transform
                    VkDescriptorBufferInfo transformBufferInfo {};
                    transformBufferInfo.buffer = transformBuffer->buffers[0].buffer;
                    transformBufferInfo.offset = 0;
                    transformBufferInfo.range = transformBuffer->bufferSize;

                    VkWriteDescriptorSet writeDescriptorSet {};
                    writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                    writeDescriptorSet.dstSet = descriptorSet;
                    writeDescriptorSet.dstBinding = static_cast<uint32_t>(writeInfo.size());
                    writeDescriptorSet.dstArrayElement = 0;
                    writeDescriptorSet.descriptorCount = 1;
                    writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                    writeDescriptorSet.pBufferInfo = &transformBufferInfo;

                    writeInfo.emplace_back(writeDescriptorSet);
                }
                {// BaseColorTexture
                    VkDescriptorImageInfo baseColorImageInfo {};
                    baseColorImageInfo.sampler = mSamplerGroup.sampler;          // TODO Each texture has it's own properties that may need it's own sampler (Not sure yet)
                    baseColorImageInfo.imageView = textures[primitive.baseColorTextureIndex].image_view();
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
                {// MetallicTexture
                    VkDescriptorImageInfo metallicImageInfo {};
                    metallicImageInfo.sampler = mSamplerGroup.sampler;          // TODO Each texture has it's own properties that may need it's own sampler (Not sure yet)
                    metallicImageInfo.imageView = textures[primitive.metallicTextureIndex].image_view();
                    metallicImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

                    VkWriteDescriptorSet writeDescriptorSet {};
                    writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                    writeDescriptorSet.dstSet = descriptorSet;
                    writeDescriptorSet.dstBinding = static_cast<uint32_t>(writeInfo.size());
                    writeDescriptorSet.dstArrayElement = 0;
                    writeDescriptorSet.descriptorCount = 1;
                    writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                    writeDescriptorSet.pImageInfo = &metallicImageInfo;

                    writeInfo.emplace_back(writeDescriptorSet);
                }
                {// RoughnessTexture
                    VkDescriptorImageInfo roughnessImageInfo {};
                    roughnessImageInfo.sampler = mSamplerGroup.sampler;          // TODO Each texture has it's own properties that may need it's own sampler (Not sure yet)
                    roughnessImageInfo.imageView = textures[primitive.roughnessTextureIndex].image_view();
                    roughnessImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

                    VkWriteDescriptorSet writeDescriptorSet {};
                    writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                    writeDescriptorSet.dstSet = descriptorSet;
                    writeDescriptorSet.dstBinding = static_cast<uint32_t>(writeInfo.size());
                    writeDescriptorSet.dstArrayElement = 0;
                    writeDescriptorSet.descriptorCount = 1;
                    writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                    writeDescriptorSet.pImageInfo = &roughnessImageInfo;

                    writeInfo.emplace_back(writeDescriptorSet);
                }
                {// NormalTexture  
                    VkDescriptorImageInfo normalImageInfo {};
                    normalImageInfo.sampler = mSamplerGroup.sampler;
                    normalImageInfo.imageView = textures[primitive.normalTextureIndex].image_view();
                    normalImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

                    VkWriteDescriptorSet writeDescriptorSet {};
                    writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                    writeDescriptorSet.dstSet = descriptorSet;
                    writeDescriptorSet.dstBinding = static_cast<uint32_t>(writeInfo.size());
                    writeDescriptorSet.dstArrayElement = 0;
                    writeDescriptorSet.descriptorCount = 1;
                    writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                    writeDescriptorSet.pImageInfo = &normalImageInfo;

                    writeInfo.emplace_back(writeDescriptorSet);
                }
                {// LightViewBuffer
                    VkDescriptorBufferInfo lightViewBufferInfo {};
                    lightViewBufferInfo.buffer = mLVBuffer.buffers[0].buffer;
                    lightViewBufferInfo.offset = 0;
                    lightViewBufferInfo.range = mLVBuffer.bufferSize;

                    VkWriteDescriptorSet writeDescriptorSet {};
                    writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                    writeDescriptorSet.dstSet = descriptorSet;
                    writeDescriptorSet.dstBinding = 5;
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
        }
    }
}

void TexturedSphereScene::destroyDrawableObject() const {
    mDrawableObject->deleteUniformBuffers();
}

void TexturedSphereScene::createDrawPipeline(uint8_t gpuShaderCount, MFA::RenderBackend::GpuShader * gpuShaders){
    VkVertexInputBindingDescription bindingDescription {};
    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(AS::MeshVertex);
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    
    std::vector<VkVertexInputAttributeDescription> inputAttributeDescriptions {};
    {// Position
        VkVertexInputAttributeDescription attributeDescription {};
        attributeDescription.location = static_cast<uint32_t>(inputAttributeDescriptions.size());
        attributeDescription.binding = 0;
        attributeDescription.format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescription.offset = offsetof(AS::MeshVertex, position);
        inputAttributeDescriptions.emplace_back(attributeDescription);
    }
    {// BaseColorUV
        VkVertexInputAttributeDescription attributeDescription {};
        attributeDescription.location = static_cast<uint32_t>(inputAttributeDescriptions.size());
        attributeDescription.binding = 0;
        attributeDescription.format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescription.offset = offsetof(AS::MeshVertex, baseColorUV);
        inputAttributeDescriptions.emplace_back(attributeDescription);
    }
    {// MetallicUV
        VkVertexInputAttributeDescription attributeDescription {};
        attributeDescription.location = static_cast<uint32_t>(inputAttributeDescriptions.size());
        attributeDescription.binding = 0;
        attributeDescription.format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescription.offset = offsetof(AS::MeshVertex, metallicUV);   
        inputAttributeDescriptions.emplace_back(attributeDescription);
    }
    {// RoughnessUV
        VkVertexInputAttributeDescription attributeDescription {};
        attributeDescription.location = 3;
        attributeDescription.binding = 0;
        attributeDescription.format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescription.offset = offsetof(AS::MeshVertex, roughnessUV); 
        inputAttributeDescriptions.emplace_back(attributeDescription);
    }
    {// NormalMapUV
        VkVertexInputAttributeDescription attributeDescription {};
        attributeDescription.location = 4;
        attributeDescription.binding = 0;
        attributeDescription.format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescription.offset = offsetof(AS::MeshVertex, normalMapUV);
        inputAttributeDescriptions.emplace_back(attributeDescription);
    }
    {// NormalValue
        VkVertexInputAttributeDescription attributeDescription {};
        attributeDescription.location = 5;
        attributeDescription.binding = 0;
        attributeDescription.format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescription.offset = offsetof(AS::MeshVertex, normalValue);  
        inputAttributeDescriptions.emplace_back(attributeDescription);
    }
    {// TangentValue
        VkVertexInputAttributeDescription attributeDescription {};
        attributeDescription.location = 6;
        attributeDescription.binding = 0;
        attributeDescription.format = VK_FORMAT_R32G32B32A32_SFLOAT;
        attributeDescription.offset = offsetof(AS::MeshVertex, tangentValue);
        inputAttributeDescriptions.emplace_back(attributeDescription);
    }
    // Create DrawPipeline
    RB::CreateGraphicPipelineOptions graphicPipelineOptions {};
    graphicPipelineOptions.depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    graphicPipelineOptions.depthStencil.depthTestEnable = VK_TRUE;
    graphicPipelineOptions.depthStencil.depthWriteEnable = VK_TRUE;
    graphicPipelineOptions.depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
    graphicPipelineOptions.depthStencil.depthBoundsTestEnable = VK_FALSE;
    graphicPipelineOptions.depthStencil.stencilTestEnable = VK_FALSE;
    graphicPipelineOptions.colorBlendAttachments.blendEnable = VK_TRUE;
    graphicPipelineOptions.colorBlendAttachments.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    graphicPipelineOptions.colorBlendAttachments.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    graphicPipelineOptions.colorBlendAttachments.colorBlendOp = VK_BLEND_OP_ADD;
    graphicPipelineOptions.colorBlendAttachments.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    graphicPipelineOptions.colorBlendAttachments.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    graphicPipelineOptions.colorBlendAttachments.alphaBlendOp = VK_BLEND_OP_ADD;
    graphicPipelineOptions.colorBlendAttachments.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    graphicPipelineOptions.useStaticViewportAndScissor = false;
    graphicPipelineOptions.primitiveTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
    mDrawPipeline = RF::CreateDrawPipeline(
        gpuShaderCount, 
        gpuShaders,
        1,
        &mDescriptorSetLayout,
        bindingDescription,
        static_cast<uint8_t>(inputAttributeDescriptions.size()),
        inputAttributeDescriptions.data(),
        graphicPipelineOptions
    );
}

void TexturedSphereScene::createDescriptorSetLayout(){
    std::vector<VkDescriptorSetLayoutBinding> bindings {};
    {// Transformation 
        VkDescriptorSetLayoutBinding layoutBinding {};
        layoutBinding.binding = static_cast<uint32_t>(bindings.size());
        layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        layoutBinding.descriptorCount = 1;
        layoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

        bindings.emplace_back(layoutBinding);
    }
    {// BaseColor
        VkDescriptorSetLayoutBinding layoutBinding {};
        layoutBinding.binding = static_cast<uint32_t>(bindings.size());
        layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        layoutBinding.descriptorCount = 1;
        layoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        bindings.emplace_back(layoutBinding);
    }
    {// Metallic
        VkDescriptorSetLayoutBinding layoutBinding {};
        layoutBinding.binding = static_cast<uint32_t>(bindings.size());
        layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        layoutBinding.descriptorCount = 1;
        layoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        bindings.emplace_back(layoutBinding);
    }
    {// Roughness
        VkDescriptorSetLayoutBinding layoutBinding {};
        layoutBinding.binding = static_cast<uint32_t>(bindings.size());
        layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        layoutBinding.descriptorCount = 1;
        layoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        bindings.emplace_back(layoutBinding);
    }
    {// Normal
        VkDescriptorSetLayoutBinding layoutBinding {};
        layoutBinding.binding = static_cast<uint32_t>(bindings.size());
        layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        layoutBinding.descriptorCount = 1;
        layoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        
        bindings.emplace_back(layoutBinding);
    }
    {// Light/View
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

void TexturedSphereScene::createGpuModel() {
    auto cpuModel = MFA::ShapeGenerator::Sphere();

    auto const importTextureForModel = [&cpuModel](char const * address) -> int16_t {
        Importer::ImportTextureOptions options {};
        options.tryToGenerateMipmaps = false;
        auto const texture = Importer::ImportUncompressedImage(
            address, 
            options
        );
        MFA_ASSERT(texture.isValid());
        cpuModel.textures.emplace_back(texture);
        return static_cast<int16_t>(cpuModel.textures.size() - 1);
    };

    // BaseColor
    auto const baseColorIndex = importTextureForModel(Path::Asset("models/rusted_sphere_texture/rustediron2_basecolor.png").c_str());
    // Metallic
    auto const metallicIndex = importTextureForModel(Path::Asset("models/rusted_sphere_texture/rustediron2_metallic.png").c_str());
    // Normal
    auto const normalIndex = importTextureForModel(Path::Asset("models/rusted_sphere_texture/rustediron2_normal.png").c_str());
    // Roughness
    auto const roughnessIndex = importTextureForModel(Path::Asset("models/rusted_sphere_texture/rustediron2_roughness.png").c_str());
    
    auto & mesh = cpuModel.mesh;
    MFA_ASSERT(mesh.isValid());
    for (uint32_t i = 0; i < mesh.getSubMeshCount(); ++i) {
        auto & subMesh = mesh.getSubMeshByIndex(i);
        for (auto & primitive : subMesh.primitives) {
            // Texture index
            primitive.baseColorTextureIndex = baseColorIndex;
            primitive.normalTextureIndex = normalIndex;
            primitive.metallicTextureIndex = metallicIndex;
            primitive.roughnessTextureIndex = roughnessIndex;
            // SubMesh features
            primitive.hasBaseColorTexture = true;
            primitive.hasNormalTexture = true;
            primitive.hasMetallicTexture = true;
            primitive.hasRoughnessTexture = true;
        }
    }

    mGpuModel = RF::CreateGpuModel(cpuModel);
}

void TexturedSphereScene::updateProjectionBuffer() {
    // Perspective
    int32_t width; int32_t height;
    RF::GetDrawableSize(width, height);
    float const ratio = static_cast<float>(width) / static_cast<float>(height);
    MFA::Matrix4X4Float perspectiveMat {};
    MFA::Matrix4X4Float::PreparePerspectiveProjectionMatrix(
        perspectiveMat,
        ratio,
        40,
        Z_NEAR,
        Z_FAR
    );
    static_assert(sizeof(mTranslateData.perspective) == sizeof(perspectiveMat.cells));
    ::memcpy(mTranslateData.perspective, perspectiveMat.cells, sizeof(perspectiveMat.cells));
}
