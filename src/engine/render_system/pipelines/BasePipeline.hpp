#pragma once

#include "engine/render_system/RenderTypesFWD.hpp"
#include "DescriptorSetSchema.hpp"

#include <memory>
#include <unordered_map>
#include <vector>

#define PIPELINE_PROPS(className)                                       \
static constexpr char const * Name = #className;                        \
char const * GetName() const override {                                 \
    return Name;                                                        \
}                                                                       \
                                                                        \
className (className const &) noexcept = delete;                        \
className (className &&) noexcept = delete;                             \
className & operator = (className const &) noexcept = delete;           \
className & operator = (className &&) noexcept = delete;                \

namespace MFA {

    class VariantBase;
    class EssenceBase;

    namespace AssetSystem
    {
        class MeshBase;
    }

    class BasePipeline {
private:

public:

    explicit BasePipeline(uint32_t maxSets);

    virtual ~BasePipeline();

    BasePipeline (BasePipeline const &) noexcept = delete;
    BasePipeline (BasePipeline &&) noexcept = delete;
    BasePipeline & operator = (BasePipeline const &) noexcept = delete;
    BasePipeline & operator = (BasePipeline && rhs) noexcept = delete;

    virtual void PreRender(
        RT::CommandRecordState & drawPass,
        float deltaTime
    );

    virtual void Render(        
        RT::CommandRecordState & drawPass,
        float deltaTime
    );

    virtual void PostRender(
        RT::CommandRecordState & drawPass,
        float deltaTime
    );

    bool CreateEssenceIfNotExists(
        std::shared_ptr<RT::GpuModel> const & gpuModel,
        std::shared_ptr<AssetSystem::MeshBase> const & cpuMesh
    );

    // Editor only function
    void DestroyEssence(RT::GpuModelId id);

    void DestroyEssence(RT::GpuModel const & gpuModel);

    virtual VariantBase * CreateVariant(RT::GpuModel const & gpuModel);

    virtual VariantBase * CreateVariant(RT::GpuModelId id);

    void RemoveVariant(VariantBase & variant);

    virtual void OnResize() = 0;

    [[nodiscard]]
    virtual char const * GetName() const = 0;

protected:

    virtual void Init();

    virtual void Shutdown();

    virtual std::shared_ptr<EssenceBase> internalCreateEssence(
        std::shared_ptr<RT::GpuModel> const & gpuModel,
        std::shared_ptr<AssetSystem::MeshBase> const & cpuMesh
    ) = 0;

    virtual std::shared_ptr<VariantBase> internalCreateVariant(EssenceBase * essence) = 0;

    struct EssenceAndVariants {

        std::shared_ptr<EssenceBase> essence;  
        std::vector<std::shared_ptr<VariantBase>> variants;

        explicit EssenceAndVariants();
        explicit EssenceAndVariants(std::shared_ptr<EssenceBase> essence);
    };
    std::unordered_map<RT::GpuModelId, EssenceAndVariants> mEssenceAndVariantsMap;
    std::vector<VariantBase *> mAllVariantsList {};
    VkDescriptorPool mDescriptorPool {};
    uint32_t mMaxSets;

};

}
