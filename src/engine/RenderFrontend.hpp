#pragma once

#include "RenderBackend.hpp"

namespace MFA::RenderFrontend {

using ScreenWidth = RenderBackend::ScreenWidth;
using ScreenHeight = RenderBackend::ScreenHeight;

struct InitParams {
    ScreenWidth screen_width;
    ScreenHeight screen_height;
    char const * application_name;
};

bool Init(InitParams const & params);
// TODO Implement this function
bool Shutdown();

// TODO OnResize

// TODO CreateDrawPipeline + asking for descriptor set layout required bindings

// TODO DestroyDrawPipeline

// TODO CreateUniformBuffer

// TODO UpdateUniformBuffer

// TODO DestroyUniformBuffer

// TODO UpdateDescriptorSet

}