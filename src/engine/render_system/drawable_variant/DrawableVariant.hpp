#pragma once

#include "engine/render_system/RenderTypes.hpp"
#include "engine/BedrockCommon.hpp"
#include "engine/FoundationAsset.hpp"

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

#include <functional>
#include <memory>
#include <unordered_map>

namespace MFA {

class DrawableEssence;
class UIRecordObject;

class DrawableVariant {
public:
    
    struct JointTransformData {
        float model[16];
    };
    
    struct Skin {
        int skinStartingIndex;
    };

    struct Node {
        AS::MeshNode const * meshNode = nullptr;

        glm::quat currentRotation;     // x, y, z, w
        glm::vec3 currentScale;
        glm::vec3 currentTranslate;
        glm::mat4 currentTransform;
        
        glm::quat previousRotation;     // x, y, z, w
        glm::vec3 previousScale;
        glm::vec3 previousTranslate;
        glm::mat4 previousTransform;

        bool isCachedDataValid = false;
        glm::mat4 cachedLocalTransform;
        glm::mat4 cachedGlobalTransform;
        glm::mat4 cachedModelTransform;
        glm::mat4 cachedGlobalInverseTransform;

        Skin * skin = nullptr;
    };
    
    explicit DrawableVariant(DrawableEssence const & essence);

    ~DrawableVariant();

    DrawableVariant (DrawableVariant const &) noexcept = delete;
    DrawableVariant (DrawableVariant &&) noexcept = delete;
    DrawableVariant & operator = (DrawableVariant const &) noexcept = delete;
    DrawableVariant & operator= (DrawableVariant && rhs) noexcept {
        this->mCachedSkinsJointsBlob = rhs.mCachedSkinsJointsBlob;
        this->mCachedSkinsJoints = rhs.mCachedSkinsJoints;
        this->mSkinsJointsBuffer = std::move(rhs.mSkinsJointsBuffer);
        this->mUniformBuffers = std::move(rhs.mUniformBuffers);
        return *this;
    }
    
    void UpdateModelTransform(float modelTransform[16]);

    [[nodiscard]]
    int GetActiveAnimationIndex() const noexcept {
        return mActiveAnimationIndex;
    }

    void SetActiveAnimationIndex(int const nextAnimationIndex, float transitionDurationInSec = 0.3f);

    void AllocStorage(char const * name, size_t size);

    Blob GetStorage(char const * name);
    
    // We can just call drawUI instead and have way more control
    void EnableUI(char const * windowName, bool * isVisible);

    void DisableUI();
    
    void Update(float deltaTimeInSec, RT::DrawPass const & drawPass);

    using BindDescriptorSetFunction = std::function<void(AS::MeshPrimitive const & primitive, Node const & node)>;
    void Draw(RT::DrawPass & drawPass, BindDescriptorSetFunction const & bindFunction);
    
    // Only for model local buffers
    RT::UniformBufferGroup const & CreateUniformBuffer(char const * name, uint32_t size, uint32_t count);

    // Only for model local buffers
    void DeleteUniformBuffers();

    void UpdateUniformBuffer(char const * name, uint32_t startingIndex, CBlob ubo);

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

private:

    void updateAnimation(float deltaTimeInSec);

    void computeNodesGlobalTransform();

    void updateAllSkinsJoints();

    void updateSkinJoints(uint32_t skinIndex, AS::MeshSkin const & skin);
    
    void drawNode(
        RT::DrawPass & drawPass,
        Node const & node,
        BindDescriptorSetFunction const & bindFunction
    );

    void drawSubMesh(
        RT::DrawPass & drawPass,
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

    void onUI();

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

    // TODO We need to separate UI from drawableEssence
    std::string mRecordWindowName {};
    std::unique_ptr<UIRecordObject> mRecordUIObject;
    bool * mIsUIVisible = nullptr;

    Blob mCachedSkinsJointsBlob {};
    std::vector<JointTransformData *> mCachedSkinsJoints {};

    struct DirtyBuffer {
        std::string bufferName;
        RT::UniformBufferGroup * bufferGroup;
        uint32_t startingIndex;
        uint32_t remainingUpdateCount;
        Blob ubo;
    };
    std::vector<DirtyBuffer> mDirtyBuffers {};

    bool mIsModelTransformChanged = true;
    glm::mat4 mModelTransform = glm::identity<glm::mat4>();

    std::vector<Skin> mSkins {};
    std::vector<Node> mNodes {};

    std::unordered_map<std::string, Blob> mStorageMap {};

    std::unordered_map<std::string, RT::DescriptorSetGroup> mDescriptorSetGroups {};

};

};
