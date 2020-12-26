#pragma once

#include "BedrockCommon.hpp"

// Note: Not all functions can be called from outside
// TODO Remove functions that are not usable from outside
namespace MFA::RenderBackend {

#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>
#include <vulkan/vulkan.h>

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
VkPresentModeKHR ChoosePresentMode(uint8_t present_modes_count, VkPresentModeKHR const * present_modes);

[[nodiscard]]
VkSurfaceFormatKHR ChooseSurfaceFormat(uint8_t available_formats_count, VkSurfaceFormatKHR const * available_formats);

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

[[nodiscard]]
VkCommandBuffer BeginSingleTimeCommand(VkDevice * device, VkCommandPool const & command_pool);

void EndAndSubmitSingleTimeCommand(
    VkDevice * device, 
    VkCommandPool const & command_pool, 
    VkQueue const & graphic_queue, 
    VkCommandBuffer const & command_buffer
);

[[nodiscard]]
VkFormat FindDepthFormat(VkPhysicalDevice * physical_device);

[[nodiscard]]
VkFormat FindSupportedFormat(
    VkPhysicalDevice * physical_device,
    uint8_t candidates_count, 
    VkFormat * candidates,
    VkImageTiling tiling, 
    VkFormatFeatureFlags features
);

}
