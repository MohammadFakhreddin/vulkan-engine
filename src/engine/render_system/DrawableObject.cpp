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

    // Skins
    mSkins.resize(mesh.GetSkinsCount());
    
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

            node.skin = meshNode.skin > -1 ? &mSkins[meshNode.skin] : nullptr;

            Matrix4X4Float::ConvertCellsToMat4(meshNode.transform, node.transform);
        }
    }

    {// Creating cachedSkinJoints array
        {
            uint32_t totalJointsCount = 0;
            for (uint32_t i = 0; i < mesh.GetSkinsCount(); ++i) {
                auto & skin = mesh.GetSkinByIndex(i);
                auto const jointsCount = static_cast<uint32_t>(skin.joints.size());
                totalJointsCount += jointsCount;
            }
            if (totalJointsCount > 0) {
                mCachedSkinsJointsBlob = Memory::Alloc(sizeof(JointTransformData) * totalJointsCount);
                mSkinsJointsBuffer = RF::CreateUniformBuffer(mCachedSkinsJointsBlob.len, RF::GetMaxFramesPerFlight());
            }
        }
        {
            mCachedSkinsJoints.resize(mesh.GetSkinsCount());
            uint32_t startingJointIndex = 0;
            for (uint32_t i = 0; i < mesh.GetSkinsCount(); ++i) {
                auto & skin = mesh.GetSkinByIndex(i);
                auto const jointsCount = static_cast<uint32_t>(skin.joints.size());
                mCachedSkinsJoints[i] = reinterpret_cast<JointTransformData *>(mCachedSkinsJointsBlob.ptr + startingJointIndex * sizeof(JointTransformData));
                mSkins[i].skinStartingIndex = startingJointIndex;
                startingJointIndex += jointsCount;
            }
        }   
    }
    MFA_ASSERT(mGpuModel->valid);
    MFA_ASSERT(mGpuModel->model.mesh.IsValid());
}

//-------------------------------------------------------------------------------------------------

DrawableObject::~DrawableObject() {
    if (mCachedSkinsJointsBlob.len > 0) {
        Memory::Free(mCachedSkinsJointsBlob);
    }
    if (mSkinsJointsBuffer.bufferSize > 0) {
        RF::DestroyUniformBuffer(mSkinsJointsBuffer);
    }
    if (mPrimitivesBuffer.bufferSize > 0) {
        RF::DestroyUniformBuffer(mPrimitivesBuffer);
    }
    DeleteUniformBuffers();

    for (auto & storedData : mStorageMap) {
        Memory::Free(storedData.second);    
    }
}

//-------------------------------------------------------------------------------------------------

RF::GpuModel * DrawableObject::GetModel() const {
    return mGpuModel;
}

//-------------------------------------------------------------------------------------------------

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

//-------------------------------------------------------------------------------------------------

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
}

//-------------------------------------------------------------------------------------------------

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

//-------------------------------------------------------------------------------------------------

RF::UniformBufferGroup * DrawableObject::GetUniformBuffer(char const * name) {
    auto const find_result = mUniformBuffers.find(name);
    if (find_result != mUniformBuffers.end()) {
        return &find_result->second;
    }
    return nullptr;
}

//-------------------------------------------------------------------------------------------------

[[nodiscard]]
RF::UniformBufferGroup const & DrawableObject::GetSkinJointsBuffer() const noexcept {
    return mSkinsJointsBuffer;
}

//-------------------------------------------------------------------------------------------------

RF::UniformBufferGroup const & DrawableObject::GetPrimitivesBuffer() const noexcept {
    return mPrimitivesBuffer;
}

//-------------------------------------------------------------------------------------------------

void DrawableObject::Update(float const deltaTimeInSec, RF::DrawPass const & drawPass) {
    updateAnimation(deltaTimeInSec);
    computeNodesGlobalTransform();
    updateAllSkinsJoints();
    
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

    if (mSkinsJointsBuffer.bufferSize > 0) {
        RF::UpdateUniformBuffer(
            mSkinsJointsBuffer.buffers[drawPass.frameIndex],
            mCachedSkinsJointsBlob
        );
    }
}

//-------------------------------------------------------------------------------------------------

void DrawableObject::Draw(
    RF::DrawPass & drawPass, 
    BindDescriptorSetFunction const & bindFunction
) {
    BindVertexBuffer(drawPass, mGpuModel->meshBuffers.verticesBuffer);
    BindIndexBuffer(drawPass, mGpuModel->meshBuffers.indicesBuffer);

    for (auto & node : mNodes) {
        drawNode(drawPass, node, bindFunction);
    }
}

//-------------------------------------------------------------------------------------------------

void DrawableObject::EnableUI(char const * windowName, bool * isVisible) {
    MFA_ASSERT(windowName != nullptr && strlen(windowName) > 0);
    mRecordUIObject.Enable();
    mRecordWindowName = "DrawableObject: " + std::string(windowName);
    mIsUIVisible = isVisible;
}

//-------------------------------------------------------------------------------------------------

void DrawableObject::DisableUI() {
    mRecordUIObject.Disable();
}

//-------------------------------------------------------------------------------------------------

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

//-------------------------------------------------------------------------------------------------

RB::DescriptorSetGroup * DrawableObject::GetDescriptorSetGroup(char const * name) {
    MFA_ASSERT(name != nullptr);
    auto const findResult = mDescriptorSetGroups.find(name);
    if (findResult != mDescriptorSetGroups.end()) {
        return &findResult->second;
    }
    return nullptr;
}

//-------------------------------------------------------------------------------------------------

void DrawableObject::UpdateModelTransform(float modelTransform[16]) {
    if (Matrix4X4Float::IsEqual(mModelTransform, modelTransform) == false) {
        mModelTransform = Matrix4X4Float::ConvertCellsToMat4(modelTransform);
        mIsModelTransformChanged = true;
    }
}

//-------------------------------------------------------------------------------------------------

void DrawableObject::AllocStorage(char const * name, size_t const size) {
    MFA_ASSERT(mStorageMap.find(name) == mStorageMap.end());
    mStorageMap[name] = Memory::Alloc(size);
}

//-------------------------------------------------------------------------------------------------

Blob DrawableObject::GetStorage(char const * name) {
    auto const findResult = mStorageMap.find(name);
    if (findResult == mStorageMap.end()) {
        return {};
    }
    return findResult->second;
}

//-------------------------------------------------------------------------------------------------

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

//-------------------------------------------------------------------------------------------------

void DrawableObject::computeNodesGlobalTransform() {
    auto & mesh = mGpuModel->model.mesh;

    auto const rootNodesCount = mesh.GetRootNodesCount();

    for (uint32_t i = 0; i < rootNodesCount; ++i) {
        auto & node = mNodes[mesh.GetIndexOfRootNode(i)];
        computeNodeGlobalTransform(node, nullptr, false);
    }
}

//-------------------------------------------------------------------------------------------------

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

//-------------------------------------------------------------------------------------------------

void DrawableObject::updateSkinJoints(uint32_t const skinIndex, AS::MeshSkin const & skin) {
    // TODO We can do some caching here as well
    auto & jointMatrices = mCachedSkinsJoints[skinIndex];
    for (size_t i = 0; i < skin.joints.size(); i++)
    {
        auto const & joint = mNodes[skin.joints[i]];
        auto const nodeMatrix = joint.cachedGlobalTransform;
        glm::mat4 matrix = nodeMatrix *
            Matrix4X4Float::ConvertCellsToMat4(skin.inverseBindMatrices[i].value);  // T - S = changes
        Matrix4X4Float::ConvertGlmToCells(matrix, jointMatrices[i].model);
    }
}

//-------------------------------------------------------------------------------------------------

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

//-------------------------------------------------------------------------------------------------

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

//-------------------------------------------------------------------------------------------------

glm::mat4 DrawableObject::computeNodeLocalTransform(Node const & node) {
    glm::mat4 result {1};
    result = glm::translate(result, node.translate);
    result = result * glm::toMat4(node.rotation);
    result = glm::scale(result, node.scale);
    result = result * node.transform;
    return result;
}

//-------------------------------------------------------------------------------------------------

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
    if (isChanged && node.meshNode->hasSubMesh() && node.meshNode->skin > -1) {
        node.cachedGlobalInverseTransform = glm::inverse(node.cachedGlobalTransform);
    }

    node.isCachedDataValid = true;

    for (auto const childIndex : node.meshNode->children) {
        auto & childNode = mNodes[childIndex];
        computeNodeGlobalTransform(childNode, &node, isChanged);
    }
}

//-------------------------------------------------------------------------------------------------

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

//-------------------------------------------------------------------------------------------------

};
