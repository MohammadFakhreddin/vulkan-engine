#include "PBR_Variant.hpp"

#include "engine/render_system/RenderTypes.hpp"
#include "engine/BedrockMemory.hpp"
#include "engine/render_system/RenderFrontend.hpp"
#include "engine/ui_system/UI_System.hpp"
#include "engine/BedrockAssert.hpp"
#include "engine/BedrockMatrix.hpp"
#include "engine/entity_system/Entity.hpp"
#include "engine/entity_system/components/TransformComponent.hpp"
#include "PBR_Essence.hpp"
#include "engine/render_system/pipelines/DescriptorSetSchema.hpp"

#include <glm/gtx/quaternion.hpp>

#include <string>

namespace MFA
{
    // TODO: We have to separate PBR and animation
    // TODO: We need an animator component
    using namespace AS::PBR;
    //-------------------------------------------------------------------------------------------------

    PBR_Variant::PBR_Variant(PBR_Essence const * essence)
        : VariantBase(essence)
        , mPBR_Essence(essence)
        , mMeshData(mPBR_Essence->getMeshData())
    {
        MFA_ASSERT(mMeshData->isValid());

        // Skins
        mSkins.resize(mMeshData->skins.size());

        {// Nodes
            mNodes.resize(mMeshData->nodes.size());
            for (int i = 0; i < static_cast<int>(mNodes.size()); ++i)
            {
                auto & meshNode = mMeshData->nodes[i];
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

                Copy<16>(node.currentTransform, meshNode.transform);
                //Copy<16>(node.previousTransform, meshNode.transform);
            }
        }

        prepareSkinJointsBuffer();
        prepareSkinnedVerticesBuffer(essence->getVertexCount());
    }

    //-------------------------------------------------------------------------------------------------

    PBR_Variant::~PBR_Variant() = default;

    //-------------------------------------------------------------------------------------------------

    void PBR_Variant::updateBuffers(RT::CommandRecordState const & recordState)
    {
        if (IsActive() == false)
        {
            return;
        }

        if (mBufferDirtyCounter > 0)
        {
            RF::UpdateHostVisibleBuffer(
                *mSkinsJointsBuffer->buffers[recordState.frameIndex],
                mCachedSkinsJointsBlob->memory
            );
            mBufferDirtyCounter -= 1;
        }
    }

    //-------------------------------------------------------------------------------------------------

    void PBR_Variant::compute(
        RT::CommandRecordState const & recordState,
        BindDescriptorSetFunction const & bindFunction
    ) const
    {
        bindComputeDescriptorSet(recordState);

        for (auto & node : mNodes)
        {
            computeNode(recordState, node, bindFunction);
        }
    }

    //-------------------------------------------------------------------------------------------------

    void PBR_Variant::render(
        RT::CommandRecordState const & recordState,
        BindDescriptorSetFunction const & bindFunction,
        AS::AlphaMode const alphaMode
    )
    {
        bindSkinnedVerticesBuffer(recordState);
        for (auto & node : mNodes)
        {
            drawNode(recordState, node, bindFunction, alphaMode);
        }
    }

    //-------------------------------------------------------------------------------------------------

    void PBR_Variant::postRender(float const deltaTimeInSec)
    {
        if (IsActive() == false)
        {
            return;
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
            mBufferDirtyCounter = 2;

            mIsSkinJointsChanged = false;
        }
    }

    //-------------------------------------------------------------------------------------------------

    void PBR_Variant::preComputeBarrier(RT::CommandRecordState const & recordState, std::vector<VkBufferMemoryBarrier> & outBarriers) const
    {
        auto & bufferAndMemory = mSkinnedVerticesBuffer->buffers[recordState.frameIndex];
        
        VkBufferMemoryBarrier barrier =
        {
            VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
            nullptr,
            0,
            VK_ACCESS_SHADER_WRITE_BIT,
            RF::GetGraphicQueueFamily(),
            RF::GetComputeQueueFamily(),
            bufferAndMemory->buffer,
            0,
            bufferAndMemory->size
        };

        outBarriers.emplace_back(barrier);
    }

    //-------------------------------------------------------------------------------------------------

    void PBR_Variant::preRenderBarrier(RT::CommandRecordState const & recordState, std::vector<VkBufferMemoryBarrier> & outBarriers) const
    {
        auto & bufferAndMemory = mSkinnedVerticesBuffer->buffers[recordState.frameIndex];
        
        VkBufferMemoryBarrier barrier =
        {
            VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
            nullptr,
            VK_ACCESS_SHADER_READ_BIT,
            0,
            RF::GetComputeQueueFamily(),
            RF::GetGraphicQueueFamily(),
            bufferAndMemory->buffer,
            0,
            bufferAndMemory->size
        };
        outBarriers.emplace_back(barrier);
    }

    //-------------------------------------------------------------------------------------------------

    bool PBR_Variant::IsCurrentAnimationFinished() const
    {
        return mIsAnimationFinished;
    }
    
    //-------------------------------------------------------------------------------------------------

    int PBR_Variant::GetActiveAnimationIndex() const noexcept
    {
        return mActiveAnimationIndex;
    }

    //-------------------------------------------------------------------------------------------------

    void PBR_Variant::SetActiveAnimationIndex(int const nextAnimationIndex, AnimationParams const & params)
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
        mActiveAnimationTimeInSec = params.startTimeOffsetInSec + mMeshData->animations[mActiveAnimationIndex].startTime;
        mActiveAnimationParams = params;

        memset(mAnimationInputIndex, 0, sizeof(mAnimationInputIndex));
    }

    //-------------------------------------------------------------------------------------------------

    void PBR_Variant::SetActiveAnimation(char const * animationName, AnimationParams const & params)
    {
        auto const index = mPBR_Essence->getAnimationIndex(animationName);
        if (MFA_VERIFY(index >= 0))
        {
            SetActiveAnimationIndex(index, params);
        }
    }

    //-------------------------------------------------------------------------------------------------

    // Idea: Maybe we can pre-compute the frame values instead
    void PBR_Variant::updateAnimation(float const deltaTimeInSec, bool isVisible)
    {
        if (mMeshData->animations.empty())
        {
            return;
        }

        glm::vec3 posPrev {};
        glm::vec3 posNext {};

        glm::quat rotPrev {};
        glm::quat rotNext {};

        glm::vec3 scalePrev {};
        glm::vec3 scaleNext {};

        {// Active animation
            auto const & activeAnimation = mMeshData->animations[mActiveAnimationIndex];

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
                                Copy<3>(posPrev, previousOutput);
                                Copy<3>(posNext, nextOutput);
                                node.currentTranslate = glm::mix(posPrev, posNext, fraction);
                            }
                            else if (channel.path == Animation::Path::Rotation)
                            {
                                Copy<4>(rotPrev, previousOutput);
                                Copy<4>(rotNext, nextOutput);

                                node.currentRotation = glm::slerp(rotPrev, rotNext, fraction);
                            }
                            else if (channel.path == Animation::Path::Scale)
                            {
                                Copy<3>(scalePrev, previousOutput);
                                Copy<3>(scaleNext, nextOutput);

                                node.currentScale = glm::mix(scalePrev, scaleNext, fraction);
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

            auto const & previousAnimation = mMeshData->animations[mPreviousAnimationIndex];

            for (auto const & channel : previousAnimation.channels)
            {
                auto const & sampler = previousAnimation.samplers[channel.samplerIndex];
                auto & node = mNodes[channel.nodeIndex];

                for (size_t i = 0; i < sampler.inputAndOutput.size() - 1; i++)
                {
                    if (sampler.interpolation != Animation::Interpolation::Linear)
                    {
                        MFA_NOT_IMPLEMENTED_YET("Mohammad Fakhreddin");
                        continue;
                    }

                    auto const previousInput = sampler.inputAndOutput[i].input;
                    auto & previousOutput = sampler.inputAndOutput[i].output;
                    auto const nextInput = sampler.inputAndOutput[i + 1].input;
                    auto & nextOutput = sampler.inputAndOutput[i + 1].output;
                    // Get the input keyframe values for the current time stamp
                    if (mPreviousAnimationTimeInSec >= previousInput && mPreviousAnimationTimeInSec <= nextInput)
                    {
                        float const fraction = (mPreviousAnimationTimeInSec - previousInput) / (nextInput - previousInput);

                        if (channel.path == Animation::Path::Translation)
                        {
                            Copy<3>(posPrev, previousOutput);
                            Copy<3>(posNext, nextOutput);
                            
                            node.previousTranslate = glm::mix(posPrev, posNext, fraction);
                        }
                        else if (channel.path == Animation::Path::Rotation)
                        {
                            Copy<4>(rotPrev, previousOutput);
                            Copy<4>(rotNext, nextOutput);

                            node.previousRotation = glm::slerp(rotPrev, rotNext, fraction);
                        }
                        else if (channel.path == Animation::Path::Scale)
                        {
                            Copy<3>(scalePrev, previousOutput);
                            Copy<3>(scaleNext, nextOutput);

                            node.previousScale = glm::mix(scalePrev, scaleNext, fraction);
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

    void PBR_Variant::computeNodesGlobalTransform()
    {
        // TODO: For root nodes we should apply root motion to transform instead if possible (But which one is the root transform ?)
        for (auto const & rootNodeIndex : mMeshData->rootNodes)
        {
            auto & node = mNodes[rootNodeIndex];
            computeNodeGlobalTransform(node, nullptr, false);
        }
    }

    //-------------------------------------------------------------------------------------------------

    void PBR_Variant::updateAllSkinsJoints()
    {
        uint32_t index = 0;
        for (auto const & skin : mMeshData->skins)
        {
            updateSkinJoints(index, skin);
            ++index;
        }
    }

    //-------------------------------------------------------------------------------------------------

    void PBR_Variant::updateSkinJoints(uint32_t const skinIndex, AS::PBR::Skin const & skin)
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

    void PBR_Variant::drawNode(
        RT::CommandRecordState const & recordState,
        Node const & node,
        BindDescriptorSetFunction const & bindFunction,
        AS::AlphaMode alphaMode
    )
    {
        // TODO We can reduce nodes count for better performance when importing
        // Question: Why can't we just render sub-meshes ?
        if (node.meshNode->hasSubMesh())
        {
            MFA_ASSERT(static_cast<int>(mMeshData->subMeshes.size()) > node.meshNode->subMeshIndex);
            drawSubMesh(
                recordState,
                mMeshData->subMeshes[node.meshNode->subMeshIndex],
                node,
                bindFunction,
                alphaMode
            );
        }
    }

    //-------------------------------------------------------------------------------------------------

    void PBR_Variant::drawSubMesh(
        RT::CommandRecordState const & recordState,
        SubMesh const & subMesh,
        Node const & node,
        BindDescriptorSetFunction const & bindFunction,
        AS::AlphaMode const alphaMode
    )
    {
        auto const & primitives = subMesh.findPrimitives(alphaMode);
        for (auto const * primitive : primitives)
        {
            if (bindFunction != nullptr)
            {
                bindFunction(*primitive, node);
            }
            RF::DrawIndexed(
                recordState,
                primitive->indicesCount,
                1,
                primitive->indicesStartingIndex
            );
        }
    }
    
    //-------------------------------------------------------------------------------------------------

    void PBR_Variant::computeNode(
        RT::CommandRecordState const & recordState,
        Node const & node,
        BindDescriptorSetFunction const & bindFunction
    ) const
    {
        // TODO We can reduce nodes count for better performance when importing
        // Question: Why can't we just render sub-meshes ?
        if (node.meshNode->hasSubMesh())
        {
            MFA_ASSERT(static_cast<int>(mMeshData->subMeshes.size()) > node.meshNode->subMeshIndex);
            computeSubMesh(
                recordState,
                mMeshData->subMeshes[node.meshNode->subMeshIndex],
                node,
                bindFunction
            );
        }
    }

    //-------------------------------------------------------------------------------------------------

    void PBR_Variant::computeSubMesh(
        RT::CommandRecordState const & recordState,
        SubMesh const & subMesh,
        Node const & node,
        BindDescriptorSetFunction const & bindFunction
    ) const
    {
        for (auto const & primitive : subMesh.primitives)
        {
            if (bindFunction != nullptr)
            {
                bindFunction(primitive, node);
            }
            RF::Dispatch(
                recordState,
                static_cast<uint32_t>(std::ceil(static_cast<float>(primitive.vertexCount) / 256.0f)),
                1,
                1
            );
        }
    }

    //-------------------------------------------------------------------------------------------------

    glm::mat4 PBR_Variant::computeNodeLocalTransform(Node const & node) const
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

    void PBR_Variant::computeNodeGlobalTransform(Node & node, Node const * parentNode, bool const isParentTransformChanged)
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
                node.cachedModelTransform = transformComponentPtr->GetWorldTransform() * node.cachedGlobalTransform;
            }
        }
        if (isChanged && node.meshNode->hasSubMesh() && node.meshNode->skin > -1)
        {
            // TODO Knowing that there is no scale we might be able to compute this cheaper
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

    void PBR_Variant::prepareSkinJointsBuffer()
    {
        // Creating cachedSkinJoints array
        {
            uint32_t totalJointsCount = 0;
            for (auto const & skin : mMeshData->skins)
            {
                auto const jointsCount = static_cast<uint32_t>(skin.joints.size());
                totalJointsCount += jointsCount;
            }
            if (totalJointsCount > 0)
            {
                mCachedSkinsJointsBlob = Memory::Alloc(sizeof(JointTransformData) * totalJointsCount);
                mSkinsJointsBuffer = RF::CreateHostVisibleUniformBuffer(
                    mCachedSkinsJointsBlob->memory.len,
                    RF::GetMaxFramesPerFlight()
                );
            }
        }
        {
            mCachedSkinsJoints.resize(mMeshData->skins.size());
            uint32_t startingJointIndex = 0;
            for (uint32_t i = 0; i < mMeshData->skins.size(); ++i)
            {
                auto & skin = mMeshData->skins[i];
                auto const jointsCount = static_cast<uint32_t>(skin.joints.size());
                mCachedSkinsJoints[i] = reinterpret_cast<JointTransformData *>(mCachedSkinsJointsBlob->memory.ptr + startingJointIndex * sizeof(JointTransformData));
                mSkins[i].skinStartingIndex = static_cast<int>(startingJointIndex);
                startingJointIndex += jointsCount;
            }
        }
    }

    //-------------------------------------------------------------------------------------------------

    void PBR_Variant::prepareSkinnedVerticesBuffer(uint32_t const vertexCount)
    {
        auto const bufferSize = sizeof(SkinnedVertex) * vertexCount;
        mSkinnedVerticesBuffer = RF::CreateBufferGroup(
            bufferSize,
            RF::GetMaxFramesPerFlight(),
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
        );
    }

    //-------------------------------------------------------------------------------------------------

    void PBR_Variant::bindSkinnedVerticesBuffer(RT::CommandRecordState const & recordState) const
    {
        RF::BindVertexBuffer(
            recordState,
            *mSkinnedVerticesBuffer->buffers[recordState.frameIndex],
            0
        );
    }
    
    //-------------------------------------------------------------------------------------------------

    void PBR_Variant::bindComputeDescriptorSet(RT::CommandRecordState const & recordState) const
    {
        RF::BindDescriptorSet(
            recordState,
            RenderFrontend::UpdateFrequency::PerEssence,
            mComputeDescriptorSet.descriptorSets[recordState.frameIndex]
        );
    }

    //-------------------------------------------------------------------------------------------------

    void PBR_Variant::createComputeDescriptorSet(
        VkDescriptorPool descriptorPool,
        VkDescriptorSetLayout descriptorSetLayout,
        RT::BufferGroup const & errorBuffer 
    )
    {
        mComputeDescriptorSet = RF::CreateDescriptorSets(
            descriptorPool,
            RF::GetMaxFramesPerFlight(),
            descriptorSetLayout
        );
        
        for (uint32_t frameIndex = 0; frameIndex < RF::GetMaxFramesPerFlight(); ++frameIndex)
        {
            auto const descriptorSet = mComputeDescriptorSet.descriptorSets[frameIndex];
            MFA_ASSERT(descriptorSet != VK_NULL_HANDLE);

            DescriptorSetSchema descriptorSetSchema{ descriptorSet };

            /////////////////////////////////////////////////////////////////
            // Vertex shader
            /////////////////////////////////////////////////////////////////

            // SkinJoints
            VkBuffer skinJointsBuffer = errorBuffer.buffers[0]->buffer;
            size_t skinJointsBufferSize = errorBuffer.bufferSize;
            if (mSkinsJointsBuffer != nullptr && mSkinsJointsBuffer->bufferSize > 0)
            {
                skinJointsBuffer = mSkinsJointsBuffer->buffers[frameIndex]->buffer;
                skinJointsBufferSize = mSkinsJointsBuffer->bufferSize;
            }
            VkDescriptorBufferInfo skinTransformBufferInfo{
                .buffer = skinJointsBuffer,
                .offset = 0,
                .range = skinJointsBufferSize,
            };
            descriptorSetSchema.AddUniformBuffer(&skinTransformBufferInfo);

            // SkinnedVertices
            VkDescriptorBufferInfo skinnedVerticesBufferInfo {
                .buffer = mSkinnedVerticesBuffer->buffers[frameIndex]->buffer,
                .offset = 0,
                .range = mSkinnedVerticesBuffer->bufferSize
            };
            descriptorSetSchema.AddStorageBuffer(&skinnedVerticesBufferInfo);

            descriptorSetSchema.UpdateDescriptorSets();
        }
    }

    //-------------------------------------------------------------------------------------------------

    void PBR_Variant::OnUI()
    {
        VariantBase::OnUI();

        std::vector<char const *> animationsList{ mMeshData->animations.size() };
        for (uint32_t i = 0; i < static_cast<uint32_t>(animationsList.size()); ++i)
        {
            animationsList[i] = mMeshData->animations[i].name.c_str();
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

}
