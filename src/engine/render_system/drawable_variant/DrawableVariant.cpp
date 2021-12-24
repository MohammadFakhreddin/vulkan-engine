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

#include <glm/gtx/quaternion.hpp>

#include <string>

namespace MFA
{

    //-------------------------------------------------------------------------------------------------

    RT::DrawableVariantId DrawableVariant::NextInstanceId = 0;

    //-------------------------------------------------------------------------------------------------

    DrawableVariant::DrawableVariant(DrawableEssence const & essence)
        : mId(NextInstanceId)
        , mEssence(&essence)
        , mMesh(*mEssence->GetGpuModel()->model->mesh)
    {
        NextInstanceId += 1;

        MFA_ASSERT(mMesh.IsValid());

        // Skins
        mSkins.resize(mMesh.GetSkinsCount());

        {// Nodes
            uint32_t const nodesCount = mMesh.GetNodesCount();
            mNodes.resize(nodesCount);
            for (uint32_t i = 0; i < nodesCount; ++i)
            {
                auto & meshNode = mMesh.GetNodeByIndex(i);
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

                Matrix::CopyCellsToGlm(meshNode.transform, node.currentTransform);
                Matrix::CopyCellsToGlm(meshNode.transform, node.previousTransform);     // This variable is unused
            }
        }

        {// Creating cachedSkinJoints array
            {
                uint32_t totalJointsCount = 0;
                for (uint32_t i = 0; i < mMesh.GetSkinsCount(); ++i)
                {
                    auto & skin = mMesh.GetSkinByIndex(i);
                    auto const jointsCount = static_cast<uint32_t>(skin.joints.size());
                    totalJointsCount += jointsCount;
                }
                if (totalJointsCount > 0)
                {
                    mCachedSkinsJointsBlob = Memory::Alloc(sizeof(JointTransformData) * totalJointsCount);
                    mSkinsJointsBuffer = RF::CreateUniformBuffer(
                        mCachedSkinsJointsBlob->memory.len,
                        RF::GetMaxFramesPerFlight()
                    );
                }
            }
            {
                mCachedSkinsJoints.resize(mMesh.GetSkinsCount());
                uint32_t startingJointIndex = 0;
                for (uint32_t i = 0; i < mMesh.GetSkinsCount(); ++i)
                {
                    auto & skin = mMesh.GetSkinByIndex(i);
                    auto const jointsCount = static_cast<uint32_t>(skin.joints.size());
                    mCachedSkinsJoints[i] = reinterpret_cast<JointTransformData *>(mCachedSkinsJointsBlob->memory.ptr + startingJointIndex * sizeof(JointTransformData));
                    mSkins[i].skinStartingIndex = static_cast<int>(startingJointIndex);
                    startingJointIndex += jointsCount;
                }
            }
        }
    }

    //-------------------------------------------------------------------------------------------------

    DrawableVariant::~DrawableVariant()
    {
        if (mIsInitialized == true)
        {
            Shutdown();
        }
    }

    //-------------------------------------------------------------------------------------------------

    RT::UniformBufferGroup const * DrawableVariant::GetUniformBuffer(char const * name)
    {
        auto const findResult = mUniformBuffers.find(name);
        if (findResult != mUniformBuffers.end())
        {
            return &findResult->second;
        }
        return nullptr;
    }

    //-------------------------------------------------------------------------------------------------

    [[nodiscard]]
    RT::UniformBufferGroup const * DrawableVariant::GetSkinJointsBuffer() const noexcept
    {
        return mSkinsJointsBuffer.get();
    }

    //-------------------------------------------------------------------------------------------------

    void DrawableVariant::Init(
        Entity * entity,
        std::weak_ptr<RendererComponent> const & rendererComponent,
        std::weak_ptr<TransformComponent> const & transformComponent,
        std::weak_ptr<BoundingVolumeComponent> const & boundingVolumeComponent
    )
    {
        if (mIsInitialized)
        {
            return;
        }
        mIsInitialized = true;

        MFA_ASSERT(entity != nullptr);
        mEntity = entity;

        MFA_ASSERT(rendererComponent.expired() == false);
        mRendererComponent = rendererComponent;

        MFA_ASSERT(transformComponent.expired() == false);
        mTransformComponent = transformComponent;

        mTransformListenerId = mTransformComponent.lock()->RegisterChangeListener([this]()->void
            {
                mIsModelTransformChanged = true;
            }
        );

        MFA_ASSERT(boundingVolumeComponent.expired() == false);
        mBoundingVolumeComponent = boundingVolumeComponent;
    }

    //-------------------------------------------------------------------------------------------------
    // TODO We should separate it into update buffer and update state phase
    void DrawableVariant::Update(float const deltaTimeInSec, RT::CommandRecordState const & drawPass)
    {
        if (IsActive() == false)
        {
            return;
        }

        // Check if object is visible in frustum
        if (auto const ptr = mBoundingVolumeComponent.lock())
        {
            mIsInFrustum = ptr->IsInFrustum();
        }

        // If object is not visible we only need to update animation time and we can avoid updating skin joints
        auto const isVisible = IsVisible();
        updateAnimation(deltaTimeInSec, isVisible);
        computeNodesGlobalTransform();
        if (isVisible)
        {
            updateAllSkinsJoints();
        }
        mIsModelTransformChanged = false;
        
        // We update buffers after all of computations

        if (mSkinsJointsBuffer != nullptr && mIsSkinJointsChanged == true)
        {
            bufferDirtyCounter = 2;

            mIsSkinJointsChanged = false;
        }
        if (bufferDirtyCounter > 0)
        {
            RF::UpdateUniformBuffer(
               *mSkinsJointsBuffer->buffers[drawPass.frameIndex],
               mCachedSkinsJointsBlob->memory
            );
            bufferDirtyCounter -= 1;
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

        if (auto const ptr = mRendererComponent.lock())
        {
            ptr->NotifyVariantDestroyed();
        }

        if (auto const ptr = mTransformComponent.lock())
        {
            ptr->UnRegisterChangeListener(mTransformListenerId);
        }
    }

    //-------------------------------------------------------------------------------------------------

    void DrawableVariant::Draw(
        RT::CommandRecordState const & drawPass,
        BindDescriptorSetFunction const & bindFunction,
        AlphaMode alphaMode
    )
    {
        for (auto & node : mNodes)
        {
            drawNode(drawPass, node, bindFunction, alphaMode);
        }
    }

    //-------------------------------------------------------------------------------------------------

    bool DrawableVariant::IsCurrentAnimationFinished() const
    {
        return mIsAnimationFinished;
    }

    //-------------------------------------------------------------------------------------------------

    bool DrawableVariant::IsInFrustum() const
    {
        return mIsInFrustum;
    }

    //-------------------------------------------------------------------------------------------------

    Entity * DrawableVariant::GetEntity() const
    {
        return mEntity;
    }

    //-------------------------------------------------------------------------------------------------

    bool DrawableVariant::IsVisible() const
    {
        return IsActive() && mIsOccluded == false && mIsInFrustum == true;
    }

    //-------------------------------------------------------------------------------------------------

    bool DrawableVariant::IsOccluded() const
    {
        return mIsOccluded;
    }

    //-------------------------------------------------------------------------------------------------

    void DrawableVariant::SetIsOccluded(bool const isOccluded)
    {
        mIsOccluded = isOccluded;
    }

    //-------------------------------------------------------------------------------------------------

    bool DrawableVariant::IsInFrustum()
    {
        return mIsInFrustum;
    }

    //-------------------------------------------------------------------------------------------------

    void DrawableVariant::SetActiveAnimationIndex(int const nextAnimationIndex, AnimationParams const & params)
    {
        if (nextAnimationIndex == mActiveAnimationIndex)
        {
            return;
        }
        mIsAnimationFinished = false;
        mPreviousAnimationIndex = mActiveAnimationIndex;
        mActiveAnimationIndex = nextAnimationIndex;
        mAnimationTransitionDurationInSec = params.transitionDuration;
        mAnimationRemainingTransitionDurationInSec = params.transitionDuration;
        mPreviousAnimationTimeInSec = mActiveAnimationTimeInSec;
        mActiveAnimationTimeInSec = params.startTimeOffsetInSec + mMesh.GetAnimationByIndex(mActiveAnimationIndex).startTime;
        mActiveAnimationParams = params;
    }

    //-------------------------------------------------------------------------------------------------

    void DrawableVariant::SetActiveAnimation(char const * animationName, AnimationParams const & params)
    {
        auto const index = mEssence->GetAnimationIndex(animationName);
        if (MFA_VERIFY(index >= 0))
        {
            SetActiveAnimationIndex(index, params);
        }
    }

    //-------------------------------------------------------------------------------------------------

    RT::StorageBufferCollection const & DrawableVariant::CreateStorageBuffer(
        uint32_t const size,
        uint32_t const count
    )
    {
        mStorageBuffer = RF::CreateStorageBuffer(size, count);
        return *mStorageBuffer;
    }

    //-------------------------------------------------------------------------------------------------

    RT::StorageBufferCollection const & DrawableVariant::GetStorageBuffer() const
    {
        return *mStorageBuffer;
    }

    //-------------------------------------------------------------------------------------------------

    BoundingVolumeComponent * DrawableVariant::GetBoundingVolume() const
    {
        return mBoundingVolumeComponent.lock().get();
    }

    //-------------------------------------------------------------------------------------------------

    void DrawableVariant::updateAnimation(float const deltaTimeInSec, bool isVisible)
    {
        using Animation = AS::Mesh::Animation;

        if (mMesh.GetAnimationsCount() <= 0)
        {
            return;
        }

        {// Active animation
            auto const & activeAnimation = mMesh.GetAnimationByIndex(mActiveAnimationIndex);

            if (isVisible)
            {
                for (auto const & channel : activeAnimation.channels)
                {
                    auto const & sampler = activeAnimation.samplers[channel.samplerIndex];
                    auto & node = mNodes[channel.nodeIndex];

                    // Just caching the input index to reduce required loop count
                    auto & inputIndex = mAnimationInputIndex[channel.samplerIndex];
                    if (inputIndex >= sampler.inputAndOutput.size() - 1)
                    {
                        inputIndex = 0;
                    }

                    for (size_t k = 0; k < sampler.inputAndOutput.size() - 1; k++)
                    {
                        MFA_ASSERT(sampler.interpolation == Animation::Interpolation::Linear);

                        auto const & previousInput = sampler.inputAndOutput[inputIndex].input;
                        auto const & previousOutput = sampler.inputAndOutput[inputIndex].output;
                        auto const & nextInput = sampler.inputAndOutput[inputIndex + 1].input;
                        auto const & nextOutput = sampler.inputAndOutput[inputIndex + 1].output;
                        // Get the input keyframe values for the current time stamp
                        if (mActiveAnimationTimeInSec >= previousInput && mActiveAnimationTimeInSec <= nextInput)
                        {
                            float const fraction = (mActiveAnimationTimeInSec - previousInput) / (nextInput - previousInput);

                            if (channel.path == Animation::Path::Translation)
                            {
                                node.currentTranslate = glm::mix(Matrix::CopyCellsToVec3(previousOutput), Matrix::CopyCellsToVec3(nextOutput), fraction);
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

                                node.currentRotation = glm::slerp(previousRotation, nextRotation, fraction);
                            }
                            else if (channel.path == Animation::Path::Scale)
                            {
                                node.currentScale = glm::mix(Matrix::CopyCellsToVec3(previousOutput), Matrix::CopyCellsToVec3(nextOutput), fraction);
                            }
                            else
                            {
                                MFA_ASSERT(false);
                            }

                            node.isCachedDataValid = false;

                            break;
                        } else
                        {
                            inputIndex += 1;
                            if (inputIndex >= sampler.inputAndOutput.size() - 1)
                            {
                                inputIndex = 0;
                            }
                        }
                    }
                }
            }
            mActiveAnimationTimeInSec += deltaTimeInSec;
            if (mActiveAnimationTimeInSec > activeAnimation.endTime)
            {
                MFA_ASSERT(activeAnimation.endTime >= activeAnimation.startTime);

                if (mActiveAnimationParams.loop)
                {
                    mActiveAnimationTimeInSec -= (activeAnimation.endTime - activeAnimation.startTime);
                }
                else
                {
                    mIsAnimationFinished = true;
                }

            }
        }
        if (isVisible)
        {// Previous animation
            if (mAnimationRemainingTransitionDurationInSec <= 0 || mPreviousAnimationIndex < 0)
            {
                return;
            }

            auto const & previousAnimation = mMesh.GetAnimationByIndex(mPreviousAnimationIndex);

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
                    auto previousOutput = Matrix::CopyCellsToVec4(sampler.inputAndOutput[i].output);
                    auto const nextInput = sampler.inputAndOutput[i + 1].input;
                    auto nextOutput = Matrix::CopyCellsToVec4(sampler.inputAndOutput[i + 1].output);
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
                            glm::quat previousRotation{};
                            previousRotation.x = previousOutput[0];
                            previousRotation.y = previousOutput[1];
                            previousRotation.z = previousOutput[2];
                            previousRotation.w = previousOutput[3];

                            glm::quat nextRotation{};
                            nextRotation.x = nextOutput[0];
                            nextRotation.y = nextOutput[1];
                            nextRotation.z = nextOutput[2];
                            nextRotation.w = nextOutput[3];

                            node.previousRotation = glm::slerp(previousRotation, nextRotation, fraction);
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

    void DrawableVariant::computeNodesGlobalTransform()
    {
        auto const rootNodesCount = mMesh.GetRootNodesCount();

        for (uint32_t i = 0; i < rootNodesCount; ++i)
        {
            auto & node = mNodes[mMesh.GetIndexOfRootNode(i)];
            computeNodeGlobalTransform(node, nullptr, false);
        }
    }

    //-------------------------------------------------------------------------------------------------

    void DrawableVariant::updateAllSkinsJoints()
    {
        auto const skinsCount = mMesh.GetSkinsCount();
        if (skinsCount > 0)
        {
            auto const * skins = mMesh.GetSkinData();
            MFA_ASSERT(skins != nullptr);

            for (uint32_t i = 0; i < skinsCount; ++i)
            {
                updateSkinJoints(i, skins[i]);
            }
        }
    }

    //-------------------------------------------------------------------------------------------------

    void DrawableVariant::updateSkinJoints(uint32_t const skinIndex, AS::MeshSkin const & skin)
    {
        // TODO We can do some caching here as well
        auto const & jointMatrices = mCachedSkinsJoints[skinIndex];
        for (size_t i = 0; i < skin.joints.size(); i++)
        {
            auto & joint = mNodes[skin.joints[i]];
            if (joint.isCachedGlobalTransformChanged)
            {
                auto const nodeMatrix = joint.cachedGlobalTransform;
                jointMatrices[i].model = nodeMatrix * skin.inverseBindMatrices[i];  // T - S = changes
                joint.isCachedGlobalTransformChanged = false;
                mIsSkinJointsChanged = true;
            }
        }
    }

    //-------------------------------------------------------------------------------------------------

    void DrawableVariant::drawNode(
        RT::CommandRecordState const & drawPass,
        Node const & node,
        BindDescriptorSetFunction const & bindFunction,
        AlphaMode alphaMode
    )
    {
        // TODO We can reduce nodes count for better performance when importing
        // Question: Why can't we just render sub-meshes ?
        if (node.meshNode->hasSubMesh())
        {
            MFA_ASSERT(static_cast<int>(mMesh.GetSubMeshCount()) > node.meshNode->subMeshIndex);
            drawSubMesh(
                drawPass,
                mMesh.GetSubMeshByIndex(node.meshNode->subMeshIndex),
                node,
                bindFunction,
                alphaMode
            );
        }
    }

    //-------------------------------------------------------------------------------------------------

    void DrawableVariant::drawSubMesh(
        RT::CommandRecordState const & drawPass,
        AssetSystem::Mesh::SubMesh const & subMesh,
        Node const & node,
        BindDescriptorSetFunction const & bindFunction,
        AlphaMode const alphaMode
    )
    {
        auto const & primitives = subMesh.findPrimitives(alphaMode);
        if (primitives.empty() == false)
        {
            for (auto const * primitive : primitives)
            {
                bindFunction(*primitive, node);
                RF::DrawIndexed(
                    drawPass,
                    primitive->indicesCount,
                    1,
                    primitive->indicesStartingIndex
                );
            }
        }
    }

    //-------------------------------------------------------------------------------------------------

    glm::mat4 DrawableVariant::computeNodeLocalTransform(Node const & node) const
    {
        glm::mat4 currentTransform{ 1 };
        auto translate = node.currentTranslate;
        auto rotation = node.currentRotation;
        auto scale = node.currentScale;
        if (mAnimationRemainingTransitionDurationInSec > 0 && mPreviousAnimationIndex >= 0)
        {
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

    void DrawableVariant::computeNodeGlobalTransform(Node & node, Node const * parentNode, bool const isParentTransformChanged)
    {
        MFA_ASSERT(parentNode == nullptr || parentNode->isCachedDataValid == true);
        MFA_ASSERT(parentNode != nullptr || isParentTransformChanged == false);

        bool isChanged = false;

        if (node.isCachedDataValid == false)
        {
            node.cachedLocalTransform = computeNodeLocalTransform(node);
        }

        if (isParentTransformChanged == true || node.isCachedDataValid == false)
        {
            if (parentNode == nullptr)
            {
                node.cachedGlobalTransform = node.cachedLocalTransform;
            }
            else
            {
                node.cachedGlobalTransform = parentNode->cachedGlobalTransform * node.cachedLocalTransform;
            }
            isChanged = true;
            node.isCachedGlobalTransformChanged = true;
        }

        if ((isChanged || mIsModelTransformChanged) && node.meshNode->hasSubMesh())
        {
            if (auto const transformComponentPtr = mTransformComponent.lock())
            {
                node.cachedModelTransform = transformComponentPtr->GetTransform() * node.cachedGlobalTransform;
            }
        }
        if (isChanged && node.meshNode->hasSubMesh() && node.meshNode->skin > -1)
        {
            node.cachedGlobalInverseTransform = glm::inverse(node.cachedGlobalTransform);
        }

        node.isCachedDataValid = true;

        for (auto const childIndex : node.meshNode->children)
        {
            auto & childNode = mNodes[childIndex];
            computeNodeGlobalTransform(childNode, &node, isChanged);
        }
    }

    //-------------------------------------------------------------------------------------------------

    void DrawableVariant::OnUI()
    {
        std::vector<char const *> animationsList{ mMesh.GetAnimationsCount() };
        for (size_t i = 0; i < animationsList.size(); ++i)
        {
            animationsList[i] = mMesh.GetAnimationByIndex(static_cast<uint32_t>(i)).name.c_str();
        }

        UI::Combo(
            "Active animation",
            &mUISelectedAnimationIndex,
            animationsList.data(),
            static_cast<int32_t>(animationsList.size())
        );
        SetActiveAnimationIndex(mUISelectedAnimationIndex);
    }

    //-------------------------------------------------------------------------------------------------

    DrawableEssence const * DrawableVariant::GetEssence() const noexcept
    {
        return mEssence;
    }

    //-------------------------------------------------------------------------------------------------

    RT::DrawableVariantId DrawableVariant::GetId() const noexcept
    {
        return mId;
    }

    //-------------------------------------------------------------------------------------------------

    RT::DescriptorSetGroup const & DrawableVariant::CreateDescriptorSetGroup(
        VkDescriptorPool descriptorPool,
        uint32_t const descriptorSetCount,
        VkDescriptorSetLayout descriptorSetLayout
    )
    {
        MFA_ASSERT(mDescriptorSetGroup.IsValid() == false);
        mDescriptorSetGroup = RF::CreateDescriptorSets(
            descriptorPool,
            descriptorSetCount,
            descriptorSetLayout
        );
        MFA_ASSERT(mDescriptorSetGroup.IsValid() == true);
        return mDescriptorSetGroup;
    }

    //-------------------------------------------------------------------------------------------------

    RT::DescriptorSetGroup const & DrawableVariant::GetDescriptorSetGroup() const
    {
        MFA_ASSERT(mDescriptorSetGroup.IsValid() == true);
        return mDescriptorSetGroup;
    }

    //-------------------------------------------------------------------------------------------------

    bool DrawableVariant::IsActive() const noexcept
    {
        if (auto const ptr = mRendererComponent.lock())
        {
            return ptr->IsActive();
        }
        return false;
    }

    //-------------------------------------------------------------------------------------------------

}
