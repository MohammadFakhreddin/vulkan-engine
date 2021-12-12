#pragma once

#include "engine/render_system/RenderTypesFWD.hpp"
#include "engine/render_system/drawable_essence/DrawableEssence.hpp"
#include "engine/render_system/drawable_variant/DrawableVariant.hpp"

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

    struct EssenceAndVariants {

        std::shared_ptr<DrawableEssence> Essence;
        std::vector<std::shared_ptr<DrawableVariant>> Variants {};

        explicit EssenceAndVariants();
        explicit EssenceAndVariants(std::shared_ptr<RT::GpuModel> const & gpuModel);
        ~EssenceAndVariants();
    };

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

    bool CreateEssenceIfNotExists(std::shared_ptr<RT::GpuModel> const & gpuModel);

    // Editor only function
    void DestroyDrawableEssence(RT::GpuModelId id);

    void DestroyDrawableEssence(RT::GpuModel const & gpuModel);

    virtual DrawableVariant * CreateDrawableVariant(RT::GpuModel const & gpuModel);

    virtual DrawableVariant * CreateDrawableVariant(RT::GpuModelId id);

    void RemoveDrawableVariant(DrawableVariant & variant);

    virtual void OnResize() = 0;

    virtual char const * GetName() const = 0;

protected:

    virtual void Init();

    virtual void Shutdown();

    virtual void internalCreateDrawableEssence(DrawableEssence & essence) {}

    std::unordered_map<RT::GpuModelId, EssenceAndVariants> mEssenceAndVariantsMap;
    std::vector<DrawableVariant *> mAllVariantsList {};
    VkDescriptorPool mDescriptorPool {};
    uint32_t mMaxSets;

};

}
