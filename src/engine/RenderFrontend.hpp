#pragma once

#include "RenderBackend.hpp"

namespace MFA::RenderFrontend {

namespace RB = RenderBackend;
using ScreenWidth = RB::ScreenWidth;
using ScreenHeight = RB::ScreenHeight;

struct InitParams {
    ScreenWidth screen_width;
    ScreenHeight screen_height;
    char const * application_name;
};

bool Init(InitParams const & params);
// TODO Implement this function
bool Shutdown();

bool Resize(ScreenWidth screen_width, ScreenHeight screen_height);

// TODO CreateDrawPipeline + asking for descriptor set layout required bindings

// TODO DestroyDrawPipeline

// TODO CreateUniformBuffer

// TODO UpdateUniformBuffer

// TODO DestroyUniformBuffer

// TODO UpdateDescriptorSet

}