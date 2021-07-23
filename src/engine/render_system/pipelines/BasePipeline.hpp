#pragma once

#include "engine/render_system/DrawableObject.hpp"
#include "engine/render_system/RenderFrontend.hpp"

namespace MFA {

namespace RB = RenderBackend;
namespace RF = RenderFrontend;

class BasePipeline {
public:
    
    virtual ~BasePipeline() = default;

    virtual void Update(
        RF::DrawPass & drawPass,
        float deltaTime,
        uint32_t idsCount, 
        DrawableObjectId * ids
    ) {};

    virtual void Render(        
        RF::DrawPass & drawPass,
        float deltaTime,
        uint32_t idsCount, 
        DrawableObjectId * ids
    ) = 0;

    virtual DrawableObjectId AddGpuModel(RF::GpuModel & gpuModel) = 0;

    virtual bool RemoveGpuModel(DrawableObjectId drawableObjectId) = 0;

    virtual void OnResize() = 0;

};

};