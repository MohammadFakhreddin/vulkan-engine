#pragma once

#include "engine/render_system/RenderTypesFWD.hpp"

namespace MFA {

class DrawableEssence;
class DrawableVariant;

class BasePipeline {
public:

    explicit BasePipeline();

    virtual ~BasePipeline();

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
    ) {}

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

    struct EssenceAndItsVariants {
        DrawableEssence * essence;
        std::vector<DrawableVariant *> variants;
        explicit EssenceAndItsVariants();
        explicit EssenceAndItsVariants(DrawableEssence * essence);
    };

    std::unordered_map<std::string, EssenceAndItsVariants> mEssenceAndVariantsMap;

};

}
