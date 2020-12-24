#include "RenderBackend.hpp"

#include "BedrockPlatforms.hpp"

namespace MFA::RenderBackend {

SDL_Window * CreateWindow(U32 screen_width, U32 screen_height) {
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS);
    auto const screen_info = MFA::Platforms::ComputeScreenSize();
    return SDL_CreateWindow(
        "VULKAN_ENGINE", 
        static_cast<uint32_t>((screen_info.screen_width / 2.0f) - (screen_width / 2.0f)), 
        static_cast<uint32_t>((screen_info.screen_height / 2.0f) - (screen_height / 2.0f)),
        screen_width, screen_height,
        SDL_WINDOW_SHOWN /*| SDL_WINDOW_FULLSCREEN */| SDL_WINDOW_VULKAN
    );
}

}