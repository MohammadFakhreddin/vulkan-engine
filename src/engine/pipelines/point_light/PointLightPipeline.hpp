#pragma once

#include "engine/DrawableObject.hpp"
#include "engine/RenderFrontend.hpp"
#include "engine/pipelines/BasePipeline.hpp"

namespace MFA {
    
    namespace RB = RenderBackend;
    namespace RF = RenderFrontend;
    
class PointLightPipeline final : public BasePipeline {
public:

    struct PrimitiveInfo {
        alignas(16) float baseColorFactor[4];
    };

    struct ViewProjectionData {
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

    void init();

    void shutdown();

    DrawableObjectId addGpuModel(RF::GpuModel & gpuModel) override;

    bool removeGpuModel(DrawableObjectId drawableObjectId) override;
    
    void render(
        RF::DrawPass & drawPass, 
        uint32_t idsCount, 
        DrawableObjectId * ids
    ) override;

    bool updateViewProjectionBuffer(
        DrawableObjectId drawableObjectId, 
        ViewProjectionData const & viewProjectionData
    );

    bool updatePrimitiveInfo(
        DrawableObjectId drawableObjectId,
        PrimitiveInfo const & info
    );

private:

    void createDescriptorSetLayout();

    void destroyDescriptorSetLayout() const;

    void createPipeline();

    void destroyPipeline();

    void destroyUniformBuffers();

    bool mIsInitialized = false;

    VkDescriptorSetLayout_T * mDescriptorSetLayout = nullptr;
    MFA::RenderFrontend::DrawPipeline mDrawPipeline {};

    std::unordered_map<DrawableObjectId, std::unique_ptr<DrawableObject>> mDrawableObjects {};

};

}
