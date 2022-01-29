#pragma once

#include "engine/render_system/RenderTypes.hpp"
#include "engine/BedrockMemory.hpp"
#include "engine/asset_system/AssetTypes.hpp"
#include "engine/asset_system/Asset_PBR_Mesh.hpp"
#include "engine/render_system/pipelines/VariantBase.hpp"

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

        explicit PBR_Variant(PBR_Essence const * essence);
        ~PBR_Variant() override;

        PBR_Variant(PBR_Variant const &) noexcept = delete;
        PBR_Variant(PBR_Variant &&) noexcept = delete;
        PBR_Variant & operator= (PBR_Variant const & rhs) noexcept = delete;
        PBR_Variant & operator= (PBR_Variant && rhs) noexcept = delete;

        [[nodiscard]]
        int GetActiveAnimationIndex() const noexcept;

        void SetActiveAnimationIndex(int nextAnimationIndex, AnimationParams const & params = AnimationParams{});

        void SetActiveAnimation(char const * animationName, AnimationParams const & params = AnimationParams{});

        void Update(float deltaTimeInSec, RT::CommandRecordState const & drawPass) override;

        using BindDescriptorSetFunction = std::function<void(AS::PBR::Primitive const & primitive, Node const & node)>;
        void Draw(
            RT::CommandRecordState const & drawPass,
            BindDescriptorSetFunction const & bindFunction,
            AS::AlphaMode alphaMode
        );
        
        [[nodiscard]]
        RT::UniformBufferGroup const * GetSkinJointsBuffer() const noexcept;

        void OnUI() override;

        [[nodiscard]]
        bool IsCurrentAnimationFinished() const;

    private:

        void updateAnimation(float deltaTimeInSec, bool isVisible);

        void computeNodesGlobalTransform();

        void updateAllSkinsJoints();

        void updateSkinJoints(uint32_t skinIndex, AS::PBR::Skin const & skin);

        void drawNode(
            RT::CommandRecordState const & drawPass,
            Node const & node,
            BindDescriptorSetFunction const & bindFunction,
            AS::AlphaMode alphaMode
        );

        void drawSubMesh(
            RT::CommandRecordState const & drawPass,
            AS::PBR::SubMesh const & subMesh,
            Node const & node,
            BindDescriptorSetFunction const & bindFunction,
            AS::AlphaMode alphaMode
        );

        [[nodiscard]]
        glm::mat4 computeNodeLocalTransform(Node const & node) const;

        void computeNodeGlobalTransform(
            Node & node,
            Node const * parentNode,
            bool isParentTransformChanged
        );

    private:

        PBR_Essence const * mPBR_Essence = nullptr;
        AS::PBR::MeshData const * mMeshData = nullptr;

        std::shared_ptr<RT::UniformBufferGroup> mSkinsJointsBuffer{};

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

    };

};
