#pragma once

#include "engine/render_system/RenderTypesFWD.hpp"
#include "engine/render_system/essence/Essence.hpp"
#include "engine/render_system/variant/Variant.hpp"

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
        std::shared_ptr<AS::Mesh> const & cpuMesh
    );

    // Editor only function
    void DestroyEssence(RT::GpuModelId id);

    void DestroyEssence(RT::GpuModel const & gpuModel);

    virtual Variant * CreateVariant(RT::GpuModel const & gpuModel);

    virtual Variant * CreateVariant(RT::GpuModelId id);

    void RemoveVariant(Variant & variant);

    virtual void OnResize() = 0;

    virtual char const * GetName() const = 0;

protected:

    virtual void Init();

    virtual void Shutdown();

    virtual std::shared_ptr<Essence> internalCreateEssence(
        std::shared_ptr<RT::GpuModel> const & gpuModel,
        std::shared_ptr<AS::Mesh> const & cpuMesh
    ) = 0;

    virtual std::shared_ptr<Variant> internalCreateVariant(Essence * essence) = 0;

    struct EssenceAndVariants {

        std::shared_ptr<Essence> essence;  
        std::vector<std::shared_ptr<Variant>> variants {};

        explicit EssenceAndVariants();
        explicit EssenceAndVariants(std::shared_ptr<Essence> essence);
    };
    std::unordered_map<RT::GpuModelId, EssenceAndVariants> mEssenceAndVariantsMap;
    std::vector<Variant *> mAllVariantsList {};
    VkDescriptorPool mDescriptorPool {};
    uint32_t mMaxSets;

};

}
