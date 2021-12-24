#pragma once

#include "engine/render_system/RenderTypes.hpp"

#include <unordered_map>

namespace MFA {

class DrawableEssence {
public:

    struct PrimitiveInfo {
        alignas(16) float baseColorFactor[4];

        float emissiveFactor[3];
        float placeholder0;

        int baseColorTextureIndex;
        float metallicFactor;
        float roughnessFactor;
        int metallicRoughnessTextureIndex;

        int normalTextureIndex;
        int emissiveTextureIndex;
        int hasSkin;
        int occlusionTextureIndex;

        int alphaMode;
        float alphaCutoff;
        float placeholder1;
        float placeholder2;
    };

    explicit DrawableEssence(std::shared_ptr<RT::GpuModel> gpuModel);

    ~DrawableEssence();
    
    DrawableEssence & operator= (DrawableEssence && rhs) noexcept = delete;

    DrawableEssence (DrawableEssence const &) noexcept = delete;

    DrawableEssence (DrawableEssence && rhs) noexcept = delete;

    DrawableEssence & operator = (DrawableEssence const &) noexcept = delete;

    [[nodiscard]]
    RT::GpuModelId GetId() const;

    [[nodiscard]]
    RT::GpuModel * GetGpuModel() const;

    [[nodiscard]] RT::UniformBufferGroup const & GetPrimitivesBuffer() const noexcept;

    [[nodiscard]]
    uint32_t GetPrimitiveCount() const noexcept;

    [[nodiscard]]
    int GetAnimationIndex(char const * name) const noexcept;

    RT::DescriptorSetGroup const & CreateDescriptorSetGroup(
        VkDescriptorPool descriptorPool,
        uint32_t descriptorSetCount,
        VkDescriptorSetLayout descriptorSetLayout
    );

    [[nodiscard]]
    RT::DescriptorSetGroup const & GetDescriptorSetGroup() const;

    void BindVertexBuffer(RT::CommandRecordState const & recordState) const;
    void BindIndexBuffer(RT::CommandRecordState const & recordState) const;
    void BindDescriptorSetGroup(RT::CommandRecordState const & recordState) const;
    void BindAllRenderRequiredData(RT::CommandRecordState const & recordState) const;

private:

    std::shared_ptr<RT::UniformBufferGroup> mPrimitivesBuffer = nullptr;

    std::shared_ptr<RT::GpuModel> const mGpuModel;
    
    uint32_t mPrimitiveCount = 0;

    std::unordered_map<std::string, int> mAnimationNameLookupTable {};

    std::unordered_map<std::string, RT::DescriptorSetGroup> mDescriptorSetGroups {};

    RT::DescriptorSetGroup mDescriptorSetGroup {};
    bool mIsDescriptorSetGroupValid = false;
};

}
