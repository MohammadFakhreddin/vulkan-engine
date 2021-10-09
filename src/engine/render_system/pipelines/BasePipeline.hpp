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
        std::vector<std::unique_ptr<DrawableVariant>> variants {};

        explicit EssenceAndItsVariants(
            char const * name,
            RT::GpuModel const & model_
        );
    };

public:

    explicit BasePipeline();

    virtual ~BasePipeline();

    BasePipeline (BasePipeline const &) noexcept = delete;
    BasePipeline (BasePipeline &&) noexcept = delete;
    BasePipeline & operator = (BasePipeline const &) noexcept = delete;
    BasePipeline & operator = (BasePipeline && rhs) noexcept = delete;

    virtual void PreRender(
        RT::DrawPass & drawPass,
        float deltaTime
    );

    virtual void Render(        
        RT::DrawPass & drawPass,
        float deltaTime
    );

    virtual void PostRender(
        RT::DrawPass & drawPass,
        float deltaTime
    );

    virtual void CreateDrawableEssence(
        char const * essenceName, 
        RT::GpuModel const & gpuModel
    );

    virtual DrawableVariant * CreateDrawableVariant(char const * essenceName);

    void RemoveDrawableVariant(DrawableVariant * variant);

    virtual void OnResize() = 0;

protected:

    virtual void Init();

    virtual void Shutdown();

    std::unordered_map<std::string, std::unique_ptr<EssenceAndItsVariants>> mEssenceAndVariantsMap;

};

}
