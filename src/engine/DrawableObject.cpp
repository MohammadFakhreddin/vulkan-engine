#include "DrawableObject.hpp"

namespace MFA {
// TODO We need RCMGMT very bad
// We need other overrides for easier use as well
DrawableObject::DrawableObject(
    RF::GpuModel & model_,
    VkDescriptorSetLayout_T * descriptorSetLayout
)
    : mGpuModel(&model_)
{
    // DescriptorSetCount
    U32 descriptorSetCount = 0;
    for (U32 i = 0; i < mGpuModel->model.mesh.getSubMeshCount(); ++i) {
        auto const & subMesh = mGpuModel->model.mesh.getSubMeshByIndex(i);
        descriptorSetCount += static_cast<U32>(subMesh.primitives.size());
    }
    mDescriptorSets = RF::CreateDescriptorSets(
        descriptorSetCount, 
        descriptorSetLayout
    );

    // NodeTransformBuffer
    mNodeTransformBuffers = RF::CreateUniformBuffer(
        sizeof(NodeTransformBuffer), 
        mGpuModel->model.mesh.getSubMeshCount()
    );

    MFA_ASSERT(mGpuModel->valid);
    MFA_ASSERT(mGpuModel->model.mesh.isValid());
}

RF::GpuModel * DrawableObject::getModel() const {
    return mGpuModel;
}

U32 DrawableObject::getDescriptorSetCount() const {
    return static_cast<U32>(mDescriptorSets.size());
}

VkDescriptorSet_T * DrawableObject::getDescriptorSetByPrimitiveUniqueId(U32 const index) {
    MFA_ASSERT(index < mDescriptorSets.size());
    return mDescriptorSets[index];
}

VkDescriptorSet_T ** DrawableObject::getDescriptorSets() {
    return mDescriptorSets.data();
}


// Only for model local buffers
RF::UniformBufferGroup * DrawableObject::createUniformBuffer(char const * name, U32 const size) {
    mUniformBuffers[name] = RF::CreateUniformBuffer(size, 1);
    return &mUniformBuffers[name];
}

RF::UniformBufferGroup * DrawableObject::createMultipleUniformBuffer(
    char const * name, 
    const U32 size, 
    const U32 count
) {
    mUniformBuffers[name] = RF::CreateUniformBuffer(size, count);
    return &mUniformBuffers[name];
}

// Only for model local buffers
void DrawableObject::deleteUniformBuffers() {
    if (mUniformBuffers.empty() == false) {
        for (auto & pair : mUniformBuffers) {
            RF::DestroyUniformBuffer(pair.second);
        }
        mUniformBuffers.clear();
    }
    RF::DestroyUniformBuffer(mNodeTransformBuffers);
}

void DrawableObject::updateUniformBuffer(char const * name, CBlob const ubo) {
    auto const find_result = mUniformBuffers.find(name);
    if (find_result != mUniformBuffers.end()) {
        RF::UpdateUniformBuffer(find_result->second.buffers[0], ubo);
    } else {
        MFA_CRASH("Buffer not found");
    }
}

RF::UniformBufferGroup * DrawableObject::getUniformBuffer(char const * name) {
    auto const find_result = mUniformBuffers.find(name);
    if (find_result != mUniformBuffers.end()) {
        return &find_result->second;
    }
    return nullptr;
}

RF::UniformBufferGroup const & DrawableObject::getNodeTransformBuffer() const noexcept {
    return mNodeTransformBuffers;
}

void DrawableObject::draw(RF::DrawPass & drawPass) {
    BindVertexBuffer(drawPass, mGpuModel->meshBuffers.verticesBuffer);
    BindIndexBuffer(drawPass, mGpuModel->meshBuffers.indicesBuffer);

    auto const & mesh = mGpuModel->model.mesh;

    auto const nodesCount = mesh.getNodesCount();
    auto const * nodes = mesh.getNodeData();
    MFA_ASSERT(nodes != nullptr);
    if (nodesCount <= 0) {
        return;
    }
    Matrix4X4Float const transform = Matrix4X4Float::Identity();
    drawNode(drawPass, 0, transform);
}

void DrawableObject::drawNode(RF::DrawPass & drawPass, int nodeIndex, Matrix4X4Float const & parentTransform) {
    auto const & mesh = mGpuModel->model.mesh;
    
    MFA_ASSERT(nodeIndex >= 0);
    MFA_ASSERT(static_cast<U32>(nodeIndex) < mGpuModel->model.mesh.getNodesCount());
    auto const & node = mGpuModel->model.mesh.getNodeByIndex(nodeIndex);
    MFA_ASSERT(static_cast<int>(mesh.getSubMeshCount()) > node.subMeshIndex);

    Matrix4X4Float nodeTransform {};
    nodeTransform.assign(node.transformMatrix);
    nodeTransform.multiply(parentTransform);
    MFA_ASSERT(nodeTransform.get(3, 0) == 0.0f);
    MFA_ASSERT(nodeTransform.get(3, 1) == 0.0f);
    MFA_ASSERT(nodeTransform.get(3, 2) == 0.0f);
    // TODO We can reduce nodes count for better performance when importing
    if (node.hasSubMesh()) {
        // Updating uniform buffer
        Matrix4X4Float nodeRotationAndScale {};
        Matrix4X4Float::ExtractRotationAndScaleMatrix(nodeTransform, nodeRotationAndScale);

        Matrix4X4Float nodeTranslate {};
        Matrix4X4Float::ExtractTranslateMatrix(nodeTransform, nodeTranslate);
        
        // Translate
        ::memcpy(mNodeTransformData.translate, nodeTranslate.cells, sizeof(nodeTranslate.cells));
        MFA_ASSERT(sizeof(nodeTranslate.cells) == sizeof(mNodeTransformData.translate));
        // RotationAndScale
        ::memcpy(mNodeTransformData.rotationAndScale, nodeRotationAndScale.cells, sizeof(nodeRotationAndScale.cells));
        MFA_ASSERT(sizeof(nodeRotationAndScale.cells) == sizeof(mNodeTransformData.rotationAndScale));

        RF::UpdateUniformBuffer(mNodeTransformBuffers.buffers[node.subMeshIndex], CBlobAliasOf(mNodeTransformData));

        drawSubMesh(drawPass, mesh.getSubMeshByIndex(node.subMeshIndex));
    }

    if (node.children.empty() == false) {
        for (auto const & child : node.children) {
            drawNode(drawPass, child, nodeTransform);
        } 
    }
}

void DrawableObject::drawSubMesh(RF::DrawPass & drawPass, AssetSystem::Mesh::SubMesh const & subMesh) {
    // We should update transform for each one before update, Fuck!
    // TODO Start from here, Draw node tree
    if (subMesh.primitives.empty() == false) {
        for (auto const & primitive : subMesh.primitives) {
            MFA_ASSERT(primitive.uniqueId >= 0);
            MFA_ASSERT(primitive.uniqueId < mDescriptorSets.size());
            RF::BindDescriptorSet(drawPass, mDescriptorSets[primitive.uniqueId]);
            RF::DrawIndexed(
                drawPass,
                primitive.indicesCount,
                1,
                primitive.indicesStartingIndex
            );
        }
    }
}

};
