#pragma once

#include "engine/render_system/RenderTypes.hpp"
#include "engine/render_system/pipelines/BasePipeline.hpp"

namespace MFA {
    
class DebugRendererPipeline final : public BasePipeline {
public:

    struct PushConstants {
        float model[16];
        float baseColorFactor[3];
    };
    static_assert(sizeof(PushConstants) == 76);

    DebugRendererPipeline();
    ~DebugRendererPipeline() override;

    PIPELINE_PROPS(DebugRendererPipeline);

    void Init() override;

    void Shutdown() override;

    void PreRender(RT::CommandRecordState & drawPass, float deltaTime) override;

    void Render(RT::CommandRecordState & drawPass, float deltaTime) override;
    
    void OnResize() override {}



private:

    struct DrawableStorageData {
        float color[3] {};
    };

    void createDescriptorSetLayout();

    void destroyDescriptorSetLayout();

    void createPipeline();

    void createDescriptorSets();

    bool mIsInitialized = false;

    VkDescriptorSetLayout mDescriptorSetLayout {};
    RT::PipelineGroup mDrawPipeline {};

    RT::DescriptorSetGroup mDescriptorSetGroup {};
    
};

}
