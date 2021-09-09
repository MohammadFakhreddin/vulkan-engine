#pragma once

#include "engine/render_system/RenderFrontend.hpp"
#include "engine/ui_system/UIRecordObject.hpp"

#include <unordered_map>

namespace MFA {

namespace RF = RenderFrontend;
namespace RB = RenderBackend;
namespace AS = AssetSystem;

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

    explicit DrawableEssence(RF::GpuModel & model_);

    ~DrawableEssence();
    
    DrawableEssence & operator= (DrawableEssence && rhs) noexcept {
        this->mGpuModel = rhs.mGpuModel;
        this->mDescriptorSetGroups = std::move(rhs.mDescriptorSetGroups);
        return *this;
    }

    DrawableEssence (DrawableEssence const &) noexcept = delete;

    DrawableEssence (DrawableEssence && rhs) noexcept = delete;

    DrawableEssence & operator = (DrawableEssence const &) noexcept = delete;

    [[nodiscard]]
    RF::GpuModel const * GetGpuModel() const;

    [[nodiscard]] RF::UniformBufferGroup const & GetPrimitivesBuffer() const noexcept;

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

private:

    std::unordered_map<std::string, RB::DescriptorSetGroup> mDescriptorSetGroups {};

    // Note: Order is important
    RF::UniformBufferGroup mPrimitivesBuffer {};

    RF::GpuModel * mGpuModel = nullptr;
    
    uint32_t mPrimitiveCount = 0;
    
};

}
