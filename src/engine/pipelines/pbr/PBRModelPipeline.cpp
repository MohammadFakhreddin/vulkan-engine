#include "PBRModelPipeline.hpp"

#include "engine/FoundationAsset.hpp"
#include "tools/Importer.hpp"
#include "engine/BedrockPath.hpp"

namespace AS = MFA::AssetSystem;

void MFA::PBRModelPipeline::init(
    RF::SamplerGroup * samplerGroup, 
    RB::GpuTexture * errorTexture
) {
    if (mIsInitialized == true) {
        MFA_ASSERT(false);
        return;
    }
    mIsInitialized = true;
    MFA_ASSERT(samplerGroup != nullptr);
    mSamplerGroup = samplerGroup;
    MFA_ASSERT(errorTexture != nullptr);
    mErrorTexture = errorTexture;

    createUniformBuffers();
    createDescriptorSetLayout();
    createPipeline();
}

void MFA::PBRModelPipeline::shutdown() {
    if (mIsInitialized == false) {
        MFA_ASSERT(false);
        return;
    }
    mIsInitialized = false;
    mSamplerGroup = nullptr;
    mErrorTexture = nullptr;

    destroyPipeline();
    destroyDescriptorSetLayout();
    destroyUniformBuffers();
}

void MFA::PBRModelPipeline::render(
    RF::DrawPass & drawPass,
    float const deltaTime,
    uint32_t const idsCount, 
    DrawableObjectId * ids
) {
    RF::BindDrawPipeline(drawPass, mDrawPipeline);

    for (uint32_t i = 0; i < idsCount; ++i) {
        auto const findResult = mDrawableObjects.find(ids[i]);
        if (findResult != mDrawableObjects.end()) {
            auto * drawableObject = findResult->second.get();
            MFA_ASSERT(drawableObject != nullptr);
            drawableObject->update(deltaTime);
            drawableObject->draw(drawPass);
        }
    }
}

MFA::DrawableObjectId MFA::PBRModelPipeline::addGpuModel(RF::GpuModel & gpuModel) {

    MFA_ASSERT(gpuModel.valid == true);
    
    auto * drawableObject = new DrawableObject(gpuModel, mDescriptorSetLayout);
    MFA_ASSERT(mDrawableObjects.find(drawableObject->getId()) == mDrawableObjects.end());
    mDrawableObjects[drawableObject->getId()] = std::unique_ptr<DrawableObject>(drawableObject);

    const auto * primitiveInfoBuffer = drawableObject->createMultipleUniformBuffer(
        "PrimitiveInfo", 
        sizeof(PrimitiveInfo), 
        drawableObject->getDescriptorSetCount()
    );
    MFA_ASSERT(primitiveInfoBuffer != nullptr);

    const auto * modelTransformBuffer = drawableObject->createUniformBuffer(
        "ViewProjection", 
        sizeof(ViewProjectionData)
    );
    MFA_ASSERT(modelTransformBuffer != nullptr);

    const auto & nodeTransformBuffer = drawableObject->getNodeTransformBuffer();

    auto const & textures = drawableObject->getModel()->textures;

    auto const & mesh = drawableObject->getModel()->model.mesh;

    for (uint32_t nodeIndex = 0; nodeIndex < mesh.getNodesCount(); ++nodeIndex) {// Updating descriptor sets
        auto const & node = mesh.getNodeByIndex(nodeIndex);
        if (node.hasSubMesh()) {
            auto const & subMesh = mesh.getSubMeshByIndex(node.subMeshIndex);
            if (subMesh.primitives.empty() == false) {
                for (auto const & primitive : subMesh.primitives) {
                    MFA_ASSERT(primitive.uniqueId >= 0);
                    auto descriptorSet = drawableObject->getDescriptorSetByPrimitiveUniqueId(primitive.uniqueId);
                    MFA_VK_VALID_ASSERT(descriptorSet);
                    std::vector<VkWriteDescriptorSet> writeInfo {};
                    // TODO Create functionality for this inside render system
                    {// ModelTransform
                        VkDescriptorBufferInfo modelTransformBufferInfo {};
                        modelTransformBufferInfo.buffer = modelTransformBuffer->buffers[0].buffer;
                        modelTransformBufferInfo.offset = 0;
                        modelTransformBufferInfo.range = modelTransformBuffer->bufferSize;

                        VkWriteDescriptorSet writeDescriptorSet {};
                        writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                        writeDescriptorSet.dstSet = descriptorSet;
                        writeDescriptorSet.dstBinding = static_cast<uint32_t>(writeInfo.size());
                        writeDescriptorSet.dstArrayElement = 0;
                        writeDescriptorSet.descriptorCount = 1;
                        writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                        writeDescriptorSet.pBufferInfo = &modelTransformBufferInfo;
                        
                        writeInfo.emplace_back(writeDescriptorSet);
                    }

                    {//NodeTransform
                        VkDescriptorBufferInfo nodeTransformBufferInfo {};
                        nodeTransformBufferInfo.buffer = nodeTransformBuffer.buffers[node.subMeshIndex].buffer,
                        nodeTransformBufferInfo.offset = 0,
                        nodeTransformBufferInfo.range = nodeTransformBuffer.bufferSize;

                        VkWriteDescriptorSet writeDescriptorSet {};
                        writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                        writeDescriptorSet.dstSet = descriptorSet;
                        writeDescriptorSet.dstBinding = static_cast<uint32_t>(writeInfo.size());
                        writeDescriptorSet.dstArrayElement = 0;
                        writeDescriptorSet.descriptorCount = 1;
                        writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                        writeDescriptorSet.pBufferInfo = &nodeTransformBufferInfo;
                        
                        writeInfo.emplace_back(writeDescriptorSet);
                    }
                    {// SkinJoints
                        auto const & skinBuffer = node.skinBufferIndex >= 0
                            ? drawableObject->getSkinTransformBuffer(node.skinBufferIndex)
                            : mErrorBuffer;

                        VkDescriptorBufferInfo skinTransformBufferInfo {};
                        skinTransformBufferInfo.buffer = skinBuffer.buffers[0].buffer;
                        skinTransformBufferInfo.offset = 0;
                        skinTransformBufferInfo.range = skinBuffer.bufferSize;

                        VkWriteDescriptorSet writeDescriptorSet {};
                        writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                        writeDescriptorSet.dstSet = descriptorSet;
                        writeDescriptorSet.dstBinding = static_cast<uint32_t>(writeInfo.size());
                        writeDescriptorSet.dstArrayElement = 0;
                        writeDescriptorSet.descriptorCount = 1;
                        writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                        writeDescriptorSet.pBufferInfo = &skinTransformBufferInfo;
                        writeInfo.emplace_back(writeDescriptorSet);
                    }
                    {// Primitive
                        VkDescriptorBufferInfo primitiveBufferInfo {};
                        primitiveBufferInfo.buffer = primitiveInfoBuffer->buffers[primitive.uniqueId].buffer;
                        primitiveBufferInfo.offset = 0;
                        primitiveBufferInfo.range = primitiveInfoBuffer->bufferSize;

                        VkWriteDescriptorSet writeDescriptorSet {};
                        writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                        writeDescriptorSet.dstSet = descriptorSet;
                        writeDescriptorSet.dstBinding = static_cast<uint32_t>(writeInfo.size());
                        writeDescriptorSet.dstArrayElement = 0;
                        writeDescriptorSet.descriptorCount = 1;
                        writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                        writeDescriptorSet.pBufferInfo = &primitiveBufferInfo;
                        writeInfo.emplace_back(writeDescriptorSet);
                    }

                    {// Update primitive buffer information
                        PrimitiveInfo primitiveInfo {};
                        primitiveInfo.hasBaseColorTexture = primitive.hasBaseColorTexture ? 1 : 0;
                        primitiveInfo.metallicFactor = primitive.metallicFactor;
                        primitiveInfo.roughnessFactor = primitive.roughnessFactor;
                        primitiveInfo.hasMixedMetallicRoughnessOcclusionTexture = primitive.hasMixedMetallicRoughnessOcclusionTexture ? 1 : 0;
                        primitiveInfo.hasNormalTexture = primitive.hasNormalTexture ? 1 : 0;
                        primitiveInfo.hasEmissiveTexture = primitive.hasEmissiveTexture ? 1 : 0;
                        primitiveInfo.hasSkin = primitive.hasSkin ? 1 : 0;

                        ::memcpy(primitiveInfo.baseColorFactor, primitive.baseColorFactor, sizeof(primitiveInfo.baseColorFactor));
                        static_assert(sizeof(primitiveInfo.baseColorFactor) == sizeof(primitive.baseColorFactor));
                        ::memcpy(primitiveInfo.emissiveFactor, primitive.emissiveFactor, sizeof(primitiveInfo.emissiveFactor));
                        static_assert(sizeof(primitiveInfo.emissiveFactor) == sizeof(primitive.emissiveFactor));
                        RF::UpdateUniformBuffer(
                            primitiveInfoBuffer->buffers[primitive.uniqueId], 
                            MFA::CBlobAliasOf(primitiveInfo)
                        );
                    }

                    {// BaseColorTexture
                        VkDescriptorImageInfo baseColorImageInfo {};
                        baseColorImageInfo.sampler = mSamplerGroup->sampler;          // TODO Each texture has it's own properties that may need it's own sampler (Not sure yet)
                        baseColorImageInfo.imageView = primitive.hasBaseColorTexture
                                ? textures[primitive.baseColorTextureIndex].image_view()
                                : mErrorTexture->image_view();
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

                    {// Metallic/RoughnessTexture
                        VkDescriptorImageInfo metallicImageInfo {};
                        metallicImageInfo.sampler = mSamplerGroup->sampler;          // TODO Each texture has it's own properties that may need it's own sampler (Not sure yet)
                        metallicImageInfo.imageView = primitive.hasMixedMetallicRoughnessOcclusionTexture
                                ? textures[primitive.mixedMetallicRoughnessOcclusionTextureIndex].image_view()
                                : mErrorTexture->image_view();
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

                    {// NormalTexture  
                        VkDescriptorImageInfo normalImageInfo {};
                        normalImageInfo.sampler = mSamplerGroup->sampler,
                        normalImageInfo.imageView = primitive.hasNormalTexture
                                ? textures[primitive.normalTextureIndex].image_view()
                                : mErrorTexture->image_view();
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

                    {// EmissiveTexture
                        VkDescriptorImageInfo emissiveImageInfo {};
                        emissiveImageInfo.sampler = mSamplerGroup->sampler;
                        emissiveImageInfo.imageView = primitive.hasEmissiveTexture
                                ? textures[primitive.emissiveTextureIndex].image_view()
                                : mErrorTexture->image_view();
                        emissiveImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

                        VkWriteDescriptorSet writeDescriptorSet {};
                        writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                        writeDescriptorSet.dstSet = descriptorSet;
                        writeDescriptorSet.dstBinding = static_cast<uint32_t>(writeInfo.size());
                        writeDescriptorSet.dstArrayElement = 0;
                        writeDescriptorSet.descriptorCount = 1;
                        writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                        writeDescriptorSet.pImageInfo = &emissiveImageInfo;

                        writeInfo.emplace_back(writeDescriptorSet);
                    }
                    {// LightViewBuffer
                        VkDescriptorBufferInfo lightViewBufferInfo {};
                        lightViewBufferInfo.buffer = mLightViewBuffer.buffers[0].buffer;
                        lightViewBufferInfo.offset = 0;
                        lightViewBufferInfo.range = mLightViewBuffer.bufferSize;

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
            }
        }
    }
    
    return drawableObject->getId();
}

bool MFA::PBRModelPipeline::removeGpuModel(DrawableObjectId const drawableObjectId) {
    auto const deleteCount = mDrawableObjects.erase(drawableObjectId);  // Unique ptr should be deleted correctly
    MFA_ASSERT(deleteCount == 1);
    return deleteCount;
}

bool MFA::PBRModelPipeline::updateViewProjectionBuffer(
    DrawableObjectId const drawableObjectId, 
    ViewProjectionData const & viewProjectionData
) {
    auto const findResult = mDrawableObjects.find(drawableObjectId);
    if (findResult == mDrawableObjects.end()) {
        MFA_ASSERT(false);
        return false;
    }
    auto * drawableObject = findResult->second.get();
    MFA_ASSERT(drawableObject != nullptr);

    drawableObject->updateUniformBuffer(
        "ViewProjection", 
        CBlobAliasOf(viewProjectionData)
    );

    return true;
}

void MFA::PBRModelPipeline::updateLightViewBuffer(
    LightViewBuffer const & lightViewData
) {
    MFA_ASSERT(mLightViewBuffer.isValid());
    MFA_ASSERT(mLightViewBuffer.buffers.size() == 1);
    RF::UpdateUniformBuffer(
        mLightViewBuffer.buffers[0],
        CBlobAliasOf(lightViewData)
    );
}

// TODO We can place common pipelines inside a class
MFA::DrawableObject * MFA::PBRModelPipeline::GetDrawableById(DrawableObjectId const objectId) {
    auto const findResult = mDrawableObjects.find(objectId);
    if (findResult != mDrawableObjects.end()) {
        auto * drawableObject = findResult->second.get();
        return drawableObject;
    }
    return nullptr;
}

void MFA::PBRModelPipeline::createDescriptorSetLayout() {
    std::vector<VkDescriptorSetLayoutBinding> bindings {};
    {// ModelTransformation
        VkDescriptorSetLayoutBinding layoutBinding {};
        layoutBinding.binding = static_cast<uint32_t>(bindings.size());
        layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        layoutBinding.descriptorCount = 1;
        layoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        bindings.emplace_back(layoutBinding);
    }
    {// NodeTransformation
        VkDescriptorSetLayoutBinding layoutBinding {};
        layoutBinding.binding = static_cast<uint32_t>(bindings.size());
        layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        layoutBinding.descriptorCount = 1;
        layoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        bindings.emplace_back(layoutBinding);
    }
    {// SkinJoints
        VkDescriptorSetLayoutBinding layoutBinding {};
        layoutBinding.binding = static_cast<uint32_t>(bindings.size());
        layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        layoutBinding.descriptorCount = 1;
        layoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        bindings.emplace_back(layoutBinding);
    }
    {// Primitive
        VkDescriptorSetLayoutBinding layoutBinding {};
        layoutBinding.binding = static_cast<uint32_t>(bindings.size());
        layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        layoutBinding.descriptorCount = 1;
        layoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
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
    {// Metallic/Roughness
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
        layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        layoutBinding.descriptorCount = 1,
        layoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
        bindings.emplace_back(layoutBinding);
    }
    {// Emissive
        VkDescriptorSetLayoutBinding layoutBinding {};
        layoutBinding.binding = 7;
        layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        layoutBinding.descriptorCount = 1;
        layoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        
        bindings.emplace_back(layoutBinding);
    }
    {// Light/View
        VkDescriptorSetLayoutBinding layoutBinding {};
        layoutBinding.binding = 8;
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

void MFA::PBRModelPipeline::destroyDescriptorSetLayout() {
    MFA_VK_VALID_ASSERT(mDescriptorSetLayout);
    RF::DestroyDescriptorSetLayout(mDescriptorSetLayout);    
}

void MFA::PBRModelPipeline::createPipeline() {
    // TODO We need path class
    // Vertex shader
    auto cpuVertexShader = Importer::ImportShaderFromSPV(
        Path::Asset("shaders/pbr_model/PbrModel.vert.spv").c_str(),
        MFA::AssetSystem::Shader::Stage::Vertex, 
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
        Path::Asset("shaders/pbr_model/PbrModel.frag.spv").c_str(),
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

    VkVertexInputBindingDescription vertexInputBindingDescription {};
    vertexInputBindingDescription.binding = 0;
    vertexInputBindingDescription.stride = sizeof(AS::MeshVertex);
    vertexInputBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    
    std::vector<VkVertexInputAttributeDescription> inputAttributeDescriptions {};

    {// Position
        VkVertexInputAttributeDescription attributeDescription {};
        attributeDescription.location = static_cast<uint32_t>(inputAttributeDescriptions.size()),
        attributeDescription.binding = 0,
        attributeDescription.format = VK_FORMAT_R32G32B32_SFLOAT,
        attributeDescription.offset = offsetof(AS::MeshVertex, position),   
        
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
    {// Metallic/Roughness
        VkVertexInputAttributeDescription attributeDescription {};
        attributeDescription.location = static_cast<uint32_t>(inputAttributeDescriptions.size());
        attributeDescription.binding = 0;
        attributeDescription.format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescription.offset = offsetof(AS::MeshVertex, metallicUV); // Metallic and roughness have same uv for gltf files  

        inputAttributeDescriptions.emplace_back(attributeDescription);
    }
    {// NormalMapUV
        VkVertexInputAttributeDescription attributeDescription {};
        attributeDescription.location = static_cast<uint32_t>(inputAttributeDescriptions.size());
        attributeDescription.binding = 0;
        attributeDescription.format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescription.offset = offsetof(AS::MeshVertex, normalMapUV);

        inputAttributeDescriptions.emplace_back(attributeDescription);
    }
    {// Tangent
        VkVertexInputAttributeDescription attributeDescription {};
        attributeDescription.location = static_cast<uint32_t>(inputAttributeDescriptions.size());
        attributeDescription.binding = 0;
        attributeDescription.format = VK_FORMAT_R32G32B32A32_SFLOAT;
        attributeDescription.offset = offsetof(AS::MeshVertex, tangentValue);
        
        inputAttributeDescriptions.emplace_back(attributeDescription);
    }
    {// Normal
        VkVertexInputAttributeDescription attributeDescription {};
        attributeDescription.location = static_cast<uint32_t>(inputAttributeDescriptions.size());
        attributeDescription.binding = 0;
        attributeDescription.format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescription.offset = offsetof(AS::MeshVertex, normalValue);   

        inputAttributeDescriptions.emplace_back(attributeDescription);
    }
    {// EmissionUV
        VkVertexInputAttributeDescription attributeDescription {};
        attributeDescription.location = static_cast<uint32_t>(inputAttributeDescriptions.size());
        attributeDescription.binding = 0;
        attributeDescription.format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescription.offset = offsetof(AS::MeshVertex, emissionUV);
        inputAttributeDescriptions.emplace_back(attributeDescription);
    }
    {// HasSkin
        VkVertexInputAttributeDescription attributeDescription {};
        attributeDescription.location = static_cast<uint32_t>(inputAttributeDescriptions.size());
        attributeDescription.binding = 0;
        attributeDescription.format = VK_FORMAT_R32_SINT;
        attributeDescription.offset = offsetof(AS::MeshVertex, hasSkin); // TODO We should use a primitiveInfo instead
        inputAttributeDescriptions.emplace_back(attributeDescription);
    }
    {// JointIndices
        VkVertexInputAttributeDescription attributeDescription {};
        attributeDescription.location = static_cast<uint32_t>(inputAttributeDescriptions.size());
        attributeDescription.binding = 0;
        attributeDescription.format = VK_FORMAT_R32G32B32A32_SINT;
        attributeDescription.offset = offsetof(AS::MeshVertex, jointIndices);
        inputAttributeDescriptions.emplace_back(attributeDescription);
    }
    {// JointWeights
        VkVertexInputAttributeDescription attributeDescription {};
        attributeDescription.location = static_cast<uint32_t>(inputAttributeDescriptions.size());
        attributeDescription.binding = 0;
        attributeDescription.format = VK_FORMAT_R32G32B32A32_SFLOAT;
        attributeDescription.offset = offsetof(AS::MeshVertex, jointWeights);
        inputAttributeDescriptions.emplace_back(attributeDescription);
    }
    MFA_ASSERT(mDrawPipeline.isValid() == false);
    mDrawPipeline = RF::CreateBasicDrawPipeline(
        static_cast<uint8_t>(shaders.size()), 
        shaders.data(),
        1,
        &mDescriptorSetLayout,
        vertexInputBindingDescription,
        static_cast<uint8_t>(inputAttributeDescriptions.size()),
        inputAttributeDescriptions.data()
    );
}

void MFA::PBRModelPipeline::destroyPipeline() {
    MFA_ASSERT(mDrawPipeline.isValid());
    RF::DestroyDrawPipeline(mDrawPipeline);
}

void MFA::PBRModelPipeline::createUniformBuffers() {
    mLightViewBuffer = RF::CreateUniformBuffer(sizeof(LightViewBuffer), 1);
    mErrorBuffer = RF::CreateUniformBuffer(sizeof(DrawableObject::JointTransformBuffer), 1);
}

void MFA::PBRModelPipeline::destroyUniformBuffers() {
    RF::DestroyUniformBuffer(mErrorBuffer);
    RF::DestroyUniformBuffer(mLightViewBuffer);

    for (const auto & [id, drawableObject] : mDrawableObjects) {
        MFA_ASSERT(drawableObject != nullptr);
        drawableObject->deleteUniformBuffers();
    }
    mDrawableObjects.clear();
}
