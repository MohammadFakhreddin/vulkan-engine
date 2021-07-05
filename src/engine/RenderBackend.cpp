#include "RenderBackend.hpp"

#include "BedrockAssert.hpp"
#include "BedrockLog.hpp"
#include "BedrockPlatforms.hpp"
#include "BedrockMath.hpp"
#include "BedrockMemory.hpp"

#include "libs/sdl/SDL.hpp"

#ifdef __IOS__
#include <vulkan/vulkan_ios.h>
#endif

#include <vector>
#include <cstring>

namespace MFA::RenderBackend {

inline static constexpr char EngineName[256] = "MFA";
inline static constexpr int EngineVersion = 1;
inline static std::string ValidationLayer = "VK_LAYER_KHRONOS_validation";
std::vector<char const *> DebugLayers = {
    ValidationLayer.c_str()
};

static void VK_Check(VkResult const result) {
  if(result != VK_SUCCESS) {
      MFA_CRASH("Vulkan command failed with code:" + std::to_string(result));
  }
}

#ifdef __DESKTOP__
static void SDL_Check(MSDL::SDL_bool const result) {
  if(result != MSDL::SDL_TRUE) {\
      MFA_CRASH("SDL command failed");
  }
}

MSDL::SDL_Window * CreateWindow(ScreenWidth const screenWidth, ScreenHeight const screenHeight) {
    MSDL::SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS);
    auto const screen_info = MFA::Platforms::ComputeScreenSize();
    auto * window = MSDL::SDL_CreateWindow(
        "VULKAN_ENGINE", 
        static_cast<uint32_t>((static_cast<float>(screen_info.screen_width) / 2.0f) - (static_cast<float>(screenWidth) / 2.0f)), 
        static_cast<uint32_t>((static_cast<float>(screen_info.screen_height) / 2.0f) - (static_cast<float>(screenHeight) / 2.0f)),
        screenWidth, screenHeight,
        MSDL::SDL_WINDOW_SHOWN /*| SDL_WINDOW_FULLSCREEN */| MSDL::SDL_WINDOW_VULKAN
    );
    return window;
}

void DestroyWindow(SDL_Window * window) {
    MFA_ASSERT(window != nullptr);
    MSDL::SDL_DestroyWindow(window);
}
#endif

#ifdef __DESKTOP__
VkSurfaceKHR CreateWindowSurface(SDL_Window * window, VkInstance_T * instance) {
    VkSurfaceKHR ret {};
    SDL_Check(MSDL::SDL_Vulkan_CreateSurface(
        window,
        instance,
        &ret
    ));
    return ret;
}
#elif defined(__ANDROID__)
VkSurfaceKHR CreateWindowSurface(ANativeWindow * window, VkInstance_T * instance) {
    VkSurfaceKHR surface {};

    VkAndroidSurfaceCreateInfoKHR createInfo {
        .sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR,
        .pNext = nullptr,
        .flags = 0,
        .window = window
    };

    VK_Check(vkCreateAndroidSurfaceKHR(
        instance,
        &createInfo,
        nullptr,
        &surface
    ));

    return surface;
}
#elif defined(__IOS__)
VkSurfaceKHR CreateWindowSurface(void * view, VkInstance_T * instance) {
    VkSurfaceKHR surface {};

    VkIOSSurfaceCreateInfoMVK const surfaceCreateInfo {
        .sType = VK_STRUCTURE_TYPE_IOS_SURFACE_CREATE_INFO_MVK,
        .pNext = nullptr,
        .flags = 0,
        .pView = view
    };
    VK_Check(vkCreateIOSSurfaceMVK(instance, &surfaceCreateInfo, nullptr, &surface));
    
    return surface;
}
#else
    #error Os is not handled
#endif

void DestroyWindowSurface(VkInstance instance, VkSurfaceKHR surface) {
    MFA_ASSERT(instance != nullptr);
    MFA_VK_VALID_ASSERT(surface);
    vkDestroySurfaceKHR(instance, surface, nullptr);
}

[[nodiscard]]
VkExtent2D ChooseSwapChainExtent(
    VkSurfaceCapabilitiesKHR const & surfaceCapabilities, 
    ScreenWidth const screenWidth, 
    ScreenHeight const screenHeight
) {
    if (surfaceCapabilities.currentExtent.width <= 0) {
        VkExtent2D const swap_chain_extent = {
            Math::Min(
                Math::Max(
                    static_cast<uint32_t>(screenWidth), 
                    surfaceCapabilities.minImageExtent.width
                ), 
                surfaceCapabilities.maxImageExtent.width
            ),
            Math::Min(
                Math::Max(
                    static_cast<uint32_t>(screenHeight), 
                    surfaceCapabilities.minImageExtent.height
                ), 
                surfaceCapabilities.maxImageExtent.height
            )
        };
        return swap_chain_extent;
    }
    return surfaceCapabilities.currentExtent;
}

[[nodiscard]]
VkPresentModeKHR ChoosePresentMode(
    uint8_t const presentModesCount, 
    VkPresentModeKHR const * present_modes
) {
    for(uint8_t index = 0; index < presentModesCount; index ++) {
        if (present_modes[index] == VK_PRESENT_MODE_MAILBOX_KHR) {
            return present_modes[index];
        } 
    }
    // If mailbox is unavailable, fall back to FIFO (guaranteed to be available)
    return VK_PRESENT_MODE_FIFO_KHR;
}

VkSurfaceFormatKHR ChooseSurfaceFormat(
    uint8_t const availableFormatsCount,
    VkSurfaceFormatKHR const * availableFormats
) {
    // We can either choose any format
    if (availableFormatsCount == 1 && availableFormats[0].format == VK_FORMAT_UNDEFINED) {
        return { VK_FORMAT_R8G8B8A8_UNORM, VK_COLORSPACE_SRGB_NONLINEAR_KHR };
    }

    // Or go with the standard format - if available
    for(uint8_t index = 0; index < availableFormatsCount; index ++) {
        if (availableFormats[index].format == VK_FORMAT_R8G8B8A8_UNORM) {
            return availableFormats[index];
        }
    }

    // Or fall back to the first available one
    return availableFormats[0];
}

VkApplicationInfo applicationInfo;
VkInstanceCreateInfo instanceInfo;

#ifdef __DESKTOP__
VkInstance CreateInstance(char const * applicationName, SDL_Window * window) {
#elif defined(__ANDROID__) || defined(__IOS__)
VkInstance CreateInstance(char const * applicationName) {
#else
#error Os not handled
#endif
    // Filling out application description:
    applicationInfo = VkApplicationInfo {
        // sType is mandatory
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        // pNext is mandatory
        .pNext = nullptr,
        // The name of our application
        .pApplicationName = applicationName,
        .applicationVersion = 1,    // TODO Ask this as parameter
        // The name of the engine (e.g: Game engine name)
        .pEngineName = EngineName,
        // The version of the engine
        .engineVersion = EngineVersion,
        // The version of Vulkan we're using for this application
        .apiVersion = VK_API_VERSION_1_1,
    };
    std::vector<char const *> instanceExtensions {};

#ifdef __DESKTOP__
    {// Filling sdl extensions
        unsigned int sdl_extenstion_count = 0;
        SDL_Check(MSDL::SDL_Vulkan_GetInstanceExtensions(window, &sdl_extenstion_count, nullptr));
        instanceExtensions.resize(sdl_extenstion_count);
        SDL_Check(MSDL::SDL_Vulkan_GetInstanceExtensions(
            window,
            &sdl_extenstion_count,
            instanceExtensions.data()
        ));
    }
#elif defined(__ANDROID__)
    // Filling android extensions
    instanceExtensions.emplace_back("VK_KHR_surface");
    instanceExtensions.emplace_back("VK_KHR_android_surface");
#elif defined(__IOS__)
    // Filling IOS extensions
    instanceExtensions.emplace_back(VK_KHR_SURFACE_EXTENSION_NAME);
    instanceExtensions.emplace_back(VK_MVK_IOS_SURFACE_EXTENSION_NAME);
#else
    #error Os not handled
#endif
    instanceExtensions.emplace_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
//    // TODO We should enumarate layers before using them (Both desktop and android)
//    {// Checking for extension support
//        uint32_t vk_supported_extension_count = 0;
//        VK_Check(vkEnumerateInstanceExtensionProperties(
//            nullptr,
//            &vk_supported_extension_count,
//            nullptr
//        ));
//        if (vk_supported_extension_count == 0) {
//            MFA_CRASH("no extensions supported!");
//        }
//    }
    // Filling out instance description:
    instanceInfo = VkInstanceCreateInfo {
        // sType is mandatory
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        // pNext is mandatory
        .pNext = nullptr,
        // flags is mandatory
        .flags = 0,
        // The application info structure is then passed through the instance
        .pApplicationInfo = &applicationInfo,
#if defined(MFA_DEBUG)
        .enabledLayerCount = static_cast<uint32_t>(DebugLayers.size()),
        .ppEnabledLayerNames = DebugLayers.data(),
#else
        .enabledLayerCount = 0,
        .ppEnabledLayerNames = nullptr,
#endif
        .enabledExtensionCount = static_cast<uint32_t>(instanceExtensions.size()),
        .ppEnabledExtensionNames = instanceExtensions.data()
    };
    VkInstance instance = nullptr;
    VK_Check(vkCreateInstance(&instanceInfo, nullptr, &instance));
    return instance;
}

void DestroyInstance(VkInstance instance) {
    MFA_ASSERT(instance != nullptr);
    vkDestroyInstance(instance, nullptr);
}

VkDebugReportCallbackEXT CreateDebugCallback(
    VkInstance vkInstance,
    PFN_vkDebugReportCallbackEXT const & debugCallback
) {
    MFA_ASSERT(vkInstance != nullptr);
    VkDebugReportCallbackCreateInfoEXT debug_info = {};
    debug_info.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
    debug_info.pfnCallback = debugCallback;
    debug_info.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | 
        VK_DEBUG_REPORT_WARNING_BIT_EXT | 
        VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;
    auto const debug_report_callback = reinterpret_cast<PFN_vkCreateDebugReportCallbackEXT>(vkGetInstanceProcAddr(
        vkInstance,
        "vkCreateDebugReportCallbackEXT"
    ));
    VkDebugReportCallbackEXT debug_report_callback_ext;
    VK_Check(debug_report_callback(
        vkInstance, 
        &debug_info, 
        nullptr, 
        &debug_report_callback_ext
    ));
    return debug_report_callback_ext;
}

void DestroyDebugReportCallback(
    VkInstance instance,
    VkDebugReportCallbackEXT const & reportCallbackExt
) {
    MFA_ASSERT(instance != nullptr);
    auto const DestroyDebugReportCallback = reinterpret_cast<PFN_vkDestroyDebugReportCallbackEXT>(vkGetInstanceProcAddr(
        instance, 
        "vkDestroyDebugReportCallbackEXT"
    ));
    DestroyDebugReportCallback(instance, reportCallbackExt, nullptr);
}

uint32_t FindMemoryType (
    VkPhysicalDevice * physicalDevice,
    uint32_t const typeFilter, 
    VkMemoryPropertyFlags const propertyFlags
) {
    MFA_ASSERT(physicalDevice != nullptr);
    VkPhysicalDeviceMemoryProperties memory_properties;
    vkGetPhysicalDeviceMemoryProperties(*physicalDevice, &memory_properties);

    for (uint32_t memory_type_index = 0; memory_type_index < memory_properties.memoryTypeCount; memory_type_index++) {
        if ((typeFilter & (1 << memory_type_index)) && (memory_properties.memoryTypes[memory_type_index].propertyFlags & propertyFlags) == propertyFlags) {
            return memory_type_index;
        }
    }

    MFA_CRASH("failed to find suitable memory type!");
}

VkImageView CreateImageView (
    VkDevice device,
    VkImage const & image, 
    VkFormat const format, 
    VkImageAspectFlags const aspect_flags,
    uint32_t mipmap_count
) {
    MFA_ASSERT(device != nullptr);

    VkImageView image_view {};

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
    // TODO Might need to ask these values from image for mipmaps (Check again later)
    createInfo.subresourceRange.baseMipLevel = 0;
    createInfo.subresourceRange.levelCount = mipmap_count;
    createInfo.subresourceRange.baseArrayLayer = 0;
    createInfo.subresourceRange.layerCount = 1;

    VK_Check(vkCreateImageView(device, &createInfo, nullptr, &image_view));

    return image_view;
}

void DestroyImageView(
    VkDevice device,
    VkImageView image_view
) {
    vkDestroyImageView(device, image_view, nullptr);
}

VkCommandBuffer BeginSingleTimeCommand(VkDevice device, VkCommandPool const & command_pool) {
    MFA_ASSERT(device != nullptr);
    
    VkCommandBufferAllocateInfo allocate_info{};
    allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocate_info.commandPool = command_pool;
    allocate_info.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(device, &allocate_info, &commandBuffer);

    VkCommandBufferBeginInfo begin_info{};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &begin_info);

    return commandBuffer;
}

void EndAndSubmitSingleTimeCommand(
    VkDevice device, 
    VkCommandPool const & commandPool, 
    VkQueue const & graphicQueue, 
    VkCommandBuffer const & commandBuffer
) {
    VK_Check(vkEndCommandBuffer(commandBuffer));

    VkSubmitInfo submit_info {};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &commandBuffer;
#ifdef __ANDROID__
    VK_Check(vkQueueSubmit(graphicQueue, 1, &submit_info, 0));
#else
    VK_Check(vkQueueSubmit(graphicQueue, 1, &submit_info, nullptr));
#endif
    VK_Check(vkQueueWaitIdle(graphicQueue));

    vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
}

[[nodiscard]]
VkFormat FindDepthFormat(VkPhysicalDevice physical_device) {
    std::vector<VkFormat> candidate_formats {
        VK_FORMAT_D32_SFLOAT,
        VK_FORMAT_D32_SFLOAT_S8_UINT,
        VK_FORMAT_D24_UNORM_S8_UINT
    };
    return FindSupportedFormat (
        physical_device,
        static_cast<uint8_t>(candidate_formats.size()),
        candidate_formats.data(),
        VK_IMAGE_TILING_OPTIMAL,
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
    );
}

[[nodiscard]]
VkFormat FindSupportedFormat(
    VkPhysicalDevice physical_device,
    uint8_t const candidates_count, 
    VkFormat * candidates,
    VkImageTiling const tiling, 
    VkFormatFeatureFlags const features
) {
    for(uint8_t index = 0; index < candidates_count; index ++) {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(physical_device, candidates[index], &props);

        if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
            return candidates[index];
        }
        if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
            return candidates[index];
        }
    }
    MFA_CRASH("Failed to find supported format!");
}

void TransferImageLayout(
    VkDevice device,
    VkQueue graphicQueue,
    VkCommandPool commandPool,
    VkImage image, 
    VkImageLayout const oldLayout, 
    VkImageLayout const newLayout,
    uint32_t const levelCount,
    uint32_t const layerCount
) {
    MFA_ASSERT(device != nullptr);
    MFA_ASSERT(graphicQueue != nullptr);
    MFA_VK_VALID_ASSERT(commandPool);
    MFA_VK_VALID_ASSERT(image);

    auto const commandBuffer = BeginSingleTimeCommand(device, commandPool);

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = levelCount;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = layerCount;

    VkPipelineStageFlags source_stage;
    VkPipelineStageFlags destination_stage;

    if (
        oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && 
        newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
    ) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        source_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destination_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } else if (
        oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && 
        newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    ) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        source_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destination_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    } else {
        MFA_CRASH("unsupported layout transition!");
    }

    vkCmdPipelineBarrier(
        commandBuffer,
        source_stage, destination_stage,
        0,
        0, nullptr,
        0, nullptr,
        1, &barrier
    );

    EndAndSubmitSingleTimeCommand(device, commandPool, graphicQueue, commandBuffer);
}

BufferGroup CreateBuffer(
    VkDevice device,
    VkPhysicalDevice physicalDevice,
    VkDeviceSize const size, 
    VkBufferUsageFlags const usage, 
    VkMemoryPropertyFlags const properties 
) {
    MFA_ASSERT(device != nullptr);
    MFA_ASSERT(physicalDevice != nullptr);

    BufferGroup ret {};

    VkBufferCreateInfo buffer_info {};
    buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_info.size = size;
    buffer_info.usage = usage;
    buffer_info.sharingMode = VkSharingMode::VK_SHARING_MODE_EXCLUSIVE;

    VK_Check(vkCreateBuffer(device, &buffer_info, nullptr, &ret.buffer));
    MFA_VK_VALID_ASSERT(ret.buffer);
    VkMemoryRequirements memory_requirements {};
    vkGetBufferMemoryRequirements(device, ret.buffer, &memory_requirements);

    VkMemoryAllocateInfo memory_allocation_info {};
    memory_allocation_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memory_allocation_info.allocationSize = memory_requirements.size;
    memory_allocation_info.memoryTypeIndex = FindMemoryType(
        &physicalDevice, 
        memory_requirements.memoryTypeBits, 
        properties
    );

    VK_Check(vkAllocateMemory(device, &memory_allocation_info, nullptr, &ret.memory));
    VK_Check(vkBindBufferMemory(device, ret.buffer, ret.memory, 0));
    return ret;
}

void MapDataToBuffer(
    VkDevice device,
    VkDeviceMemory bufferMemory,
    CBlob const dataBlob
) {
    MFA_ASSERT(dataBlob.ptr != nullptr);
    MFA_ASSERT(dataBlob.len > 0);
    void * tempBufferData = nullptr;
    VK_Check(vkMapMemory(device, bufferMemory, 0, dataBlob.len, 0, &tempBufferData));
    MFA_ASSERT(tempBufferData != nullptr);
    ::memcpy(tempBufferData, dataBlob.ptr, dataBlob.len);
    vkUnmapMemory(device, bufferMemory);
}

void CopyBuffer(
    VkDevice device,
    VkCommandPool commandPool,
    VkQueue graphicQueue,
    VkBuffer sourceBuffer,
    VkBuffer destinationBuffer,
    VkDeviceSize const size
) {
    auto * command_buffer = BeginSingleTimeCommand(device, commandPool);

    VkBufferCopy copyRegion{};
    copyRegion.size = size;
    vkCmdCopyBuffer(
        command_buffer,
        sourceBuffer,
        destinationBuffer,
        1, 
        &copyRegion
    );

    EndAndSubmitSingleTimeCommand(device, commandPool, graphicQueue, command_buffer); 
}

void DestroyBuffer(
    VkDevice device,
    BufferGroup & bufferGroup
) {
    MFA_ASSERT(device != nullptr);
    MFA_ASSERT(bufferGroup.isValid() == true);
    vkFreeMemory(device, bufferGroup.memory, nullptr);
    vkDestroyBuffer(device, bufferGroup.buffer, nullptr);
    bufferGroup.revoke();
}

ImageGroup CreateImage(
    VkDevice device,
    VkPhysicalDevice physical_device,
    uint32_t const width, 
    uint32_t const height,
    uint32_t const depth,
    uint8_t const mip_levels,
    uint16_t const slice_count,
    VkFormat const format, 
    VkImageTiling const tiling, 
    VkImageUsageFlags const usage, 
    VkMemoryPropertyFlags const properties
) {
    ImageGroup ret {};

    VkImageCreateInfo image_info{};
    image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_info.imageType = VK_IMAGE_TYPE_2D;
    image_info.extent.width = width;
    image_info.extent.height = height;
    image_info.extent.depth = depth;
    image_info.mipLevels = mip_levels;
    image_info.arrayLayers = slice_count;
    image_info.format = format;
    image_info.tiling = tiling;
    image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    image_info.usage = usage;
    image_info.samples = VK_SAMPLE_COUNT_1_BIT;
    image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VK_Check(vkCreateImage(device, &image_info, nullptr, &ret.image));

    VkMemoryRequirements memory_requirements;
    vkGetImageMemoryRequirements(device, ret.image, &memory_requirements);

    VkMemoryAllocateInfo allocate_info{};
    allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocate_info.allocationSize = memory_requirements.size;
    allocate_info.memoryTypeIndex = FindMemoryType(
        &physical_device, 
        memory_requirements.memoryTypeBits, 
        properties
    );

    VK_Check(vkAllocateMemory(device, &allocate_info, nullptr, &ret.memory));
    VK_Check(vkBindImageMemory(device, ret.image, ret.memory, 0));

    return ret;
}

void DestroyImage(
    VkDevice device,
    ImageGroup const & image_group
) {
    vkDestroyImage(device, image_group.image, nullptr);
    vkFreeMemory(device, image_group.memory, nullptr);
}

GpuTexture CreateTexture(
    CpuTexture & cpuTexture,
    VkDevice device,
    VkPhysicalDevice physicalDevice,
    VkQueue graphicQueue,
    VkCommandPool commandPool
) {
    MFA_ASSERT(device != nullptr);
    MFA_ASSERT(physicalDevice != nullptr);
    MFA_ASSERT(graphicQueue != nullptr);
    MFA_VK_VALID_ASSERT(commandPool);

    GpuTexture gpuTexture {};
    if(cpuTexture.isValid()) {
        auto const format = cpuTexture.GetFormat();
        auto const mipCount = cpuTexture.GetMipCount();
        auto const sliceCount = cpuTexture.GetSlices();
        auto const & largestMipmapInfo = cpuTexture.GetMipmap(0);
        auto const buffer = cpuTexture.GetBuffer();
        MFA_ASSERT(buffer.ptr != nullptr && buffer.len > 0);
        // Create upload buffer
        auto uploadBufferGroup = CreateBuffer(
            device,
            physicalDevice,
            buffer.len,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        );
        MFA_DEFER {DestroyBuffer(device, uploadBufferGroup);};
        // Map texture data to buffer
        MapDataToBuffer(device, uploadBufferGroup.memory, buffer);

        auto const vulkan_format = ConvertCpuTextureFormatToGpu(format);

        auto const imageGroup = CreateImage(
            device, 
            physicalDevice, 
            largestMipmapInfo.dimension.width,
            largestMipmapInfo.dimension.height,
            largestMipmapInfo.dimension.depth,
            mipCount,
            sliceCount,
            vulkan_format,
            VK_IMAGE_TILING_OPTIMAL, 
            VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, 
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
        );
        
        TransferImageLayout(
            device,
            graphicQueue,
            commandPool,
            imageGroup.image,
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            mipCount,
            sliceCount
        );
        
        CopyBufferToImage(
            device,
            commandPool,
            uploadBufferGroup.buffer,
            imageGroup.image,
            graphicQueue,
            cpuTexture
        );

        TransferImageLayout(
            device,
            graphicQueue,
            commandPool,
            imageGroup.image,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            mipCount,
            sliceCount
        );

        auto const imageView = CreateImageView(
            device,
            imageGroup.image,
            vulkan_format,
            VK_IMAGE_ASPECT_COLOR_BIT,
            mipCount
        );

        gpuTexture.mImageGroup = imageGroup;
        gpuTexture.mImageView = imageView;
        gpuTexture.mCpuTexture = cpuTexture;
        MFA_ASSERT(gpuTexture.isValid());
    }
    return gpuTexture;
}

bool DestroyTexture(VkDevice device, GpuTexture & gpuTexture) {
    MFA_ASSERT(device != nullptr);
    bool success = false;
    if(gpuTexture.isValid()) {
        MFA_ASSERT(device != nullptr);
        DestroyImage(
            device,
            gpuTexture.mImageGroup
        );
        DestroyImageView(
            device,
            gpuTexture.mImageView
        );
        success = true;
        gpuTexture.revoke();
    }
    return success;
}

VkFormat ConvertCpuTextureFormatToGpu(AssetSystem::TextureFormat const cpuFormat) {
    using namespace AssetSystem;
    switch (cpuFormat)
    {
    case TextureFormat::UNCOMPRESSED_UNORM_R8_SRGB:
        return VkFormat::VK_FORMAT_R8_SRGB;
    case TextureFormat::UNCOMPRESSED_UNORM_R8_LINEAR:
        return VkFormat::VK_FORMAT_R8_UNORM;
    case TextureFormat::UNCOMPRESSED_UNORM_R8G8_LINEAR:
        return VkFormat::VK_FORMAT_R8G8_UNORM;
    case TextureFormat::UNCOMPRESSED_UNORM_R8G8B8A8_SRGB:
        return VkFormat::VK_FORMAT_R8G8B8A8_SRGB;
    case TextureFormat::UNCOMPRESSED_UNORM_R8G8B8A8_LINEAR:
        return VkFormat::VK_FORMAT_R8G8B8A8_UNORM;
    case TextureFormat::BC4_SNorm_Linear_R:
        return VkFormat::VK_FORMAT_BC4_SNORM_BLOCK;
    case TextureFormat::BC4_UNorm_Linear_R:
        return VkFormat::VK_FORMAT_BC4_UNORM_BLOCK;
    case TextureFormat::BC5_SNorm_Linear_RG:
        return VkFormat::VK_FORMAT_BC5_SNORM_BLOCK;
    case TextureFormat::BC5_UNorm_Linear_RG:
        return VkFormat::VK_FORMAT_BC5_UNORM_BLOCK;
    case TextureFormat::BC6H_SFloat_Linear_RGB:
        return VkFormat::VK_FORMAT_BC6H_SFLOAT_BLOCK;
    case TextureFormat::BC6H_UFloat_Linear_RGB:
        return VkFormat::VK_FORMAT_BC6H_UFLOAT_BLOCK;
    case TextureFormat::BC7_UNorm_Linear_RGB:
    case TextureFormat::BC7_UNorm_Linear_RGBA:
        return VkFormat::VK_FORMAT_BC7_UNORM_BLOCK;
    case TextureFormat::BC7_UNorm_sRGB_RGB:
    case TextureFormat::BC7_UNorm_sRGB_RGBA:
        return VkFormat::VK_FORMAT_BC7_SRGB_BLOCK;
    default:
        MFA_NOT_IMPLEMENTED_YET("Mohammad Fakhreddin");
    }
}

void CopyBufferToImage(
    VkDevice device,
    VkCommandPool commandPool,
    VkBuffer buffer,
    VkImage image,
    VkQueue graphicQueue,
    CpuTexture const & cpuTexture
) {
    MFA_ASSERT(device != nullptr);
    MFA_VK_VALID_ASSERT(commandPool);
    MFA_VK_VALID_ASSERT(buffer);
    MFA_VK_VALID_ASSERT(image);
    MFA_ASSERT(cpuTexture.isValid());
    MFA_ASSERT(
        cpuTexture.GetMipmap(cpuTexture.GetMipCount() - 1).offset + 
        cpuTexture.GetMipmap(cpuTexture.GetMipCount() - 1).size == cpuTexture.GetBuffer().len
    );

    auto const commandBuffer = BeginSingleTimeCommand(device, commandPool);

    auto const mipCount = cpuTexture.GetMipCount();
    auto const slices = cpuTexture.GetSlices();
    auto const regionCount = mipCount * slices;
    // TODO Maybe add a function allocate from heap
    auto const regionsBlob = Memory::Alloc(regionCount * sizeof(VkBufferImageCopy));
    MFA_DEFER {Memory::Free(regionsBlob);};
    auto * regionsArray = regionsBlob.as<VkBufferImageCopy>();
    for(uint8_t sliceIndex = 0; sliceIndex < slices; sliceIndex++) {
        for(uint8_t mipLevel = 0; mipLevel < mipCount; mipLevel++) {
            auto const & mipInfo = cpuTexture.GetMipmap(mipLevel);
            auto & region = regionsArray[mipLevel];
            region.imageExtent.width = mipInfo.dimension.width; 
            region.imageExtent.height = mipInfo.dimension.height; 
            region.imageExtent.depth = mipInfo.dimension.depth;
            region.imageOffset.x = 0;
            region.imageOffset.y = 0;
            region.imageOffset.z = 0;
            region.bufferOffset = static_cast<uint32_t>(cpuTexture.mipOffsetInBytes(mipLevel, sliceIndex)); 
            region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            region.imageSubresource.mipLevel = mipLevel;
            region.imageSubresource.baseArrayLayer = sliceIndex;
            region.imageSubresource.layerCount = 1;
            region.bufferRowLength = 0;
            region.bufferImageHeight = 0;
        }
    }

    vkCmdCopyBufferToImage(
        commandBuffer,
        buffer,
        image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        regionCount,
        regionsArray
    );

    EndAndSubmitSingleTimeCommand(device, commandPool, graphicQueue, commandBuffer);

}

LogicalDevice CreateLogicalDevice(
    VkPhysicalDevice physicalDevice,
    uint32_t const graphicsQueueFamily,
    uint32_t const presentQueueFamily,
    VkPhysicalDeviceFeatures const & enabledPhysicalDeviceFeatures
) {
    LogicalDevice ret {};

    MFA_ASSERT(physicalDevice != nullptr);
    
    // Create one graphics queue and optionally a separate presentation queue
    float queue_priority = 1.0f;

    VkDeviceQueueCreateInfo queue_create_info[2] = {};

    queue_create_info[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_create_info[0].queueFamilyIndex = graphicsQueueFamily;
    queue_create_info[0].queueCount = 1;
    queue_create_info[0].pQueuePriorities = &queue_priority;

    queue_create_info[1].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_create_info[1].queueFamilyIndex = presentQueueFamily;
    queue_create_info[1].queueCount = 1;
    queue_create_info[1].pQueuePriorities = &queue_priority;

    // Create logical device from physical device
    // Note: there are separate instance and device extensions!
    VkDeviceCreateInfo deviceCreateInfo = {};
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.pQueueCreateInfos = queue_create_info;

    if (graphicsQueueFamily == presentQueueFamily) {
        deviceCreateInfo.queueCreateInfoCount = 1;
    } else {
        deviceCreateInfo.queueCreateInfoCount = 2;
    }

    const char * device_extensions = VK_KHR_SWAPCHAIN_EXTENSION_NAME;
    deviceCreateInfo.enabledExtensionCount = 1;
    deviceCreateInfo.ppEnabledExtensionNames = &device_extensions;
    // Necessary for shader (for some reason)
    deviceCreateInfo.pEnabledFeatures = &enabledPhysicalDeviceFeatures;
#if defined(MFA_DEBUG) && defined(__ANDROID__) == false
    deviceCreateInfo.enabledLayerCount = static_cast<uint32_t>(DebugLayers.size());
    deviceCreateInfo.ppEnabledLayerNames = DebugLayers.data();
#else
    deviceCreateInfo.enabledLayerCount = 0;
    deviceCreateInfo.ppEnabledLayerNames = nullptr;
#endif
    VK_Check(vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &ret.device));
    MFA_ASSERT(ret.device != nullptr);
    MFA_LOG_INFO("Logical device create was successful");    
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &ret.physical_memory_properties);

    return ret;
}

void DestroyLogicalDevice(LogicalDevice const & logical_device) {
    vkDestroyDevice(logical_device.device, nullptr);
}

VkQueue GetQueueByFamilyIndex(
    VkDevice device,
    uint32_t const queue_family_index
) {
    VkQueue graphic_queue = nullptr;
    vkGetDeviceQueue(
        device, 
        queue_family_index, 
        0, 
        &graphic_queue
    );
    return graphic_queue;
}

// TODO Consider oop and storing data
VkSampler CreateSampler(
    VkDevice device, 
    CreateSamplerParams const & params
) {
    MFA_ASSERT(device != nullptr);
    VkSampler sampler {};

    VkSamplerCreateInfo sampler_info{};
    sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sampler_info.magFilter = VK_FILTER_LINEAR;
    sampler_info.minFilter = VK_FILTER_LINEAR;
    // TODO Important, Get this parameter from outside
    sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
#if defined(__DESKTOP__) || defined(__IOS__)
    sampler_info.anisotropyEnable = params.anisotropy_enabled;
#elif defined(__ANDROID__)
    sampler_info.anisotropyEnable = false;
#else
    #error Os not handled
#endif
    sampler_info.maxAnisotropy = params.max_anisotropy;
    sampler_info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    sampler_info.unnormalizedCoordinates = VK_FALSE;
    sampler_info.compareEnable = VK_TRUE;
    sampler_info.compareOp = VK_COMPARE_OP_ALWAYS;
    sampler_info.minLod = params.min_lod;
    sampler_info.maxLod = params.max_lod;
    sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    sampler_info.mipLodBias = 0.0f;

    VK_Check(vkCreateSampler(device, &sampler_info, nullptr, &sampler));
    
    return sampler;
}

// TODO Create wrapper for all vk types
void DestroySampler(VkDevice device, VkSampler sampler) {
    MFA_ASSERT(device != nullptr);
    MFA_VK_VALID_ASSERT(sampler);
    vkDestroySampler(device, sampler, nullptr);
}

VkDebugReportCallbackEXT SetDebugCallback(
    VkInstance instance, 
    PFN_vkDebugReportCallbackEXT const & callback
) {
    VkDebugReportCallbackCreateInfoEXT debug_info = {};
    debug_info.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
    debug_info.pfnCallback = callback;
    debug_info.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
    auto const debug_report_callback = reinterpret_cast<PFN_vkCreateDebugReportCallbackEXT>(vkGetInstanceProcAddr(
        instance,
        "vkCreateDebugReportCallbackEXT"
    ));
    VkDebugReportCallbackEXT ret {};
    VK_Check(debug_report_callback(
        instance, 
        &debug_info, 
        nullptr, 
        &ret
    ));
    return ret;
}

static FindPhysicalDeviceResult FindPhysicalDevice(VkInstance vk_instance, uint8_t const retry_count) {
    FindPhysicalDeviceResult ret {};

    uint32_t deviceCount = 0;
    //Getting number of physical devices
    VK_Check(vkEnumeratePhysicalDevices(
        vk_instance, 
        &deviceCount, 
        nullptr
    ));
    MFA_ASSERT(deviceCount > 0);

    std::vector<VkPhysicalDevice> devices (deviceCount);
    const auto phyDevResult = vkEnumeratePhysicalDevices(
        vk_instance, 
        &deviceCount, 
        devices.data()
    );
    //TODO Search about incomplete
    MFA_ASSERT(phyDevResult == VK_SUCCESS || phyDevResult == VK_INCOMPLETE);
    // TODO We need to choose physical device based on features that we need
    if(retry_count >= devices.size()) {
        MFA_CRASH("No suitable physical device exists");
    }
    ret.physicalDevice = devices[retry_count];
    MFA_ASSERT(ret.physicalDevice != nullptr);
    ret.physicalDeviceFeatures.samplerAnisotropy = VK_TRUE;
    ret.physicalDeviceFeatures.shaderClipDistance = VK_TRUE;
    ret.physicalDeviceFeatures.shaderCullDistance = VK_TRUE;
    
    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(ret.physicalDevice, &deviceProperties);
    vkGetPhysicalDeviceFeatures(ret.physicalDevice, &ret.physicalDeviceFeatures);

    uint32_t supportedVersion[] = {
        VK_VERSION_MAJOR(deviceProperties.apiVersion),
        VK_VERSION_MINOR(deviceProperties.apiVersion),
        VK_VERSION_PATCH(deviceProperties.apiVersion)
    };

    MFA_LOG_INFO(
        "Physical device supports version %d.%d.%d\n", 
        supportedVersion[0], supportedVersion[1], supportedVersion[2]
    );

    return ret;
}

FindPhysicalDeviceResult FindPhysicalDevice(VkInstance vk_instance) {
    return FindPhysicalDevice(vk_instance, 0);
}

bool CheckSwapChainSupport(VkPhysicalDevice physical_device) {
    bool ret = false;
    uint32_t extension_count = 0;
    VK_Check(vkEnumerateDeviceExtensionProperties(
        physical_device, 
        nullptr, 
        &extension_count, 
        nullptr
    ));
    std::vector<VkExtensionProperties> device_extensions(extension_count);
    vkEnumerateDeviceExtensionProperties(physical_device, nullptr, &extension_count, device_extensions.data());
    for (const auto& extension : device_extensions) {
        if (strcmp(extension.extensionName, VK_KHR_SWAPCHAIN_EXTENSION_NAME) == 0) {
            MFA_LOG_INFO("Physical device supports swap chains");
            ret = true;
            break;
        }
    }
    return ret;
}

FindPresentAndGraphicQueueFamilyResult FindPresentAndGraphicQueueFamily(
    VkPhysicalDevice physical_device, 
    VkSurfaceKHR window_surface
) {
    FindPresentAndGraphicQueueFamilyResult ret {};

    uint32_t queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, nullptr);
    if (queue_family_count == 0) {
        MFA_CRASH("physical device has no queue families!");
    }
    // Find queue family with graphics support
    // Note: is a transfer queue necessary to copy vertices to the gpu or can a graphics queue handle that?
    std::vector<VkQueueFamilyProperties> queueFamilies(queue_family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(
        physical_device, 
        &queue_family_count, 
        queueFamilies.data()
    );
    
    MFA_LOG_INFO("physical device has %d queue families.", queue_family_count);

    bool found_graphic_queue_family = false;
    bool found_present_queue_family = false;
    for (uint32_t i = 0; i < queue_family_count; i++) {
        VkBool32 present_is_supported = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, i, window_surface, &present_is_supported);
        if (queueFamilies[i].queueCount > 0 && queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            ret.graphic_queue_family = i;
            found_graphic_queue_family = true;
            if (present_is_supported) {
                ret.present_queue_family = i;
                found_present_queue_family = true;
                break;
            }
        }
        if (!found_present_queue_family && present_is_supported) {
            ret.present_queue_family = i;
            found_present_queue_family = true;
        }
    }
    if (found_graphic_queue_family) {
        MFA_LOG_INFO("Queue family # %d supports graphics", ret.graphic_queue_family);
        if (found_present_queue_family) {
            MFA_LOG_INFO("Queue family # %d supports presentation", ret.present_queue_family);
        } else {
            MFA_CRASH("Could not find a valid queue family with present support");
        }
    } else {
        MFA_CRASH("Could not find a valid queue family with graphics support");
    }
    return ret; 
}

VkCommandPool CreateCommandPool(VkDevice device, uint32_t const queue_family_index) {
    MFA_ASSERT(device);
    // Create graphics command pool
    VkCommandPoolCreateInfo poolCreateInfo = {};
    poolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolCreateInfo.queueFamilyIndex = queue_family_index;
    poolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    VkCommandPool command_pool {};
    VK_Check(vkCreateCommandPool(device, &poolCreateInfo, nullptr, &command_pool));

    return command_pool;
}

void DestroyCommandPool(VkDevice device, VkCommandPool commandPool) {
    MFA_ASSERT(device);
    vkDestroyCommandPool(device, commandPool, nullptr);
}

VkSurfaceCapabilitiesKHR GetSurfaceCapabilities(
    VkPhysicalDevice physicalDevice, 
    VkSurfaceKHR windowSurface
) {
    MFA_VK_VALID_ASSERT(physicalDevice);
    MFA_VK_VALID_ASSERT(windowSurface);
    VkSurfaceCapabilitiesKHR surfaceCapabilities;
    VK_Check (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, windowSurface, &surfaceCapabilities));
    return surfaceCapabilities;
}

SwapChainGroup CreateSwapChain(
    VkDevice device,
    VkPhysicalDevice physicalDevice, 
    VkSurfaceKHR windowSurface,
    VkSurfaceCapabilitiesKHR surfaceCapabilities,
    VkSwapchainKHR oldSwapChain
) {
    // Find supported surface formats
    uint32_t formatCount;
    if (
        vkGetPhysicalDeviceSurfaceFormatsKHR(
            physicalDevice, 
            windowSurface, 
            &formatCount, 
            nullptr
        ) != VK_SUCCESS || 
        formatCount == 0
    ) {
        MFA_CRASH("Failed to get number of supported surface formats");
    }
    
    std::vector<VkSurfaceFormatKHR> surface_formats(formatCount);
    VK_Check (vkGetPhysicalDeviceSurfaceFormatsKHR(
        physicalDevice, 
        windowSurface, 
        &formatCount, 
        surface_formats.data()
    ));

    // Find supported present modes
    uint32_t presentModeCount;
    if (vkGetPhysicalDeviceSurfacePresentModesKHR(
        physicalDevice, 
        windowSurface, 
        &presentModeCount, 
        nullptr
    ) != VK_SUCCESS || presentModeCount == 0) {
        MFA_CRASH("Failed to get number of supported presentation modes");
    }

    std::vector<VkPresentModeKHR> present_modes(presentModeCount);
    VK_Check (vkGetPhysicalDeviceSurfacePresentModesKHR(
        physicalDevice, 
        windowSurface, 
        &presentModeCount, 
        present_modes.data())
    );

    // Determine number of images for swap chain
    uint32_t imageCount = surfaceCapabilities.minImageCount + 1;
    if (surfaceCapabilities.maxImageCount != 0 && imageCount > surfaceCapabilities.maxImageCount) {
        imageCount = surfaceCapabilities.maxImageCount;
    }

    MFA_LOG_INFO(
        "Surface extend width: %d, height: %d"
        , surfaceCapabilities.currentExtent.width
        , surfaceCapabilities.currentExtent.height
    );

    MFA_LOG_INFO("Using %d images for swap chain.", imageCount);
    MFA_ASSERT(surface_formats.size() <= 255);
    // Select a surface format
    auto const selected_surface_format = ChooseSurfaceFormat(
        static_cast<uint8_t>(surface_formats.size()),
        surface_formats.data()
    );

    // Select swap chain size
    auto const selected_swap_chain_extent = ChooseSwapChainExtent(
        surfaceCapabilities, 
        surfaceCapabilities.currentExtent.width, 
        surfaceCapabilities.currentExtent.height
    );

    // Determine transformation to use (preferring no transform)
    VkSurfaceTransformFlagBitsKHR surfaceTransform;
    //if (surfaceCapabilities.supportedTransforms & VK_SURFACE_TRANSFORM_ROTATE_90_BIT_KHR) {
        //surfaceTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    //} else if (surfaceCapabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR) {
        //surfaceTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    //} else {
    surfaceTransform = surfaceCapabilities.currentTransform;
    //}

    MFA_ASSERT(present_modes.size() <= 255);
    // Choose presentation mode (preferring MAILBOX ~= triple buffering)
    auto const selected_present_mode = ChoosePresentMode(static_cast<uint8_t>(present_modes.size()), present_modes.data());

    // Finally, create the swap chain
    VkSwapchainCreateInfoKHR createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = windowSurface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = selected_surface_format.format;
    createInfo.imageColorSpace = selected_surface_format.colorSpace;
    createInfo.imageExtent = selected_swap_chain_extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    createInfo.queueFamilyIndexCount = 0;
    createInfo.pQueueFamilyIndices = nullptr;
    createInfo.preTransform = surfaceTransform;
#if defined(__DESKTOP__) || defined(__IOS__)
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
#elif defined(__ANDROID__)
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR;
#else
    #error Os is not supported
#endif
    createInfo.presentMode = selected_present_mode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = oldSwapChain;

    SwapChainGroup ret {};

    VK_Check (vkCreateSwapchainKHR(device, &createInfo, nullptr, &ret.swapChain));

    MFA_LOG_INFO("Created swap chain");

    ret.swapChainFormat = selected_surface_format.format;
    
    // Store the images used by the swap chain
    // Note: these are the images that swap chain image indices refer to
    // Note: actual number of images may differ from requested number, since it's a lower bound
    uint32_t actualImageCount = 0;
    if (
        vkGetSwapchainImagesKHR(device, ret.swapChain, &actualImageCount, nullptr) != VK_SUCCESS || 
        actualImageCount == 0
    ) {
        MFA_CRASH("Failed to acquire number of swap chain images");
    }

    ret.swapChainImages.resize(actualImageCount);
    VK_Check (vkGetSwapchainImagesKHR(
        device,
        ret.swapChain,
        &actualImageCount,
        ret.swapChainImages.data()
    ));

    ret.swapChainImageViews.resize(actualImageCount);
    for(uint32_t image_index = 0; image_index < actualImageCount; image_index++) {
        MFA_VK_VALID_ASSERT(ret.swapChainImages[image_index]);
        ret.swapChainImageViews[image_index] = CreateImageView(
            device,
            ret.swapChainImages[image_index],
            ret.swapChainFormat,
            VK_IMAGE_ASPECT_COLOR_BIT,
            1
        );
        MFA_VK_VALID_ASSERT(ret.swapChainImageViews[image_index]);
    }

    MFA_LOG_INFO("Acquired swap chain images");
    return ret;
}

void DestroySwapChain(VkDevice device, SwapChainGroup const & swapChainGroup) {
    MFA_ASSERT(device);
    MFA_ASSERT(swapChainGroup.swapChain);
    MFA_ASSERT(swapChainGroup.swapChainImageViews.empty() == false);
    MFA_ASSERT(swapChainGroup.swapChainImages.empty() == false);
    MFA_ASSERT(swapChainGroup.swapChainImages.size() == swapChainGroup.swapChainImageViews.size());
    vkDestroySwapchainKHR(device, swapChainGroup.swapChain, nullptr);
    for(auto const & swap_chain_image_view : swapChainGroup.swapChainImageViews) {
        vkDestroyImageView(device, swap_chain_image_view, nullptr);
    }
}

DepthImageGroup CreateDepth(
    VkPhysicalDevice physical_device,
    VkDevice device,
    VkExtent2D const swap_chain_extend
) {
    DepthImageGroup ret {};
    auto const depthFormat = FindDepthFormat(physical_device);
    ret.imageGroup = CreateImage(
        device,
        physical_device,
        swap_chain_extend.width,
        swap_chain_extend.height,
        1,
        1,
        1,
        depthFormat,
        VK_IMAGE_TILING_OPTIMAL, 
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, 
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
    );
    MFA_ASSERT(ret.imageGroup.image);
    MFA_ASSERT(ret.imageGroup.memory);
    ret.imageView = CreateImageView(
        device,
        ret.imageGroup.image,
        depthFormat,
        VK_IMAGE_ASPECT_DEPTH_BIT,
        1
    );
    MFA_ASSERT(ret.imageView);
    return ret;
}

void DestroyDepth(VkDevice device, DepthImageGroup const & depthGroup) {
    MFA_ASSERT(device != nullptr);
    DestroyImageView(device, depthGroup.imageView);
    DestroyImage(device, depthGroup.imageGroup);
 }

VkRenderPass CreateRenderPass(
    VkPhysicalDevice const physicalDevice, 
    VkDevice const device, 
    VkFormat const swapChainFormat
) {
    VkAttachmentDescription colorAttachment = {};
    colorAttachment.format = swapChainFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    
    VkAttachmentDescription depthAttachment {};
    depthAttachment.format = FindDepthFormat(physicalDevice);
    depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    
    // Note: hardware will automatically transition attachment to the specified layout
    // Note: index refers to attachment descriptions array
    VkAttachmentReference colorAttachmentReference {};
    colorAttachmentReference.attachment = 0;
    colorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    
    VkAttachmentReference depthAttachmentRef {};
    depthAttachmentRef.attachment = 1;
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    
    // Note: this is a description of how the attachments of the render pass will be used in this sub pass
    // e.g. if they will be read in shaders and/or drawn to
    VkSubpassDescription subPassDescription = {};
    subPassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subPassDescription.colorAttachmentCount = 1;
    subPassDescription.pColorAttachments = &colorAttachmentReference;
    subPassDescription.pDepthStencilAttachment = &depthAttachmentRef;

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    std::vector<VkAttachmentDescription> attachments = {colorAttachment, depthAttachment};
    // Create the render pass
    VkRenderPassCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    createInfo.attachmentCount = static_cast<uint32_t>(attachments.size());;
    createInfo.pAttachments = attachments.data();
    createInfo.subpassCount = 1;
    createInfo.pSubpasses = &subPassDescription;
    createInfo.dependencyCount = 1;
    createInfo.pDependencies = &dependency;

    VkRenderPass render_pass {};
    VK_Check (vkCreateRenderPass(device, &createInfo, nullptr, &render_pass));

    MFA_LOG_INFO("Created render pass.");

    return render_pass;
}

void DestroyRenderPass(VkDevice device, VkRenderPass renderPass) {
    MFA_ASSERT(device != nullptr);
    MFA_VK_VALID_ASSERT(renderPass);
    vkDestroyRenderPass(device, renderPass, nullptr);
}

std::vector<VkFramebuffer> CreateFrameBuffers(
    VkDevice device,
    VkRenderPass renderPass,
    uint32_t const swapChainImageViewsCount, 
    VkImageView * swapChainImageViews,
    VkImageView const depthImageView,
    VkExtent2D const swapChainExtent
) {
    MFA_ASSERT(device != nullptr);
    MFA_VK_VALID_ASSERT(renderPass);
    MFA_ASSERT(swapChainImageViewsCount > 0);
    MFA_ASSERT(swapChainImageViews != nullptr);
    std::vector<VkFramebuffer> swap_chain_frame_buffers (swapChainImageViewsCount);

    // Note: FrameBuffer is basically a specific choice of attachments for a render pass
    // That means all attachments must have the same dimensions, interesting restriction
    for (size_t i = 0; i < swapChainImageViewsCount; i++) {

        std::vector<VkImageView> attachments = {
            swapChainImageViews[i],
            depthImageView
        };

        VkFramebufferCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        create_info.renderPass = renderPass;
        create_info.attachmentCount = static_cast<uint32_t>(attachments.size());
        create_info.pAttachments = attachments.data();
        create_info.width = swapChainExtent.width;
        create_info.height = swapChainExtent.height;
        create_info.layers = 1;

        VK_Check(vkCreateFramebuffer(device, &create_info, nullptr, &swap_chain_frame_buffers[i]));
    }

    MFA_LOG_INFO("Created frame-buffers for swap chain image views");

    return swap_chain_frame_buffers;
}

void DestroyFrameBuffers(
    VkDevice device,
    uint32_t const frameBuffersCount,
    VkFramebuffer * frameBuffers
) {
    for(uint32_t index = 0; index < frameBuffersCount; index++) {
        vkDestroyFramebuffer(device, frameBuffers[index], nullptr);
    }
}

GpuShader CreateShader(VkDevice device, CpuShader const & cpuShader) {
    MFA_ASSERT(device != nullptr);
    GpuShader gpuShader {};
    if(cpuShader.isValid()) {
        gpuShader.mCpuShader = cpuShader;
        auto const shaderCode = cpuShader.getCompiledShaderCode();
        VkShaderModuleCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = shaderCode.len;
        createInfo.pCode = reinterpret_cast<uint32_t const *>(shaderCode.ptr);
        VK_Check(vkCreateShaderModule(device, &createInfo, nullptr, &gpuShader.mShaderModule));
        MFA_LOG_INFO("Creating shader module was successful");
        MFA_ASSERT(gpuShader.valid());
    }
    return gpuShader;
}

bool DestroyShader(VkDevice device, GpuShader & gpuShader) {
    MFA_ASSERT(device != nullptr);
    bool ret = false;
    if(gpuShader.valid()) {
        vkDestroyShaderModule(device, gpuShader.mShaderModule, nullptr);
        gpuShader.revoke();
        ret = true;
    }
    return ret;
}

GraphicPipelineGroup CreateGraphicPipeline(
    VkDevice device, 
    uint8_t shaderStagesCount, 
    GpuShader const * shaderStages,
    VkVertexInputBindingDescription vertexBindingDescription,
    uint32_t attributeDescriptionCount,
    VkVertexInputAttributeDescription * attributeDescriptionData,
    VkExtent2D swapChainExtent,
    VkRenderPass renderPass,
    uint32_t descriptorSetLayoutCount,
    VkDescriptorSetLayout* descriptorSetLayouts,
    CreateGraphicPipelineOptions const & options
) {
    MFA_ASSERT(shaderStages);
    MFA_ASSERT(renderPass);
    MFA_ASSERT(descriptorSetLayouts);
    // Set up shader stage info
    std::vector<VkPipelineShaderStageCreateInfo> shader_stages_create_infos (shaderStagesCount);
    for(uint8_t i = 0; i < shaderStagesCount; i++) {
        VkPipelineShaderStageCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        create_info.stage = ConvertAssetShaderStageToGpu(shaderStages[i].cpuShader()->getStage());
        create_info.module = shaderStages[i].shaderModule();
        create_info.pName = shaderStages[i].cpuShader()->getEntryPoint();
        shader_stages_create_infos[i] = create_info;
    }

    // Describe vertex input
    VkPipelineVertexInputStateCreateInfo vertex_input_state_create_info = {};
    vertex_input_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_input_state_create_info.vertexBindingDescriptionCount = 1;
    vertex_input_state_create_info.pVertexBindingDescriptions = &vertexBindingDescription;
    vertex_input_state_create_info.vertexAttributeDescriptionCount = attributeDescriptionCount;
    vertex_input_state_create_info.pVertexAttributeDescriptions = attributeDescriptionData;

    // Describe input assembly
    VkPipelineInputAssemblyStateCreateInfo input_assembly_create_info = {};
    input_assembly_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_assembly_create_info.topology = options.primitiveTopology;
    input_assembly_create_info.primitiveRestartEnable = VK_FALSE;

    // Note: scissor test is always enabled (although dynamic scissor is possible)
    // Number of viewports must match number of scissors
    VkPipelineViewportStateCreateInfo viewport_create_info = {};
    viewport_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_create_info.viewportCount = 1;
    viewport_create_info.scissorCount = 1;
    // Do not move these objects into if conditions otherwise it will be removed for release mode
    // thus you may face strange validation error about min depth is not set
    VkViewport viewport = {};
    VkRect2D scissor = {};

    if(true == options.useStaticViewportAndScissor) {
        // Describe viewport and scissor
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = static_cast<float>(swapChainExtent.width);
        viewport.height = static_cast<float>(swapChainExtent.height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        scissor.offset.x = 0;
        scissor.offset.y = 0;
        scissor.extent.width = swapChainExtent.width;
        scissor.extent.height = swapChainExtent.height;

        viewport_create_info.pViewports = &viewport;
        viewport_create_info.pScissors = &scissor;
    }

    // Describe rasterization
    // Note: depth bias and using polygon modes other than fill require changes to logical device creation (device features)
    VkPipelineRasterizationStateCreateInfo rasterization_create_info = {};
    rasterization_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    // TODO Might need to ask some of them from outside
    rasterization_create_info.polygonMode = VK_POLYGON_MODE_FILL;
    rasterization_create_info.cullMode = VK_CULL_MODE_NONE;
    rasterization_create_info.frontFace = options.fontFace;
    rasterization_create_info.depthBiasEnable = VK_FALSE;
    rasterization_create_info.lineWidth = 1.0f;
    rasterization_create_info.depthClampEnable = VK_FALSE;
    rasterization_create_info.rasterizerDiscardEnable = VK_FALSE;

    // Describe multi-sampling
    // Note: using multi-sampling also requires turning on device features
    VkPipelineMultisampleStateCreateInfo multi_sample_state_create_info = {};
    multi_sample_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multi_sample_state_create_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multi_sample_state_create_info.sampleShadingEnable = VK_FALSE;
    multi_sample_state_create_info.minSampleShading = 1.0f;
    multi_sample_state_create_info.alphaToCoverageEnable = VK_FALSE;
    multi_sample_state_create_info.alphaToOneEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo color_blend_create_info = {};
    color_blend_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    color_blend_create_info.logicOpEnable = VK_FALSE;
    color_blend_create_info.logicOp = VK_LOGIC_OP_COPY;
    color_blend_create_info.attachmentCount = 1;
    color_blend_create_info.pAttachments = &options.colorBlendAttachments;
    color_blend_create_info.blendConstants[0] = 0.0f;
    color_blend_create_info.blendConstants[1] = 0.0f;
    color_blend_create_info.blendConstants[2] = 0.0f;
    color_blend_create_info.blendConstants[3] = 0.0f;

    MFA_LOG_INFO("Created descriptor layout");

    VkPipelineLayoutCreateInfo layout_create_info = {};
    layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layout_create_info.setLayoutCount = descriptorSetLayoutCount;
    layout_create_info.pSetLayouts = descriptorSetLayouts; // Array of descriptor set layout, Order matter when more than 1
    layout_create_info.pushConstantRangeCount = options.pushConstantsRangeCount;
    layout_create_info.pPushConstantRanges = options.pushConstantRanges;
    
    VkPipelineLayout pipelineLayout {};
    VK_Check(vkCreatePipelineLayout(device, &layout_create_info, nullptr, &pipelineLayout));

    VkPipelineDynamicStateCreateInfo * dynamicStateCreateInfoRef = nullptr;
    VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo {};
    std::vector<VkDynamicState> dynamic_states = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    if (options.useStaticViewportAndScissor == false) {
        if (options.dynamicStateCreateInfo == nullptr) {
            dynamicStateCreateInfo = VkPipelineDynamicStateCreateInfo {};
            dynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
            dynamicStateCreateInfo.dynamicStateCount = static_cast<uint32_t>(dynamic_states.size());
            dynamicStateCreateInfo.pDynamicStates = dynamic_states.data();
            dynamicStateCreateInfoRef = &dynamicStateCreateInfo;
        } else {
            dynamicStateCreateInfoRef = options.dynamicStateCreateInfo;
        }
    }

    // Create the graphics pipeline
    VkGraphicsPipelineCreateInfo pipelineCreateInfo {};
    pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineCreateInfo.stageCount = static_cast<uint32_t>(shader_stages_create_infos.size());
    pipelineCreateInfo.pStages = shader_stages_create_infos.data();
    pipelineCreateInfo.pVertexInputState = &vertex_input_state_create_info;
    pipelineCreateInfo.pInputAssemblyState = &input_assembly_create_info;
    pipelineCreateInfo.pViewportState = &viewport_create_info;
    pipelineCreateInfo.pRasterizationState = &rasterization_create_info;
    pipelineCreateInfo.pMultisampleState = &multi_sample_state_create_info;
    pipelineCreateInfo.pColorBlendState = &color_blend_create_info;
    pipelineCreateInfo.layout = pipelineLayout;
    pipelineCreateInfo.renderPass = renderPass;
    pipelineCreateInfo.subpass = 0;
    MFA_VK_MAKE_NULL(pipelineCreateInfo.basePipelineHandle);
    pipelineCreateInfo.basePipelineIndex = -1;
    pipelineCreateInfo.pDepthStencilState = &options.depthStencil;
    pipelineCreateInfo.pDynamicState = dynamicStateCreateInfoRef;
    
    VkPipeline pipeline {};
#ifdef __ANDROID__
    VK_Check(vkCreateGraphicsPipelines(
        device,
        0,
        1,
        &pipelineCreateInfo,
        nullptr,
        &pipeline
    ));
#else
    VK_Check(vkCreateGraphicsPipelines(
        device, 
        nullptr, 
        1, 
        &pipelineCreateInfo, 
        nullptr, 
        &pipeline
    ));
#endif

    MFA_LOG_INFO("Created graphics pipeline");

    GraphicPipelineGroup group {};
    group.graphicPipeline = pipeline;
    group.pipelineLayout = pipelineLayout;
    MFA_ASSERT(group.isValid());

    return group;
}

void AssignViewportAndScissorToCommandBuffer(
    VkExtent2D const & extent2D, 
    VkCommandBuffer commandBuffer
) {
    MFA_ASSERT(commandBuffer != nullptr);

    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(extent2D.width);
    viewport.height = static_cast<float>(extent2D.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor = {};
    scissor.offset.x = 0;
    scissor.offset.y = 0;
    scissor.extent.width = extent2D.width;
    scissor.extent.height = extent2D.height;

    SetViewport(commandBuffer, viewport);
    SetScissor(commandBuffer, scissor);
}

void DestroyGraphicPipeline(VkDevice device, GraphicPipelineGroup & graphicPipelineGroup) {
    MFA_ASSERT(device != nullptr);
    MFA_ASSERT(graphicPipelineGroup.isValid());
    vkDestroyPipeline(device, graphicPipelineGroup.graphicPipeline, nullptr);
    vkDestroyPipelineLayout(device, graphicPipelineGroup.pipelineLayout, nullptr);
    graphicPipelineGroup.revoke();
}

[[nodiscard]]
VkDescriptorSetLayout CreateDescriptorSetLayout(
    VkDevice device,
    uint8_t const bindings_count,
    VkDescriptorSetLayoutBinding * bindings
) {
    VkDescriptorSetLayoutCreateInfo descriptor_layout_create_info = {};
    descriptor_layout_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptor_layout_create_info.bindingCount = static_cast<uint32_t>(bindings_count);
    descriptor_layout_create_info.pBindings = bindings;
    VkDescriptorSetLayout descriptor_set_layout {};
    VK_Check (vkCreateDescriptorSetLayout(
        device,
        &descriptor_layout_create_info,
        nullptr,
        &descriptor_set_layout
    ));
    return descriptor_set_layout;
}

void DestroyDescriptorSetLayout(
    VkDevice device,
    VkDescriptorSetLayout descriptor_set_layout
) {
    vkDestroyDescriptorSetLayout(
        device,
        descriptor_set_layout,
        nullptr
    );
}

VkShaderStageFlagBits ConvertAssetShaderStageToGpu(AssetSystem::ShaderStage const stage) {
    switch(stage) {
        case AssetSystem::Shader::Stage::Vertex:
        return VK_SHADER_STAGE_VERTEX_BIT;
        case AssetSystem::Shader::Stage::Fragment:
        return VK_SHADER_STAGE_FRAGMENT_BIT;
        case AssetSystem::Shader::Stage::Invalid:
        default:
        MFA_CRASH("Unhandled shader stage");
    }
}

BufferGroup CreateVertexBuffer(
    VkDevice device,
    VkPhysicalDevice physical_device,
    VkCommandPool command_pool,
    VkQueue graphic_queue,
    CBlob const vertices_blob
) {
    VkDeviceSize const bufferSize = vertices_blob.len;

    auto staging_buffer_group = CreateBuffer(
        device, 
        physical_device, 
        bufferSize, 
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
    );

    MapDataToBuffer(device, staging_buffer_group.memory, vertices_blob);
    
    auto const vertex_buffer_group = CreateBuffer(
        device, 
        physical_device, 
        bufferSize, 
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, 
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
    );

    CopyBuffer(
        device,
        command_pool,
        graphic_queue,
        staging_buffer_group.buffer, 
        vertex_buffer_group.buffer, 
        bufferSize
    );

    DestroyBuffer(device, staging_buffer_group);

    MFA_ASSERT(vertex_buffer_group.memory);
    MFA_ASSERT(vertex_buffer_group.buffer);

    return vertex_buffer_group;
}

void DestroyVertexBuffer(
    VkDevice device,
    BufferGroup & vertex_buffer_group
) {
    DestroyBuffer(device, vertex_buffer_group);
}

BufferGroup CreateIndexBuffer (
    VkDevice const device,
    VkPhysicalDevice const physical_device,
    VkCommandPool const command_pool,
    VkQueue const graphic_queue,
    CBlob const indices_blob
) {
    auto const bufferSize = indices_blob.len;

    auto staging_buffer_group = CreateBuffer(
        device,
        physical_device,
        bufferSize, 
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
    );

    MapDataToBuffer(device, staging_buffer_group.memory, indices_blob);
    
    auto const indicesBufferGroup = CreateBuffer(
        device,
        physical_device,
        bufferSize, 
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, 
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT 
    );

    CopyBuffer(
        device,
        command_pool,
        graphic_queue,
        staging_buffer_group.buffer, 
        indicesBufferGroup.buffer, 
        bufferSize
    );

    DestroyBuffer(device, staging_buffer_group);

    MFA_ASSERT(indicesBufferGroup.isValid());

    return indicesBufferGroup;

}

void DestroyIndexBuffer(
    VkDevice const device,
    BufferGroup & index_buffer_group
) {
    DestroyBuffer(device, index_buffer_group);
}

std::vector<BufferGroup> CreateUniformBuffer(
    VkDevice device,
    VkPhysicalDevice const physicalDevice,
    uint32_t const buffersCount,
    VkDeviceSize const buffersSize 
) {
    std::vector<BufferGroup> uniform_buffers_group (buffersCount);
    for(uint8_t index = 0; index < buffersCount; index++) {
        uniform_buffers_group[index] = CreateBuffer(
            device,
            physicalDevice,
            buffersSize,
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
        );
    }
    return uniform_buffers_group;
}

void UpdateUniformBuffer(
    VkDevice device,
    BufferGroup const & uniform_buffer_group,
    CBlob const data
) {
    MapDataToBuffer(device, uniform_buffer_group.memory, data);
}

void DestroyUniformBuffer(
    VkDevice device,
    BufferGroup & buffer_group
) {
    DestroyBuffer(device, buffer_group);
}

VkDescriptorPool CreateDescriptorPool(
    VkDevice device,
    uint32_t const maxSets
) {
    std::vector<VkDescriptorPoolSize> poolSizes {};
    // TODO Check if both of these variables must have same value as maxSets
    {// Uniform buffers
        VkDescriptorPoolSize poolSize;
        poolSize.type =  VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSize.descriptorCount = maxSets;
        poolSizes.emplace_back(poolSize);
    }
    {// Combined image sampler
        VkDescriptorPoolSize poolSize;
        poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        poolSize.descriptorCount = maxSets;
        poolSizes.emplace_back(poolSize);
    }
    VkDescriptorPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = maxSets;

    VkDescriptorPool descriptor_pool {};

    VK_Check(vkCreateDescriptorPool(
        device, 
        &poolInfo, 
        nullptr, 
        &descriptor_pool
    ));

    return descriptor_pool;
}

void DestroyDescriptorPool(
    VkDevice device, 
    VkDescriptorPool pool
) {
    MFA_ASSERT(device != nullptr);
    MFA_VK_VALID_ASSERT(pool);
    vkDestroyDescriptorPool(device, pool, nullptr);
}

std::vector<VkDescriptorSet> CreateDescriptorSet(
    VkDevice device,
    VkDescriptorPool descriptorPool,
    VkDescriptorSetLayout descriptorSetLayout,
    uint32_t const descriptorSetCount,
    uint8_t const schemasCount,
    VkWriteDescriptorSet * schemas
) {
    MFA_ASSERT(device != nullptr);
    MFA_VK_VALID_ASSERT(descriptorPool);
    MFA_VK_VALID_ASSERT(descriptorSetLayout);

    std::vector<VkDescriptorSetLayout> layouts(descriptorSetCount, descriptorSetLayout);
    // There needs to be one descriptor set per binding point in the shader
    VkDescriptorSetAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = descriptorSetCount;
    allocInfo.pSetLayouts = layouts.data();

    std::vector<VkDescriptorSet> descriptor_sets {};
    descriptor_sets.resize(allocInfo.descriptorSetCount);
    // Descriptor sets gets destroyed automatically when descriptor pool is destroyed
    VK_Check(vkAllocateDescriptorSets (
        device, 
        &allocInfo, 
        descriptor_sets.data()
    ));
    MFA_LOG_INFO("Create descriptor set is successful");

    if(schemasCount > 0 && schemas != nullptr) {
        MFA_ASSERT(descriptor_sets.size() < 256);
        UpdateDescriptorSets(
            device,
            static_cast<uint8_t>(descriptor_sets.size()),
            descriptor_sets.data(),
            schemasCount,
            schemas
        );
    }

    return descriptor_sets;
}

void UpdateDescriptorSets(
    VkDevice const device,
    uint8_t const descriptor_set_count,
    VkDescriptorSet* descriptor_sets,
    uint8_t const schemas_count,
    VkWriteDescriptorSet * schemas
) {
    for(auto descriptor_set_index = 0; descriptor_set_index < descriptor_set_count; descriptor_set_index++){
        std::vector<VkWriteDescriptorSet> descriptor_writes (schemas_count);
        for(uint8_t schema_index = 0; schema_index < schemas_count; schema_index++) {
            descriptor_writes[schema_index] = schemas[schema_index];
            descriptor_writes[schema_index].dstSet = descriptor_sets[descriptor_set_index];
        }

        vkUpdateDescriptorSets(
            device,
            static_cast<uint32_t>(descriptor_writes.size()), 
            descriptor_writes.data(), 
            0, 
            nullptr
        );
    }
}

std::vector<VkCommandBuffer> CreateCommandBuffers(
    VkDevice const device,
    uint8_t const swap_chain_images_count,
    VkCommandPool const command_pool
) {
    std::vector<VkCommandBuffer> command_buffers (swap_chain_images_count);
    
    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = command_pool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = swap_chain_images_count;

    VK_Check(vkAllocateCommandBuffers(
        device, 
        &allocInfo, 
        command_buffers.data()
    ));
    
    MFA_LOG_INFO("Allocated graphics command buffers.");

    // Command buffer data gets recorded each time
    return command_buffers;
}

void DestroyCommandBuffers(
    VkDevice device,
    VkCommandPool commandPool,
    uint32_t const commandBuffersCount,
    VkCommandBuffer* commandBuffers
) {
    MFA_ASSERT(device != nullptr);
    MFA_VK_VALID_ASSERT(commandPool);
    MFA_ASSERT(commandBuffersCount > 0);
    MFA_ASSERT(commandBuffers != nullptr);
    vkFreeCommandBuffers(
        device,
        commandPool,
        commandBuffersCount,
        commandBuffers
    );
}

SyncObjects CreateSyncObjects(
    VkDevice const device,
    uint8_t const maxFramesInFlight,
    uint8_t const swapChainImagesCount
) {
    SyncObjects syncObjects {};
    syncObjects.image_availability_semaphores.resize(maxFramesInFlight);
    syncObjects.render_finish_indicator_semaphores.resize(maxFramesInFlight);
    syncObjects.fences_in_flight.resize(maxFramesInFlight);
#ifdef __ANDROID__
    syncObjects.images_in_flight.resize(swapChainImagesCount, 0);
#else
    syncObjects.images_in_flight.resize(swapChainImagesCount, nullptr);
#endif

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (uint8_t i = 0; i < maxFramesInFlight; i++) {
        VK_Check(vkCreateSemaphore(
            device, 
            &semaphoreInfo, 
            nullptr, 
            &syncObjects.image_availability_semaphores[i]
        ));
        VK_Check(vkCreateSemaphore(
            device, 
            &semaphoreInfo,
            nullptr,
            &syncObjects.render_finish_indicator_semaphores[i]
        ));
        VK_Check(vkCreateFence(
            device, 
            &fenceInfo, 
            nullptr, 
            &syncObjects.fences_in_flight[i]
        ));
    }
    return syncObjects;
}

void DestroySyncObjects(VkDevice device, SyncObjects const & syncObjects) {
    MFA_ASSERT(device != nullptr);
    MFA_ASSERT(syncObjects.fences_in_flight.size() == syncObjects.image_availability_semaphores.size());
    MFA_ASSERT(syncObjects.fences_in_flight.size() == syncObjects.render_finish_indicator_semaphores.size());
    for(uint8_t i = 0; i < syncObjects.fences_in_flight.size(); i++) {
        vkDestroySemaphore(device, syncObjects.render_finish_indicator_semaphores[i], nullptr);
        vkDestroySemaphore(device, syncObjects.image_availability_semaphores[i], nullptr);
        vkDestroyFence(device, syncObjects.fences_in_flight[i], nullptr);
    }
}

void DeviceWaitIdle(VkDevice device) {
    MFA_ASSERT(device != nullptr);
    VK_Check(vkDeviceWaitIdle(device));
}

void WaitForFence(VkDevice device, VkFence inFlightFence) {
    MFA_ASSERT(device != nullptr);
    MFA_VK_VALID_ASSERT(inFlightFence);
    VK_Check(vkWaitForFences(
        device, 
        1, 
        &inFlightFence, 
        VK_TRUE, 
        UINT64_MAX
    ));
}

VkResult AcquireNextImage(
    VkDevice device, 
    VkSemaphore imageAvailabilitySemaphore, 
    SwapChainGroup const & swapChainGroup,
    uint32_t & outImageIndex
) {
    MFA_ASSERT(device != nullptr);
    MFA_VK_VALID_ASSERT(imageAvailabilitySemaphore);
#ifdef __ANDROID__
    return vkAcquireNextImageKHR(
        device,
        swapChainGroup.swapChain,
        UINT64_MAX,
        imageAvailabilitySemaphore,
        0,
        &outImageIndex
    );
#else
    return vkAcquireNextImageKHR(
            device,
            swapChainGroup.swapChain,
            UINT64_MAX,
            imageAvailabilitySemaphore,
            nullptr,
            &outImageIndex
    );
#endif

}

void BindVertexBuffer(
    VkCommandBuffer command_buffer,
    BufferGroup vertex_buffer,
    VkDeviceSize offset
) {
    vkCmdBindVertexBuffers(
        command_buffer, 
        0, 
        1, 
        &vertex_buffer.buffer, 
        &offset
    );
}

void BindIndexBuffer(
    VkCommandBuffer commandBuffer,
    BufferGroup const indexBuffer,
    VkDeviceSize const offset,
    VkIndexType const indexType
) {
    vkCmdBindIndexBuffer(
        commandBuffer,
        indexBuffer.buffer,
        offset, 
        indexType
    );
}

void DrawIndexed(
    VkCommandBuffer const command_buffer,
    uint32_t const indices_count,
    uint32_t const instance_count,
    uint32_t const first_index,
    uint32_t const vertex_offset,
    uint32_t const first_instance
) {
    vkCmdDrawIndexed(
        command_buffer, 
        indices_count, 
        instance_count, 
        first_index, 
        vertex_offset, 
        first_instance
    );
}

void SetScissor(VkCommandBuffer commandBuffer, VkRect2D const & scissor) {
    MFA_ASSERT(commandBuffer != nullptr);
    vkCmdSetScissor(
        commandBuffer, 
        0, 
        1, 
        &scissor
    );
}

void SetViewport(VkCommandBuffer commandBuffer, VkViewport const & viewport) {
    MFA_ASSERT(commandBuffer != nullptr);
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
}

void PushConstants(
    VkCommandBuffer command_buffer,
    VkPipelineLayout pipeline_layout, 
    AssetSystem::ShaderStage const shader_stage, 
    uint32_t const offset, 
    CBlob const data
) {
    vkCmdPushConstants(
        command_buffer,
        pipeline_layout,
        ConvertAssetShaderStageToGpu(shader_stage),
        offset,
        static_cast<uint32_t>(data.len), 
        data.ptr
    );
}

void UpdateDescriptorSetsBasic(
    VkDevice device,
    uint8_t const descriptorSetsCount,
    VkDescriptorSet* descriptorSets,
    VkDescriptorBufferInfo const & bufferInfo,
    uint32_t const imageInfoCount,
    VkDescriptorImageInfo const * imageInfos
) {
    MFA_ASSERT(device != nullptr);
    MFA_ASSERT(descriptorSetsCount > 0);
    MFA_ASSERT(descriptorSets != nullptr);
    for(uint8_t i = 0; i < descriptorSetsCount; i++) {
        std::vector<VkWriteDescriptorSet> descriptorWrites (2);
        
        descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].dstSet = descriptorSets[i];
        descriptorWrites[0].dstBinding = 0;
        descriptorWrites[0].dstArrayElement = 0;
        descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].pBufferInfo = &bufferInfo;

        descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[1].dstSet = descriptorSets[i];
        descriptorWrites[1].dstBinding = 1;
        descriptorWrites[1].dstArrayElement = 0;
        descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrites[1].descriptorCount = imageInfoCount;
        descriptorWrites[1].pImageInfo = imageInfos;

        vkUpdateDescriptorSets(
            device, 
            static_cast<uint32_t>(descriptorWrites.size()), 
            descriptorWrites.data(), 
            0, 
            nullptr
        );
    }
};

void UpdateDescriptorSets(
    VkDevice device,
    uint8_t descriptorWritesCount,
    VkWriteDescriptorSet * descriptorWrites
) {
    MFA_ASSERT(device != nullptr);
    MFA_ASSERT(descriptorWritesCount > 0);
    MFA_ASSERT(descriptorWrites != nullptr);

    vkUpdateDescriptorSets(
        device, 
        static_cast<uint32_t>(descriptorWritesCount), 
        descriptorWrites, 
        0, 
        nullptr
    );
}

}
