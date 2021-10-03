#include "DrawableVariant.hpp"

#include "engine/render_system/RenderTypes.hpp"
#include "engine/BedrockMemory.hpp"
#include "engine/render_system/drawable_essence/DrawableEssence.hpp"
#include "engine/render_system/RenderFrontend.hpp"
#include "engine/FoundationAsset.hpp"
#include "engine/ui_system/UISystem.hpp"
#include "engine/BedrockAssert.hpp"
#include "engine/BedrockMatrix.hpp"
#include "engine/entity_system/Entity.hpp"
#include "engine/entity_system/components/TransformComponent.hpp"
#include "engine/entity_system/components/BoundingVolumeComponent.hpp"
#include "engine/entity_system/components/MeshRendererComponent.hpp"

#include <ext/matrix_transform.hpp>

#include <string>
#include <ranges>

using namespace MFA;

//-------------------------------------------------------------------------------------------------

RT::DrawableVariantId DrawableVariant::NextInstanceId = 0;

//-------------------------------------------------------------------------------------------------

DrawableVariant::DrawableVariant(DrawableEssence const & essence)
    : mId(NextInstanceId)
    , mEssence(&essence)
{
    NextInstanceId += 1;

    //SetActive(true);
    //mName = mEssence->GetName() + " Clone(" + std::to_string(mId) + ")";
    mMesh = &mEssence->GetGpuModel().model.mesh;
    MFA_ASSERT(mMesh != nullptr);
    MFA_ASSERT(mMesh->IsValid());

    // Skins
    mSkins.resize(mMesh->GetSkinsCount());
    
    {// Nodes
        uint32_t const nodesCount = mMesh->GetNodesCount();
        mNodes.resize(nodesCount);
        for (uint32_t i = 0; i < nodesCount; ++i) {
            auto & meshNode = mMesh->GetNodeByIndex(i);
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

            Matrix::CopyCellsToMat4(meshNode.transform, node.currentTransform);
            Matrix::CopyCellsToMat4(meshNode.transform, node.previousTransform);
        }
    }

    {// Creating cachedSkinJoints array
        {
            uint32_t totalJointsCount = 0;
            for (uint32_t i = 0; i < mMesh->GetSkinsCount(); ++i) {
                auto & skin = mMesh->GetSkinByIndex(i);
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
            mCachedSkinsJoints.resize(mMesh->GetSkinsCount());
            uint32_t startingJointIndex = 0;
            for (uint32_t i = 0; i < mMesh->GetSkinsCount(); ++i) {
                auto & skin = mMesh->GetSkinByIndex(i);
                auto const jointsCount = static_cast<uint32_t>(skin.joints.size());
                mCachedSkinsJoints[i] = reinterpret_cast<JointTransformData *>(mCachedSkinsJointsBlob.ptr + startingJointIndex * sizeof(JointTransformData));
                mSkins[i].skinStartingIndex = static_cast<int>(startingJointIndex);
                startingJointIndex += jointsCount;
            }
        }
    }
}

//-------------------------------------------------------------------------------------------------

DrawableVariant::~DrawableVariant()
{
    MFA_ASSERT (mIsInitialized == false);
    
    if (mCachedSkinsJointsBlob.len > 0) {
        Memory::Free(mCachedSkinsJointsBlob);
    }
    if (mSkinsJointsBuffer.bufferSize > 0) {
        RF::DestroyUniformBuffer(mSkinsJointsBuffer);
    }
    //DeleteUniformBuffers();

};

//-------------------------------------------------------------------------------------------------

//// Only for model local buffers
//RT::UniformBufferGroup const & DrawableVariant::CreateUniformBuffer(
//    char const * name,
//    uint32_t const size,
//    uint32_t const count
//) {
//    MFA_ASSERT(mUniformBuffers.find(name) == mUniformBuffers.end());
//    mUniformBuffers[name] = RF::CreateUniformBuffer(size, count);
//    return mUniformBuffers[name];
//}
//
////-------------------------------------------------------------------------------------------------
//
//// Only for model local buffers
//void DrawableVariant::DeleteUniformBuffers() {
//    for (auto const & dirtyBuffer : mDirtyBuffers) {
//        delete dirtyBuffer.ubo.ptr;
//    }
//    mDirtyBuffers.clear();
//    
//    for (auto & pair : mUniformBuffers) {
//        RF::DestroyUniformBuffer(pair.second);
//    }
//    mUniformBuffers.clear();
//}
//
////-------------------------------------------------------------------------------------------------
//
//void DrawableVariant::UpdateUniformBuffer(
//    char const * name,
//    uint32_t startingIndex,
//    CBlob const ubo
//) {
//    for (auto & dirtyBuffer : mDirtyBuffers) {
//        if (
//            strcmp(dirtyBuffer.bufferName.c_str(), name) == 0 &&
//            dirtyBuffer.startingIndex == startingIndex
//        ) {
//            MFA_ASSERT(dirtyBuffer.ubo.len == ubo.len);
//            Memory::Free(dirtyBuffer.ubo);
//            dirtyBuffer.ubo.ptr = new uint8_t[ubo.len];
//            dirtyBuffer.ubo.len = ubo.len;
//            ::memcpy(dirtyBuffer.ubo.ptr, ubo.ptr, ubo.len);
//            
//            dirtyBuffer.remainingUpdateCount = RF::GetMaxFramesPerFlight();
//            return;
//        }
//    }
//    
//    auto const findResult = mUniformBuffers.find(name);
//    if (findResult == mUniformBuffers.end()) {
//        MFA_CRASH("Buffer not found");
//        return;
//    }
//    
//    DirtyBuffer dirtyBuffer {
//        .bufferName = name,
//        .bufferGroup = &findResult->second,
//        .startingIndex = startingIndex,
//        .remainingUpdateCount = RF::GetMaxFramesPerFlight(),
//        .ubo = Memory::Alloc(ubo.len),
//    };
//    ::memcpy(dirtyBuffer.ubo.ptr, ubo.ptr, ubo.len);
//    
//    mDirtyBuffers.emplace_back(dirtyBuffer);
//}

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

void DrawableVariant::Init(MeshRendererComponent * meshRendererComponent)
{
    if (mIsInitialized)
    {
        return;
    }
    mIsInitialized = true;

    MFA_ASSERT(meshRendererComponent != nullptr);
    mMeshRendererComponent = meshRendererComponent; 

    mEntity = mMeshRendererComponent->GetEntity();
    MFA_ASSERT(mEntity != nullptr);

    mTransformComponent = mEntity->GetComponent<TransformComponent>();
    MFA_ASSERT(mTransformComponent != nullptr);

    mTransformListenerId = mTransformComponent->RegisterChangeListener([this]()->void{
        mIsModelTransformChanged = true;
    });

    mBoundingVolumeComponent = mEntity->GetComponent<BoundingVolumeComponent>();
    MFA_ASSERT(mBoundingVolumeComponent != nullptr);
}

//-------------------------------------------------------------------------------------------------

void DrawableVariant::Update(float const deltaTimeInSec, RT::DrawPass const & drawPass) {
    if (IsActive() == false) {
        return;
    }

    // Check if object is visible in frustum
    bool const isInFrustum = mBoundingVolumeComponent->IsInFrustum();

    // If object is not visible we only need to update animation time
    
    updateAnimation(deltaTimeInSec, isInFrustum);
    if (isInFrustum == false)
    {
        return;
    }
    computeNodesGlobalTransform();
    updateAllSkinsJoints();
    
    mIsModelTransformChanged = false;
    // We update buffers after all of computations
    
    //for (int i = static_cast<int>(mDirtyBuffers.size()) - 1; i >= 0; --i) {
    //    auto & dirtyBuffer = mDirtyBuffers[i];
    //    if (dirtyBuffer.remainingUpdateCount > 0) {
    //        --dirtyBuffer.remainingUpdateCount;
    //        RF::UpdateUniformBuffer(
    //            dirtyBuffer.bufferGroup->buffers[dirtyBuffer.startingIndex + drawPass.frameIndex],
    //            dirtyBuffer.ubo
    //        );
    //    } else {
    //        Memory::Free(dirtyBuffer.ubo);
    //        mDirtyBuffers.erase(mDirtyBuffers.begin() + i);
    //    }
    //}

    if (mSkinsJointsBuffer.bufferSize > 0) {
        RF::UpdateUniformBuffer(
            mSkinsJointsBuffer.buffers[drawPass.frameIndex],
            mCachedSkinsJointsBlob
        );
    }
}

//-------------------------------------------------------------------------------------------------

void DrawableVariant::Shutdown()
{
    if (mIsInitialized == false)
    {
        return;
    }
    mIsInitialized = false;

    mTransformComponent->UnRegisterChangeListener(mTransformListenerId);

}

//-------------------------------------------------------------------------------------------------

void DrawableVariant::Draw(
    RT::DrawPass const & drawPass,
    BindDescriptorSetFunction const & bindFunction
) {
    for (auto & node : mNodes) {
        drawNode(drawPass, node, bindFunction);
    }
}

//-------------------------------------------------------------------------------------------------

bool DrawableVariant::IsCurrentAnimationFinished() const {
    return mIsAnimationFinished;
}

//-------------------------------------------------------------------------------------------------

bool DrawableVariant::IsInFrustum() const
{
    MFA_ASSERT(mBoundingVolumeComponent != nullptr);
    return mBoundingVolumeComponent->IsInFrustum();
}

//-------------------------------------------------------------------------------------------------

Entity * DrawableVariant::GetEntity() const
{
    return mEntity;
}

//-------------------------------------------------------------------------------------------------

void DrawableVariant::SetActiveAnimationIndex(int const nextAnimationIndex, AnimationParams const & params) {
    if (nextAnimationIndex == mActiveAnimationIndex) {
        return;
    }
    mIsAnimationFinished = false;
    mPreviousAnimationIndex = mActiveAnimationIndex;
    mActiveAnimationIndex = nextAnimationIndex;
    mAnimationTransitionDurationInSec = params.transitionDuration;
    mAnimationRemainingTransitionDurationInSec = params.transitionDuration;
    mPreviousAnimationTimeInSec = mActiveAnimationTimeInSec;
    mActiveAnimationTimeInSec = mEssence->GetGpuModel().model.mesh.GetAnimationByIndex(mActiveAnimationIndex).startTime;
    mActiveAnimationParams = params;
}

//-------------------------------------------------------------------------------------------------

void DrawableVariant::SetActiveAnimation(char const * animationName, AnimationParams const & params) {
    auto const index = mEssence->GetAnimationIndex(animationName);
    if (MFA_VERIFY(index >= 0)) {
        SetActiveAnimationIndex(index, params);
    }
}

//-------------------------------------------------------------------------------------------------

void DrawableVariant::updateAnimation(float const deltaTimeInSec, bool isInFrustum) {
    using Animation = AS::Mesh::Animation;

    auto & mesh = mEssence->GetGpuModel().model.mesh;

    if (mesh.GetAnimationsCount() <= 0) {
        return;
    }

    {// Active animation
        auto const & activeAnimation = mMesh->GetAnimationByIndex(mActiveAnimationIndex);

        if (isInFrustum) {
            for (auto const & channel : activeAnimation.channels)
            {
                auto const & sampler = activeAnimation.samplers[channel.samplerIndex];
                auto & node = mNodes[channel.nodeIndex];

                for (size_t i = 0; i < sampler.inputAndOutput.size() - 1; i++)
                {
                    MFA_ASSERT(sampler.interpolation == Animation::Interpolation::Linear);
                    
                    auto const & previousInput = sampler.inputAndOutput[i].input;
                    auto const & previousOutput = sampler.inputAndOutput[i].output;
                    auto const & nextInput = sampler.inputAndOutput[i + 1].input;
                    auto const & nextOutput = sampler.inputAndOutput[i + 1].output;
                    // Get the input keyframe values for the current time stamp
                    if (mActiveAnimationTimeInSec >= previousInput && mActiveAnimationTimeInSec <= nextInput)
                    {
                        float const fraction = (mActiveAnimationTimeInSec - previousInput) / (nextInput - previousInput);

                        if (channel.path == Animation::Path::Translation)
                        {
                            node.currentTranslate = glm::mix(Matrix::ConvertCellsToVec3(previousOutput), Matrix::ConvertCellsToVec3(nextOutput), fraction);
                        }
                        else if (channel.path == Animation::Path::Rotation)
                        {
                            glm::quat previousRotation;
                            previousRotation.x = previousOutput[0];
                            previousRotation.y = previousOutput[1];
                            previousRotation.z = previousOutput[2];
                            previousRotation.w = previousOutput[3];

                            glm::quat nextRotation;
                            nextRotation.x = nextOutput[0];
                            nextRotation.y = nextOutput[1];
                            nextRotation.z = nextOutput[2];
                            nextRotation.w = nextOutput[3];

                            node.currentRotation = glm::normalize(glm::slerp(previousRotation, nextRotation, fraction));
                        }
                        else if (channel.path == Animation::Path::Scale)
                        {
                            node.currentScale = glm::mix(Matrix::ConvertCellsToVec3(previousOutput), Matrix::ConvertCellsToVec3(nextOutput), fraction);
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
        mActiveAnimationTimeInSec += deltaTimeInSec;
        if (mActiveAnimationTimeInSec > activeAnimation.endTime) {
            MFA_ASSERT(activeAnimation.endTime >= activeAnimation.startTime);

            if (mActiveAnimationParams.loop) {
                mActiveAnimationTimeInSec -= (activeAnimation.endTime - activeAnimation.startTime);
            } else {
                mIsAnimationFinished = true;
            }

        }
    }
    if (isInFrustum) {// Previous animation
        if (mAnimationRemainingTransitionDurationInSec <= 0 || mPreviousAnimationIndex < 0) {
            return;
        }
        
        auto const & previousAnimation = mMesh->GetAnimationByIndex(mPreviousAnimationIndex);
        
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
                auto previousOutput = Matrix::ConvertCellsToVec4(sampler.inputAndOutput[i].output);
                auto const nextInput = sampler.inputAndOutput[i + 1].input;
                auto nextOutput = Matrix::ConvertCellsToVec4(sampler.inputAndOutput[i + 1].output);
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

    auto const rootNodesCount = mMesh->GetRootNodesCount();

    for (uint32_t i = 0; i < rootNodesCount; ++i) {
        auto & node = mNodes[mesh.GetIndexOfRootNode(i)];
        computeNodeGlobalTransform(node, nullptr, false);
    }
}

//-------------------------------------------------------------------------------------------------

void DrawableVariant::updateAllSkinsJoints() {
    auto const skinsCount = mMesh->GetSkinsCount();
    if (skinsCount > 0) {
        auto const * skins = mMesh->GetSkinData();
        MFA_ASSERT(skins != nullptr);

        for (uint32_t i = 0; i < skinsCount; ++i) {
            updateSkinJoints(i, skins[i]);
        }
    }
}

//-------------------------------------------------------------------------------------------------

void DrawableVariant::updateSkinJoints(uint32_t const skinIndex, AS::MeshSkin const & skin) {
    // TODO We can do some caching here as well
    auto const & jointMatrices = mCachedSkinsJoints[skinIndex];
    for (size_t i = 0; i < skin.joints.size(); i++)
    {
        auto const & joint = mNodes[skin.joints[i]];
        auto const nodeMatrix = joint.cachedGlobalTransform;
        jointMatrices[i].model = nodeMatrix * skin.inverseBindMatrices[i];  // T - S = changes
    }
}

//-------------------------------------------------------------------------------------------------

void DrawableVariant::drawNode(
    RT::DrawPass const & drawPass,
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
            mMesh->GetSubMeshByIndex(node.meshNode->subMeshIndex),
            node,
            bindFunction
        );
    }
}

//-------------------------------------------------------------------------------------------------

void DrawableVariant::drawSubMesh(
    RT::DrawPass const & drawPass,
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
    if (mAnimationRemainingTransitionDurationInSec > 0 && mPreviousAnimationIndex >= 0) {
        auto const fraction = (mAnimationTransitionDurationInSec - mAnimationRemainingTransitionDurationInSec) / mAnimationTransitionDurationInSec;
        translate = glm::mix(node.previousTranslate, translate, fraction);
        rotation = glm::normalize(glm::slerp(node.previousRotation, rotation, fraction));
        scale = glm::mix(node.previousScale, scale, fraction);
    }
    currentTransform = glm::translate(currentTransform, translate);
    currentTransform = currentTransform * glm::toMat4(rotation);
    currentTransform = glm::scale(currentTransform, scale);
    currentTransform = currentTransform * node.currentTransform;
    return currentTransform;
}

//-------------------------------------------------------------------------------------------------

void DrawableVariant::computeNodeGlobalTransform(Node & node, Node const * parentNode, bool const isParentTransformChanged) {
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
        node.cachedModelTransform = mTransformComponent->GetTransform() * node.cachedGlobalTransform;
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
        animationsList[i] = mMesh->GetAnimationByIndex(static_cast<uint32_t>(i)).name.c_str();
    }

    UI::BeginWindow(mTransformComponent->GetEntity()->GetName().data());
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
    return mMeshRendererComponent->IsActive();
}

//-------------------------------------------------------------------------------------------------
