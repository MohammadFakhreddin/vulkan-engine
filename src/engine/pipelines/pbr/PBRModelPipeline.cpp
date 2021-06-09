#include "PBRModelPipeline.hpp"

#include "engine/FoundationAsset.hpp"
#include "tools/Importer.hpp"

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

    const auto & skinTransformBuffer = drawableObject->getSkinTransformBuffer();

    auto const & textures = drawableObject->getModel()->textures;

    auto const & mesh = drawableObject->getModel()->model.mesh;

    for (uint32_t nodeIndex = 0; nodeIndex < mesh.getNodesCount(); ++nodeIndex) {// Updating descriptor sets
        auto const & node = mesh.getNodeByIndex(nodeIndex);
        if (node.hasSubMesh()) {
            auto const & subMesh = mesh.getSubMeshByIndex(node.subMeshIndex);
            if (subMesh.primitives.empty() == false) {
                for (auto const & primitive : subMesh.primitives) {
                    MFA_ASSERT(primitive.uniqueId >= 0);
                    auto * descriptorSet = drawableObject->getDescriptorSetByPrimitiveUniqueId(primitive.uniqueId);
                    MFA_ASSERT(descriptorSet != nullptr);

                    std::vector<VkWriteDescriptorSet> writeInfo {};

                    // ModelTransform
                    VkDescriptorBufferInfo modelTransformBufferInfo {
                        .buffer = modelTransformBuffer->buffers[0].buffer,
                        .offset = 0,
                        .range = modelTransformBuffer->bufferSize
                    };
                    writeInfo.emplace_back(VkWriteDescriptorSet {
                        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                        .dstSet = descriptorSet,
                        .dstBinding = 0,
                        .dstArrayElement = 0,
                        .descriptorCount = 1,
                        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                        .pBufferInfo = &modelTransformBufferInfo,
                    });

                    //NodeTransform
                    VkDescriptorBufferInfo nodeTransformBufferInfo {
                        .buffer = nodeTransformBuffer.buffers[node.subMeshIndex].buffer,
                        .offset = 0,
                        .range = nodeTransformBuffer.bufferSize
                    };
                    writeInfo.emplace_back(VkWriteDescriptorSet {
                        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                        .dstSet = descriptorSet,
                        .dstBinding = 1,
                        .dstArrayElement = 0,
                        .descriptorCount = 1,
                        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                        .pBufferInfo = &nodeTransformBufferInfo,
                    });

                    // SkinJoints
                    VkDescriptorBufferInfo skinTransformBufferInfo {
                        .buffer = primitive.hasSkin ? skinTransformBuffer.buffers[node.skinBufferIndex].buffer : mErrorBuffer.buffers[0].buffer,
                        .offset = 0,
                        .range = primitive.hasSkin ? skinTransformBuffer.bufferSize : mErrorBuffer.bufferSize
                    };
                    writeInfo.emplace_back(VkWriteDescriptorSet {
                        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                        .dstSet = descriptorSet,
                        .dstBinding = 2,
                        .dstArrayElement = 0,
                        .descriptorCount = 1,
                        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                        .pBufferInfo = &skinTransformBufferInfo,
                    });

                    // Primitive
                    VkDescriptorBufferInfo primitiveBufferInfo {
                        .buffer = primitiveInfoBuffer->buffers[primitive.uniqueId].buffer,
                        .offset = 0,
                        .range = primitiveInfoBuffer->bufferSize
                    };
                    writeInfo.emplace_back(VkWriteDescriptorSet {
                        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                        .dstSet = descriptorSet,
                        .dstBinding = 3,
                        .dstArrayElement = 0,
                        .descriptorCount = 1,
                        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                        .pBufferInfo = &primitiveBufferInfo,
                    });

                    // Update primitive buffer information
                    PrimitiveInfo info {
                        .baseColorFactor {},
                        .emissiveFactor {},
                        .placeholder0 {},
                        .hasBaseColorTexture = primitive.hasBaseColorTexture ? 1 : 0,
                        .metallicFactor = primitive.metallicFactor,
                        .roughnessFactor = primitive.roughnessFactor,
                        .hasMixedMetallicRoughnessOcclusionTexture = primitive.hasMixedMetallicRoughnessOcclusionTexture ? 1 : 0,
                        .hasNormalTexture = primitive.hasNormalTexture ? 1 : 0,
                        .hasEmissiveTexture = primitive.hasEmissiveTexture ? 1 : 0,
                        .hasSkin = primitive.hasSkin ? 1 : 0
                    };
                    ::memcpy(info.baseColorFactor, primitive.baseColorFactor, sizeof(info.baseColorFactor));
                    static_assert(sizeof(info.baseColorFactor) == sizeof(primitive.baseColorFactor));
                    ::memcpy(info.emissiveFactor, primitive.emissiveFactor, sizeof(info.emissiveFactor));
                    static_assert(sizeof(info.emissiveFactor) == sizeof(primitive.emissiveFactor));
                    RF::UpdateUniformBuffer(
                        primitiveInfoBuffer->buffers[primitive.uniqueId], 
                        MFA::CBlobAliasOf(info)
                    );

                    // BaseColorTexture
                    VkDescriptorImageInfo baseColorImageInfo {
                        .sampler = mSamplerGroup->sampler,          // TODO Each texture has it's own properties that may need it's own sampler (Not sure yet)
                        .imageView = primitive.hasBaseColorTexture
                            ? textures[primitive.baseColorTextureIndex].image_view()
                            : mErrorTexture->image_view(),
                        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                    };
                    writeInfo.emplace_back(VkWriteDescriptorSet {
                        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                        .dstSet = descriptorSet,
                        .dstBinding = 4,
                        .dstArrayElement = 0,
                        .descriptorCount = 1,
                        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                        .pImageInfo = &baseColorImageInfo,
                    });

                    // Metallic/RoughnessTexture
                    VkDescriptorImageInfo metallicImageInfo {
                        .sampler = mSamplerGroup->sampler,          // TODO Each texture has it's own properties that may need it's own sampler (Not sure yet)
                        .imageView = primitive.hasMixedMetallicRoughnessOcclusionTexture
                            ? textures[primitive.mixedMetallicRoughnessOcclusionTextureIndex].image_view()
                            : mErrorTexture->image_view(),
                        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
                    };
                    writeInfo.emplace_back(VkWriteDescriptorSet {
                        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                        .dstSet = descriptorSet,
                        .dstBinding = 5,
                        .dstArrayElement = 0,
                        .descriptorCount = 1,
                        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                        .pImageInfo = &metallicImageInfo,
                    });

                    // NormalTexture  
                    VkDescriptorImageInfo normalImageInfo {
                        .sampler = mSamplerGroup->sampler,
                        .imageView = primitive.hasNormalTexture
                            ? textures[primitive.normalTextureIndex].image_view()
                            : mErrorTexture->image_view(),
                        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
                    };
                    writeInfo.emplace_back(VkWriteDescriptorSet {
                        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                        .dstSet = descriptorSet,
                        .dstBinding = 6,
                        .dstArrayElement = 0,
                        .descriptorCount = 1,
                        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                        .pImageInfo = &normalImageInfo,
                    });

                    // EmissiveTexture
                    VkDescriptorImageInfo emissiveImageInfo {
                        .sampler = mSamplerGroup->sampler,
                        .imageView = primitive.hasEmissiveTexture
                            ? textures[primitive.emissiveTextureIndex].image_view()
                            : mErrorTexture->image_view(),
                        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
                    };
                    writeInfo.emplace_back(VkWriteDescriptorSet {
                        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                        .dstSet = descriptorSet,
                        .dstBinding = 7,
                        .dstArrayElement = 0,
                        .descriptorCount = 1,
                        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                        .pImageInfo = &emissiveImageInfo,
                    });

                    // LightViewBuffer
                    VkDescriptorBufferInfo light_view_buffer_info {
                        .buffer = mLightViewBuffer.buffers[0].buffer,
                        .offset = 0,
                        .range = mLightViewBuffer.bufferSize
                    };
                    writeInfo.emplace_back(VkWriteDescriptorSet {
                        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                        .dstSet = descriptorSet,
                        .dstBinding = 8,
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

void MFA::PBRModelPipeline::createDescriptorSetLayout() {
    std::vector<VkDescriptorSetLayoutBinding> bindings {};
    // ModelTransformation 
    bindings.emplace_back(VkDescriptorSetLayoutBinding {
        .binding = 0,
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
        .pImmutableSamplers = nullptr, // Optional
    });
    // NodeTransformation
    bindings.emplace_back(VkDescriptorSetLayoutBinding {
        .binding = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
        .pImmutableSamplers = nullptr, // Optional
    });
    // SkinJoints
    bindings.emplace_back(VkDescriptorSetLayoutBinding {
        .binding = 2,
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT
    });
    // Primitive
    bindings.emplace_back(VkDescriptorSetLayoutBinding {
        .binding = 3,
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
        .pImmutableSamplers = nullptr, // Optional
    });
    // BaseColor
    bindings.emplace_back(VkDescriptorSetLayoutBinding {
        .binding = 4,
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
        .pImmutableSamplers = nullptr,
    });
    // Metallic/Roughness
    bindings.emplace_back(VkDescriptorSetLayoutBinding {
        .binding = 5,
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
        .pImmutableSamplers = nullptr,
    });
    // Normal
    bindings.emplace_back(VkDescriptorSetLayoutBinding {
        .binding = 6,
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
        .pImmutableSamplers = nullptr,
    });
    // Emissive
    bindings.emplace_back(VkDescriptorSetLayoutBinding {
        .binding = 7,
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
        .pImmutableSamplers = nullptr
    });
    // Light/View
    bindings.emplace_back(VkDescriptorSetLayoutBinding {
        .binding = 8,
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

void MFA::PBRModelPipeline::destroyDescriptorSetLayout() {
    MFA_ASSERT(mDescriptorSetLayout != nullptr);
    RF::DestroyDescriptorSetLayout(mDescriptorSetLayout);    
}

void MFA::PBRModelPipeline::createPipeline() {
    // TODO We need path class
    // Vertex shader
    auto cpuVertexShader = Importer::ImportShaderFromSPV(
        "../assets/shaders/pbr_model/PBRModel.vert.spv", 
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
        "../assets/shaders/pbr_model/PBRModel.frag.spv",
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

    VkVertexInputBindingDescription const vertex_binding_description {
        .binding = 0,
        .stride = sizeof(AS::MeshVertex),
        .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
    };

    std::vector<VkVertexInputAttributeDescription> input_attribute_descriptions {};
    // Position
    input_attribute_descriptions.emplace_back(VkVertexInputAttributeDescription {
        .location = static_cast<uint32_t>(input_attribute_descriptions.size()),
        .binding = 0,
        .format = VK_FORMAT_R32G32B32_SFLOAT,
        .offset = offsetof(AS::MeshVertex, position),   
    });
    // BaseColorUV
    input_attribute_descriptions.emplace_back(VkVertexInputAttributeDescription {
        .location = static_cast<uint32_t>(input_attribute_descriptions.size()),
        .binding = 0,
        .format = VK_FORMAT_R32G32_SFLOAT,
        .offset = offsetof(AS::MeshVertex, baseColorUV),   
    });
    // Metallic/Roughness
    input_attribute_descriptions.emplace_back(VkVertexInputAttributeDescription {
        .location = static_cast<uint32_t>(input_attribute_descriptions.size()),
        .binding = 0,
        .format = VK_FORMAT_R32G32_SFLOAT,
        .offset = offsetof(AS::MeshVertex, metallicUV), // Metallic and roughness have same uv for gltf files  
    });
    // NormalMapUV
    input_attribute_descriptions.emplace_back(VkVertexInputAttributeDescription {
        .location = static_cast<uint32_t>(input_attribute_descriptions.size()),
        .binding = 0,
        .format = VK_FORMAT_R32G32_SFLOAT,
        .offset = offsetof(AS::MeshVertex, normalMapUV),   
    });
    input_attribute_descriptions.emplace_back(VkVertexInputAttributeDescription {
        .location = static_cast<uint32_t>(input_attribute_descriptions.size()),
        .binding = 0,
        .format = VK_FORMAT_R32G32B32A32_SFLOAT,
        .offset = offsetof(AS::MeshVertex, tangentValue),   
    });
    input_attribute_descriptions.emplace_back(VkVertexInputAttributeDescription {
        .location = static_cast<uint32_t>(input_attribute_descriptions.size()),
        .binding = 0,
        .format = VK_FORMAT_R32G32B32_SFLOAT,
        .offset = offsetof(AS::MeshVertex, normalValue),   
    });
    // EmissionUV
    input_attribute_descriptions.emplace_back(VkVertexInputAttributeDescription {
        .location = static_cast<uint32_t>(input_attribute_descriptions.size()),
        .binding = 0,
        .format = VK_FORMAT_R32G32_SFLOAT,
        .offset = offsetof(AS::MeshVertex, emissionUV)
    });
    // HasSkin
    input_attribute_descriptions.emplace_back(VkVertexInputAttributeDescription {
        .location = static_cast<uint32_t>(input_attribute_descriptions.size()),
        .binding = 0,
        .format = VK_FORMAT_R32_SINT,
        .offset = offsetof(AS::MeshVertex, hasSkin) // TODO We should use a primitiveInfo instead
    });
    // JointIndices
    input_attribute_descriptions.emplace_back(VkVertexInputAttributeDescription {
        .location = static_cast<uint32_t>(input_attribute_descriptions.size()),
        .binding = 0,
        .format = VK_FORMAT_R32G32B32A32_SINT,
        .offset = offsetof(AS::MeshVertex, jointIndices)
    });
    // JointWeights
    input_attribute_descriptions.emplace_back(VkVertexInputAttributeDescription {
        .location = static_cast<uint32_t>(input_attribute_descriptions.size()),
        .binding = 0,
        .format = VK_FORMAT_R32G32B32A32_SFLOAT,
        .offset = offsetof(AS::MeshVertex, jointWeights)
    });
    MFA_ASSERT(mDrawPipeline.isValid() == false);
    mDrawPipeline = RF::CreateBasicDrawPipeline(
        static_cast<uint8_t>(shaders.size()), 
        shaders.data(),
        1,
        &mDescriptorSetLayout,
        vertex_binding_description,
        static_cast<uint8_t>(input_attribute_descriptions.size()),
        input_attribute_descriptions.data()
    );
}

void MFA::PBRModelPipeline::destroyPipeline() {
    MFA_ASSERT(mDrawPipeline.isValid());
    RF::DestroyDrawPipeline(mDrawPipeline);
}

void MFA::PBRModelPipeline::createUniformBuffers() {
    mLightViewBuffer = RF::CreateUniformBuffer(sizeof(LightViewBuffer), 1);
    mErrorBuffer = RF::CreateUniformBuffer(1, 1);
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
