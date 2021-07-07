#pragma once

#include "engine/render_system/DrawableObject.hpp"
#include "engine/render_system/RenderFrontend.hpp"

namespace MFA {

namespace RB = RenderBackend;
namespace RF = RenderFrontend;

class BasePipeline {
public:
    
    virtual ~BasePipeline() = default;

    virtual void render(        
        RF::DrawPass & drawPass,
        float deltaTime,
        uint32_t idsCount, 
        DrawableObjectId * ids
    ) = 0;

    virtual DrawableObjectId addGpuModel(RF::GpuModel & gpuModel) = 0;

    virtual bool removeGpuModel(DrawableObjectId drawableObjectId) = 0;

};

};