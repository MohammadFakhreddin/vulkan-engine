#pragma once

#include "engine/render_system/RenderTypes.hpp"
#include "engine/BedrockCommon.hpp"
#include "engine/FoundationAsset.hpp"

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

namespace MFA {

class Component;
class Entity;
class BoundingVolumeComponent;
class TransformComponent;
class DrawableEssence;


struct AnimationParams {
    float transitionDuration = 0.3f;
    bool loop = true;
    float startTimeOffsetInSec = 0.0f;
};

// TODO: We need bounding sphere to check for visibility

class DrawableVariant {
public:
    
    struct JointTransformData {
        glm::mat4 model;
    };
    
    struct Skin {
        int skinStartingIndex;
    };

    struct Node {
        AS::MeshNode const * meshNode = nullptr;

        glm::quat currentRotation {};     // x, y, z, w
        glm::vec3 currentScale {};
        glm::vec3 currentTranslate {};
        glm::mat4 currentTransform {};
        
        glm::quat previousRotation {};     // x, y, z, w
        glm::vec3 previousScale {};
        glm::vec3 previousTranslate {};
        glm::mat4 previousTransform {};

        bool isCachedDataValid = false;
        bool isCachedGlobalTransformChanged = false;
        glm::mat4 cachedLocalTransform {};
        glm::mat4 cachedGlobalTransform {};
        glm::mat4 cachedModelTransform {};
        glm::mat4 cachedGlobalInverseTransform {};

        Skin * skin = nullptr;
    };
    
    explicit DrawableVariant(DrawableEssence const & essence);

    ~DrawableVariant();

    DrawableVariant (DrawableVariant const &) noexcept = delete;
    DrawableVariant (DrawableVariant &&) noexcept = delete;
    DrawableVariant & operator= (DrawableVariant const & rhs) noexcept = delete;
    DrawableVariant & operator= (DrawableVariant && rhs) noexcept = delete;

    bool operator== (DrawableVariant const & rhs) const noexcept
    {
        return mId == rhs.mId;
    }
    
    [[nodiscard]]
    int GetActiveAnimationIndex() const noexcept {
        return mActiveAnimationIndex;
    }

    void SetActiveAnimationIndex(int nextAnimationIndex, AnimationParams const & params = AnimationParams {});

    void SetActiveAnimation(char const * animationName, AnimationParams const & params = AnimationParams {});
    
    void Init(Component * rendererComponent, TransformComponent * transformComponent);

    void Update(float deltaTimeInSec, RT::DrawPass const & drawPass);

    void Shutdown();

    using BindDescriptorSetFunction = std::function<void(AS::MeshPrimitive const & primitive, Node const & node)>;
    void Draw(RT::DrawPass const & drawPass, BindDescriptorSetFunction const & bindFunction);
    
    [[nodiscard]]
    RT::UniformBufferGroup const * GetUniformBuffer(char const * name);
    
    [[nodiscard]] 
    RT::UniformBufferGroup const & GetSkinJointsBuffer() const noexcept;

    [[nodiscard]]
    DrawableEssence const * GetEssence() const noexcept;

    [[nodiscard]]
    RT::DrawableVariantId GetId() const noexcept;

    RT::DescriptorSetGroup const & CreateDescriptorSetGroup(
        char const * name, 
        VkDescriptorSetLayout descriptorSetLayout, 
        uint32_t descriptorSetCount
    );

    [[nodiscard]]
    RT::DescriptorSetGroup const * GetDescriptorSetGroup(char const * name);

    [[nodiscard]]
    bool IsActive() const noexcept;

    void OnUI();

    [[nodiscard]]
    bool IsCurrentAnimationFinished() const;

    [[nodiscard]]
    bool IsInFrustum() const;

    [[nodiscard]]
    Entity * GetEntity() const;

private:

    void updateAnimation(float deltaTimeInSec, bool isInFrustum);

    void computeNodesGlobalTransform();

    void updateAllSkinsJoints();

    void updateSkinJoints(uint32_t skinIndex, AS::MeshSkin const & skin);
    
    void drawNode(
        RT::DrawPass const & drawPass,
        Node const & node,
        BindDescriptorSetFunction const & bindFunction
    );

    void drawSubMesh(
        RT::DrawPass const & drawPass,
        AS::Mesh::SubMesh const & subMesh,
        Node const & node,
        BindDescriptorSetFunction const & bindFunction
    );

    [[nodiscard]]
    glm::mat4 computeNodeLocalTransform(Node const & node) const;

    void computeNodeGlobalTransform(
        Node & node,
        Node const * parentNode,
        bool isParentTransformChanged
    );

private:

    static RT::DrawableVariantId NextInstanceId;
    RT::DrawableVariantId mId = 0;

    DrawableEssence const * mEssence = nullptr;
    
    RT::UniformBufferGroup mSkinsJointsBuffer {};
    
    std::unordered_map<std::string, RT::UniformBufferGroup> mUniformBuffers {};

    int mActiveAnimationIndex = 0;
    int mPreviousAnimationIndex = -1;
    float mActiveAnimationTimeInSec = 0.0f;
    float mPreviousAnimationTimeInSec = 0.0f;
    float mAnimationTransitionDurationInSec = 0.0f;
    float mAnimationRemainingTransitionDurationInSec = 0.0f;

    int mUISelectedAnimationIndex = 0;

    Blob mCachedSkinsJointsBlob {};
    std::vector<JointTransformData *> mCachedSkinsJoints {};

    std::vector<Skin> mSkins {};
    std::vector<Node> mNodes {};
    
    std::unordered_map<std::string, RT::DescriptorSetGroup> mDescriptorSetGroups {};

    AnimationParams mActiveAnimationParams {};

    bool mIsAnimationFinished = false;

    bool mIsInitialized = false;

    AS::Mesh const * mMesh = nullptr; 

    Entity * mEntity = nullptr;

    Component * mRendererComponent = nullptr;

    BoundingVolumeComponent * mBoundingVolumeComponent = nullptr;

    bool mIsModelTransformChanged = true;
    TransformComponent * mTransformComponent = nullptr;
    int mTransformListenerId = 0;

    bool mIsSkinJointsChanged = true;

};

};
