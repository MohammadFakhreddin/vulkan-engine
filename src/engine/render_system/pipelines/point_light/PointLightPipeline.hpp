#pragma once

#include "engine/render_system/RenderTypes.hpp"
#include "engine/render_system/pipelines/BasePipeline.hpp"

#include <glm/glm.hpp>

namespace MFA {
    
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

    void Init() override;

    void Shutdown() override;

    void PreRender(RT::DrawPass & drawPass, float deltaTime) override;

    void Render(RT::DrawPass & drawPass, float deltaTime) override;
    
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

    void createDescriptorSets();

    void updateViewProjectionBuffer(RT::DrawPass const & drawPass);

    void createUniformBuffers();

    void destroyUniformBuffers();

    bool mIsInitialized = false;

    VkDescriptorSetLayout mDescriptorSetLayout {};
    RT::PipelineGroup mDrawPipeline {};

    RT::DescriptorSetGroup mDescriptorSetGroup {};

    RT::UniformBufferGroup mViewProjectionBuffers {};

    glm::mat4 mCameraTransform {};
    glm::mat4 mCameraProjection {};
    ViewProjectionData mViewProjectionData {};
    uint32_t mViewProjectionBufferDirtyCounter = 0;
};

}
