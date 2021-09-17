#pragma once

#include "engine/render_system/RenderTypesFWD.hpp"

#include <string>
#include <memory>
#include <unordered_map>
#include <vector>

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
        std::unique_ptr<DrawableEssence> essence;
        std::vector<std::unique_ptr<DrawableVariant>> variants;
        explicit EssenceAndItsVariants(std::unique_ptr<DrawableEssence> && essence);
    };

    std::unordered_map<std::string, std::unique_ptr<EssenceAndItsVariants>> mEssenceAndVariantsMap;

};

}
