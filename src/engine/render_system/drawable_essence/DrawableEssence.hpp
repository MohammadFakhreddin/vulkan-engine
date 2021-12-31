#pragma once

#include "engine/render_system/RenderTypes.hpp"

#include <unordered_map>

#include "engine/render_system/essence/Essence.hpp"

namespace MFA {
// TODO Maybe we should rename this to MeshEssence
class DrawableEssence final : public Essence {
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

    explicit DrawableEssence(
        std::shared_ptr<RT::GpuModel> const & gpuModel,
        std::shared_ptr<AS::Mesh> const & cpuMesh
    );
    ~DrawableEssence() override;
    
    DrawableEssence & operator= (DrawableEssence && rhs) noexcept = delete;
    DrawableEssence (DrawableEssence const &) noexcept = delete;
    DrawableEssence (DrawableEssence && rhs) noexcept = delete;
    DrawableEssence & operator = (DrawableEssence const &) noexcept = delete;

    [[nodiscard]] RT::UniformBufferGroup const & getPrimitivesBuffer() const noexcept;

    [[nodiscard]]
    uint32_t getPrimitiveCount() const noexcept;

    [[nodiscard]]
    int getAnimationIndex(char const * name) const noexcept;

    [[nodiscard]]
    AS::Mesh const * getCpuMesh() const;

private:

    std::shared_ptr<RT::UniformBufferGroup> mPrimitivesBuffer = nullptr;
    uint32_t mPrimitiveCount = 0;

    std::unordered_map<std::string, int> mAnimationNameLookupTable {};

    std::shared_ptr<AS::Mesh> mCpuMesh;

};

}
