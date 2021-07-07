#include "DrawableObject.hpp"

#include "engine/BedrockMemory.hpp"
#include "engine/ui_system/UISystem.hpp"

#include <ext/matrix_transform.hpp>
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

namespace MFA {

namespace UI = UISystem;

DrawableObjectId DrawableObject::NextId = 0;

// TODO We need RCMGMT very bad
// We need other overrides for easier use as well
DrawableObject::DrawableObject(
    RF::GpuModel & model_,
    VkDescriptorSetLayout descriptorSetLayout
)
    : mId(NextId)
    , mGpuModel(&model_)
    , mRecordUIObject([this]()->void {onUI();})
{
    NextId += 1;

    auto & mesh = mGpuModel->model.mesh;

    // DescriptorSetCount
    uint32_t descriptorSetCount = 0;
    for (uint32_t i = 0; i < mesh.getSubMeshCount(); ++i) {
        auto const & subMesh = mesh.getSubMeshByIndex(i);
        descriptorSetCount += static_cast<uint32_t>(subMesh.primitives.size());
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


    uint32_t jointBuffersCount = 0;
    for (uint32_t i = 0; i < mesh.getNodesCount(); ++i) {
        auto & node = mesh.getNodeByIndex(i);
        if (node.skin > -1) {
            node.skinBufferIndex = jointBuffersCount;
            jointBuffersCount++;
        }
    }
    if (jointBuffersCount > 0) {
        mSkinJointsBuffers = RF::CreateUniformBuffer(
            JointTransformBufferSize,                
            jointBuffersCount
        );
    }

    MFA_ASSERT(mGpuModel->valid);
    MFA_ASSERT(mGpuModel->model.mesh.isValid());
}

// TODO Support for multiple descriptorSetLayout
//DrawableObject::DrawableObject(
//    RF::GpuModel & model_, 
//    uint32_t descriptorSetLayoutsCount, 
//    VkDescriptorSetLayout * descriptorSetLayouts
//) {}

RF::GpuModel * DrawableObject::getModel() const {
    return mGpuModel;
}

uint32_t DrawableObject::getDescriptorSetCount() const {
    return static_cast<uint32_t>(mDescriptorSets.size());
}

VkDescriptorSet DrawableObject::getDescriptorSetByPrimitiveUniqueId(uint32_t const index) {
    MFA_ASSERT(index < mDescriptorSets.size());
    return mDescriptorSets[index];
}

VkDescriptorSet * DrawableObject::getDescriptorSets() {
    return mDescriptorSets.data();
}


// Only for model local buffers
RF::UniformBufferGroup * DrawableObject::createUniformBuffer(char const * name, uint32_t const size) {
    mUniformBuffers[name] = RF::CreateUniformBuffer(size, 1);
    return &mUniformBuffers[name];
}

RF::UniformBufferGroup * DrawableObject::createMultipleUniformBuffer(
    char const * name, 
    const uint32_t size, 
    const uint32_t count
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
    RF::DestroyUniformBuffer(mSkinJointsBuffers);
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

RF::UniformBufferGroup const & DrawableObject::getSkinTransformBuffer() const noexcept {
    return mSkinJointsBuffers;
}

void DrawableObject::update(float const deltaTimeInSec) {
    updateAnimation(deltaTimeInSec);
    computeNodesGlobalTransform();
    updateJoints(deltaTimeInSec);
}

void DrawableObject::draw(RF::DrawPass & drawPass) {
    BindVertexBuffer(drawPass, mGpuModel->meshBuffers.verticesBuffer);
    BindIndexBuffer(drawPass, mGpuModel->meshBuffers.indicesBuffer);

    auto const & mesh = mGpuModel->model.mesh;

    auto const nodesCount = mesh.getNodesCount();
    auto const * nodes = mesh.getNodeData();
    MFA_ASSERT(nodes != nullptr);
    
    for (uint32_t i = 0; i < nodesCount; ++i) {
        drawNode(drawPass, nodes[i]);
    }
}

void DrawableObject::EnableUI(char const * windowName, bool * isVisible) {
    MFA_ASSERT(windowName != nullptr && strlen(windowName) > 0);
    mRecordUIObject.Enable();
    mRecordWindowName = "DrawableObject: " + std::string(windowName);
    mIsUIVisible = isVisible;
}

void DrawableObject::DisableUI() {
    mRecordUIObject.Disable();
}

void DrawableObject::updateAnimation(float deltaTimeInSec) {
    using Animation = AS::Mesh::Animation;

    auto & mesh = mGpuModel->model.mesh;

    if (mesh.getAnimationsCount() <= 0) {
        return;
    }

    auto const & activeAnimation = mesh.getAnimationByIndex(mActiveAnimationIndex);

    if (mActiveAnimationIndex != mPreviousAnimationIndex) {
        mAnimationCurrentTime = activeAnimation.startTime;
        mPreviousAnimationIndex = mActiveAnimationIndex;
    }

    mAnimationCurrentTime += deltaTimeInSec;
    
    if (mAnimationCurrentTime > activeAnimation.endTime) {
        MFA_ASSERT(activeAnimation.endTime >= activeAnimation.startTime);
        mAnimationCurrentTime -= (activeAnimation.endTime - activeAnimation.startTime);
    }

    for (auto const & channel : activeAnimation.channels)
    {
        auto const & sampler = activeAnimation.samplers[channel.samplerIndex];
        auto & node = mesh.getNodeByIndex(channel.nodeIndex);

        for (size_t i = 0; i < sampler.inputAndOutput.size() - 1; i++)
        {
            if (sampler.interpolation != Animation::Interpolation::Linear)
            {
                MFA_LOG_ERROR("This sample only supports linear interpolations");
                continue;
            }

            auto const previousInput = sampler.inputAndOutput[i].input;
            auto previousOutput = Matrix4X1Float::ConvertCellsToVec4(sampler.inputAndOutput[i].output);
            auto const nextInput = sampler.inputAndOutput[i + 1].input;
            auto nextOutput = Matrix4X1Float::ConvertCellsToVec4(sampler.inputAndOutput[i + 1].output);
            // Get the input keyframe values for the current time stamp
            if (mAnimationCurrentTime >= previousInput && mAnimationCurrentTime <= nextInput)
            {
                float const fraction = (mAnimationCurrentTime - previousInput) / (nextInput - previousInput);

                if (channel.path == Animation::Path::Translation)
                {
                    auto const translate = glm::mix(previousOutput, nextOutput, fraction);

                    node.translate[0] = translate[0];
                    node.translate[1] = translate[1];
                    node.translate[2] = translate[2];
                }
                else if (channel.path == Animation::Path::Rotation)
                {
                    glm::quat previousRotation {};
                    previousRotation.x = previousOutput[0];
                    previousRotation.y = previousOutput[1];
                    previousRotation.z = previousOutput[2];
                    previousRotation.w = previousOutput[3];

                    glm::quat nextRotation {};
                    nextRotation.x = nextOutput[0];
                    nextRotation.y = nextOutput[1];
                    nextRotation.z = nextOutput[2];
                    nextRotation.w = nextOutput[3];

                    auto const rotation = glm::normalize(glm::slerp(previousRotation, nextRotation, fraction));

                    node.rotation[0] = rotation[0];
                    node.rotation[1] = rotation[1];
                    node.rotation[2] = rotation[2];
                    node.rotation[3] = rotation[3];
                }
                else if (channel.path == Animation::Path::Scale)
                {
                    auto const scale = glm::mix(previousOutput, nextOutput, fraction);
                    
                    node.scale[0] = scale[0];
                    node.scale[1] = scale[1];
                    node.scale[2] = scale[2];
                } else {
                    MFA_ASSERT(false);
                }

                node.isCachedDataValid = false;

                break;
            }
        }
    }
}

void DrawableObject::computeNodesGlobalTransform() {
    auto & mesh = mGpuModel->model.mesh;

    auto const nodesCount = mesh.getNodesCount();

    for (uint32_t i = 0; i < nodesCount; ++i) {
        auto & node = mesh.getNodeByIndex(i);
        if (node.parent < 0) { // Root node
            computeNodeGlobalTransform(node, nullptr, false);
        }
    }
}

void DrawableObject::updateJoints(float const deltaTimeInSec) {
    auto const & mesh = mGpuModel->model.mesh;

    auto const nodesCount = mesh.getNodesCount();
    auto const * nodes = mesh.getNodeData();
    MFA_ASSERT(nodes != nullptr);

    for (uint32_t i = 0; i < nodesCount; ++i) {
        updateJoint(deltaTimeInSec, nodes[i]);
    }
}

void DrawableObject::updateJoint(float const deltaTimeInSec, AssetSystem::Mesh::Node const & node) {
    auto const & mesh = mGpuModel->model.mesh;
    if (node.skin > -1)
    {
        // TODO: Performance, We can cache joints if node.cachedGlobalTransform has not changed
        // Update the joint matrices
        MFA_ASSERT(node.isCachedDataValid == true);
        glm::mat4 const inverseTransform = glm::inverse(node.cachedGlobalTransform);
        auto const & skin = mesh.getSkinByIndex(node.skin);

        // TODO Allocate this memory once
        auto jointMatricesBlob = Memory::Alloc(skin.joints.size() * sizeof(JointTransformBuffer));
        MFA_DEFER {
            Memory::Free(jointMatricesBlob);
        };
        auto * jointMatrices = jointMatricesBlob.as<JointTransformBuffer>();
        for (size_t i = 0; i < skin.joints.size(); i++)
        {
            // It's too much computation (Not as much as rasterizer but still too much)
            auto const & joint = mesh.getNodeByIndex(skin.joints[i]);
            MFA_ASSERT(node.isCachedDataValid == true);
            auto const nodeMatrix = joint.cachedGlobalTransform;
            glm::mat4 matrix = nodeMatrix * 
                Matrix4X4Float::ConvertCellsToMat4(skin.inverseBindMatrices[i].value);  // T - S = changes
            matrix = inverseTransform * matrix;                                         // Why ?
            Matrix4X4Float::ConvertGmToCells(matrix, jointMatrices[i].model);
        }
        // Update skin buffer
        RF::UpdateUniformBuffer(
            mSkinJointsBuffers.buffers[node.skinBufferIndex], 
            CBlob {jointMatricesBlob.ptr, jointMatricesBlob.len}
        );
    }
}

void DrawableObject::drawNode(RF::DrawPass & drawPass, AssetSystem::Mesh::Node const & node) {
    // TODO We can reduce nodes count for better performance when importing
    if (node.hasSubMesh()) {
        auto const & mesh = mGpuModel->model.mesh;
        MFA_ASSERT(static_cast<int>(mesh.getSubMeshCount()) > node.subMeshIndex);

        MFA_ASSERT(node.isCachedDataValid == true);
        Matrix4X4Float::ConvertGmToCells(node.cachedGlobalTransform, mNodeTransformData.model);
        
        RF::UpdateUniformBuffer(mNodeTransformBuffers.buffers[node.subMeshIndex], CBlobAliasOf(mNodeTransformData));

        drawSubMesh(drawPass, mesh.getSubMeshByIndex(node.subMeshIndex));
    }
}

void DrawableObject::drawSubMesh(RF::DrawPass & drawPass, AssetSystem::Mesh::SubMesh const & subMesh) {
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

glm::mat4 DrawableObject::computeNodeLocalTransform(Node const & node) const {
    glm::mat4 result {1};
    result = glm::translate(result, Matrix4X4Float::ConvertCellsToVec3(node.translate));
    result = result * glm::toMat4(Matrix4X1Float::ConvertCellsToQuat(node.rotation));
    result = glm::scale(result, Matrix4X4Float::ConvertCellsToVec3(node.scale));
    result = result * Matrix4X4Float::ConvertCellsToMat4(node.transform);
    return result;
}

// TODO Instead of this we should get node and its parent global transform
void DrawableObject::computeNodeGlobalTransform(Node & node, Node const * parentNode, bool isParentTransformChanged) const {
    MFA_ASSERT(parentNode == nullptr || parentNode->isCachedDataValid == true);
    MFA_ASSERT(parentNode != nullptr || isParentTransformChanged == false);

    bool isChanged = false;

    if (node.isCachedDataValid == false) {
        node.cachedLocalTransform = computeNodeLocalTransform(node);
    }

    if (isParentTransformChanged == true || node.isCachedDataValid == false) {
        if (parentNode == nullptr) {
            node.cachedGlobalTransform = node.cachedLocalTransform;
        } else {
            node.cachedGlobalTransform = parentNode->cachedGlobalTransform * node.cachedLocalTransform;
        }
        isChanged = true;
    }

    node.isCachedDataValid = true;

    auto & mesh = mGpuModel->model.mesh;
    for (auto const child : node.children) {
        auto & childNode = mesh.getNodeByIndex(child);
        computeNodeGlobalTransform(childNode, &node, isChanged);
    }
}

void DrawableObject::onUI() {
    MFA_ASSERT(mIsUIVisible != nullptr);
    if (*mIsUIVisible == false) {
        return;
    } 

    auto & mesh = mGpuModel->model.mesh;

    std::vector<char const *> animationsList {mesh.getAnimationsCount()};
    for (size_t i = 0; i < animationsList.size(); ++i) {
        animationsList[i] = mesh.getAnimationByIndex(static_cast<uint32_t>(i)).name.c_str();
    }

    UI::BeginWindow(mRecordWindowName.c_str());
    UI::Combo(
        "Active animation", 
        &mActiveAnimationIndex, 
        animationsList.data(), 
        static_cast<int32_t>(animationsList.size())
    );
    UI::EndWindow();
}

};
