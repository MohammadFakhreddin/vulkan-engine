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
    
    struct NodeTransformBuffer {
        float model[16];
    } mNodeTransformData {};

    struct JointTransformBuffer {
        float model[16];
    };

    explicit DrawableObject(RF::GpuModel & model_);

    ~DrawableObject();
    
    DrawableObject & operator= (DrawableObject && rhs) noexcept {
        this->mNodeTransformBuffers = std::move(rhs.mNodeTransformBuffers);
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
    RF::UniformBufferGroup * CreateUniformBuffer(char const * name, uint32_t size);

    RF::UniformBufferGroup * CreateMultipleUniformBuffer(char const * name, uint32_t size, uint32_t count);

    // Only for model local buffers
    void DeleteUniformBuffers();

    void UpdateUniformBuffer(char const * name, CBlob ubo);

    [[nodiscard]] RF::UniformBufferGroup * GetUniformBuffer(char const * name);

    [[nodiscard]] RF::UniformBufferGroup const & GetNodeTransformBuffer() const noexcept;

    [[nodiscard]] RF::UniformBufferGroup const & GetSkinTransformBuffer(uint32_t nodeIndex) const noexcept;

    void Update(float deltaTimeInSec);

    using BindDescriptorSetFunction = std::function<void(AS::MeshPrimitive const &)>;
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

private:

    void updateAnimation(float deltaTimeInSec);

    void computeNodesGlobalTransform();

    void updateAllSkinsJoints();

    void updateSkinJoints(uint32_t skinIndex, Skin const & skin);

    void updateAllNodesJoints();

    void updateNodeJoint(Node const & node);

    void drawNode(
        RF::DrawPass & drawPass, 
        Node const & node, 
        BindDescriptorSetFunction const & bindFunction
    );

    void drawSubMesh(
        RF::DrawPass & drawPass, 
        AS::Mesh::SubMesh const & subMesh,
        BindDescriptorSetFunction const & bindFunction
    );

    [[nodiscard]]
    glm::mat4 computeNodeLocalTransform(Node const & node) const;

    void computeNodeGlobalTransform(Node & node, Node const * parentNode, bool isParentTransformChanged) const;

    void onUI();

    [[nodiscard]]
    int GetActiveAnimationIndex() const noexcept {
        return mActiveAnimationIndex;
    }

    void SetActiveAnimationIndex(int const activeAnimationIndex) {
        mActiveAnimationIndex = activeAnimationIndex;
    }

private:

    std::unordered_map<std::string, RB::DescriptorSetGroup> mDescriptorSetGroups {};

    // Note: Order is important
    RF::UniformBufferGroup mNodeTransformBuffers {};
    std::vector<RF::UniformBufferGroup> mSkinJointsBuffers {};
    RF::GpuModel * mGpuModel = nullptr;
    std::unordered_map<std::string, RF::UniformBufferGroup> mUniformBuffers {};

    int mActiveAnimationIndex = 0;
    int mPreviousAnimationIndex = -1;
    float mAnimationCurrentTime = 0.0f;

    std::string mRecordWindowName {};
    UIRecordObject mRecordUIObject;
    bool * mIsUIVisible = nullptr;

    std::vector<Blob> mCachedSkinsJoints {};
    std::vector<Blob> mNodesJoints {};

    uint32_t mPrimitiveCount = 0;
};

}
