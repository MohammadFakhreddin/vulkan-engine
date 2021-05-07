#pragma once

#include "engine/RenderFrontend.hpp"

namespace MFA {

namespace RB = RenderBackend;
namespace RF = RenderFrontend;

class BasePipeline {
public:
    virtual ~BasePipeline() = default;
    virtual void render(        
        RF::DrawPass & drawPass, 
        uint32_t idsCount, 
        DrawableObjectId * ids
    ) = 0;
};

};