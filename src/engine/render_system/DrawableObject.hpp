#pragma once

#include "RenderFrontend.hpp"

#include <unordered_map>
#include <glm/fwd.hpp>

#include "engine/ui_system/UIRecordObject.hpp"

namespace MFA {

namespace RF = RenderFrontend;
namespace RB = RenderBackend;
namespace AS = AssetSystem;

using DrawableObjectId = uint32_t;

using Node = AS::Mesh::Node;
using Skin = AS::Mesh::Skin;

class DrawableObject {
public:

    struct JointTransformData {
        float model[16];
    };

    struct PrimitiveInfo {
        alignas(16) float baseColorFactor[4];

        float emissiveFactor[3];
        int placeholder0;

        int baseColorTextureIndex;
        float metallicFactor;
        float roughnessFactor;
        int metallicRoughnessTextureIndex;

        int normalTextureIndex;
        int emissiveTextureIndex;
        int hasSkin;
        int placeholder1;
    };

    struct Skin {
        int skinStartingIndex;
    };

    struct Node {
        AS::MeshNode * meshNode = nullptr;

        glm::quat rotation {0.0f, 0.0f, 0.0f, 1.0f};     // x, y, z, w
        glm::vec3 scale {1.0f, 1.0f, 1.0f};                  
        glm::vec3 translate {0.0f, 0.0f, 0.0f};
        glm::mat4 transform {};

        bool isCachedDataValid = false;
        glm::mat4 cachedLocalTransform {};
        glm::mat4 cachedGlobalTransform {};
        glm::mat4 cachedModelTransform {};
        glm::mat4 cachedGlobalInverseTransform {};

        Skin * skin = nullptr;
    };

    explicit DrawableObject(RF::GpuModel & model_);

    ~DrawableObject();
    
    DrawableObject & operator= (DrawableObject && rhs) noexcept {
        this->mCachedSkinsJointsBlob = rhs.mCachedSkinsJointsBlob;
        this->mCachedSkinsJoints = rhs.mCachedSkinsJoints;
        this->mSkinsJointsBuffer = std::move(rhs.mSkinsJointsBuffer);
        this->mGpuModel = rhs.mGpuModel;
        this->mDescriptorSetGroups = std::move(rhs.mDescriptorSetGroups);
        this->mUniformBuffers = std::move(rhs.mUniformBuffers);
        return *this;
    }

    DrawableObject (DrawableObject const &) noexcept = delete;

    DrawableObject (DrawableObject && rhs) noexcept = delete;

    DrawableObject & operator = (DrawableObject const &) noexcept = delete;

    [[nodiscard]]
    RF::GpuModel * GetModel() const;

    // Only for model local buffers
    RF::UniformBufferGroup * CreateUniformBuffer(char const * name, uint32_t size, uint32_t count);

    // Only for model local buffers
    void DeleteUniformBuffers();

    void UpdateUniformBuffer(char const * name, uint32_t startingIndex, CBlob ubo);

    [[nodiscard]] RF::UniformBufferGroup * GetUniformBuffer(char const * name);
    
    [[nodiscard]] RF::UniformBufferGroup const & GetSkinJointsBuffer() const noexcept;

    [[nodiscard]] RF::UniformBufferGroup const & GetPrimitivesBuffer() const noexcept;

    void Update(float deltaTimeInSec, RF::DrawPass const & drawPass);

    using BindDescriptorSetFunction = std::function<void(AS::MeshPrimitive const &, Node const & node)>;
    void Draw(RF::DrawPass & drawPass, BindDescriptorSetFunction const & bindFunction);

    void EnableUI(char const * windowName, bool * isVisible);

    void DisableUI();

    [[nodiscard]]
    uint32_t GetPrimitiveCount() const noexcept {
        return mPrimitiveCount;
    }

    RB::DescriptorSetGroup const & CreateDescriptorSetGroup(
        char const * name, 
        VkDescriptorSetLayout descriptorSetLayout, 
        uint32_t descriptorSetCount
    );

    RB::DescriptorSetGroup * GetDescriptorSetGroup(char const * name);

    void UpdateModelTransform(float modelTransform[16]);

    [[nodiscard]]
    int GetActiveAnimationIndex() const noexcept {
        return mActiveAnimationIndex;
    }

    void SetActiveAnimationIndex(int const activeAnimationIndex) {
        mActiveAnimationIndex = activeAnimationIndex;
    }

    void AllocStorage(char const * name, size_t size);

    Blob GetStorage(char const * name);

private:

    void updateAnimation(float deltaTimeInSec);

    void computeNodesGlobalTransform();

    void updateAllSkinsJoints();

    void updateSkinJoints(uint32_t skinIndex, AS::MeshSkin const & skin);

    //void updateAllNodes(RF::DrawPass const & drawPass);

    //void updateNodes(RF::DrawPass const & drawPass, Node & node);
    
    void drawNode(
        RF::DrawPass & drawPass, 
        Node const & node, 
        BindDescriptorSetFunction const & bindFunction
    );

    void drawSubMesh(
        RF::DrawPass & drawPass, 
        AS::Mesh::SubMesh const & subMesh,
        Node const & node,
        BindDescriptorSetFunction const & bindFunction
    );

    [[nodiscard]]
    static glm::mat4 computeNodeLocalTransform(Node const & node);

    void computeNodeGlobalTransform(
        Node & node, 
        Node const * parentNode, 
        bool isParentTransformChanged
    );

    void onUI();

private:

    std::unordered_map<std::string, RB::DescriptorSetGroup> mDescriptorSetGroups {};

    // Note: Order is important
    RF::UniformBufferGroup mSkinsJointsBuffer {};
    RF::UniformBufferGroup mPrimitivesBuffer {};

    //std::vector<JointTransformData> mNodesJointTransformData {};
    RF::GpuModel * mGpuModel = nullptr;
    std::unordered_map<std::string, RF::UniformBufferGroup> mUniformBuffers {};

    int mActiveAnimationIndex = 0;
    int mPreviousAnimationIndex = -1;
    float mAnimationCurrentTime = 0.0f;

    std::string mRecordWindowName {};
    UIRecordObject mRecordUIObject;
    bool * mIsUIVisible = nullptr;

    Blob mCachedSkinsJointsBlob {};
    std::vector<JointTransformData *> mCachedSkinsJoints {};

    uint32_t mPrimitiveCount = 0;
    
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
    std::vector<Node> mNodes {};

    std::unordered_map<std::string, Blob> mStorageMap {};

};

}
