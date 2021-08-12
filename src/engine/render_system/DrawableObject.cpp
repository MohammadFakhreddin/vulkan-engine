#include "DrawableObject.hpp"

#include "engine/BedrockMemory.hpp"
#include "engine/ui_system/UISystem.hpp"

#include <ext/matrix_transform.hpp>
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

namespace MFA {

namespace UI = UISystem;

// TODO We need RCMGMT very bad
// We need other overrides for easier use as well
DrawableObject::DrawableObject(RF::GpuModel & model_)
    : mGpuModel(&model_)
    , mRecordUIObject([this]()->void {onUI();})
{

    auto & mesh = mGpuModel->model.mesh;

    {// PrimitiveCount
        mPrimitiveCount = 0;
        for (uint32_t i = 0; i < mesh.GetSubMeshCount(); ++i) {
            auto const & subMesh = mesh.GetSubMeshByIndex(i);
            mPrimitiveCount += static_cast<uint32_t>(subMesh.primitives.size());
        }
    }
    //mDescriptorSets = RF::CreateDescriptorSets(
    //    descriptorSetCount, 
    //    descriptorSetLayout
    //);

    // NodeTransformBuffer
    mNodeTransformBuffers = RF::CreateUniformBuffer(
        sizeof(NodeTransformBuffer), 
        mGpuModel->model.mesh.GetSubMeshCount()
    );

    // Creating cachedSkinJoints array
    for (uint32_t i = 0; i < mesh.GetSkinsCount(); ++i) {
        auto & skin = mesh.GetSkinByIndex(i);
        auto const jointsCount = static_cast<uint32_t>(skin.joints.size());
        mCachedSkinsJoints.emplace_back(Memory::Alloc(sizeof(JointTransformBuffer) * jointsCount));
    }

    // Creating nodesJoints array
    for (uint32_t i = 0; i < mesh.GetNodesCount(); ++i) {
        auto & node = mesh.GetNodeByIndex(i);
        if (node.skin > -1) {
            node.skinBufferIndex = static_cast<uint32_t>(mSkinJointsBuffers.size());
            auto & skin = mesh.GetSkinByIndex(node.skin);
            mNodesJoints.emplace_back(Memory::Alloc(sizeof(JointTransformBuffer) * skin.joints.size()));
            mSkinJointsBuffers.emplace_back(RF::CreateUniformBuffer(
                sizeof(JointTransformBuffer) * skin.joints.size(), 
                1
            ));
        }
    }

    MFA_ASSERT(mGpuModel->valid);
    MFA_ASSERT(mGpuModel->model.mesh.IsValid());
}

DrawableObject::~DrawableObject() {
    for (auto & cachedSkin : mCachedSkinsJoints) {
        Memory::Free(cachedSkin);
    }
    for (auto & nodeJoints : mNodesJoints) {
        Memory::Free(nodeJoints);
    }
}

RF::GpuModel * DrawableObject::GetModel() const {
    return mGpuModel;
}

// Only for model local buffers
RF::UniformBufferGroup * DrawableObject::CreateUniformBuffer(char const * name, uint32_t const size) {
    MFA_ASSERT(mUniformBuffers.find(name) ==  mUniformBuffers.end());
    mUniformBuffers[name] = RF::CreateUniformBuffer(size, 1);
    return &mUniformBuffers[name];
}

RF::UniformBufferGroup * DrawableObject::CreateMultipleUniformBuffer(
    char const * name, 
    const uint32_t size, 
    const uint32_t count
) {
    mUniformBuffers[name] = RF::CreateUniformBuffer(size, count);
    return &mUniformBuffers[name];
}

// Only for model local buffers
void DrawableObject::DeleteUniformBuffers() {
    if (mUniformBuffers.empty() == false) {
        for (auto & pair : mUniformBuffers) {
            RF::DestroyUniformBuffer(pair.second);
        }
        mUniformBuffers.clear();
    }
    for (auto & skinJointBuffer : mSkinJointsBuffers) {
        RF::DestroyUniformBuffer(skinJointBuffer);
    }
    RF::DestroyUniformBuffer(mNodeTransformBuffers);
}

void DrawableObject::UpdateUniformBuffer(char const * name, CBlob const ubo) {
    auto const find_result = mUniformBuffers.find(name);
    if (find_result != mUniformBuffers.end()) {
        RF::UpdateUniformBuffer(find_result->second.buffers[0], ubo);
    } else {
        MFA_CRASH("Buffer not found");
    }
}

RF::UniformBufferGroup * DrawableObject::GetUniformBuffer(char const * name) {
    auto const find_result = mUniformBuffers.find(name);
    if (find_result != mUniformBuffers.end()) {
        return &find_result->second;
    }
    return nullptr;
}

RF::UniformBufferGroup const & DrawableObject::GetNodeTransformBuffer() const noexcept {
    return mNodeTransformBuffers;
}

RF::UniformBufferGroup const & DrawableObject::GetSkinTransformBuffer(uint32_t const nodeIndex) const noexcept {
    return mSkinJointsBuffers[nodeIndex];
}

void DrawableObject::Update(float const deltaTimeInSec) {
    updateAnimation(deltaTimeInSec);
    computeNodesGlobalTransform();
    updateAllSkinsJoints();
    updateAllNodesJoints();
}

void DrawableObject::Draw(
    RF::DrawPass & drawPass, 
    BindDescriptorSetFunction const & bindFunction
) {
    BindVertexBuffer(drawPass, mGpuModel->meshBuffers.verticesBuffer);
    BindIndexBuffer(drawPass, mGpuModel->meshBuffers.indicesBuffer);

    auto const & mesh = mGpuModel->model.mesh;

    auto const nodesCount = mesh.GetNodesCount();
    auto const * nodes = mesh.GetNodeData();
    MFA_ASSERT(nodes != nullptr);
    
    for (uint32_t i = 0; i < nodesCount; ++i) {
        drawNode(drawPass, nodes[i], bindFunction);
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

RB::DescriptorSetGroup const & DrawableObject::CreateDescriptorSetGroup(
    char const * name, 
    VkDescriptorSetLayout descriptorSetLayout, 
    uint32_t const descriptorSetCount
) {
    MFA_ASSERT(mDescriptorSetGroups.find(name) == mDescriptorSetGroups.end());
    auto const descriptorSetGroup = RF::CreateDescriptorSets(
        descriptorSetCount, 
        descriptorSetLayout
    );
    mDescriptorSetGroups[name] = descriptorSetGroup;
    return mDescriptorSetGroups[name];
}

RB::DescriptorSetGroup * DrawableObject::GetDescriptorSetGroup(char const * name) {
    MFA_ASSERT(name != nullptr);
    auto const findResult = mDescriptorSetGroups.find(name);
    if (findResult != mDescriptorSetGroups.end()) {
        return &findResult->second;
    }
    return nullptr;
}

void DrawableObject::updateAnimation(float deltaTimeInSec) {
    using Animation = AS::Mesh::Animation;

    auto & mesh = mGpuModel->model.mesh;

    if (mesh.GetAnimationsCount() <= 0) {
        return;
    }

    auto const & activeAnimation = mesh.GetAnimationByIndex(mActiveAnimationIndex);

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
        auto & node = mesh.GetNodeByIndex(channel.nodeIndex);

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

    auto const rootNodesCount = mesh.GetRootNodesCount();

    for (uint32_t i = 0; i < rootNodesCount; ++i) {
        auto & node = mesh.GetRootNodeByIndex(i);
        computeNodeGlobalTransform(node, nullptr, false);
    }
}

void DrawableObject::updateAllSkinsJoints() {
    auto const & mesh = mGpuModel->model.mesh;

    auto const skinsCount = mesh.GetSkinsCount();
    if (skinsCount > 0) {
        auto const * skins = mesh.GetSkinData();
        MFA_ASSERT(skins != nullptr);

        for (uint32_t i = 0; i < skinsCount; ++i) {
            updateSkinJoints(i, skins[i]);
        }
    }
}

void DrawableObject::updateSkinJoints(uint32_t const skinIndex, Skin const & skin) {
    auto const & mesh = mGpuModel->model.mesh;

    auto * jointMatrices = mCachedSkinsJoints[skinIndex].as<NodeTransformBuffer>();
    for (size_t i = 0; i < skin.joints.size(); i++)
    {
        auto const & joint = mesh.GetNodeByIndex(skin.joints[i]);
        auto const nodeMatrix = joint.cachedGlobalTransform;
        glm::mat4 matrix = nodeMatrix *
            Matrix4X4Float::ConvertCellsToMat4(skin.inverseBindMatrices[i].value);  // T - S = changes
        Matrix4X4Float::ConvertGmToCells(matrix, jointMatrices[i].model);
    }
}

void DrawableObject::updateAllNodesJoints() {
    auto const & mesh = mGpuModel->model.mesh;

    auto const nodesCount = mesh.GetNodesCount();
    if (nodesCount > 0) {
        auto const * nodes = mesh.GetNodeData();
        MFA_ASSERT(nodes != nullptr);

        for (uint32_t i = 0; i < nodesCount; ++i) {
            updateNodeJoint(nodes[i]);
        }
    }
}

void DrawableObject::updateNodeJoint(Node const & node) {
    auto const & mesh = mGpuModel->model.mesh;
    if (node.skin > -1)
    {
        // Update the joint matrices
        MFA_ASSERT(node.isCachedDataValid == true);
        glm::mat4 const inverseTransform = glm::inverse(node.cachedGlobalTransform);
        auto const & skin = mesh.GetSkinByIndex(node.skin);

        auto const nodeJointsBlob = mNodesJoints[node.skinBufferIndex];
        auto * nodeJoints = nodeJointsBlob.as<NodeTransformBuffer>();

        auto const cachedJointMatricesBlob = mCachedSkinsJoints[node.skin]; 
        auto * cachedJointMatrices = cachedJointMatricesBlob.as<NodeTransformBuffer>();
        for (size_t i = 0; i < skin.joints.size(); i++)
        {
            glm::mat4 matrix = inverseTransform * Matrix4X4Float::ConvertCellsToMat4(cachedJointMatrices[i].model);   // Because jointTransform already has a transform inside which needs to be undone
            Matrix4X4Float::ConvertGmToCells(matrix, nodeJoints[i].model);
        }
        // Update skin buffer
        RF::UpdateUniformBuffer(
            mSkinJointsBuffers[node.skinBufferIndex].buffers[0],
            nodeJointsBlob
        );
    }
}

void DrawableObject::drawNode(
    RF::DrawPass & drawPass, 
    AS::MeshNode const & node, 
    BindDescriptorSetFunction const & bindFunction
) {
    // TODO We can reduce nodes count for better performance when importing
    if (node.hasSubMesh()) {
        auto const & mesh = mGpuModel->model.mesh;
        MFA_ASSERT(static_cast<int>(mesh.GetSubMeshCount()) > node.subMeshIndex);

        MFA_ASSERT(node.isCachedDataValid == true);
        Matrix4X4Float::ConvertGmToCells(node.cachedGlobalTransform, mNodeTransformData.model);
        
        RF::UpdateUniformBuffer(mNodeTransformBuffers.buffers[node.subMeshIndex], CBlobAliasOf(mNodeTransformData));

        drawSubMesh(drawPass, mesh.GetSubMeshByIndex(node.subMeshIndex), bindFunction);
    }
}

void DrawableObject::drawSubMesh(
    RF::DrawPass & drawPass, 
    AssetSystem::Mesh::SubMesh const & subMesh,
    BindDescriptorSetFunction const & bindFunction
) {
    if (subMesh.primitives.empty() == false) {
        for (auto const & primitive : subMesh.primitives) {
            /*MFA_ASSERT(primitive.uniqueId >= 0);
            MFA_ASSERT(primitive.uniqueId < mDescriptorSets.size());
            RF::BindDescriptorSet(drawPass, mDescriptorSets[primitive.uniqueId]);*/
            bindFunction(primitive);
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
        auto & childNode = mesh.GetNodeByIndex(child);
        computeNodeGlobalTransform(childNode, &node, isChanged);
    }
}

void DrawableObject::onUI() {
    MFA_ASSERT(mIsUIVisible != nullptr);
    if (*mIsUIVisible == false) {
        return;
    } 

    auto & mesh = mGpuModel->model.mesh;

    std::vector<char const *> animationsList {mesh.GetAnimationsCount()};
    for (size_t i = 0; i < animationsList.size(); ++i) {
        animationsList[i] = mesh.GetAnimationByIndex(static_cast<uint32_t>(i)).name.c_str();
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
