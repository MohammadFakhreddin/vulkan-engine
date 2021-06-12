#pragma once

#include "RenderFrontend.hpp"

#include <unordered_map>
#include <glm/fwd.hpp>

namespace MFA {

namespace RF = RenderFrontend;
namespace RB = RenderBackend;
namespace AS = AssetSystem;

using DrawableObjectId = uint32_t;

using Node = AS::Mesh::Node;

class DrawableObject {
private:
    struct NodeTransformBuffer {
        float model[16];
    } mNodeTransformData {};

    struct JointTransformBuffer {
        float model[16];
    };
    // TODO Find a better number, other than 1000
    static constexpr uintmax_t JointTransformBufferSize = sizeof(JointTransformBuffer) * 1000;
public:

    explicit DrawableObject(
        RF::GpuModel & model_,
        VkDescriptorSetLayout_T * descriptorSetLayout
    );

    // TODO Multiple descriptor set layout support
    /*explicit DrawableObject(
        RF::GpuModel & model_,
        uint32_t descriptorSetLayoutsCount,
        VkDescriptorSetLayout_T ** descriptorSetLayouts
    );*/

    ~DrawableObject() = default;
    
    DrawableObject & operator= (DrawableObject && rhs) noexcept {
        this->mNodeTransformBuffers = std::move(rhs.mNodeTransformBuffers);
        this->mGpuModel = rhs.mGpuModel;
        this->mDescriptorSets = std::move(rhs.mDescriptorSets);
        this->mUniformBuffers = std::move(rhs.mUniformBuffers);
        return *this;
    }

    DrawableObject (DrawableObject const &) noexcept = delete;

    DrawableObject (DrawableObject && rhs) noexcept {
        this->mNodeTransformBuffers = std::move(rhs.mNodeTransformBuffers);
        this->mGpuModel = rhs.mGpuModel;
        this->mDescriptorSets = std::move(rhs.mDescriptorSets);
        this->mUniformBuffers = std::move(rhs.mUniformBuffers);
    }

    DrawableObject & operator = (DrawableObject const &) noexcept = delete;

    [[nodiscard]]
    RF::GpuModel * getModel() const;

    // Value equals to primitiveCount
    [[nodiscard]]
    uint32_t getDescriptorSetCount() const;

    [[nodiscard]] VkDescriptorSet_T * getDescriptorSetByPrimitiveUniqueId(uint32_t index);

    [[nodiscard]] VkDescriptorSet_T ** getDescriptorSets();

    // Only for model local buffers
    RF::UniformBufferGroup * createUniformBuffer(char const * name, uint32_t size);

    RF::UniformBufferGroup * createMultipleUniformBuffer(char const * name, uint32_t size, uint32_t count);

    // Only for model local buffers
    void deleteUniformBuffers();

    void updateUniformBuffer(char const * name, CBlob ubo);

    [[nodiscard]] RF::UniformBufferGroup * getUniformBuffer(char const * name);

    [[nodiscard]] RF::UniformBufferGroup const & getNodeTransformBuffer() const noexcept;

    [[nodiscard]] RF::UniformBufferGroup const & getSkinTransformBuffer() const noexcept;

    void update(float deltaTimeInSec);

    void draw(RF::DrawPass & drawPass);

    [[nodiscard]]
    uint32_t getId() const noexcept {
        return mId;
    }

private:

    void updateAnimation(float deltaTimeInSec);

    void computeNodesGlobalTransform();

    void updateJoints(float deltaTimeInSec);

    void updateJoint(float deltaTimeInSec, Node const & node);

    void drawNode(RF::DrawPass & drawPass, const Node & node);

    void drawSubMesh(RF::DrawPass & drawPass, AS::Mesh::SubMesh const & subMesh);

    [[nodiscard]]
    glm::mat4 computeNodeLocalTransform(Node const & node) const;

    void computeNodeGlobalTransform(Node & node, Node const * parentNode, bool isParentTransformChanged) const;

private:

    static DrawableObjectId NextId;

    DrawableObjectId const mId = 0;
    // Note: Order is important
    RF::UniformBufferGroup mNodeTransformBuffers {};
    RF::UniformBufferGroup mSkinJointsBuffers {};
    RF::GpuModel * mGpuModel = nullptr;
    std::vector<VkDescriptorSet_T *> mDescriptorSets {};
    std::unordered_map<std::string, RF::UniformBufferGroup> mUniformBuffers {};

    int mActiveAnimationIndex = 0;
    int mPreviousAnimationIndex = -1;
    float mAnimationCurrentTime = 0.0f;
};

}
