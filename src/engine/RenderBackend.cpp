#include "RenderBackend.hpp"

#include <vector>

#include "BedrockAssert.hpp"
#include "BedrockLog.hpp"
#include "BedrockPlatforms.hpp"
#include "BedrockMath.hpp"

namespace MFA::RenderBackend {

inline static constexpr char EngineName[256] = "MFA";
inline static constexpr int EngineVersion = 1;
inline static constexpr char DebugLayer[256] = "VK_LAYER_LUNARG_standard_validation";

static void VK_Check(VkResult const result) {
  if(result != VK_SUCCESS) {
      MFA_CRASH("Vulkan command failed");
  }
}

static void SDL_Check(SDL_bool const result){
  if(result != SDL_TRUE) {
      MFA_CRASH("SDL command failed");
  }
}

SDL_Window * CreateWindow(ScreenWidth const screen_width, ScreenHeight const screen_height) {
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

[[nodiscard]]
VkExtent2D ChooseSwapChainExtent(
    VkSurfaceCapabilitiesKHR const & surface_capabilities, 
    ScreenWidth const screen_width, 
    ScreenHeight const screen_height
) {
    if (surface_capabilities.currentExtent.width <= 0) {
        VkExtent2D const swap_chain_extent = {
            Math::Min(
                Math::Max(screen_width, surface_capabilities.minImageExtent.width), 
                surface_capabilities.maxImageExtent.width
            ),
            Math::Min(
                Math::Max(screen_height, surface_capabilities.minImageExtent.height), 
                surface_capabilities.maxImageExtent.height
            )
        };
        return swap_chain_extent;
    }
    return surface_capabilities.currentExtent;
}

[[nodiscard]]
VkPresentModeKHR ChoosePresentMode(
    uint8_t const present_modes_count, 
    VkPresentModeKHR const * present_modes
) {
    for(uint8_t index = 0; index < present_modes_count; index ++) {
        if (present_modes[index] == VK_PRESENT_MODE_MAILBOX_KHR) {
            return present_modes[index];
        } 
    }
    // If mailbox is unavailable, fall back to FIFO (guaranteed to be available)
    return VK_PRESENT_MODE_FIFO_KHR;
}

VkSurfaceFormatKHR ChooseSurfaceFormat(
    uint8_t const available_formats_count,
    VkSurfaceFormatKHR const * available_formats
) {
    // We can either choose any format
    if (available_formats_count == 1 && available_formats[0].format == VK_FORMAT_UNDEFINED) {
        return { VK_FORMAT_R8G8B8A8_UNORM, VK_COLORSPACE_SRGB_NONLINEAR_KHR };
    }

    // Or go with the standard format - if available
    for(uint8_t index = 0; index < available_formats_count; index ++) {
        if (available_formats[index].format == VK_FORMAT_R8G8B8A8_UNORM) {
            return available_formats[index];
        }
    }

    // Or fall back to the first available one
    return available_formats[0];
}

VkInstance CreateInstance(char const * application_name, SDL_Window * window) {
    // Filling out application description:
    VkApplicationInfo application_info = {};
    {
        // sType is mandatory
        application_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        // pNext is mandatory
        application_info.pNext = nullptr;
        // The name of our application
        application_info.pApplicationName = application_name;
        // The name of the engine (e.g: Game engine name)
        application_info.pEngineName = EngineName;
        // The version of the engine
        application_info.engineVersion = EngineVersion;
        // The version of Vulkan we're using for this application
        application_info.apiVersion = VK_API_VERSION_1_1;
    }
    std::vector<const char *> instance_extensions {};
    {// Filling sdl extensions
        unsigned int sdl_extenstion_count = 0;
        SDL_Check(SDL_Vulkan_GetInstanceExtensions(window, &sdl_extenstion_count, nullptr));
        instance_extensions.resize(sdl_extenstion_count);
        SDL_Vulkan_GetInstanceExtensions(
            window,
            &sdl_extenstion_count,
            instance_extensions.data()
        );
        instance_extensions.emplace_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
    }
    {// Checking for extension support
        uint32_t vk_supported_extension_count = 0;
        VK_Check(vkEnumerateInstanceExtensionProperties(
            nullptr,
            &vk_supported_extension_count,
            nullptr
        ));
        if (vk_supported_extension_count == 0) {
            MFA_CRASH("no extensions supported!");
        }
    }
    // Filling out instance description:
    VkInstanceCreateInfo instanceInfo = {};
    {
        std::vector<char const *> debug_layers {};
        debug_layers.emplace_back(DebugLayer);
        // sType is mandatory
        instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        // pNext is mandatory
        instanceInfo.pNext = nullptr;
        // flags is mandatory
        instanceInfo.flags = 0;
        // The application info structure is then passed through the instance
        instanceInfo.pApplicationInfo = &application_info;
        instanceInfo.enabledLayerCount = static_cast<uint32_t>(debug_layers.size());
        instanceInfo.ppEnabledLayerNames = debug_layers.data();
        instanceInfo.enabledExtensionCount = static_cast<uint32_t>(instance_extensions.size());
        instanceInfo.ppEnabledExtensionNames = instance_extensions.data();
    }
    VkInstance vk_instance;
    VK_Check(vkCreateInstance(&instanceInfo, nullptr, &vk_instance));
    return vk_instance;
}

VkDebugReportCallbackEXT CreateDebugCallback(
    VkInstance * vk_instance,
    PFN_vkDebugReportCallbackEXT const & debug_callback
) {
    MFA_PTR_ASSERT(vk_instance);
    VkDebugReportCallbackCreateInfoEXT debug_info = {};
    debug_info.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
    debug_info.pfnCallback = debug_callback;
    debug_info.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
    auto const debug_report_callback = reinterpret_cast<PFN_vkCreateDebugReportCallbackEXT>(vkGetInstanceProcAddr(
        *vk_instance,
        "vkCreateDebugReportCallbackEXT"
    ));
    VkDebugReportCallbackEXT debug_report_callback_ext;
    VK_Check(debug_report_callback(
        *vk_instance, 
        &debug_info, 
        nullptr, 
        &debug_report_callback_ext
    ));
    return debug_report_callback_ext;
}

U32 FindMemoryType (
    VkPhysicalDevice * physical_device,
    U32 const type_filter, 
    VkMemoryPropertyFlags const property_flags
) {
    MFA_PTR_ASSERT(physical_device);
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(*physical_device, &memProperties);

    for (uint32_t memory_type_index = 0; memory_type_index < memProperties.memoryTypeCount; memory_type_index++) {
        if ((type_filter & (1 << memory_type_index)) && (memProperties.memoryTypes[memory_type_index].propertyFlags & property_flags) == property_flags) {
            return memory_type_index;
        }
    }

    MFA_CRASH("failed to find suitable memory type!");
}

bool CreateImageView (
    VkImageView * out_image_view,
    VkDevice * device,
    VkImage const & image, 
    VkFormat const format, 
    VkImageAspectFlags const aspect_flags
) {
    bool ret = true;
    MFA_PTR_ASSERT(device);
    VkImageViewCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    createInfo.image = image;
    createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    createInfo.format = format;
    createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.subresourceRange.aspectMask = aspect_flags;
    createInfo.subresourceRange.baseMipLevel = 0;
    createInfo.subresourceRange.levelCount = 1;
    createInfo.subresourceRange.baseArrayLayer = 0;
    createInfo.subresourceRange.layerCount = 1;
    if (vkCreateImageView(*device, &createInfo, nullptr, out_image_view) != VK_SUCCESS) {
        ret = false;
    }
    return ret;
}

[[nodiscard]]
VkCommandBuffer BeginSingleTimeCommands(VkDevice * device, VkCommandPool const & command_pool) {
    MFA_PTR_ASSERT(device);
    
    VkCommandBufferAllocateInfo allocate_info{};
    allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocate_info.commandPool = command_pool;
    allocate_info.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(*device, &allocate_info, &commandBuffer);

    VkCommandBufferBeginInfo begin_info{};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &begin_info);

    return commandBuffer;
}

void EndAndSubmitSingleTimeCommand(
    VkDevice * device, 
    VkCommandPool const & command_pool, 
    VkQueue const & graphic_queue, 
    VkCommandBuffer const & command_buffer
) {
    VK_Check(vkEndCommandBuffer(command_buffer));

    VkSubmitInfo submit_info {};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &command_buffer;

    VK_Check(vkQueueSubmit(graphic_queue, 1, &submit_info, nullptr));
    VK_Check(vkQueueWaitIdle(graphic_queue));

    vkFreeCommandBuffers(*device, command_pool, 1, &command_buffer);
}

[[nodiscard]]
VkFormat FindDepthFormat(VkPhysicalDevice * physical_device) {
    std::vector<VkFormat> candidate_formats {
        VK_FORMAT_D32_SFLOAT,
        VK_FORMAT_D32_SFLOAT_S8_UINT,
        VK_FORMAT_D24_UNORM_S8_UINT
    };
    return FindSupportedFormat (
        physical_device,
        candidate_formats.size(),
        candidate_formats.data(),
        VK_IMAGE_TILING_OPTIMAL,
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
    );
}

[[nodiscard]]
VkFormat FindSupportedFormat(
    VkPhysicalDevice * physical_device,
    uint8_t const candidates_count, 
    VkFormat * candidates,
    VkImageTiling const tiling, 
    VkFormatFeatureFlags const features
) {
    for(uint8_t index = 0; index < candidates_count; index ++) {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(*physical_device, candidates[index], &props);

        if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
            return candidates[index];
        }
        if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
            return candidates[index];
        }
    }
    MFA_CRASH("Failed to find supported format!");
}

}