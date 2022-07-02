#pragma once

#include "engine/render_system/RenderTypes.hpp"
#include "engine/BedrockMemory.hpp"
#include "engine/asset_system/AssetTypes.hpp"
#include "engine/asset_system/Asset_PBR_Mesh.hpp"
#include "engine/render_system/pipelines/VariantBase.hpp"
#include "engine/render_system/RenderTypesFWD.hpp"

#include <glm/gtc/quaternion.hpp>
#include <glm/vec3.hpp>

#include <functional>
#include <memory>

namespace MFA
{

    class Component;
    class Entity;
    class BoundingVolumeComponent;
    class TransformComponent;
    class PBR_Essence;
    class RendererComponent;

    struct AnimationParams
    {
        float transitionDuration = 0.3f;
        bool loop = true;
        float startTimeOffsetInSec = 0.0f;
    };

    class PBR_Variant final : public VariantBase
    {
    public:

        struct SkinnedVertex
        {
            float worldPosition[4];
        
            float worldNormal[3];
            float placeholder1;

            float worldTangent[3];
            float placeholder2;

            float worldBiTangent[3];
            float placeholder3;
        };

        struct JointTransformData
        {
            glm::mat4 model;
        };

        struct Skin
        {
            int skinStartingIndex;
        };

        struct Node
        {
            AS::PBR::Node const * meshNode = nullptr;

            glm::quat currentRotation{};     // x, y, z, w
            glm::vec3 currentScale{};
            glm::vec3 currentTranslate{};
            glm::mat4 currentTransform{};

            glm::quat previousRotation{};     // x, y, z, w
            glm::vec3 previousScale{};
            glm::vec3 previousTranslate{};
            glm::mat4 previousTransform{};

            bool isCachedDataValid = false;
            bool isCachedGlobalTransformChanged = false;
            glm::mat4 cachedLocalTransform{};
            glm::mat4 cachedGlobalTransform{};
            glm::mat4 cachedModelTransform{};
            glm::mat4 cachedGlobalInverseTransform{};

            Skin * skin = nullptr;
        };

        using BindDescriptorSetFunction = std::function<void(AS::PBR::Primitive const & primitive, Node const & node)>;

        explicit PBR_Variant(PBR_Essence const * essence);
        ~PBR_Variant() override;

        PBR_Variant(PBR_Variant const &) noexcept = delete;
        PBR_Variant(PBR_Variant &&) noexcept = delete;
        PBR_Variant & operator= (PBR_Variant const & rhs) noexcept = delete;
        PBR_Variant & operator= (PBR_Variant && rhs) noexcept = delete;

        void createComputeDescriptorSet(
            VkDescriptorPool descriptorPool,
            VkDescriptorSetLayout descriptorSetLayout,
            RT::BufferGroup const & errorBuffer
        );

        [[nodiscard]]
        int GetActiveAnimationIndex() const noexcept;

        void SetActiveAnimationIndex(int nextAnimationIndex, AnimationParams const & params = AnimationParams{});

        void SetActiveAnimation(char const * animationName, AnimationParams const & params = AnimationParams{});

        void updateBuffers(RT::CommandRecordState const & recordState);

        void compute(
            RT::CommandRecordState const & recordState,
            BindDescriptorSetFunction const & bindFunction
        ) const;

        void render(
            RT::CommandRecordState const & recordState,
            BindDescriptorSetFunction const & bindFunction,
            AS::AlphaMode alphaMode
        );

        void postRender(float deltaTimeInSec);

        void preComputeBarrier(RT::CommandRecordState const & recordState, std::vector<VkBufferMemoryBarrier> & outBarriers) const;

        void preRenderBarrier(RT::CommandRecordState const & recordState, std::vector<VkBufferMemoryBarrier> & outBarriers) const;

        void OnUI() override;

        [[nodiscard]]
        bool IsCurrentAnimationFinished() const;

    private:

        void updateAnimation(float deltaTimeInSec, bool isVisible);

        void computeNodesGlobalTransform();

        void updateAllSkinsJoints();

        void updateSkinJoints(uint32_t skinIndex, AS::PBR::Skin const & skin);

        void drawNode(
            RT::CommandRecordState const & recordState,
            Node const & node,
            BindDescriptorSetFunction const & bindFunction,
            AS::AlphaMode alphaMode
        );

        void drawSubMesh(
            RT::CommandRecordState const & recordState,
            AS::PBR::SubMesh const & subMesh,
            Node const & node,
            BindDescriptorSetFunction const & bindFunction,
            AS::AlphaMode alphaMode
        );

        void computeNode(
            RT::CommandRecordState const & recordState,
            Node const & node,
            BindDescriptorSetFunction const & bindFunction
        ) const;

        void computeSubMesh(
            RT::CommandRecordState const & recordState,
            AS::PBR::SubMesh const & subMesh,
            Node const & node,
            BindDescriptorSetFunction const & bindFunction
        ) const;

        [[nodiscard]]
        glm::mat4 computeNodeLocalTransform(Node const & node) const;

        void computeNodeGlobalTransform(
            Node & node,
            Node const * parentNode,
            bool isParentTransformChanged
        );

        void prepareSkinJointsBuffer();

        void prepareSkinnedVerticesBuffer(uint32_t vertexCount);

        void bindSkinnedVerticesBuffer(RT::CommandRecordState const & recordState) const;

        void bindComputeDescriptorSet(RT::CommandRecordState const & recordState) const;

    private:

        PBR_Essence const * mPBR_Essence = nullptr;
        AS::PBR::MeshData const * mMeshData = nullptr;

        int mActiveAnimationIndex = 0;
        int mPreviousAnimationIndex = -1;
        float mActiveAnimationTimeInSec = 0.0f;
        float mPreviousAnimationTimeInSec = 0.0f;
        float mAnimationTransitionDurationInSec = 0.0f;
        float mAnimationRemainingTransitionDurationInSec = 0.0f;

        int mUISelectedAnimationIndex = 0;

        std::shared_ptr<SmartBlob> mCachedSkinsJointsBlob{};
        std::vector<JointTransformData *> mCachedSkinsJoints{};

        std::vector<Skin> mSkins{};
        std::vector<Node> mNodes{};
        
        AnimationParams mActiveAnimationParams{};

        bool mIsAnimationFinished = false;

        bool mIsSkinJointsChanged = true;

        int mBufferDirtyCounter = 0;

        size_t mAnimationInputIndex[300]{};

        RT::DescriptorSetGroup mComputeDescriptorSet {};

        std::shared_ptr<RT::BufferGroup> mSkinsJointsBuffer{};

        std::shared_ptr<RT::BufferGroup> mSkinnedVerticesBuffer {};

    };

};
