#pragma once

#include "engine/render_system/RenderTypesFWD.hpp"
#include "engine/render_system/drawable_essence/DrawableEssence.hpp"
#include "engine/render_system/drawable_variant/DrawableVariant.hpp"

#include <string>
#include <memory>
#include <unordered_map>
#include <vector>

namespace MFA {

class BasePipeline {
private:

    struct EssenceAndItsVariants {

        DrawableEssence essence;
        std::vector<std::shared_ptr<DrawableVariant>> variants {};

        explicit EssenceAndItsVariants(
            char const * name,
            RT::GpuModel const & model_
        );
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

    void CreateDrawableEssence(
        char const * essenceName, 
        RT::GpuModel const & gpuModel
    );

    // Editor only function
    void DestroyDrawableEssence(char const * essenceName);

    virtual std::weak_ptr<DrawableVariant> CreateDrawableVariant(char const * essenceName);

    void RemoveDrawableVariant(DrawableVariant * variant);

    virtual void OnResize() = 0;

protected:

    virtual void Init();

    virtual void Shutdown();

    virtual void internalCreateDrawableEssence(DrawableEssence & essence) {}

    std::unordered_map<std::string, std::unique_ptr<EssenceAndItsVariants>> mEssenceAndVariantsMap;

    std::vector<DrawableVariant *> mVariantsRef {};

    VkDescriptorPool mDescriptorPool {};

    uint32_t mMaxSets;

};

}
