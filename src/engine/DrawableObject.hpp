#pragma once

#include "RenderFrontend.hpp"

#include <unordered_map>

namespace MFA {

namespace RF = RenderFrontend;
namespace RB = RenderBackend;
// TODO Start from here, Drawable object currently can be a struct that holds data, Also we can update all descriptor sets once at start
// TODO Handle system could be useful
class DrawableObject {
public:

    DrawableObject() = default;

    ~DrawableObject() = default;

    DrawableObject(
        RF::GpuModel & model_,
        VkDescriptorSetLayout_T * descriptorSetLayout
    );

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

    void draw(RF::DrawPass & drawPass);

private:

    struct NodeTransformBuffer {
        //float rotationAndScale[16];
        //float translate[16];
        float model[16];
    } mNodeTransformData {};

    void drawNode(RF::DrawPass & drawPass, const AssetSystem::Mesh::Node & node);

    void drawSubMesh(RF::DrawPass & drawPass, AssetSystem::Mesh::SubMesh const & subMesh);

    // Note: Order is important
    RF::UniformBufferGroup mNodeTransformBuffers {};
    RF::GpuModel * mGpuModel = nullptr;
    std::vector<VkDescriptorSet_T *> mDescriptorSets {};
    std::unordered_map<std::string, RF::UniformBufferGroup> mUniformBuffers {};
};

}
