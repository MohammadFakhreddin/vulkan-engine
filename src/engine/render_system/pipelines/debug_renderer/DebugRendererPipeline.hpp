#pragma once

#include "engine/render_system/RenderTypes.hpp"
#include "engine/render_system/pipelines/BasePipeline.hpp"

namespace MFA {

    class DebugEssence;
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
        EventTypes::RenderEvent,
        RenderOrder::DontCare
    )

    void init() override;

    void shutdown() override;

    void render(RT::CommandRecordState & recordState, float deltaTime) override;
    
    void onResize() override;

    void freeUnusedEssences() override;

    std::weak_ptr<DebugEssence> GetEssence(std::string const & nameId);

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
    std::shared_ptr<RT::PipelineGroup> mPipeline {};

    RT::DescriptorSetGroup mDescriptorSetGroup {};
    
};

}
