#pragma once

#include "BedrockCommon.hpp"
#include "FoundationAsset.hpp"

// Note: Not all functions can be called from outside
// TODO Remove functions that are not usable from outside
namespace MFA::RenderBackend {

#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>
#include <vulkan/vulkan.h>

using ScreenWidth = Platforms::ScreenSizeType;
using ScreenHeight = Platforms::ScreenSizeType;
using CpuTexture = Asset::TextureAsset;

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
VkInstance_T * CreateInstance(char const * application_name, SDL_Window * window);

[[nodiscard]]
VkDebugReportCallbackEXT CreateDebugCallback(
    VkInstance_T * vk_instance,
    PFN_vkDebugReportCallbackEXT const & debug_callback
);

[[nodiscard]]
U32 FindMemoryType (
    VkPhysicalDevice * physical_device,
    U32 type_filter, 
    VkMemoryPropertyFlags property_flags
);

[[nodiscard]]
VkImageView_T * CreateImageView (
    VkImage_T const & image, 
    VkFormat format, 
    VkImageAspectFlags const aspect_flags
);

[[nodiscard]]
VkCommandBuffer BeginSingleTimeCommand(VkDevice_T * device, VkCommandPool const & command_pool);

void EndAndSubmitSingleTimeCommand(
    VkDevice * device, 
    VkCommandPool const & command_pool, 
    VkQueue const & graphic_queue, 
    VkCommandBuffer const & command_buffer
);

[[nodiscard]]
VkFormat FindDepthFormat(VkPhysicalDevice_T * physical_device);

[[nodiscard]]
VkFormat FindSupportedFormat(
    VkPhysicalDevice_T * physical_device,
    uint8_t candidates_count, 
    VkFormat * candidates,
    VkImageTiling tiling, 
    VkFormatFeatureFlags features
);

void TransferImageLayout(
    VkDevice_T * device,
    VkQueue const & graphic_queue,
    VkCommandPool const & command_pool,
    VkImage const & image, 
    VkImageLayout old_layout, 
    VkImageLayout new_layout
);

void CreateBuffer(
    VkBuffer_T * & out_buffer, 
    VkDeviceMemory_T * & out_buffer_memory,
    VkDevice_T * device,
    VkDeviceSize size, 
    VkBufferUsageFlags usage, 
    VkMemoryPropertyFlags properties
);

void MapDataToBuffer(
    VkDevice_T * device,
    VkDeviceMemory_T * buffer_memory,
    CBlob data_blob
);

void DestroyBuffer(
    VkDevice_T * device,
    VkBuffer_T * buffer,
    VkDeviceMemory_T * memory
);

void CreateImage(
    VkImage_T * & out_image, 
    VkDeviceMemory_T * & out_image_memory,
    VkDevice_T * device,
    VkPhysicalDevice_T * physical_device,
    U32 width, 
    U32 height,
    U32 depth,
    U8 mip_levels,
    U8 slice_count,
    VkFormat format, 
    VkImageTiling tiling, 
    VkImageUsageFlags usage, 
    VkMemoryPropertyFlags properties
);

void DestroyImage(
    VkImage_T * image,
    VkDeviceMemory_T * memory,
    VkDevice_T * device
);

class GpuTexture;

GpuTexture CreateTexture(CpuTexture & cpu_texture, VkDevice_T * device);

bool DestroyTexture(GpuTexture & gpu_texture);

// TODO It needs handle system // TODO Might be moved to a new class called render_types
class GpuTexture {
friend GpuTexture CreateTexture(CpuTexture & cpu_texture, VkDevice_T * device);
friend bool DestroyTexture(GpuTexture & gpu_texture);
public:
    ~GpuTexture() {
        DestroyTexture(*this);
    };
    CpuTexture const * cpu_texture() const {return &m_cpu_texture;};
    bool valid () const {return m_is_valid;};
    VkImage_T * const image() const {return m_image;};
    VkImageView_T * image_view() const {return m_image_view;};
private:
    VkDevice_T * m_device = nullptr;
    VkImage_T * m_image = nullptr;
    VkDeviceMemory_T * m_image_memory = nullptr;
    VkImageView_T * m_image_view = nullptr;
    CpuTexture m_cpu_texture {};
    bool m_is_valid = false;
};

[[nodiscard]]
VkFormat ConvertCpuTextureFormatToGpu(Asset::TextureFormat cpu_format);

}
