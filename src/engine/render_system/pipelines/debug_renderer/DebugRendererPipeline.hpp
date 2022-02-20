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

    explicit DebugRendererPipeline();
    ~DebugRendererPipeline() override;

    PIPELINE_PROPS(
        DebugRendererPipeline,
        EventTypes::PreRenderEvent | EventTypes::RenderEvent,
        RenderOrder::DontCare
    )

    void init() override;

    void shutdown() override;

    void preRender(RT::CommandRecordState & recordState, float deltaTimeInSec) override;

    void render(RT::CommandRecordState & drawPass, float deltaTime) override;
    
    void onResize() override;

    void freeUnusedEssences() override;

protected:

    void internalAddEssence(EssenceBase * essence) override;

    std::shared_ptr<VariantBase> internalCreateVariant(EssenceBase * essence) override;

private:

    struct DrawableStorageData {
        float color[3] {};
    };

    void createDescriptorSetLayout();

    void createPipeline();

    void createDescriptorSets();

private:

    std::shared_ptr<RT::DescriptorSetLayoutGroup> mDescriptorSetLayout {};
    std::shared_ptr<RT::PipelineGroup> mpipeline {};

    RT::DescriptorSetGroup mDescriptorSetGroup {};
    
};

}
