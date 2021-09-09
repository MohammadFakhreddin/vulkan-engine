#pragma once


#include <glm/fwd.hpp>

#include <memory>

namespace MFA {

class DrawableEssence;
class RenderFrontend {          // TODO I have to define RFTypes! or FWD headers
    struct UniformBufferGroup;
};
class  UIRecordObject;

namespace RF = RenderFrontend;
namespace RB = RenderBackend;
namespace AS = AssetSystem;

class DrawableVariant {
public:
    
    using InstanceId = uint32_t;
    
    struct JointTransformData {
        float model[16];
    };
    
    struct Skin {
        int skinStartingIndex;
    };

    struct Node;
    
    explicit DrawableVariant(DrawableEssence const & essence);

    ~DrawableVariant();
    
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
    
    // We can just call drawUI instead and have way more controll
    void EnableUI(char const * windowName, bool * isVisible);

    void DisableUI();
    
    void Update(float deltaTimeInSec, RF::DrawPass const & drawPass);

    using BindDescriptorSetFunction = std::function<void(AS::MeshPrimitive const & primitive, Node const & node)>;
    void Draw(RF::DrawPass & drawPass, BindDescriptorSetFunction const & bindFunction);
    
    // Only for model local buffers
    RF::UniformBufferGroup const * CreateUniformBuffer(char const * name, uint32_t size, uint32_t count);

    // Only for model local buffers
    void DeleteUniformBuffers();

    void UpdateUniformBuffer(char const * name, uint32_t startingIndex, CBlob ubo);

    [[nodiscard]] 
    RF::UniformBufferGroup const * GetUniformBuffer(char const * name);
    
    [[nodiscard]] 
    RF::UniformBufferGroup const * GetSkinJointsBuffer() const noexcept;

private:

    void updateAnimation(float deltaTimeInSec);

    void computeNodesGlobalTransform();

    void updateAllSkinsJoints();

    void updateSkinJoints(uint32_t skinIndex, AS::MeshSkin const & skin);
    
    void drawNode(
        RF::DrawPass & drawPass,
        Node const * node,
        BindDescriptorSetFunction const & bindFunction
    );

    void drawSubMesh(
        RF::DrawPass & drawPass,
        AS::Mesh::SubMesh const & subMesh,
        Node const * node,
        BindDescriptorSetFunction const & bindFunction
    );

    [[nodiscard]]
    glm::mat4 computeNodeLocalTransform(Node const * node);

    void computeNodeGlobalTransform(
        Node * node,
        Node const * parentNode,
        bool isParentTransformChanged
    );

    void onUI();

private:

    static InstanceId NextInstanceId ;
    InstanceId mId;

    DrawableEssence const * mEssence = nullptr;
    
    std::unique_ptr<RF::UniformBufferGroup> mSkinsJointsBuffer {};
    
    std::unordered_map<std::string, std::unique_ptr<RF::UniformBufferGroup>> mUniformBuffers {};

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
        RF::UniformBufferGroup * bufferGroup;
        uint32_t startingIndex;
        uint32_t remainingUpdateCount;
        Blob ubo;
    };
    std::vector<DirtyBuffer> mDirtyBuffers {};

    bool mIsModelTransformChanged = true;
    glm::mat4 mModelTransform = glm::identity<glm::mat4>();

    std::vector<Skin> mSkins {};
    std::vector<std::unique_ptr<Node>> mNodes {};

    std::unordered_map<std::string, Blob> mStorageMap {};
    
};

};
