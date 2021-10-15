#pragma once

#include "engine/render_system/RenderTypes.hpp"

#include <unordered_map>

namespace MFA {

class DrawableEssence {
public:

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

    explicit DrawableEssence(char const * name, RT::GpuModel const & model_);

    ~DrawableEssence();
    
    DrawableEssence & operator= (DrawableEssence && rhs) noexcept = delete;

    DrawableEssence (DrawableEssence const &) noexcept = delete;

    DrawableEssence (DrawableEssence && rhs) noexcept = delete;

    DrawableEssence & operator = (DrawableEssence const &) noexcept = delete;

    [[nodiscard]]
    RT::GpuModel const & GetGpuModel() const;

    [[nodiscard]] RT::UniformBufferGroup const & GetPrimitivesBuffer() const noexcept;

    [[nodiscard]]
    uint32_t GetPrimitiveCount() const noexcept {
        return mPrimitiveCount;
    }

    [[nodiscard]]
    std::string const & GetName() const noexcept;

    [[nodiscard]]
    int GetAnimationIndex(char const * name) const noexcept;

    RT::DescriptorSetGroup const & CreateDescriptorSetGroup(
        VkDescriptorPool descriptorPool,
        uint32_t descriptorSetCount,
        VkDescriptorSetLayout descriptorSetLayout
    );

    [[nodiscard]]
    RT::DescriptorSetGroup const & GetDescriptorSetGroup() const;

private:

    std::string mName {};

    RT::UniformBufferGroup mPrimitivesBuffer;

    RT::GpuModel const & mGpuModel;
    
    uint32_t mPrimitiveCount = 0;

    std::unordered_map<std::string, int> mAnimationNameLookupTable {};

    std::unordered_map<std::string, RT::DescriptorSetGroup> mDescriptorSetGroups {};

    RT::DescriptorSetGroup mDescriptorSetGroup {};
    bool mIsDescriptorSetGroupValid = false;
    
};

}
