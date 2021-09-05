#include "DrawableObject.hpp"

#include "engine/BedrockMemory.hpp"
#include "engine/ui_system/UISystem.hpp"

#include <ext/matrix_transform.hpp>
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

namespace MFA {

namespace UI = UISystem;

// TODO: We should have one drawableFactory per gpuModel and drawableInstance per instance
// TODO: Drawables should be separate from factory
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
        if (mPrimitiveCount > 0) {
            size_t const bufferSize = sizeof(PrimitiveInfo) * mPrimitiveCount;
            mPrimitivesBuffer = RF::CreateUniformBuffer(bufferSize, 1);
            Blob primitiveData = Memory::Alloc(bufferSize);
            MFA_DEFER {
                Memory::Free(primitiveData);
            };

            auto * primitivesArray = primitiveData.as<PrimitiveInfo>();
            for (uint32_t i = 0; i < mesh.GetSubMeshCount(); ++i) {
                auto const & subMesh = mesh.GetSubMeshByIndex(i);
                for (auto const & primitive : subMesh.primitives) {

                    // Copy primitive into primitive info
                    PrimitiveInfo & primitiveInfo = primitivesArray[primitive.uniqueId];
                    primitiveInfo.baseColorTextureIndex = primitive.hasBaseColorTexture ? primitive.baseColorTextureIndex : -1;
                    primitiveInfo.metallicFactor = primitive.metallicFactor;
                    primitiveInfo.roughnessFactor = primitive.roughnessFactor;
                    primitiveInfo.metallicRoughnessTextureIndex = primitive.hasMetallicRoughnessTexture ? primitive.metallicRoughnessTextureIndex : -1;
                    primitiveInfo.normalTextureIndex = primitive.hasNormalTexture ? primitive.normalTextureIndex : -1;
                    primitiveInfo.emissiveTextureIndex = primitive.hasEmissiveTexture ? primitive.emissiveTextureIndex : -1;
                    primitiveInfo.hasSkin = primitive.hasSkin ? 1 : 0;
                    // TODO Occlusion

                    ::memcpy(primitiveInfo.baseColorFactor, primitive.baseColorFactor, sizeof(primitiveInfo.baseColorFactor));
                    static_assert(sizeof(primitiveInfo.baseColorFactor) == sizeof(primitive.baseColorFactor));
                    ::memcpy(primitiveInfo.emissiveFactor, primitive.emissiveFactor, sizeof(primitiveInfo.emissiveFactor));
                    static_assert(sizeof(primitiveInfo.emissiveFactor) == sizeof(primitive.emissiveFactor));
                }
            }

            RF::UpdateUniformBuffer(mPrimitivesBuffer.buffers[0], primitiveData);
        }
    }

    {// Nodes
        uint32_t const nodesCount = mesh.GetNodesCount();
        mNodes.resize(nodesCount);
        for (uint32_t i = 0; i < nodesCount; ++i) {
            auto & meshNode = mesh.GetNodeByIndex(i);

            auto & node = mNodes[i];

            node.meshNode = &meshNode;
            node.isCachedDataValid = false;
            
            node.rotation.x = meshNode.rotation[0];
            node.rotation.y = meshNode.rotation[1];
            node.rotation.z = meshNode.rotation[2];
            node.rotation.w = meshNode.rotation[3];

            node.scale.x = meshNode.scale[0];
            node.scale.y = meshNode.scale[1];
            node.scale.z = meshNode.scale[2];

            node.translate.x = meshNode.translate[0];
            node.translate.y = meshNode.translate[1];
            node.translate.z = meshNode.translate[2];

        }
    }

    // Creating cachedSkinJoints array
    mCachedSkinsJoints.resize(mesh.GetSkinsCount());
    for (uint32_t i = 0; i < mesh.GetSkinsCount(); ++i) {
        auto & skin = mesh.GetSkinByIndex(i);
        auto const jointsCount = static_cast<uint32_t>(skin.joints.size());
        mCachedSkinsJoints[i].resize(jointsCount);
    }

    {// Creating nodesJoints array
        size_t totalBufferSize = 0;
        mNodesJointsSubBufferCount = 0;
        for (auto & node : mNodes) {
            if (node.meshNode->skin > -1) {
                node.skinBufferIndex = mNodesJointsSubBufferCount;
                auto & skin = mesh.GetSkinByIndex(node.meshNode->skin);
                size_t const localBufferSize = sizeof(JointTransformData) * skin.joints.size();
                totalBufferSize += localBufferSize;
                ++mNodesJointsSubBufferCount;
            }
        }
        if (totalBufferSize > 0) {
            mNodesJointsData = Memory::Alloc(totalBufferSize);
            mNodesJointsBuffer = RF::CreateUniformBuffer(totalBufferSize, 1);
        }
    }
    MFA_ASSERT(mGpuModel->valid);
    MFA_ASSERT(mGpuModel->model.mesh.IsValid());
}

DrawableObject::~DrawableObject() {
    mCachedSkinsJoints.clear();

    if (mNodesJointsData.len > 0) {
        Memory::Free(mNodesJointsData);
    }
    if (mNodesJointsBuffer.bufferSize > 0) {
        RF::DestroyUniformBuffer(mNodesJointsBuffer);
    }
    if (mPrimitivesBuffer.bufferSize > 0) {
        RF::DestroyUniformBuffer(mPrimitivesBuffer);
    }
    DeleteUniformBuffers();
}

RF::GpuModel * DrawableObject::GetModel() const {
    return mGpuModel;
}

// Only for model local buffers
RF::UniformBufferGroup * DrawableObject::CreateUniformBuffer(
    char const * name,
    uint32_t const size,
    uint32_t const count
) {
    MFA_ASSERT(mUniformBuffers.find(name) ==  mUniformBuffers.end());
    mUniformBuffers[name] = RF::CreateUniformBuffer(size, count);
    return &mUniformBuffers[name];
}

// Only for model local buffers
void DrawableObject::DeleteUniformBuffers() {
    for (auto & dirtyBuffer : mDirtyBuffers) {
        delete dirtyBuffer.ubo.ptr;
    }
    mDirtyBuffers.clear();
    
    for (auto & pair : mUniformBuffers) {
        RF::DestroyUniformBuffer(pair.second);
    }
    mUniformBuffers.clear();
    
    //RF::DestroyUniformBuffer(mNodeTransformBuffers);
}

void DrawableObject::UpdateUniformBuffer(
    char const * name,
    uint32_t startingIndex,
    CBlob const ubo
) {
    for (auto & dirtyBuffer : mDirtyBuffers) {
        if (
            strcmp(dirtyBuffer.bufferName.c_str(), name) == 0 &&
            dirtyBuffer.startingIndex == startingIndex
        ) {
            MFA_ASSERT(dirtyBuffer.ubo.len == ubo.len);
            Memory::Free(dirtyBuffer.ubo);
            dirtyBuffer.ubo.ptr = new uint8_t[ubo.len];
            dirtyBuffer.ubo.len = ubo.len;
            ::memcpy(dirtyBuffer.ubo.ptr, ubo.ptr, ubo.len);
            
            dirtyBuffer.remainingUpdateCount = RF::GetMaxFramesPerFlight();
            return;
        }
    }
    
    auto const findResult = mUniformBuffers.find(name);
    if (findResult == mUniformBuffers.end()) {
        MFA_CRASH("Buffer not found");
        return;
    }
    
    DirtyBuffer dirtyBuffer {
        .bufferName = name,
        .bufferGroup = &findResult->second,
        .startingIndex = startingIndex,
        .remainingUpdateCount = RF::GetMaxFramesPerFlight(),
        .ubo = Memory::Alloc(ubo.len),
    };
    ::memcpy(dirtyBuffer.ubo.ptr, ubo.ptr, ubo.len);
    
    mDirtyBuffers.emplace_back(dirtyBuffer);
}

RF::UniformBufferGroup * DrawableObject::GetUniformBuffer(char const * name) {
    auto const find_result = mUniformBuffers.find(name);
    if (find_result != mUniformBuffers.end()) {
        return &find_result->second;
    }
    return nullptr;
}

//RF::UniformBufferGroup const & DrawableObject::GetNodeTransformBuffer() const noexcept {
//    return mNodeTransformBuffers;
//}

[[nodiscard]]
RF::UniformBufferGroup const & DrawableObject::GetNodesJointsBuffer() const noexcept {
    return mNodesJointsBuffer;
}

RF::UniformBufferGroup const & DrawableObject::GetPrimitivesBuffer() const noexcept {
    return mPrimitivesBuffer;
}

void DrawableObject::Update(float const deltaTimeInSec, RF::DrawPass const & drawPass) {
    updateAnimation(deltaTimeInSec);
    computeNodesGlobalTransform();
    updateAllSkinsJoints();
    updateAllNodes(drawPass);

    mIsModelTransformChanged = false;
    // We update buffers after all of computations
    
    for (int i = static_cast<int>(mDirtyBuffers.size()) - 1; i >= 0; --i) {
        auto & dirtyBuffer = mDirtyBuffers[i];
        if (dirtyBuffer.remainingUpdateCount > 0) {
            --dirtyBuffer.remainingUpdateCount;
            RF::UpdateUniformBuffer(
                dirtyBuffer.bufferGroup->buffers[dirtyBuffer.startingIndex + drawPass.frameIndex],
                dirtyBuffer.ubo
            );
        } else {
            Memory::Free(dirtyBuffer.ubo);
            mDirtyBuffers.erase(mDirtyBuffers.begin() + i);
        }
    }


    // TODO We should update only if data is changed!
    if (mNodesJointsBuffer.bufferSize > 0) {
        RF::UpdateUniformBuffer(
            mNodesJointsBuffer.buffers[0],
            mNodesJointsData
        );
    }
}

void DrawableObject::Draw(
    RF::DrawPass & drawPass, 
    BindDescriptorSetFunction const & bindFunction
) {
    BindVertexBuffer(drawPass, mGpuModel->meshBuffers.verticesBuffer);
    BindIndexBuffer(drawPass, mGpuModel->meshBuffers.indicesBuffer);

    //auto const & mesh = mGpuModel->model.mesh;

    //auto const nodesCount = mesh.GetNodesCount();
    //auto const * nodes = mesh.GetNodeData();
    //MFA_ASSERT(nodes != nullptr);
    //
    //for (uint32_t i = 0; i < nodesCount; ++i) {
    //    drawNode(drawPass, nodes[i], bindFunction);
    //}

    for (auto & node : mNodes) {
        drawNode(drawPass, node, bindFunction);
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

void DrawableObject::UpdateModelTransform(float modelTransform[16]) {
    if (Matrix4X4Float::IsEqual(mModelTransform, modelTransform) == false) {
        mModelTransform = Matrix4X4Float::ConvertCellsToMat4(modelTransform);
        mIsModelTransformChanged = true;
    }
}

void DrawableObject::updateAnimation(float const deltaTimeInSec) {
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
        auto & node = mNodes[channel.nodeIndex];

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
                    node.translate = glm::mix(previousOutput, nextOutput, fraction);
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

                    node.rotation = glm::normalize(glm::slerp(previousRotation, nextRotation, fraction));
                }
                else if (channel.path == Animation::Path::Scale)
                {
                    node.scale = glm::mix(previousOutput, nextOutput, fraction);
                }
                else 
                {
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
        auto & node = mNodes[mesh.GetIndexOfRootNode(i)];
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
    auto jointMatrices = mCachedSkinsJoints[skinIndex];
    for (size_t i = 0; i < skin.joints.size(); i++)
    {
        auto const & joint = mNodes[skin.joints[i]];
        auto const nodeMatrix = joint.cachedGlobalTransform;
        glm::mat4 matrix = nodeMatrix *
            Matrix4X4Float::ConvertCellsToMat4(skin.inverseBindMatrices[i].value);  // T - S = changes
        Matrix4X4Float::ConvertGmToCells(matrix, jointMatrices[i].model);
    }
}

void DrawableObject::updateAllNodes(RF::DrawPass const & drawPass) {
    //auto const nodesCount = mesh.GetNodesCount();
    //if (nodesCount > 0) {
    //    auto const * nodes = mesh.GetNodeData();
    //    MFA_ASSERT(nodes != nullptr);

    //    for (uint32_t i = 0; i < nodesCount; ++i) {
    //        updateNodeJoint(drawPass, nodes[i]);
    //    }
    //}
    for (auto & node : mNodes) {
        updateNodes(drawPass, node);
    }
}

void DrawableObject::updateNodes(
    RF::DrawPass const & drawPass,
    Node & node
) {
    auto const & mesh = mGpuModel->model.mesh;
    if (node.meshNode->skin > -1)
    {
        // Update the joint matrices
        MFA_ASSERT(node.isCachedDataValid == true);
        glm::mat4 const inverseTransform = glm::inverse(node.cachedGlobalTransform);
        auto const & skin = mesh.GetSkinByIndex(node.meshNode->skin);

        auto const nodeJointsBlob = mNodesJointsData.as<Blob>()[node.skinBufferIndex];
        auto * nodeJoints = nodeJointsBlob.as<JointTransformData>();

        auto cachedJointMatrices = mCachedSkinsJoints[node.meshNode->skin]; 
        for (size_t i = 0; i < skin.joints.size(); i++)
        {
            glm::mat4 matrix = inverseTransform * Matrix4X4Float::ConvertCellsToMat4(cachedJointMatrices[i].model);   // Because jointTransform already has a transform inside which needs to be undone
            Matrix4X4Float::ConvertGmToCells(matrix, nodeJoints[i].model);
        }
    }
}

void DrawableObject::drawNode(
    RF::DrawPass & drawPass, 
    Node const & node, 
    BindDescriptorSetFunction const & bindFunction
) {
    // TODO We can reduce nodes count for better performance when importing
    if (node.meshNode->hasSubMesh()) 
    {
        auto const & mesh = mGpuModel->model.mesh;
        MFA_ASSERT(static_cast<int>(mesh.GetSubMeshCount()) > node.meshNode->subMeshIndex);
        
        drawSubMesh(
            drawPass, 
            mesh.GetSubMeshByIndex(node.meshNode->subMeshIndex), 
            node, 
            bindFunction
        );
    }
}

void DrawableObject::drawSubMesh(
    RF::DrawPass & drawPass, 
    AssetSystem::Mesh::SubMesh const & subMesh,
    Node const & node,
    BindDescriptorSetFunction const & bindFunction
) {
    if (subMesh.primitives.empty() == false) {
        for (auto const & primitive : subMesh.primitives) {
            bindFunction(primitive, node);
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
    result = glm::translate(result, Matrix4X4Float::ConvertCellsToVec3(node.meshNode->translate));
    result = result * glm::toMat4(Matrix4X1Float::ConvertCellsToQuat(node.meshNode->rotation));
    result = glm::scale(result, Matrix4X4Float::ConvertCellsToVec3(node.meshNode->scale));
    result = result * Matrix4X4Float::ConvertCellsToMat4(node.meshNode->transform);
    return result;
}

// TODO Instead of this we should get node and its parent global transform
void DrawableObject::computeNodeGlobalTransform(Node & node, Node const * parentNode, bool isParentTransformChanged) {
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

    if ((isChanged || mIsModelTransformChanged) && node.meshNode->hasSubMesh()) {
        node.cachedModelTransform = mModelTransform * node.cachedGlobalTransform;
    }

    node.isCachedDataValid = true;

    for (auto const childIndex : node.meshNode->children) {
        auto & childNode = mNodes[childIndex];
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
