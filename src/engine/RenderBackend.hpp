#pragma once

#include "BedrockCommon.hpp"
#include "FoundationAsset.hpp"

#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>
#include <vulkan/vulkan.h>
#include <functional>

// Note: Not all functions can be called from outside
// TODO Remove functions that are not usable from outside
namespace MFA::RenderBackend {

using ScreenWidth = Platforms::ScreenSizeType;
using ScreenHeight = Platforms::ScreenSizeType;
using CpuTexture = Asset::TextureAsset;

[[nodiscard]]
SDL_Window * CreateWindow(ScreenWidth screen_width, ScreenHeight screen_height);

[[nodiscard]]
VkSurfaceKHR_T * CreateWindowSurface(SDL_Window * window, VkInstance_T * instance);

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

struct CreateBufferResult {
    VkBuffer_T * buffer = nullptr;
    VkDeviceMemory_T * buffer_memory = nullptr;
};
[[nodiscard]]
CreateBufferResult CreateBuffer(
    VkDevice_T * device,
    VkPhysicalDevice_T * physical_device,
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

struct CreateImageResult {
    VkImage_T * image = nullptr;
    VkDeviceMemory_T * image_memory = nullptr;
};
[[nodiscard]]
CreateImageResult CreateImage(
    VkDevice_T * device,
    VkPhysicalDevice_T * physical_device,
    U32 width, 
    U32 height,
    U32 depth,
    U8 mip_levels,
    U16 slice_count,
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

[[nodiscard]]
GpuTexture CreateTexture(
    CpuTexture & cpu_texture,
    VkDevice_T * device,
    VkPhysicalDevice_T * physical_device,
    VkQueue const & graphic_queue,
    VkCommandPool_T * command_pool
);

bool DestroyTexture(GpuTexture & gpu_texture);

// TODO It needs handle system // TODO Might be moved to a new class called render_types
class GpuTexture {
friend GpuTexture CreateTexture(
    CpuTexture & cpu_texture,
    VkDevice_T * device,
    VkPhysicalDevice_T * physical_device,
    VkQueue const & graphic_queue,
    VkCommandPool_T * command_pool
);
friend bool DestroyTexture(GpuTexture & gpu_texture);
public:
    ~GpuTexture() {
        DestroyTexture(*this);
    }
    [[nodiscard]]
    CpuTexture const * cpu_texture() const {return &m_cpu_texture;}
    [[nodiscard]]
    bool valid () const {return m_is_valid;}
    [[nodiscard]]
    VkImage_T const * image() const {return m_image;}
    [[nodiscard]]
    VkImageView_T * image_view() const {return m_image_view;}
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

void CopyBufferToImage(
    VkDevice_T * device,
    VkCommandPool_T * command_pool,
    VkBuffer_T * buffer,
    VkImage_T * image,
    VkQueue_T * graphic_queue,
    CpuTexture const & cpu_texture
);

struct CreateLogicalDeviceResult {
    VkDevice_T * device;
    VkPhysicalDeviceMemoryProperties physical_memory_properties;
};
[[nodiscard]]
CreateLogicalDeviceResult CreateLogicalDevice(
    VkPhysicalDevice_T * physical_device,
    U32 graphics_queue_family,
    VkQueue_T * graphic_queue,
    U32 present_queue_family,
    VkQueue_T * present_queue,
    VkPhysicalDeviceFeatures const & enabled_physical_device_features
);

// TODO This function should ask for options
[[nodiscard]]
VkSampler_T * CreateSampler(VkDevice_T * device);

void DestroySampler(VkDevice_T * device, VkSampler_T * sampler);

using DebugCallback = std::function<VkBool32(
    VkDebugReportFlagsEXT flags,
    VkDebugReportObjectTypeEXT object_type,
    uint64_t src_object, 
    size_t location,
    int32_t message_code,
    char const * player_prefix,
    char const * message,
    void * user_data
)>;
[[nodiscard]]
VkDebugReportCallbackEXT_T * SetDebugCallback(VkInstance_T * instance, DebugCallback const & callback);

// TODO Might need to ask features from outside instead
struct FindPhysicalDeviceResult {
    VkPhysicalDevice_T * physical_device = nullptr;
    VkPhysicalDeviceFeatures physical_device_features {};
};
[[nodiscard]]
FindPhysicalDeviceResult FindPhysicalDevice(uint8_t retry_count);

}
