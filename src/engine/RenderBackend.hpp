#pragma once

#include "BedrockCommon.hpp"

#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>
#include <vulkan/vulkan.h>

namespace MFA::RenderBackend {

[[nodiscard]]
SDL_Window * CreateWindow(U32 screen_width, U32 screen_height);

}
