#pragma once

#include "engine/render_system/RenderTypes.hpp"
#include "engine/asset_system/Asset_PBR_Mesh.hpp"
#include "engine/render_system/pipelines/EssenceBase.hpp"

#include <unordered_map>

namespace MFA {

class PBR_Essence final : public EssenceBase {
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

    explicit PBR_Essence(
        std::shared_ptr<RT::GpuModel> const & gpuModel,
        std::shared_ptr<AS::PBR::MeshData> meshData
    );
    ~PBR_Essence() override;
    
    PBR_Essence & operator= (PBR_Essence && rhs) noexcept = delete;
    PBR_Essence (PBR_Essence const &) noexcept = delete;
    PBR_Essence (PBR_Essence && rhs) noexcept = delete;
    PBR_Essence & operator = (PBR_Essence const &) noexcept = delete;

    [[nodiscard]] RT::UniformBufferGroup const & getPrimitivesBuffer() const noexcept;

    [[nodiscard]]
    uint32_t getPrimitiveCount() const noexcept;

    [[nodiscard]]
    int getAnimationIndex(char const * name) const noexcept;

    [[nodiscard]]
    AS::PBR::MeshData const * getMeshData() const;

    void bindVertexBuffer(RT::CommandRecordState const & recordState) const override;

private:

    std::shared_ptr<RT::UniformBufferGroup> mPrimitivesBuffer = nullptr;
    uint32_t mPrimitiveCount = 0;

    std::unordered_map<std::string, int> mAnimationNameLookupTable {};

    std::shared_ptr<AS::PBR::MeshData> mMeshData {};

};

}
