#include "PointLightPipeline.hpp"

#include "tools/Importer.hpp"

namespace AS = MFA::AssetSystem;

MFA::PointLightPipeline::~PointLightPipeline() {
    if (mIsInitialized) {
        shutdown();
    }
}

void MFA::PointLightPipeline::init() {
    if (mIsInitialized == true) {
        MFA_ASSERT(false);
        return;
    }
    mIsInitialized = true;

    createDescriptorSetLayout();
    createPipeline();
}

void MFA::PointLightPipeline::shutdown() {
    if (mIsInitialized == false) {
        MFA_ASSERT(false);
        return;
    }
    mIsInitialized = false;

    destroyPipeline();
    destroyDescriptorSetLayout();
    destroyUniformBuffers();
}

MFA::DrawableObjectId MFA::PointLightPipeline::addGpuModel(RF::GpuModel & gpuModel) {
    MFA_ASSERT(gpuModel.valid == true);
    
    auto * drawableObject = new DrawableObject(gpuModel, mDescriptorSetLayout);
    MFA_ASSERT(mDrawableObjects.find(drawableObject->getId()) == mDrawableObjects.end());
    mDrawableObjects[drawableObject->getId()] = std::unique_ptr<DrawableObject>(drawableObject);

    const auto * primitiveInfoBuffer = drawableObject->createUniformBuffer(
        "PrimitiveInfo", 
        sizeof(PrimitiveInfo)
    );
    MFA_ASSERT(primitiveInfoBuffer != nullptr);

    const auto * viewProjectionBuffer = drawableObject->createUniformBuffer(
        "ViewProjection", 
        sizeof(ViewProjectionData)
    );
    MFA_ASSERT(viewProjectionBuffer != nullptr);

    const auto & nodeTransformBuffer = drawableObject->getNodeTransformBuffer();

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

                    //NodeTransform
                    VkDescriptorBufferInfo nodeTransformBufferInfo {
                        .buffer = nodeTransformBuffer.buffers[node.subMeshIndex].buffer,
                        .offset = 0,
                        .range = nodeTransformBuffer.bufferSize
                    };
                    writeInfo.emplace_back(VkWriteDescriptorSet {
                        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                        .dstSet = descriptorSet,
                        .dstBinding = static_cast<uint32_t>(writeInfo.size()),
                        .dstArrayElement = 0,
                        .descriptorCount = 1,
                        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                        .pBufferInfo = &nodeTransformBufferInfo,
                    });

                    // Primitive
                    VkDescriptorBufferInfo primitiveBufferInfo {
                        .buffer = primitiveInfoBuffer->buffers[0].buffer,
                        .offset = 0,
                        .range = primitiveInfoBuffer->bufferSize
                    };
                    writeInfo.emplace_back(VkWriteDescriptorSet {
                        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                        .dstSet = descriptorSet,
                        .dstBinding = static_cast<uint32_t>(writeInfo.size()),
                        .dstArrayElement = 0,
                        .descriptorCount = 1,
                        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                        .pBufferInfo = &primitiveBufferInfo,
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

bool MFA::PointLightPipeline::removeGpuModel(DrawableObjectId const drawableObjectId) {
    auto const deleteCount = mDrawableObjects.erase(drawableObjectId);  // Unique ptr should be deleted correctly
    MFA_ASSERT(deleteCount == 1);
    return deleteCount;
}

void MFA::PointLightPipeline::render(
    RF::DrawPass & drawPass,
    float deltaTime,
    uint32_t const idsCount, 
    DrawableObjectId * ids
) {
    RF::BindDrawPipeline(drawPass, mDrawPipeline);

    for (uint32_t i = 0; i < idsCount; ++i) {
        auto const findResult = mDrawableObjects.find(ids[i]);
        if (findResult != mDrawableObjects.end()) {
            auto * drawableObject = findResult->second.get();
            MFA_ASSERT(drawableObject != nullptr);
            drawableObject->draw(drawPass);
        }
    }
}

bool MFA::PointLightPipeline::updateViewProjectionBuffer(
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

bool MFA::PointLightPipeline::updatePrimitiveInfo(
    DrawableObjectId const drawableObjectId, 
    PrimitiveInfo const & info
) {
    auto const findResult = mDrawableObjects.find(drawableObjectId);
    if (findResult == mDrawableObjects.end()) {
        MFA_ASSERT(false);
        return false;
    }
    auto * drawableObject = findResult->second.get();
    MFA_ASSERT(drawableObject != nullptr);

    drawableObject->updateUniformBuffer(
        "PrimitiveInfo", 
        CBlobAliasOf(info)
    );

    return true;                 
}

void MFA::PointLightPipeline::createDescriptorSetLayout() {
    std::vector<VkDescriptorSetLayoutBinding> bindings {};
    // ViewProjection 
    bindings.emplace_back(VkDescriptorSetLayoutBinding {
        .binding = static_cast<uint32_t>(bindings.size()),
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
        .pImmutableSamplers = nullptr, // Optional
    });
    // NodeTransformation
    bindings.emplace_back(VkDescriptorSetLayoutBinding {
        .binding = static_cast<uint32_t>(bindings.size()),
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
        .pImmutableSamplers = nullptr, // Optional
    });
    // Primitive
    bindings.emplace_back(VkDescriptorSetLayoutBinding {
        .binding = static_cast<uint32_t>(bindings.size()),
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
        .pImmutableSamplers = nullptr, // Optional
    });
    MFA_ASSERT(mDescriptorSetLayout == nullptr);
    mDescriptorSetLayout = RF::CreateDescriptorSetLayout(
        static_cast<uint8_t>(bindings.size()),
        bindings.data()
    );
}

void MFA::PointLightPipeline::destroyDescriptorSetLayout() const {
    MFA_ASSERT(mDescriptorSetLayout != nullptr);
    RF::DestroyDescriptorSetLayout(mDescriptorSetLayout); 
}

void MFA::PointLightPipeline::createPipeline() {
    // Vertex shader
    auto cpuVertexShader = Importer::ImportShaderFromSPV(
        "../assets/shaders/point_light/PointLight.vert.spv", 
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
        "../assets/shaders/point_light/PointLight.frag.spv",
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
    MFA_ASSERT(mDrawPipeline.isValid() == false);
    mDrawPipeline = RF::CreateDrawPipeline(
        static_cast<uint32_t>(shaders.size()), 
        shaders.data(),
        1,
        &mDescriptorSetLayout,
        vertex_binding_description,
        static_cast<uint8_t>(input_attribute_descriptions.size()),
        input_attribute_descriptions.data(),
        RB::CreateGraphicPipelineOptions {
            .use_static_viewport_and_scissor = false,
            .primitive_topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP
        }
    );
}

void MFA::PointLightPipeline::destroyPipeline() {
    MFA_ASSERT(mDrawPipeline.isValid());
    RF::DestroyDrawPipeline(mDrawPipeline);
}

void MFA::PointLightPipeline::destroyUniformBuffers() {
    for (const auto & [id, drawableObject] : mDrawableObjects) {
        MFA_ASSERT(drawableObject != nullptr);
        drawableObject->deleteUniformBuffers();
    }
    mDrawableObjects.clear();
}
