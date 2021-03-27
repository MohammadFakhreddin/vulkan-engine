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
    mNodeTransformBuffers = RF::CreateUniformBuffer(
        sizeof(NodeTransformBuffer), 
        mGpuModel->model.mesh.getNodesCount()
    );
    mDescriptorSets = RF::CreateDescriptorSets(
        static_cast<U32>(model_.meshBuffers.subMeshBuffers.size()), 
        descriptorSetLayout
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

VkDescriptorSet_T * DrawableObject::getDescriptorSet(U32 const index) {
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
    const U8 count
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
    auto const nodesCount = mGpuModel->model.mesh.getNodesCount();
    auto const * nodes = mGpuModel->model.mesh.getNodeData();
    MFA_ASSERT(nodes != nullptr);
    if (nodesCount <= 0) {
        return;
    }
    Matrix4X4Float const transform = Matrix4X4Float::Identity();
    drawNode(drawPass, 0, transform);
}

void DrawableObject::drawNode(RF::DrawPass & drawPass, int nodeIndex, Matrix4X4Float const & parentTransform) {
    MFA_ASSERT(nodeIndex >= 0);
    MFA_ASSERT(nodeIndex < mGpuModel->model.mesh.getNodesCount());
    auto const & node = mGpuModel->model.mesh.getNodeByIndex(nodeIndex);
    MFA_ASSERT(node.subMeshIndex >= 0);
    MFA_ASSERT(mGpuModel->meshBuffers.subMeshBuffers.size() > node.subMeshIndex);

    Matrix4X4Float nodeTransform {};
    nodeTransform.assign(node.transformMatrix);
    nodeTransform.multiply(parentTransform);
    ::memcpy(mNodeTransformData.transform, nodeTransform.cells, sizeof(nodeTransform.cells));
    MFA_ASSERT(sizeof(nodeTransform.cells) == sizeof(mNodeTransformData.transform));
    RF::UpdateUniformBuffer(mNodeTransformBuffers.buffers[nodeIndex], CBlobAliasOf(mNodeTransformData));
    RF::BindDescriptorSet(drawPass, mDescriptorSets[nodeIndex]);
    drawSubMesh(drawPass, mGpuModel->meshBuffers.subMeshBuffers[node.subMeshIndex]);

    if (node.children.empty() == false) {
        for (auto const & child : node.children) {
            drawNode(drawPass, child, nodeTransform);
        } 
    }
}

void DrawableObject::drawSubMesh(RF::DrawPass & drawPass, RF::MeshBuffers::SubMesh & subMeshBuffers) {
    // We should update transform for each one before update, Fuck!
    // TODO Start from here, Draw node tree
    if (subMeshBuffers.primitives.empty() == false) {
        for (auto const & primitive : subMeshBuffers.primitives) {
            RF::DrawIndexed(
                drawPass,
                primitive.indicesCount,
                1,
                primitive.indicesStartingIndex,
                primitive.verticesOffset
            );
        }
    }
    //for (U32 i = 0; i < subMeshBuffers.primitives.size(); ++i) {
    //    auto * currentDescriptorSet = mDescriptorSets[i];
    //    RF::BindDescriptorSet(drawPass, currentDescriptorSet);
    //    auto const & currentSubMesh = header_object->sub_meshes[i];
    //    auto const & subMeshBuffers = mGpuModel->meshBuffers.subMeshBuffers[i];
    //    DrawIndexed(
    //        drawPass,
    //        subMeshBuffers.index_count,
    //        1,
    //        currentSubMesh.indices_starting_index
    //    );
    //}
}

};
