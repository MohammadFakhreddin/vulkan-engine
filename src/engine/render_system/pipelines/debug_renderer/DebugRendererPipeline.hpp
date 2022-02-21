#pragma once

#include "engine/render_system/RenderTypes.hpp"
#include "engine/render_system/pipelines/BasePipeline.hpp"
#include "engine/asset_system/AssetTypes.hpp"

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

    PIPELINE_PROPS(DebugRendererPipeline, EventTypes::RenderEvent)

    void init() override;

    void shutdown() override;

    void render(RT::CommandRecordState & drawPass, float deltaTime) override;
    
    void onResize() override;

    bool createEssenceWithoutModel(
        std::shared_ptr<RT::GpuModel> const & gpuModel,
        uint32_t indicesCount
    );

    void freeUnusedEssences() override;

protected:

    std::shared_ptr<EssenceBase> internalCreateEssence(
        std::shared_ptr<RT::GpuModel> const & gpuModel,
        std::shared_ptr<AS::MeshBase> const & cpuMesh
    ) override;

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
