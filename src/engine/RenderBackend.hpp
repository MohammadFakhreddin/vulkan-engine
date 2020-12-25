#pragma once

#include "BedrockCommon.hpp"

#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>
#include <vulkan/vulkan.h>
#include <vector>
// Note: Not all functions can be called from outside
// TODO Remove functions that are not usable from outside
namespace MFA::RenderBackend {

using ScreenWidth = Platforms::ScreenSizeType;
using ScreenHeight = Platforms::ScreenSizeType;

[[nodiscard]]
SDL_Window * CreateWindow(ScreenWidth screen_width, ScreenHeight screen_height);

[[nodiscard]]
VkExtent2D ChooseSwapChainExtent(
    VkSurfaceCapabilitiesKHR const & surface_capabilities, 
    ScreenWidth screen_width, 
    ScreenHeight screen_height
);

[[nodiscard]]
VkPresentModeKHR ChoosePresentMode(std::vector<VkPresentModeKHR> const & present_modes);

[[nodiscard]]
VkSurfaceFormatKHR ChooseSurfaceFormat(std::vector<VkSurfaceFormatKHR> const & available_formats);

[[nodiscard]]
VkInstance CreateInstance(char const * application_name, SDL_Window * window);

[[nodiscard]]
VkDebugReportCallbackEXT CreateDebugCallback(
    VkInstance * vk_instance,
    PFN_vkDebugReportCallbackEXT const & debug_callback
);

[[nodiscard]]
U32 FindMemoryType (
    VkPhysicalDevice * physical_device,
    U32 type_filter, 
    VkMemoryPropertyFlags property_flags
);

bool CreateImageView (
    VkImageView * out_image_view, 
    VkImage const & image, 
    VkFormat const format, 
    VkImageAspectFlags const aspect_flags
);

}
