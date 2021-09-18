#include "DrawableVariant.hpp"

#include "engine/render_system/RenderTypes.hpp"
#include "engine/BedrockMemory.hpp"
#include "engine/render_system/drawable_essence/DrawableEssence.hpp"
#include "engine/render_system/RenderFrontend.hpp"
#include "engine/FoundationAsset.hpp"
#include "engine/ui_system/UISystem.hpp"

#include <ext/matrix_transform.hpp>

#include <string>

namespace MFA {

namespace UI = UISystem;

//-------------------------------------------------------------------------------------------------

RT::DrawableVariantId DrawableVariant::NextInstanceId = 0;

//-------------------------------------------------------------------------------------------------

DrawableVariant::DrawableVariant(DrawableEssence const & essence)
    : mId(NextInstanceId)
    , mEssence(&essence)
{
    NextInstanceId += 1;

    auto & mesh = mEssence->GetGpuModel().model.mesh;
    MFA_ASSERT(mesh.IsValid());
    
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
            
            node.currentRotation.x = meshNode.rotation[0];
            node.currentRotation.y = meshNode.rotation[1];
            node.currentRotation.z = meshNode.rotation[2];
            node.currentRotation.w = meshNode.rotation[3];

            node.currentScale.x = meshNode.scale[0];
            node.currentScale.y = meshNode.scale[1];
            node.currentScale.z = meshNode.scale[2];

            node.currentTranslate.x = meshNode.translate[0];
            node.currentTranslate.y = meshNode.translate[1];
            node.currentTranslate.z = meshNode.translate[2];

            node.previousRotation.x = meshNode.rotation[0];
            node.previousRotation.y = meshNode.rotation[1];
            node.previousRotation.z = meshNode.rotation[2];
            node.previousRotation.w = meshNode.rotation[3];

            node.previousScale.x = meshNode.scale[0];
            node.previousScale.y = meshNode.scale[1];
            node.previousScale.z = meshNode.scale[2];

            node.previousTranslate.x = meshNode.translate[0];
            node.previousTranslate.y = meshNode.translate[1];
            node.previousTranslate.z = meshNode.translate[2];

            node.skin = meshNode.skin > -1 ? &mSkins[meshNode.skin] : nullptr;

            Matrix4X4Float::ConvertCellsToMat4(meshNode.transform, node.currentTransform);
            Matrix4X4Float::ConvertCellsToMat4(meshNode.transform, node.previousTransform);
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
                mSkinsJointsBuffer = RF::CreateUniformBuffer(
                    mCachedSkinsJointsBlob.len, 
                    RF::GetMaxFramesPerFlight()
                );
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

    SetActive(true);
    mName = mEssence->GetName() + " Clone(" + std::to_string(mId) + ")";
}

//-------------------------------------------------------------------------------------------------

DrawableVariant::~DrawableVariant() {
    if (mCachedSkinsJointsBlob.len > 0) {
        Memory::Free(mCachedSkinsJointsBlob);
    }
    if (mSkinsJointsBuffer.bufferSize > 0) {
        RF::DestroyUniformBuffer(mSkinsJointsBuffer);
    }
    DeleteUniformBuffers();

    for (auto & storedData : mStorageMap) {
        Memory::Free(storedData.second);
    }
}

//-------------------------------------------------------------------------------------------------

// Only for model local buffers
RT::UniformBufferGroup const & DrawableVariant::CreateUniformBuffer(
    char const * name,
    uint32_t const size,
    uint32_t const count
) {
    MFA_ASSERT(mUniformBuffers.find(name) == mUniformBuffers.end());
    mUniformBuffers[name] = RF::CreateUniformBuffer(size, count);
    return mUniformBuffers[name];
}

//-------------------------------------------------------------------------------------------------

// Only for model local buffers
void DrawableVariant::DeleteUniformBuffers() {
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

void DrawableVariant::UpdateUniformBuffer(
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

RT::UniformBufferGroup const * DrawableVariant::GetUniformBuffer(char const * name) {
    auto const findResult = mUniformBuffers.find(name);
    if (findResult != mUniformBuffers.end()) {
        return &findResult->second;
    }
    return nullptr;
}

//-------------------------------------------------------------------------------------------------

[[nodiscard]]
RT::UniformBufferGroup const & DrawableVariant::GetSkinJointsBuffer() const noexcept {
    return mSkinsJointsBuffer;
}

//-------------------------------------------------------------------------------------------------

void DrawableVariant::Update(float const deltaTimeInSec, RT::DrawPass const & drawPass) {
    if (mIsActive == false) {
        return;
    }

    // If object is not visible we only need to update animation time
    
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

void DrawableVariant::Draw(
    RT::DrawPass & drawPass,
    BindDescriptorSetFunction const & bindFunction
) {

    if (mIsActive == false) {
        return;
    } 

    RF::BindVertexBuffer(drawPass, mEssence->GetGpuModel().meshBuffers.verticesBuffer);
    RF::BindIndexBuffer(drawPass, mEssence->GetGpuModel().meshBuffers.indicesBuffer);

    for (auto & node : mNodes) {
        drawNode(drawPass, node, bindFunction);
    }
}

//-------------------------------------------------------------------------------------------------

void DrawableVariant::UpdateModelTransform(float modelTransform[16]) {
    if (Matrix4X4Float::IsEqual(mModelTransform, modelTransform) == false) {
        mModelTransform = Matrix4X4Float::ConvertCellsToMat4(modelTransform);
        mIsModelTransformChanged = true;
    }
}

//-------------------------------------------------------------------------------------------------

void DrawableVariant::SetActiveAnimationIndex(int const nextAnimationIndex, float transitionDuration) {
    if (nextAnimationIndex == mActiveAnimationIndex) {
        return;
    }
    mPreviousAnimationIndex = mActiveAnimationIndex;
    mActiveAnimationIndex = nextAnimationIndex;
    mAnimationTransitionDurationInSec = transitionDuration;
    mAnimationRemainingTransitionDurationInSec = transitionDuration;
    mPreviousAnimationTimeInSec = mActiveAnimationTimeInSec;
    mActiveAnimationTimeInSec = mEssence->GetGpuModel().model.mesh.GetAnimationByIndex(mActiveAnimationIndex).startTime;
}

//-------------------------------------------------------------------------------------------------

void DrawableVariant::AllocStorage(char const * name, size_t const size) {
    MFA_ASSERT(mStorageMap.find(name) == mStorageMap.end());
    mStorageMap[name] = Memory::Alloc(size);
}

//-------------------------------------------------------------------------------------------------

Blob DrawableVariant::GetStorage(char const * name) {
    auto const findResult = mStorageMap.find(name);
    if (findResult == mStorageMap.end()) {
        return {};
    }
    return findResult->second;
}

//-------------------------------------------------------------------------------------------------

void DrawableVariant::updateAnimation(float const deltaTimeInSec) {
    using Animation = AS::Mesh::Animation;

    auto & mesh = mEssence->GetGpuModel().model.mesh;

    if (mesh.GetAnimationsCount() <= 0) {
        return;
    }

    {// Active animation
        auto const & activeAnimation = mesh.GetAnimationByIndex(mActiveAnimationIndex);
        
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
                if (mActiveAnimationTimeInSec >= previousInput && mActiveAnimationTimeInSec <= nextInput)
                {
                    float const fraction = (mActiveAnimationTimeInSec - previousInput) / (nextInput - previousInput);

                    if (channel.path == Animation::Path::Translation)
                    {
                        node.currentTranslate = glm::mix(previousOutput, nextOutput, fraction);
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

                        node.currentRotation = glm::normalize(glm::slerp(previousRotation, nextRotation, fraction));
                    }
                    else if (channel.path == Animation::Path::Scale)
                    {
                        node.currentScale = glm::mix(previousOutput, nextOutput, fraction);
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
        
        mActiveAnimationTimeInSec += deltaTimeInSec;
        if (mActiveAnimationTimeInSec > activeAnimation.endTime) {
            MFA_ASSERT(activeAnimation.endTime >= activeAnimation.startTime);

            // TODO This should be a setting, We also have to define whether an animation needs to be looped or not
            //mPreviousAnimationTimeInSec = activeAnimation.endTime;
            //mPreviousAnimationIndex = mActiveAnimationIndex;
            //mAnimationTransitionDurationInSec = 0.1f;
            //mAnimationRemainingTransitionDurationInSec = 0.3f;

            mActiveAnimationTimeInSec -= (activeAnimation.endTime - activeAnimation.startTime);

        }
    }
    {// Previous animation
        if (mAnimationRemainingTransitionDurationInSec <= 0 || mPreviousAnimationIndex < 0) {
            return;
        }
        
        auto const & previousAnimation = mesh.GetAnimationByIndex(mPreviousAnimationIndex);
        
        for (auto const & channel : previousAnimation.channels)
        {
            auto const & sampler = previousAnimation.samplers[channel.samplerIndex];
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
                if (mPreviousAnimationTimeInSec >= previousInput && mPreviousAnimationTimeInSec <= nextInput)
                {
                    float const fraction = (mPreviousAnimationTimeInSec - previousInput) / (nextInput - previousInput);

                    if (channel.path == Animation::Path::Translation)
                    {
                        node.previousTranslate = glm::mix(previousOutput, nextOutput, fraction);
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

                        node.previousRotation = glm::normalize(glm::slerp(previousRotation, nextRotation, fraction));
                    }
                    else if (channel.path == Animation::Path::Scale)
                    {
                        node.previousScale = glm::mix(previousOutput, nextOutput, fraction);
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
        
        mAnimationRemainingTransitionDurationInSec -= deltaTimeInSec;
    }
}

//-------------------------------------------------------------------------------------------------

void DrawableVariant::computeNodesGlobalTransform() {
    auto const & mesh = mEssence->GetGpuModel().model.mesh;

    auto const rootNodesCount = mesh.GetRootNodesCount();

    for (uint32_t i = 0; i < rootNodesCount; ++i) {
        auto & node = mNodes[mesh.GetIndexOfRootNode(i)];
        computeNodeGlobalTransform(node, nullptr, false);
    }
}

//-------------------------------------------------------------------------------------------------

void DrawableVariant::updateAllSkinsJoints() {
    auto const & mesh = mEssence->GetGpuModel().model.mesh;

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

void DrawableVariant::updateSkinJoints(uint32_t const skinIndex, AS::MeshSkin const & skin) {
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

void DrawableVariant::drawNode(
    RT::DrawPass & drawPass,
    Node const & node,
    BindDescriptorSetFunction const & bindFunction
) {
    // TODO We can reduce nodes count for better performance when importing
    if (node.meshNode->hasSubMesh())
    {
        auto const & mesh = mEssence->GetGpuModel().model.mesh;
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

void DrawableVariant::drawSubMesh(
    RT::DrawPass & drawPass,
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

glm::mat4 DrawableVariant::computeNodeLocalTransform(Node const & node) const {
    glm::mat4 currentTransform {1};
    auto translate = node.currentTranslate;
    auto rotation = node.currentRotation;
    auto scale = node.currentScale;
    if (mAnimationRemainingTransitionDurationInSec > 0 && mPreviousAnimationIndex > 0) {
        auto const fraction = (mAnimationTransitionDurationInSec - mAnimationRemainingTransitionDurationInSec) / mAnimationTransitionDurationInSec;
        translate = glm::mix(node.previousTranslate, translate, fraction);
        rotation = glm::slerp(node.previousRotation, rotation, fraction);
        scale = glm::mix(node.previousScale, scale, fraction);
    }
    currentTransform = glm::translate(currentTransform, translate);
    currentTransform = currentTransform * glm::toMat4(rotation);
    currentTransform = glm::scale(currentTransform, scale);
    currentTransform = currentTransform * node.currentTransform;
    return currentTransform;
}

//-------------------------------------------------------------------------------------------------

void DrawableVariant::computeNodeGlobalTransform(Node & node, Node const * parentNode, bool isParentTransformChanged) {
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

void DrawableVariant::OnUI() {
    auto & mesh = mEssence->GetGpuModel().model.mesh;

    std::vector<char const *> animationsList {mesh.GetAnimationsCount()};
    for (size_t i = 0; i < animationsList.size(); ++i) {
        animationsList[i] = mesh.GetAnimationByIndex(static_cast<uint32_t>(i)).name.c_str();
    }

    UI::BeginWindow(mName.data());
    UI::Combo(
        "Active animation",
        &mUISelectedAnimationIndex,
        animationsList.data(),
        static_cast<int32_t>(animationsList.size())
    );
    SetActiveAnimationIndex(mUISelectedAnimationIndex);
    UI::EndWindow();
}

//-------------------------------------------------------------------------------------------------

std::string const & DrawableVariant::GetName() const noexcept {
    return mName;
}

//-------------------------------------------------------------------------------------------------

void DrawableVariant::SetName(char const * name) {
    MFA_ASSERT(name != nullptr && strlen(name) > 0);
    mName = name;
}

//-------------------------------------------------------------------------------------------------

DrawableEssence const * DrawableVariant::GetEssence() const noexcept {
    return mEssence;
}

//-------------------------------------------------------------------------------------------------

RT::DrawableVariantId DrawableVariant::GetId() const noexcept {
    return mId;
}

//-------------------------------------------------------------------------------------------------

RT::DescriptorSetGroup const & DrawableVariant::CreateDescriptorSetGroup(
    char const * name, 
    VkDescriptorSetLayout descriptorSetLayout, 
    uint32_t descriptorSetCount
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

RT::DescriptorSetGroup const * DrawableVariant::GetDescriptorSetGroup(char const * name) {
    MFA_ASSERT(name != nullptr);
    auto const findResult = mDescriptorSetGroups.find(name);
    if (findResult != mDescriptorSetGroups.end()) {
        return &findResult->second;
    }
    return nullptr;
}

//-------------------------------------------------------------------------------------------------

bool DrawableVariant::IsActive() const noexcept {
    return mIsActive;
}

//-------------------------------------------------------------------------------------------------

void DrawableVariant::SetActive(const bool isActive) {
    mIsActive = isActive;
}

//-------------------------------------------------------------------------------------------------

}
