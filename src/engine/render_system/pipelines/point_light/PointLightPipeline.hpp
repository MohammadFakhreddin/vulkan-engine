#pragma once

#include "engine/render_system/DrawableObject.hpp"
#include "engine/render_system/RenderFrontend.hpp"
#include "engine/render_system/pipelines/BasePipeline.hpp"

namespace MFA {
    
    namespace RB = RenderBackend;
    namespace RF = RenderFrontend;
    
class PointLightPipeline final : public BasePipeline {
public:

    struct PushConstants {
        float model[16];
        float baseColorFactor[3];
    };
    static_assert(sizeof(PushConstants) == 76);

    struct ViewProjectionData {
        alignas(64) float viewProjection[16];
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

    void UpdateLightTransform(uint32_t id, float lightTransform[16]);

    void UpdateLightColor(uint32_t id, float lightColor[3]);

    void UpdateCameraView(float cameraTransform[16]);

    void UpdateCameraProjection(float cameraProjection[16]);

    void OnResize() override {}

private:

    struct DrawableStorageData {
        float color[3] {};
    };

    void createDescriptorSetLayout();

    void destroyDescriptorSetLayout();

    void createPipeline();

    void destroyPipeline();

    void destroyDrawableObjects();

    void createDescriptorSets();

    void updateViewProjectionBuffer(RF::DrawPass const & drawPass);

    void createUniformBuffers();

    void destroyUniformBuffers();

    bool mIsInitialized = false;

    VkDescriptorSetLayout mDescriptorSetLayout {};
    MFA::RenderFrontend::DrawPipeline mDrawPipeline {};

    std::unordered_map<DrawableObjectId, std::unique_ptr<DrawableObject>> mDrawableObjects {};

    DrawableObjectId mNextDrawableId = 0;

    RB::DescriptorSetGroup mDescriptorSetGroup {};

    RF::UniformBufferGroup mViewProjectionBuffers {};

    glm::mat4 mCameraTransform {};
    glm::mat4 mCameraProjection {};
    ViewProjectionData mViewProjectionData {};
    uint32_t mViewProjectionBufferDirtyCounter = 0;
};

}
