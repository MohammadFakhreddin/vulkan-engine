#pragma once

#include "engine/render_system/RenderTypes.hpp"
#include "engine/FoundationAsset.hpp"
#include "engine/render_system/variant/Variant.hpp"

#include <glm/gtc/quaternion.hpp>

#include <functional>
#include <memory>

namespace MFA
{

    class Component;
    class Entity;
    class BoundingVolumeComponent;
    class TransformComponent;
    class DrawableEssence;
    class RendererComponent;

    struct AnimationParams
    {
        float transitionDuration = 0.3f;
        bool loop = true;
        float startTimeOffsetInSec = 0.0f;
    };

    class DrawableVariant final : public Variant
    {
    public:

        using AlphaMode = AS::AlphaMode;

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
            AS::MeshNode const * meshNode = nullptr;

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

        explicit DrawableVariant(DrawableEssence const * essence);
        ~DrawableVariant() override;

        DrawableVariant(DrawableVariant const &) noexcept = delete;
        DrawableVariant(DrawableVariant &&) noexcept = delete;
        DrawableVariant & operator= (DrawableVariant const & rhs) noexcept = delete;
        DrawableVariant & operator= (DrawableVariant && rhs) noexcept = delete;

        [[nodiscard]]
        int GetActiveAnimationIndex() const noexcept;

        void SetActiveAnimationIndex(int nextAnimationIndex, AnimationParams const & params = AnimationParams{});

        void SetActiveAnimation(char const * animationName, AnimationParams const & params = AnimationParams{});

        void Update(float deltaTimeInSec, RT::CommandRecordState const & drawPass) override;

        using BindDescriptorSetFunction = std::function<void(AS::MeshPrimitive const & primitive, Node const & node)>;
        void Draw(
            RT::CommandRecordState const & drawPass,
            BindDescriptorSetFunction const & bindFunction,
            AlphaMode alphaMode
        );
        
        [[nodiscard]]
        RT::UniformBufferGroup const * GetSkinJointsBuffer() const noexcept;

        void OnUI();

        [[nodiscard]]
        bool IsCurrentAnimationFinished() const;

    private:

        void updateAnimation(float deltaTimeInSec, bool isVisible);

        void computeNodesGlobalTransform();

        void updateAllSkinsJoints();

        void updateSkinJoints(uint32_t skinIndex, AS::MeshSkin const & skin);

        void drawNode(
            RT::CommandRecordState const & drawPass,
            Node const & node,
            BindDescriptorSetFunction const & bindFunction,
            AlphaMode alphaMode
        );

        void drawSubMesh(
            RT::CommandRecordState const & drawPass,
            AS::Mesh::SubMesh const & subMesh,
            Node const & node,
            BindDescriptorSetFunction const & bindFunction,
            AlphaMode alphaMode
        );

        [[nodiscard]]
        glm::mat4 computeNodeLocalTransform(Node const & node) const;

        void computeNodeGlobalTransform(
            Node & node,
            Node const * parentNode,
            bool isParentTransformChanged
        );

    private:

        DrawableEssence const * mDrawableEssence = nullptr;
        AS::Mesh const * mMesh = nullptr;

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
