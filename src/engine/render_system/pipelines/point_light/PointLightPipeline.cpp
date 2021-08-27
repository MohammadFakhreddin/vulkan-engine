#include "PointLightPipeline.hpp"

#include "tools/Importer.hpp"
#include "engine/BedrockPath.hpp"
#include "engine/render_system/render_passes/DisplayRenderPass.hpp"

namespace MFA {

namespace AS = AssetSystem;

PointLightPipeline::~PointLightPipeline() {
    if (mIsInitialized) {
        shutdown();
    }
}

void PointLightPipeline::init() {
    if (mIsInitialized == true) {
        MFA_ASSERT(false);
        return;
    }
    mIsInitialized = true;

    createDescriptorSetLayout();
    createPipeline();
}

void PointLightPipeline::shutdown() {
    if (mIsInitialized == false) {
        MFA_ASSERT(false);
        return;
    }
    mIsInitialized = false;

    destroyPipeline();
    destroyDescriptorSetLayout();
    destroyUniformBuffers();
}

DrawableObjectId PointLightPipeline::AddGpuModel(RF::GpuModel & gpuModel) {
    MFA_ASSERT(gpuModel.valid == true);

    auto const drawableId = mNextDrawableId++;

    auto * drawableObject = new DrawableObject(gpuModel);
    MFA_ASSERT(mDrawableObjects.find(drawableId) == mDrawableObjects.end());
    mDrawableObjects[drawableId] = std::unique_ptr<DrawableObject>(drawableObject);

    const auto * primitiveInfoBuffer = drawableObject->CreateUniformBuffer(
        "PrimitiveInfo", 
        sizeof(PrimitiveInfo),
        RF::GetMaxFramesPerFlight()
    );
    MFA_ASSERT(primitiveInfoBuffer != nullptr);

    const auto * viewProjectionBuffer = drawableObject->CreateUniformBuffer(
        "ViewProjection", 
        sizeof(ViewProjectionData),
        RF::GetMaxFramesPerFlight()
    );
    MFA_ASSERT(viewProjectionBuffer != nullptr);

    const auto & nodeTransformBuffer = drawableObject->GetNodeTransformBuffer();

    auto const & mesh = drawableObject->GetModel()->model.mesh;

    auto const descriptorSetGroup = drawableObject->CreateDescriptorSetGroup(
        "DisplayPipeline", 
        mDescriptorSetLayout, 
        drawableObject->GetPrimitiveCount()
    );
    
    for (uint32_t nodeIndex = 0; nodeIndex < mesh.GetNodesCount(); ++nodeIndex) {// Updating descriptor sets
        auto const & node = mesh.GetNodeByIndex(nodeIndex);
        if (node.hasSubMesh()) {
            auto const & subMesh = mesh.GetSubMeshByIndex(node.subMeshIndex);
            if (subMesh.primitives.empty() == false) {
                for (auto const & primitive : subMesh.primitives) {
                    MFA_ASSERT(primitive.uniqueId >= 0);

                    auto descriptorSet = descriptorSetGroup.descriptorSets[primitive.uniqueId];
                    MFA_VK_VALID_ASSERT(descriptorSet);

                    std::vector<VkWriteDescriptorSet> writeInfo {};

                    {// ViewProjection
                        VkDescriptorBufferInfo viewProjectionBufferInfo {};
                        viewProjectionBufferInfo.buffer = viewProjectionBuffer->buffers[0].buffer;
                        viewProjectionBufferInfo.offset = 0;
                        viewProjectionBufferInfo.range = viewProjectionBuffer->bufferSize;

                        VkWriteDescriptorSet writeDescriptorSet {};
                        writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                        writeDescriptorSet.dstSet = descriptorSet;
                        writeDescriptorSet.dstBinding = static_cast<uint32_t>(writeInfo.size());
                        writeDescriptorSet.dstArrayElement = 0;
                        writeDescriptorSet.descriptorCount = 1;
                        writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                        writeDescriptorSet.pBufferInfo = &viewProjectionBufferInfo;
                        
                        writeInfo.emplace_back(writeDescriptorSet);
                    }
                    {//NodeTransform
                        VkDescriptorBufferInfo nodeTransformBufferInfo {};
                        nodeTransformBufferInfo.buffer = nodeTransformBuffer.buffers[node.subMeshIndex].buffer;
                        nodeTransformBufferInfo.offset = 0;
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
                    {// Primitive
                        VkDescriptorBufferInfo primitiveBufferInfo {};
                        primitiveBufferInfo.buffer = primitiveInfoBuffer->buffers[0].buffer;
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
                    RF::UpdateDescriptorSets(
                        static_cast<uint8_t>(writeInfo.size()),
                        writeInfo.data()
                    );
                }
            }
        }
    }
    
    return drawableId;
}

bool PointLightPipeline::RemoveGpuModel(DrawableObjectId const drawableObjectId) {
    auto const deleteCount = mDrawableObjects.erase(drawableObjectId);  // Unique ptr should be deleted correctly
    MFA_ASSERT(deleteCount == 1);
    return deleteCount;
}

void PointLightPipeline::Render(
    RF::DrawPass & drawPass,
    float const deltaTimeInSec,
    uint32_t const idsCount, 
    DrawableObjectId * ids
) {
    RF::BindDrawPipeline(drawPass, mDrawPipeline);

    for (uint32_t i = 0; i < idsCount; ++i) {
        auto const findResult = mDrawableObjects.find(ids[i]);
        if (findResult != mDrawableObjects.end()) {
            auto * drawableObject = findResult->second.get();
            MFA_ASSERT(drawableObject != nullptr);
            drawableObject->Update(deltaTimeInSec, drawPass);
            drawableObject->Draw(drawPass, [&drawPass, &drawableObject](AS::MeshPrimitive const & primitive)-> void {
                RF::BindDescriptorSet(
                    drawPass, 
                    drawableObject->GetDescriptorSetGroup("DisplayPipeline")->descriptorSets[primitive.uniqueId]
                );
            });
        }
    }
}

bool PointLightPipeline::updateViewProjectionBuffer(
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

    drawableObject->UpdateUniformBuffer(
        "ViewProjection",
        0,
        CBlobAliasOf(viewProjectionData)
    );

    return true;
}

bool PointLightPipeline::updatePrimitiveInfo(
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

    drawableObject->UpdateUniformBuffer(
        "PrimitiveInfo",
        0,
        CBlobAliasOf(info)
    );

    return true;                 
}

void PointLightPipeline::createDescriptorSetLayout() {
    std::vector<VkDescriptorSetLayoutBinding> bindings {};
    {// ViewProjection
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
    {// Primitive
        VkDescriptorSetLayoutBinding layoutBinding {};
        layoutBinding.binding = static_cast<uint32_t>(bindings.size()),
        layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        layoutBinding.descriptorCount = 1,
        layoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
        
        bindings.emplace_back(layoutBinding);
    }
    MFA_VK_INVALID_ASSERT(mDescriptorSetLayout);
    mDescriptorSetLayout = RF::CreateDescriptorSetLayout(
        static_cast<uint8_t>(bindings.size()),
        bindings.data()
    );
}

void PointLightPipeline::destroyDescriptorSetLayout() {
    MFA_VK_VALID_ASSERT(mDescriptorSetLayout);
    RF::DestroyDescriptorSetLayout(mDescriptorSetLayout);
    MFA_VK_MAKE_NULL(mDescriptorSetLayout);
}

void PointLightPipeline::createPipeline() {
    // Vertex shader
    auto cpuVertexShader = Importer::ImportShaderFromSPV(
        Path::Asset("shaders/point_light/PointLight.vert.spv").c_str(), 
        AssetSystem::Shader::Stage::Vertex, 
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
        Path::Asset("shaders/point_light/PointLight.frag.spv").c_str(),
        AssetSystem::Shader::Stage::Fragment,
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
    MFA_ASSERT(mDrawPipeline.isValid() == false);

    RB::CreateGraphicPipelineOptions pipelineOptions {};
    pipelineOptions.useStaticViewportAndScissor = false;
    pipelineOptions.primitiveTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
    pipelineOptions.rasterizationSamples = RF::GetMaxSamplesCount();

    mDrawPipeline = RF::CreateDrawPipeline(
        RF::GetDisplayRenderPass()->GetVkRenderPass(),
        static_cast<uint32_t>(shaders.size()), 
        shaders.data(),
        1,
        &mDescriptorSetLayout,
        bindingDescription,
        static_cast<uint8_t>(inputAttributeDescriptions.size()),
        inputAttributeDescriptions.data(),
        pipelineOptions
    );
}

void PointLightPipeline::destroyPipeline() {
    MFA_ASSERT(mDrawPipeline.isValid());
    RF::DestroyDrawPipeline(mDrawPipeline);
}

void PointLightPipeline::destroyUniformBuffers() {
    for (const auto & [id, drawableObject] : mDrawableObjects) {
        MFA_ASSERT(drawableObject != nullptr);
        drawableObject->DeleteUniformBuffers();
    }
    mDrawableObjects.clear();
}

}
