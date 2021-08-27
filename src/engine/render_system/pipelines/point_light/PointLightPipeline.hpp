#pragma once

#include "engine/render_system/DrawableObject.hpp"
#include "engine/render_system/RenderFrontend.hpp"
#include "engine/render_system/pipelines/BasePipeline.hpp"

namespace MFA {
    
    namespace RB = RenderBackend;
    namespace RF = RenderFrontend;
    
class PointLightPipeline final : public BasePipeline {
public:

    struct PrimitiveInfo {
        alignas(16) float baseColorFactor[4];
    };

    struct ModelViewProjectionData {
        alignas(64) float model[16];
        alignas(64) float view[16];
        alignas(64) float projection[16];
    };

    PointLightPipeline() = default;
    ~PointLightPipeline() override;

    PointLightPipeline (PointLightPipeline const &) noexcept = delete;
    PointLightPipeline (PointLightPipeline &&) noexcept = delete;
    PointLightPipeline & operator = (PointLightPipeline const &) noexcept = delete;
    PointLightPipeline & operator = (PointLightPipeline &&) noexcept = delete;

    void Init();

    void Shutdown();

    DrawableObjectId AddGpuModel(RF::GpuModel & gpuModel) override;

    bool RemoveGpuModel(DrawableObjectId drawableObjectId) override;

    void PreRender(
        RF::DrawPass & drawPass, 
        float deltaTimeInSec,
        uint32_t idsCount, 
        DrawableObjectId * ids 
    ) override;

    void Render(
        RF::DrawPass & drawPass,
        float deltaTimeInSec,
        uint32_t idsCount, 
        DrawableObjectId * ids
    ) override;

    bool UpdateViewProjectionBuffer(
        DrawableObjectId drawableObjectId, 
        ModelViewProjectionData const & viewProjectionData
    );

    bool UpdatePrimitiveInfo(
        DrawableObjectId drawableObjectId,
        PrimitiveInfo const & info
    );

    void OnResize() override {}

private:

    void createDescriptorSetLayout();

    void destroyDescriptorSetLayout();

    void createPipeline();

    void destroyPipeline();

    void destroyUniformBuffers();

    bool mIsInitialized = false;

    VkDescriptorSetLayout mDescriptorSetLayout {};
    MFA::RenderFrontend::DrawPipeline mDrawPipeline {};

    std::unordered_map<DrawableObjectId, std::unique_ptr<DrawableObject>> mDrawableObjects {};

    DrawableObjectId mNextDrawableId = 0;
};

}
