#include "RenderBackend.hpp"

#include "engine/BedrockAssert.hpp"
#include "engine/BedrockLog.hpp"
#include "engine/BedrockPlatforms.hpp"
#include "engine/BedrockMemory.hpp"
#include "engine/render_system/RenderTypes.hpp"
#include "engine/asset_system/AssetTexture.hpp"
#include "engine/asset_system/AssetShader.hpp"

#include "libs/sdl/SDL.hpp"

#ifdef __IOS__
#include <vulkan/vulkan_ios.h>
#endif

#ifdef __ANDROID__
#include "vulkan_wrapper.h"
#include <android_native_app_glue.h>
#else
#include <vulkan/vulkan.h>
#endif

#include <vector>
#include <cstring>

namespace MFA::RenderBackend
{

    constexpr char EngineName[256] = "MFA";
    constexpr int EngineVersion = 1;
    constexpr char ValidationLayer[100] = "VK_LAYER_KHRONOS_validation";

    std::vector<char const *> DebugLayers = {
        ValidationLayer
    };

    //-------------------------------------------------------------------------------------------------

    static void VK_Check(VkResult const result)
    {
        if (result != VK_SUCCESS)
        {
            MFA_CRASH(("Vulkan command failed with code:" + std::to_string(result)));
        }
    }

    //-------------------------------------------------------------------------------------------------

#ifdef __DESKTOP__
    static void SDL_Check(MSDL::SDL_bool const result)
    {
        if (result != MSDL::SDL_TRUE)
        {
            MFA_CRASH("SDL command failed");
        }
    }

    static void SDL_CheckForError()
    {
        std::string error = MSDL::SDL_GetError();
        if (error.empty() == false) {
            MFA_LOG_ERROR("SDL Error: %s", error.c_str());
        }
    }

    //-------------------------------------------------------------------------------------------------

    // TODO: Ask for full-screen
    MSDL::SDL_Window * CreateWindow(ScreenWidth const screenWidth, ScreenHeight const screenHeight)
    {
        MSDL::SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS);
        auto const screen_info = Platforms::ComputeScreenSize();
        auto * window = MSDL::SDL_CreateWindow(
            "VULKAN_ENGINE",
            static_cast<int>((static_cast<float>(screen_info.screenWidth) / 2.0f) - (static_cast<float>(screenWidth) / 2.0f)),
            static_cast<int>((static_cast<float>(screen_info.screenHeight) / 2.0f) - (static_cast<float>(screenHeight) / 2.0f)),
            screenWidth, screenHeight,
            MSDL::SDL_WINDOW_SHOWN /*| SDL_WINDOW_FULLSCREEN */ | MSDL::SDL_WINDOW_VULKAN
        );
        SDL_CheckForError();
        return window;
    }

    //-------------------------------------------------------------------------------------------------

    void DestroyWindow(MSDL::SDL_Window * window)
    {
        MFA_ASSERT(window != nullptr);
        MSDL::SDL_DestroyWindow(window);
        SDL_CheckForError();
    }
#endif

    //-------------------------------------------------------------------------------------------------

#ifdef __DESKTOP__
    VkSurfaceKHR CreateWindowSurface(MSDL::SDL_Window * window, VkInstance_T * instance)
    {
        VkSurfaceKHR ret{};
        SDL_Check(MSDL::SDL_Vulkan_CreateSurface(
            window,
            instance,
            &ret
        ));
        return ret;
    }
#elif defined(__ANDROID__)
    VkSurfaceKHR CreateWindowSurface(ANativeWindow * window, VkInstance_T * instance)
    {
        VkSurfaceKHR surface{};

        VkAndroidSurfaceCreateInfoKHR createInfo{
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
    VkSurfaceKHR CreateWindowSurface(void * view, VkInstance_T * instance)
    {
        VkSurfaceKHR surface{};

        VkIOSSurfaceCreateInfoMVK const surfaceCreateInfo{
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

//-------------------------------------------------------------------------------------------------

    void DestroyWindowSurface(VkInstance instance, VkSurfaceKHR surface)
    {
        MFA_ASSERT(instance != nullptr);
        MFA_VK_VALID_ASSERT(surface);
        vkDestroySurfaceKHR(instance, surface, nullptr);
    }

    //-------------------------------------------------------------------------------------------------

    [[nodiscard]]
    VkExtent2D ChooseSwapChainExtent(
        VkSurfaceCapabilitiesKHR const & surfaceCapabilities,
        ScreenWidth const screenWidth,
        ScreenHeight const screenHeight
    )
    {
        if (surfaceCapabilities.currentExtent.width <= 0)
        {
            VkExtent2D const swap_chain_extent = {
                std::min(
                    std::max(
                        static_cast<uint32_t>(screenWidth),
                        surfaceCapabilities.minImageExtent.width
                    ),
                    surfaceCapabilities.maxImageExtent.width
                ),
                std::min(
                    std::max(
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

    //-------------------------------------------------------------------------------------------------

    [[nodiscard]]
    VkPresentModeKHR ChoosePresentMode(
        uint8_t const presentModesCount,
        VkPresentModeKHR const * present_modes
    )
    {
        for (uint8_t index = 0; index < presentModesCount; index++)
        {
            if (present_modes[index] == VK_PRESENT_MODE_MAILBOX_KHR)
            {
                return present_modes[index];
            }
        }
        // If mailbox is unavailable, fall back to FIFO (guaranteed to be available)
        return VK_PRESENT_MODE_FIFO_KHR;
    }

    //-------------------------------------------------------------------------------------------------

    VkSurfaceFormatKHR ChooseSurfaceFormat(
        uint8_t const availableFormatsCount,
        VkSurfaceFormatKHR const * availableFormats
    )
    {
        // We can either choose any format
        if (availableFormatsCount == 1 && availableFormats[0].format == VK_FORMAT_UNDEFINED)
        {
            return { VK_FORMAT_R8G8B8A8_UNORM, VK_COLORSPACE_SRGB_NONLINEAR_KHR };
        }

        // Or go with the standard format - if available
        for (uint8_t index = 0; index < availableFormatsCount; index++)
        {
            if (availableFormats[index].format == VK_FORMAT_R8G8B8A8_UNORM)
            {
                return availableFormats[index];
            }
        }

        // Or fall back to the first available one
        return availableFormats[0];
    }

    //-------------------------------------------------------------------------------------------------

#ifdef __DESKTOP__
    VkInstance CreateInstance(char const * applicationName, MSDL::SDL_Window * window)
    {
#elif defined(__ANDROID__) || defined(__IOS__)
    VkInstance CreateInstance(char const * applicationName)
    {
#else
#error Os not handled
#endif
        // Filling out application description:
        auto applicationInfo = VkApplicationInfo{
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
            .apiVersion = VK_API_VERSION_1_1,   // TODO Make api version 1.0 for iphone
        };
        std::vector<char const *> instanceExtensions{};

#ifdef __DESKTOP__
        MFA_ASSERT(window != nullptr);
        {// Filling sdl extensions
            unsigned int sdl_extenstion_count = 0;
            SDL_Check(MSDL::SDL_Vulkan_GetInstanceExtensions(window, &sdl_extenstion_count, nullptr));
            instanceExtensions.resize(sdl_extenstion_count);
            SDL_Check(MSDL::SDL_Vulkan_GetInstanceExtensions(
                window,
                &sdl_extenstion_count,
                instanceExtensions.data()
            ));
#ifdef __PLATFORM_MAC__
        instanceExtensions.emplace_back("VK_KHR_get_physical_device_properties2");
#endif
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
#ifdef MFA_DEBUG
        instanceExtensions.emplace_back(VK_KHR_EXTERNAL_FENCE_CAPABILITIES_EXTENSION_NAME);
        instanceExtensions.emplace_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
        instanceExtensions.emplace_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
        instanceExtensions.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif
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
    auto instanceInfo = VkInstanceCreateInfo{
        // sType is mandatory
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        // pNext is mandatory
        .pNext = nullptr,
        // flags is mandatory
        .flags = 0,
        // The application info structure is then passed through the instance
        .pApplicationInfo = &applicationInfo,
#if defined(MFA_DEBUG) && !defined(__IOS__)
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
        MFA_ASSERT(instance != nullptr);
        return instance;
    }

    //-------------------------------------------------------------------------------------------------

    void DestroyInstance(VkInstance instance)
    {
        MFA_ASSERT(instance != nullptr);
        vkDestroyInstance(instance, nullptr);
    }

    //-------------------------------------------------------------------------------------------------

    VkDebugReportCallbackEXT CreateDebugCallback(
        VkInstance vkInstance,
        PFN_vkDebugReportCallbackEXT const & debugCallback
    )
    {
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

    //-------------------------------------------------------------------------------------------------

    void DestroyDebugReportCallback(
        VkInstance instance,
        VkDebugReportCallbackEXT const & reportCallbackExt
    )
    {
        MFA_ASSERT(instance != nullptr);
        auto const DestroyDebugReportCallback = reinterpret_cast<PFN_vkDestroyDebugReportCallbackEXT>(vkGetInstanceProcAddr(
            instance,
            "vkDestroyDebugReportCallbackEXT"
        ));
        DestroyDebugReportCallback(instance, reportCallbackExt, nullptr);
    }

    //-------------------------------------------------------------------------------------------------

    uint32_t FindMemoryType(
        VkPhysicalDevice * physicalDevice,
        uint32_t const typeFilter,
        VkMemoryPropertyFlags const propertyFlags
    )
    {
        MFA_ASSERT(physicalDevice != nullptr);
        VkPhysicalDeviceMemoryProperties memory_properties;
        vkGetPhysicalDeviceMemoryProperties(*physicalDevice, &memory_properties);

        for (uint32_t memory_type_index = 0; memory_type_index < memory_properties.memoryTypeCount; memory_type_index++)
        {
            if ((typeFilter & (1 << memory_type_index)) && (memory_properties.memoryTypes[memory_type_index].propertyFlags & propertyFlags) == propertyFlags)
            {
                return memory_type_index;
            }
        }

        MFA_CRASH("failed to find suitable memory type!");
    }

    //-------------------------------------------------------------------------------------------------

    std::shared_ptr<RT::ImageViewGroup> CreateImageView(
        VkDevice device,
        VkImage const & image,
        VkFormat const format,
        VkImageAspectFlags const aspectFlags,
        uint32_t const mipmapCount,
        uint32_t const layerCount,
        VkImageViewType const imageViewType
    )
    {
        MFA_ASSERT(device != nullptr);

        VkImageViewCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = image;
        createInfo.viewType = imageViewType;
        createInfo.format = format;
        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.subresourceRange.aspectMask = aspectFlags;
        // TODO Might need to ask these values from image for mipmaps (Check again later)
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = mipmapCount;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = layerCount;

        VkImageView imageView{};
        VK_Check(vkCreateImageView(device, &createInfo, nullptr, &imageView));

        return std::make_shared<RT::ImageViewGroup>(imageView);
    }

    //-------------------------------------------------------------------------------------------------

    void DestroyImageView(
        VkDevice device,
        RT::ImageViewGroup const & imageView
    )
    {
        vkDestroyImageView(device, imageView.imageView, nullptr);
    }

    //-------------------------------------------------------------------------------------------------

    VkCommandBuffer BeginSingleTimeCommand(VkDevice device, VkCommandPool const & command_pool)
    {
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

    //-------------------------------------------------------------------------------------------------

    void EndAndSubmitSingleTimeCommand(
        VkDevice device,
        VkCommandPool const & commandPool,
        VkQueue const & graphicQueue,
        VkCommandBuffer const & commandBuffer
    )
    {
        VK_Check(vkEndCommandBuffer(commandBuffer));

        VkSubmitInfo submit_info{};
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

    //-------------------------------------------------------------------------------------------------

    [[nodiscard]]
    VkFormat FindDepthFormat(VkPhysicalDevice physical_device)
    {
        std::vector<VkFormat> candidate_formats{
            VK_FORMAT_D32_SFLOAT,
            VK_FORMAT_D32_SFLOAT_S8_UINT,
            VK_FORMAT_D24_UNORM_S8_UINT
        };
        return FindSupportedFormat(
            physical_device,
            static_cast<uint8_t>(candidate_formats.size()),
            candidate_formats.data(),
            VK_IMAGE_TILING_OPTIMAL,
            VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
        );
    }

    //-------------------------------------------------------------------------------------------------

    [[nodiscard]]
    VkFormat FindSupportedFormat(
        VkPhysicalDevice physical_device,
        uint8_t const candidates_count,
        VkFormat * candidates,
        VkImageTiling const tiling,
        VkFormatFeatureFlags const features
    )
    {
        for (uint8_t index = 0; index < candidates_count; index++)
        {
            VkFormatProperties props;
            vkGetPhysicalDeviceFormatProperties(physical_device, candidates[index], &props);

            if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features)
            {
                return candidates[index];
            }
            if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features)
            {
                return candidates[index];
            }
        }
        MFA_CRASH("Failed to find supported format!");
    }

    //-------------------------------------------------------------------------------------------------

    void TransferImageLayout(
        VkDevice device,
        VkQueue graphicQueue,
        VkCommandPool commandPool,
        VkImage image,
        VkImageLayout const oldLayout,
        VkImageLayout const newLayout,
        uint32_t const levelCount,
        uint32_t const layerCount
    )
    {
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
        )
        {
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

            source_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            destination_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        }
        else if (
             oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
             newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        )
        {
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            source_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            destination_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        }
        else
        {
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

    //-------------------------------------------------------------------------------------------------

    std::shared_ptr<RT::BufferAndMemory> CreateBuffer(
        VkDevice device,
        VkPhysicalDevice physicalDevice,
        VkDeviceSize const size,
        VkBufferUsageFlags const usage,
        VkMemoryPropertyFlags const properties
    )
    {
        MFA_ASSERT(device != nullptr);
        MFA_ASSERT(physicalDevice != nullptr);

        VkBufferCreateInfo buffer_info{};
        buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        buffer_info.size = size;
        buffer_info.usage = usage;
        buffer_info.sharingMode = VkSharingMode::VK_SHARING_MODE_EXCLUSIVE;

        VkBuffer buffer{};
        VK_Check(vkCreateBuffer(device, &buffer_info, nullptr, &buffer));
        MFA_VK_VALID_ASSERT(buffer);
        VkMemoryRequirements memory_requirements{};
        vkGetBufferMemoryRequirements(device, buffer, &memory_requirements);

        VkMemoryAllocateInfo memoryAllocationInfo{};
        memoryAllocationInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        memoryAllocationInfo.allocationSize = memory_requirements.size;
        memoryAllocationInfo.memoryTypeIndex = FindMemoryType(
            &physicalDevice,
            memory_requirements.memoryTypeBits,
            properties
        );

        VkDeviceMemory memory{};
        VK_Check(vkAllocateMemory(device, &memoryAllocationInfo, nullptr, &memory));
        VK_Check(vkBindBufferMemory(device, buffer, memory, 0));

        return std::make_shared<RT::BufferAndMemory>(buffer, memory);
    }

    //-------------------------------------------------------------------------------------------------

    void MapDataToBuffer(
        VkDevice device,
        VkDeviceMemory bufferMemory,
        CBlob const dataBlob
    )
    {
        MFA_ASSERT(dataBlob.ptr != nullptr);
        MFA_ASSERT(dataBlob.len > 0);
        void * tempBufferData = nullptr;
        VK_Check(vkMapMemory(device, bufferMemory, 0, dataBlob.len, 0, &tempBufferData));
        MFA_ASSERT(tempBufferData != nullptr);
        ::memcpy(tempBufferData, dataBlob.ptr, dataBlob.len);
        vkUnmapMemory(device, bufferMemory);
    }

    //-------------------------------------------------------------------------------------------------

    void CopyBuffer(
        VkDevice device,
        VkCommandPool commandPool,
        VkQueue graphicQueue,
        VkBuffer sourceBuffer,
        VkBuffer destinationBuffer,
        VkDeviceSize const size
    )
    {
        auto * commandBuffer = BeginSingleTimeCommand(device, commandPool);

        VkBufferCopy copyRegion{};
        copyRegion.size = size;
        vkCmdCopyBuffer(
            commandBuffer,
            sourceBuffer,
            destinationBuffer,
            1,
            &copyRegion
        );

        EndAndSubmitSingleTimeCommand(device, commandPool, graphicQueue, commandBuffer);
    }

    //-------------------------------------------------------------------------------------------------

    void DestroyBuffer(
        VkDevice device,
        const RT::BufferAndMemory & bufferGroup
    )
    {
        MFA_ASSERT(device != nullptr);
        MFA_VK_VALID_ASSERT(bufferGroup.memory);
        MFA_VK_VALID_ASSERT(bufferGroup.buffer);
        vkFreeMemory(device, bufferGroup.memory, nullptr);
        vkDestroyBuffer(device, bufferGroup.buffer, nullptr);
    }

    //-------------------------------------------------------------------------------------------------

    std::shared_ptr<RT::ImageGroup> CreateImage(
        VkDevice device,
        VkPhysicalDevice physicalDevice,
        uint32_t const width,
        uint32_t const height,
        uint32_t const depth,
        uint8_t const mipLevels,
        uint16_t const sliceCount,
        VkFormat const format,
        VkImageTiling const tiling,
        VkImageUsageFlags const usage,
        VkSampleCountFlagBits const samplesCount,
        VkMemoryPropertyFlags const properties,
        VkImageCreateFlags const imageCreateFlags,
        VkImageType const imageType
    )
    {
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = imageType;
        imageInfo.extent.width = width;
        imageInfo.extent.height = height;
        imageInfo.extent.depth = depth;
        imageInfo.mipLevels = mipLevels;
        imageInfo.arrayLayers = sliceCount;
        imageInfo.format = format;
        imageInfo.tiling = tiling;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = usage;
        imageInfo.samples = samplesCount;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageInfo.flags = imageCreateFlags;

        VkImage image{};
        VK_Check(vkCreateImage(device, &imageInfo, nullptr, &image));

        VkMemoryRequirements memory_requirements;
        vkGetImageMemoryRequirements(device, image, &memory_requirements);

        VkMemoryAllocateInfo allocate_info{};
        allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocate_info.allocationSize = memory_requirements.size;
        allocate_info.memoryTypeIndex = FindMemoryType(
            &physicalDevice,
            memory_requirements.memoryTypeBits,
            properties
        );

        VkDeviceMemory memory{};
        VK_Check(vkAllocateMemory(device, &allocate_info, nullptr, &memory));
        VK_Check(vkBindImageMemory(device, image, memory, 0));

        return std::make_shared<RT::ImageGroup>(image, memory);
    }

    //-------------------------------------------------------------------------------------------------

    void DestroyImage(
        VkDevice device,
        RT::ImageGroup const & imageGroup
    )
    {
        vkDestroyImage(device, imageGroup.image, nullptr);
        vkFreeMemory(device, imageGroup.memory, nullptr);
    }

    //-------------------------------------------------------------------------------------------------

    std::shared_ptr<RT::GpuTexture> CreateTexture(
        AS::Texture const & cpuTexture,
        VkDevice device,
        VkPhysicalDevice physicalDevice,
        VkQueue graphicQueue,
        VkCommandPool commandPool
    )
    {
        MFA_ASSERT(device != nullptr);
        MFA_ASSERT(physicalDevice != nullptr);
        MFA_ASSERT(graphicQueue != nullptr);
        MFA_VK_VALID_ASSERT(commandPool);
        MFA_ASSERT(cpuTexture.isValid());

        if (cpuTexture.isValid())
        {
            auto const format = cpuTexture.GetFormat();
            auto const mipCount = cpuTexture.GetMipCount();
            auto const sliceCount = cpuTexture.GetSlices();
            auto const & largestMipmapInfo = cpuTexture.GetMipmap(0);
            auto const buffer = cpuTexture.GetBuffer();
            MFA_ASSERT(buffer.ptr != nullptr && buffer.len > 0);
            // Create upload buffer
            auto const uploadBufferGroup = CreateBuffer(
                device,
                physicalDevice,
                buffer.len,
                VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
            );
            //MFA_DEFER{ DestroyBuffer(device, uploadBufferGroup); };
            // Map texture data to buffer
            MapDataToBuffer(device, uploadBufferGroup->memory, buffer);

            auto const vulkan_format = ConvertCpuTextureFormatToGpu(format);

            auto imageGroup = CreateImage(
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
                VK_SAMPLE_COUNT_1_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
            );

            TransferImageLayout(
                device,
                graphicQueue,
                commandPool,
                imageGroup->image,
                VK_IMAGE_LAYOUT_UNDEFINED,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                mipCount,
                sliceCount
            );

            CopyBufferToImage(
                device,
                commandPool,
                uploadBufferGroup->buffer,
                imageGroup->image,
                graphicQueue,
                cpuTexture
            );

            TransferImageLayout(
                device,
                graphicQueue,
                commandPool,
                imageGroup->image,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                mipCount,
                sliceCount
            );

            auto imageView = CreateImageView(
                device,
                imageGroup->image,
                vulkan_format,
                VK_IMAGE_ASPECT_COLOR_BIT,
                mipCount,
                sliceCount,
                VK_IMAGE_VIEW_TYPE_2D
            );

            std::shared_ptr<RT::GpuTexture> gpuTexture = std::make_shared<RT::GpuTexture>(
                std::move(imageGroup),
                std::move(imageView)
            );
            return gpuTexture;
        }
        return nullptr;
    }

    //-------------------------------------------------------------------------------------------------
    // I probable will delete many of these functions because they are no longer required to exist
    void DestroyTexture(VkDevice device, RT::GpuTexture const & gpuTexture)
    {
        MFA_ASSERT(device != nullptr);
        MFA_ASSERT(device != nullptr);
        DestroyImage(
            device,
            *gpuTexture.imageGroup
        );
        DestroyImageView(
            device,
            *gpuTexture.imageView
        );
    }

    //-------------------------------------------------------------------------------------------------

    VkFormat ConvertCpuTextureFormatToGpu(AssetSystem::TextureFormat const cpuFormat)
    {
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

    //-------------------------------------------------------------------------------------------------

    void CopyBufferToImage(
        VkDevice device,
        VkCommandPool commandPool,
        VkBuffer buffer,
        VkImage image,
        VkQueue graphicQueue,
        AS::Texture const & cpuTexture
    )
    {
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
        auto * regionsArray = regionsBlob->memory.as<VkBufferImageCopy>();
        for (uint8_t sliceIndex = 0; sliceIndex < slices; sliceIndex++)
        {
            for (uint8_t mipLevel = 0; mipLevel < mipCount; mipLevel++)
            {
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

    //-------------------------------------------------------------------------------------------------

    RT::LogicalDevice CreateLogicalDevice(
        VkPhysicalDevice physicalDevice,
        uint32_t const graphicsQueueFamily,
        uint32_t const presentQueueFamily,
        VkPhysicalDeviceFeatures const & enabledPhysicalDeviceFeatures
    )
    {
        RT::LogicalDevice logicalDevice{};

        MFA_ASSERT(physicalDevice != nullptr);

        // Create one graphics queue and optionally a separate presentation queue
        float const queuePriority = 1.0f;

        VkDeviceQueueCreateInfo queueCreateInfo[2] = {};

        queueCreateInfo[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo[0].queueFamilyIndex = graphicsQueueFamily;
        queueCreateInfo[0].queueCount = 1;
        queueCreateInfo[0].pQueuePriorities = &queuePriority;

        queueCreateInfo[1].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo[1].queueFamilyIndex = presentQueueFamily;
        queueCreateInfo[1].queueCount = 1;
        queueCreateInfo[1].pQueuePriorities = &queuePriority;

        // Create logical device from physical device
        // Note: there are separate instance and device extensions!
        VkDeviceCreateInfo deviceCreateInfo = {};
        deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        deviceCreateInfo.pQueueCreateInfos = queueCreateInfo;

        if (graphicsQueueFamily == presentQueueFamily)
        {
            deviceCreateInfo.queueCreateInfoCount = 1;
        }
        else
        {
            deviceCreateInfo.queueCreateInfoCount = 2;
        }

        std::vector<char const *> enabledExtensionNames{ VK_KHR_SWAPCHAIN_EXTENSION_NAME };
    #if defined(__PLATFORM_MAC__)// TODO We should query instead
        enabledExtensionNames.emplace_back("VK_KHR_portability_subset");
    #endif
        enabledExtensionNames.emplace_back(VK_KHR_STORAGE_BUFFER_STORAGE_CLASS_EXTENSION_NAME);
        deviceCreateInfo.ppEnabledExtensionNames = enabledExtensionNames.data();
        deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(enabledExtensionNames.size());
        // Necessary for shader (for some reason)
        deviceCreateInfo.pEnabledFeatures = &enabledPhysicalDeviceFeatures;
    #if defined(MFA_DEBUG) && defined(__ANDROID__) == false
        deviceCreateInfo.enabledLayerCount = static_cast<uint32_t>(DebugLayers.size());
        deviceCreateInfo.ppEnabledLayerNames = DebugLayers.data();
    #else
        deviceCreateInfo.enabledLayerCount = 0;
        deviceCreateInfo.ppEnabledLayerNames = nullptr;
    #endif

        VK_Check(vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &logicalDevice.device));
        MFA_ASSERT(logicalDevice.device != nullptr);
        MFA_LOG_INFO("Logical device create was successful");
        vkGetPhysicalDeviceMemoryProperties(physicalDevice, &logicalDevice.physicalMemoryProperties);

        return logicalDevice;
    }

    //-------------------------------------------------------------------------------------------------

    void DestroyLogicalDevice(RT::LogicalDevice const & logicalDevice)
    {
        vkDestroyDevice(logicalDevice.device, nullptr);
    }

    //-------------------------------------------------------------------------------------------------

    VkQueue GetQueueByFamilyIndex(
        VkDevice device,
        uint32_t const queueFamilyIndex
    )
    {
        VkQueue graphic_queue = nullptr;
        vkGetDeviceQueue(
            device,
            queueFamilyIndex,
            0,
            &graphic_queue
        );
        return graphic_queue;
    }

    //-------------------------------------------------------------------------------------------------

    std::shared_ptr<RT::SamplerGroup> CreateSampler(
        VkDevice device,
        RT::CreateSamplerParams const & params
    )
    {
        MFA_ASSERT(device != nullptr);
        VkSampler sampler{};

        VkSamplerCreateInfo sampler_info{};
        sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        sampler_info.magFilter = VK_FILTER_LINEAR;
        sampler_info.minFilter = VK_FILTER_LINEAR;
        // TODO Important, Get this parameter from outside
        sampler_info.addressModeU = params.addressModeU;
        sampler_info.addressModeV = params.addressModeV;
        sampler_info.addressModeW = params.addressModeW;
    #if defined(__DESKTOP__) || defined(__IOS__)
        sampler_info.anisotropyEnable = params.anisotropyEnabled;
    #elif defined(__ANDROID__)
        sampler_info.anisotropyEnable = false;
    #else
    #error Os not handled
    #endif
        sampler_info.maxAnisotropy = params.maxAnisotropy;
        sampler_info.borderColor = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
        sampler_info.unnormalizedCoordinates = VK_FALSE;
        /*
         *TODO Issue for mac:
        "Message code: 0\nMessage: Validation Error: [ VUID-VkDescriptorImageInfo-mutableComparisonSamplers-04450 ] Object 0: handle = 0x62200004c918, type = VK_OBJECT_TYPE_DEVICE; | MessageID = 0xf467460 | vkUpdateDescriptorSets() (portability error): sampler comparison not available. The Vulkan spec states: If the [VK_KHR_portability_subset] extension is enabled, and VkPhysicalDevicePortabilitySubsetFeaturesKHR::mutableComparisonSamplers is VK_FALSE, then sampler must have been created with VkSamplerCreateInfo::compareEnable set to VK_FALSE. (https://vulkan.lunarg.com/doc/view/1.2.182.0/mac/1.2-extensions/vkspec.html#VUID-VkDescriptorImageInfo-mutableComparisonSamplers-04450)\nLocation: 256275552\n"
         **/
    #ifdef __PLATFORM_MAC__
        sampler_info.compareEnable = VK_FALSE;
    #else
        sampler_info.compareEnable = VK_TRUE;
    #endif
        sampler_info.compareOp = VK_COMPARE_OP_ALWAYS;
        sampler_info.minLod = params.minLod;
        sampler_info.maxLod = params.maxLod;
        sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        sampler_info.mipLodBias = 0.0f;

        VK_Check(vkCreateSampler(device, &sampler_info, nullptr, &sampler));

        return std::make_shared<RT::SamplerGroup>(sampler);
    }

    //-------------------------------------------------------------------------------------------------

    // TODO Create wrapper for all vk types
    void DestroySampler(VkDevice device, RT::SamplerGroup const & sampler)
    {
        MFA_ASSERT(device != nullptr);
        MFA_VK_VALID_ASSERT(sampler.sampler);
        vkDestroySampler(device, sampler.sampler, nullptr);
    }

    //-------------------------------------------------------------------------------------------------

    VkDebugReportCallbackEXT SetDebugCallback(
        VkInstance instance,
        PFN_vkDebugReportCallbackEXT const & callback
    )
    {
        VkDebugReportCallbackCreateInfoEXT debug_info = {};
        debug_info.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
        debug_info.pfnCallback = callback;
        debug_info.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
        auto const debug_report_callback = reinterpret_cast<PFN_vkCreateDebugReportCallbackEXT>(vkGetInstanceProcAddr(
            instance,
            "vkCreateDebugReportCallbackEXT"
        ));
        VkDebugReportCallbackEXT ret{};
        VK_Check(debug_report_callback(
            instance,
            &debug_info,
            nullptr,
            &ret
        ));
        return ret;
    }

    //-------------------------------------------------------------------------------------------------

    static FindPhysicalDeviceResult findPhysicalDevice(VkInstance instance, uint8_t const retryCount)
    {
        FindPhysicalDeviceResult ret{};

        uint32_t deviceCount = 0;
        //Getting number of physical devices
        VK_Check(vkEnumeratePhysicalDevices(
            instance,
            &deviceCount,
            nullptr
        ));
        MFA_ASSERT(deviceCount > 0);

        std::vector<VkPhysicalDevice> devices(deviceCount);
        const auto phyDevResult = vkEnumeratePhysicalDevices(
            instance,
            &deviceCount,
            devices.data()
        );
        //TODO Search about incomplete
        MFA_ASSERT(phyDevResult == VK_SUCCESS || phyDevResult == VK_INCOMPLETE);
        // TODO We need to choose physical device based on features that we need
        if (retryCount >= devices.size())
        {
            MFA_CRASH("No suitable physical device exists");
        }
        ret.physicalDevice = devices[retryCount];
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

        ret.maxSampleCount = ComputeMaxUsableSampleCount(deviceProperties);
        ret.physicalDeviceProperties = deviceProperties;

        return ret;
    }

    //-------------------------------------------------------------------------------------------------

    FindPhysicalDeviceResult FindPhysicalDevice(VkInstance instance)
    {
        return findPhysicalDevice(instance, 0);
    }

    //-------------------------------------------------------------------------------------------------

    bool CheckSwapChainSupport(VkPhysicalDevice physical_device)
    {
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
        for (const auto & extension : device_extensions)
        {
            if (strcmp(extension.extensionName, VK_KHR_SWAPCHAIN_EXTENSION_NAME) == 0)
            {
                MFA_LOG_INFO("Physical device supports swap chains");
                ret = true;
                break;
            }
        }
        return ret;
    }

    //-------------------------------------------------------------------------------------------------

    FindQueueFamilyResult FindQueueFamilies(
        VkPhysicalDevice physicalDevice,
        VkSurfaceKHR windowSurface
    )
    {

        bool isPresentQueueSet = false;
        uint32_t presentQueueFamily = -1;

        bool isGraphicQueueSet = false;
        uint32_t graphicQueueFamily = -1;

        bool isComputeQueueSet = false;
        uint32_t computeQueueFamily = -1;

        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);
        if (queueFamilyCount == 0)
        {
            MFA_CRASH("physical device has no queue families!");
        }
        // Find queue family with graphics support
        // Note: is a transfer queue necessary to copy vertices to the gpu or can a graphics queue handle that?
        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(
            physicalDevice,
            &queueFamilyCount,
            queueFamilies.data()
        );

        MFA_LOG_INFO("physical device has %d queue families.", queueFamilyCount);

        for (uint32_t queueIndex = 0; queueIndex < queueFamilyCount; queueIndex++)
        {
            if (isPresentQueueSet == false)
            {
                VkBool32 presentIsSupported = false;
                vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, queueIndex, windowSurface, &presentIsSupported);
                if (presentIsSupported) {
                    presentQueueFamily = queueIndex;
                    isPresentQueueSet = true;
                }
            }

            auto & queueFamily = queueFamilies[queueIndex];
            if (queueFamily.queueCount > 0)
            {
                if (isGraphicQueueSet == false && (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT))
                {
                    graphicQueueFamily = queueIndex;
                    isGraphicQueueSet = true;
                }
                if (isComputeQueueSet == false && (queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT))
                {
                    computeQueueFamily = queueIndex;
                    isComputeQueueSet = true;
                }
            }

            if (isPresentQueueSet && isGraphicQueueSet && isComputeQueueSet)
            {
                break;
            }
        }

        MFA_REQUIRE(isPresentQueueSet);
        MFA_REQUIRE(isGraphicQueueSet);
        MFA_REQUIRE(isComputeQueueSet);

        return FindQueueFamilyResult {
            .isPresentQueueValid = isPresentQueueSet,
            .presentQueueFamily = presentQueueFamily,

            .isGraphicQueueValid = isGraphicQueueSet,
            .graphicQueueFamily = graphicQueueFamily,

            .isComputeQueueValid = isComputeQueueSet,
            .computeQueueFamily = computeQueueFamily
        };
    }

    //-------------------------------------------------------------------------------------------------

    VkCommandPool CreateCommandPool(VkDevice device, uint32_t const queue_family_index)
    {
        MFA_ASSERT(device);
        // Create graphics command pool
        VkCommandPoolCreateInfo poolCreateInfo = {};
        poolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolCreateInfo.queueFamilyIndex = queue_family_index;
        poolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

        VkCommandPool command_pool{};
        VK_Check(vkCreateCommandPool(device, &poolCreateInfo, nullptr, &command_pool));

        return command_pool;
    }

    //-------------------------------------------------------------------------------------------------

    void DestroyCommandPool(VkDevice device, VkCommandPool commandPool)
    {
        MFA_ASSERT(device);
        vkDestroyCommandPool(device, commandPool, nullptr);
    }

    //-------------------------------------------------------------------------------------------------

    VkSurfaceCapabilitiesKHR GetSurfaceCapabilities(
        VkPhysicalDevice physicalDevice,
        VkSurfaceKHR windowSurface
    )
    {
        MFA_VK_VALID_ASSERT(physicalDevice);
        MFA_VK_VALID_ASSERT(windowSurface);
        VkSurfaceCapabilitiesKHR surfaceCapabilities;
        VK_Check(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, windowSurface, &surfaceCapabilities));
        return surfaceCapabilities;
    }

    //-------------------------------------------------------------------------------------------------

    std::shared_ptr<RT::SwapChainGroup> CreateSwapChain(
        VkDevice device,
        VkPhysicalDevice physicalDevice,
        VkSurfaceKHR windowSurface,
        VkSurfaceCapabilitiesKHR surfaceCapabilities,
        VkSwapchainKHR oldSwapChain
    )
    {
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
        )
        {
            MFA_CRASH("Failed to get number of supported surface formats");
        }

        std::vector<VkSurfaceFormatKHR> surfaceFormats(formatCount);
        VK_Check(vkGetPhysicalDeviceSurfaceFormatsKHR(
            physicalDevice,
            windowSurface,
            &formatCount,
            surfaceFormats.data()
        ));

        // Find supported present modes
        uint32_t presentModeCount;
        if (vkGetPhysicalDeviceSurfacePresentModesKHR(
            physicalDevice,
            windowSurface,
            &presentModeCount,
            nullptr
        ) != VK_SUCCESS || presentModeCount == 0)
        {
            MFA_CRASH("Failed to get number of supported presentation modes");
        }

        std::vector<VkPresentModeKHR> present_modes(presentModeCount);
        VK_Check(vkGetPhysicalDeviceSurfacePresentModesKHR(
            physicalDevice,
            windowSurface,
            &presentModeCount,
            present_modes.data())
        );

        // Determine number of images for swap chain
        auto const imageCount = ComputeSwapChainImagesCount(surfaceCapabilities);

        MFA_LOG_INFO(
            "Surface extend width: %d, height: %d"
            , surfaceCapabilities.currentExtent.width
            , surfaceCapabilities.currentExtent.height
        );

        MFA_LOG_INFO("Using %d images for swap chain.", imageCount);
        MFA_ASSERT(surfaceFormats.size() <= 255);
        // Select a surface format
        auto const selectedSurfaceFormat = ChooseSurfaceFormat(
            static_cast<uint8_t>(surfaceFormats.size()),
            surfaceFormats.data()
        );

        // Select swap chain size
        auto const selected_swap_chain_extent = ChooseSwapChainExtent(
            surfaceCapabilities,
            surfaceCapabilities.currentExtent.width,
            surfaceCapabilities.currentExtent.height
        );

        // Determine transformation to use (preferring no transform)
        /*VkSurfaceTransformFlagBitsKHR surfaceTransform;*/
        //if (surfaceCapabilities.supportedTransforms & VK_SURFACE_TRANSFORM_ROTATE_90_BIT_KHR) {
            //surfaceTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
        //} else if (surfaceCapabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR) {
            //surfaceTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
        //} else {
        //surfaceTransform = surfaceCapabilities.currentTransform;
        //}
        VkSurfaceTransformFlagBitsKHR const surfaceTransform = surfaceCapabilities.currentTransform;

        MFA_ASSERT(present_modes.size() <= 255);
        // Choose presentation mode (preferring MAILBOX ~= triple buffering)
        auto const selected_present_mode = ChoosePresentMode(static_cast<uint8_t>(present_modes.size()), present_modes.data());

        // Finally, create the swap chain
        VkSwapchainCreateInfoKHR createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.surface = windowSurface;
        createInfo.minImageCount = imageCount;
        createInfo.imageFormat = selectedSurfaceFormat.format;
        createInfo.imageColorSpace = selectedSurfaceFormat.colorSpace;
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

        VkSwapchainKHR swapChain{};
        VK_Check(vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain));

        MFA_LOG_INFO("Created swap chain");

        //swapChainGroup.swapChainFormat = selected_surface_format.format;
        VkFormat const swapChainFormat = selectedSurfaceFormat.format;

        // Store the images used by the swap chain
        // Note: these are the images that swap chain image indices refer to
        // Note: actual number of images may differ from requested number, since it's a lower bound
        uint32_t actualImageCount = 0;
        if (
            vkGetSwapchainImagesKHR(device, swapChain, &actualImageCount, nullptr) != VK_SUCCESS ||
            actualImageCount == 0
        )
        {
            MFA_CRASH("Failed to acquire number of swap chain images");
        }

        std::vector<VkImage> swapChainImages(actualImageCount);
        VK_Check(vkGetSwapchainImagesKHR(
            device,
            swapChain,
            &actualImageCount,
            swapChainImages.data()
        ));

        std::vector<std::shared_ptr<RT::ImageViewGroup>> swapChainImageViews(actualImageCount);
        for (uint32_t imageIndex = 0; imageIndex < actualImageCount; imageIndex++)
        {
            MFA_VK_VALID_ASSERT(swapChainImages[imageIndex]);
            swapChainImageViews[imageIndex] = CreateImageView(
                device,
                swapChainImages[imageIndex],
                swapChainFormat,
                VK_IMAGE_ASPECT_COLOR_BIT,
                1,
                1,
                VK_IMAGE_VIEW_TYPE_2D
            );
            MFA_VK_VALID_ASSERT(swapChainImageViews[imageIndex]);
        }

        MFA_LOG_INFO("Acquired swap chain images");
        return std::make_shared<RT::SwapChainGroup>(
            swapChain,
            swapChainFormat,
            swapChainImages,
            swapChainImageViews
        );
    }

    //-------------------------------------------------------------------------------------------------

    void DestroySwapChain(VkDevice device, RT::SwapChainGroup const & swapChainGroup)
    {
        MFA_ASSERT(device);
        MFA_ASSERT(swapChainGroup.swapChain);
        MFA_ASSERT(swapChainGroup.swapChainImageViews.empty() == false);
        MFA_ASSERT(swapChainGroup.swapChainImages.empty() == false);
        MFA_ASSERT(swapChainGroup.swapChainImages.size() == swapChainGroup.swapChainImageViews.size());
        vkDestroySwapchainKHR(device, swapChainGroup.swapChain, nullptr);
    }

    //-------------------------------------------------------------------------------------------------

    std::shared_ptr<RT::DepthImageGroup> CreateDepthImage(
        VkPhysicalDevice physicalDevice,
        VkDevice device,
        VkExtent2D const imageExtent,
        VkFormat depthFormat,
        RT::CreateDepthImageOptions const & options

    )
    {
        //RT::DepthImageGroup depthImageGroup{};
        auto imageGroup = CreateImage(
            device,
            physicalDevice,
            imageExtent.width,
            imageExtent.height,
            1,
            1,
            options.layerCount,
            depthFormat,
            VK_IMAGE_TILING_OPTIMAL,
            options.usageFlags,
            options.samplesCount,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            options.imageCreateFlags,
            options.imageType
        );
        MFA_ASSERT(imageGroup->image);
        MFA_ASSERT(imageGroup->memory);
        auto imageView = CreateImageView(
            device,
            imageGroup->image,
            depthFormat,
            VK_IMAGE_ASPECT_DEPTH_BIT,
            1,
            options.layerCount,
            options.viewType
        );
        MFA_ASSERT(imageView);
        return std::make_shared<RT::DepthImageGroup>(
            std::move(imageGroup),
            std::move(imageView),
            depthFormat
        );
    }

    //-------------------------------------------------------------------------------------------------

    std::shared_ptr<RT::ColorImageGroup> CreateColorImage(
        VkPhysicalDevice physicalDevice,
        VkDevice device,
        VkExtent2D const & imageExtent,
        VkFormat const imageFormat,
        RT::CreateColorImageOptions const & options
    )
    {
        auto imageGroup = CreateImage(
            device,
            physicalDevice,
            imageExtent.width,
            imageExtent.height,
            1,
            1,
            options.layerCount,
            imageFormat,
            VK_IMAGE_TILING_OPTIMAL,
            options.usageFlags,
            options.samplesCount,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            options.imageCreateFlags,
            options.imageType
        );
        MFA_ASSERT(imageGroup->image);
        MFA_ASSERT(imageGroup->memory);
        auto imageView = CreateImageView(
            device,
            imageGroup->image,
            imageFormat,
            VK_IMAGE_ASPECT_COLOR_BIT,
            1,
            options.layerCount,
            options.viewType
        );
        MFA_ASSERT(imageView);
        return std::make_shared<RT::ColorImageGroup>(
            std::move(imageGroup),
            std::move(imageView),
            imageFormat
        );
    }

    //-------------------------------------------------------------------------------------------------

    VkRenderPass CreateRenderPass(
        VkDevice device,
        VkAttachmentDescription * attachments,
        uint32_t attachmentsCount,
        VkSubpassDescription * subPasses,
        uint32_t subPassesCount,
        VkSubpassDependency * dependencies,
        uint32_t dependenciesCount
    )
    {
        VkRenderPassCreateInfo const createInfo{
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
            .attachmentCount = attachmentsCount,
            .pAttachments = attachments,
            .subpassCount = subPassesCount,
            .pSubpasses = subPasses,
            .dependencyCount = dependenciesCount,
            .pDependencies = dependencies,
        };

        VkRenderPass renderPass{};
        VK_Check(vkCreateRenderPass(device, &createInfo, nullptr, &renderPass));

        MFA_LOG_INFO("Created render pass.");

        return renderPass;
    }

    //-------------------------------------------------------------------------------------------------

    void DestroyRenderPass(VkDevice device, VkRenderPass renderPass)
    {
        MFA_ASSERT(device != nullptr);
        MFA_VK_VALID_ASSERT(renderPass);
        vkDestroyRenderPass(device, renderPass, nullptr);
    }

    //-------------------------------------------------------------------------------------------------

    void BeginRenderPass(
        VkCommandBuffer commandBuffer,
        VkRenderPassBeginInfo const & renderPassBeginInfo
    )
    {
        vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
    }

    //-------------------------------------------------------------------------------------------------

    void EndRenderPass(VkCommandBuffer commandBuffer)
    {
        vkCmdEndRenderPass(commandBuffer);
    }

    //-------------------------------------------------------------------------------------------------

    VkFramebuffer CreateFrameBuffers(
        VkDevice device,
        VkRenderPass renderPass,
        VkImageView const * attachments,
        uint32_t attachmentsCount,
        VkExtent2D const swapChainExtent,
        uint32_t layersCount
    )
    {
        MFA_ASSERT(device != nullptr);
        MFA_VK_VALID_ASSERT(renderPass);
        MFA_ASSERT(attachments != nullptr);
        MFA_ASSERT(attachmentsCount > 0);

        VkFramebuffer frameBuffer{};

        // Note: FrameBuffer is basically a specific choice of attachments for a render pass
        // That means all attachments must have the same dimensions, interesting restriction

        VkFramebufferCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        createInfo.renderPass = renderPass;
        createInfo.attachmentCount = attachmentsCount;
        createInfo.pAttachments = attachments;
        createInfo.width = swapChainExtent.width;
        createInfo.height = swapChainExtent.height;
        createInfo.layers = layersCount;

        VK_Check(vkCreateFramebuffer(device, &createInfo, nullptr, &frameBuffer));

        return frameBuffer;
    }

    //-------------------------------------------------------------------------------------------------

    void DestroyFrameBuffers(
        VkDevice device,
        uint32_t const frameBuffersCount,
        VkFramebuffer * frameBuffers
    )
    {
        for (uint32_t index = 0; index < frameBuffersCount; index++)
        {
            vkDestroyFramebuffer(device, frameBuffers[index], nullptr);
        }
    }

    //-------------------------------------------------------------------------------------------------

    std::shared_ptr<RT::GpuShader> CreateShader(
        VkDevice device,
        std::shared_ptr<AS::Shader> const & cpuShader
    )
    {
        MFA_ASSERT(device != nullptr);
        std::shared_ptr<RT::GpuShader> gpuShader = nullptr;
        if (cpuShader->isValid())
        {
            auto const & shaderCode = cpuShader->compiledShaderCode->memory;

            VkShaderModule shaderModule{};

            VkShaderModuleCreateInfo const createInfo{
                .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
                .codeSize = shaderCode.len,
                .pCode = reinterpret_cast<uint32_t const *>(shaderCode.ptr),
            };
            VK_Check(vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule));
            MFA_LOG_INFO("Creating shader module was successful");
            gpuShader = std::make_shared<RT::GpuShader>(
                shaderModule,
                ConvertAssetShaderStageToGpu(cpuShader->stage),
                cpuShader->entryPoint
            );
        }
        return gpuShader;
    }

    //-------------------------------------------------------------------------------------------------

    void DestroyShader(VkDevice device, RT::GpuShader const & gpuShader)
    {
        MFA_ASSERT(device != nullptr);
        vkDestroyShaderModule(device, gpuShader.shaderModule, nullptr);
    }

    //-------------------------------------------------------------------------------------------------

    std::shared_ptr<RT::PipelineGroup> CreatePipelineGroup(
        VkDevice device,
        uint8_t shaderStagesCount,
        RT::GpuShader const ** shaderStages,
        uint32_t vertexBindingDescriptionCount,
        VkVertexInputBindingDescription const * vertexBindingDescriptionData,
        uint32_t attributeDescriptionCount,
        VkVertexInputAttributeDescription * attributeDescriptionData,
        VkExtent2D swapChainExtent,
        VkRenderPass renderPass,
        uint32_t descriptorSetLayoutCount,
        VkDescriptorSetLayout const * descriptorSetLayouts,
        RT::CreateGraphicPipelineOptions const & options
    )
    {
        MFA_ASSERT(shaderStages);
        MFA_ASSERT(renderPass);
        MFA_ASSERT(descriptorSetLayouts);
        // Set up shader stage info
        std::vector<VkPipelineShaderStageCreateInfo> shaderStagesCreateInfos(shaderStagesCount);
        for (uint8_t i = 0; i < shaderStagesCount; i++)
        {
            VkPipelineShaderStageCreateInfo create_info = {};
            create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            create_info.stage = shaderStages[i]->stageFlags;
            create_info.module = shaderStages[i]->shaderModule;
            create_info.pName = shaderStages[i]->entryPointName.c_str();
            shaderStagesCreateInfos[i] = create_info;
        }

        // Describe vertex input
        VkPipelineVertexInputStateCreateInfo vertex_input_state_create_info = {};
        vertex_input_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertex_input_state_create_info.vertexBindingDescriptionCount = vertexBindingDescriptionCount;
        vertex_input_state_create_info.pVertexBindingDescriptions = vertexBindingDescriptionData;
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
        VkViewport viewport{};
        VkRect2D scissor{};

        if (true == options.useStaticViewportAndScissor)
        {
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
        rasterization_create_info.polygonMode = VK_POLYGON_MODE_FILL;
        rasterization_create_info.cullMode = options.cullMode;
        rasterization_create_info.frontFace = options.frontFace;
        rasterization_create_info.depthBiasEnable = VK_FALSE;
        rasterization_create_info.lineWidth = 1.0f;
        rasterization_create_info.depthClampEnable = VK_FALSE;
        rasterization_create_info.rasterizerDiscardEnable = VK_FALSE;

        // Describe multi-sampling
        // Note: using multi-sampling also requires turning on device features
        VkPipelineMultisampleStateCreateInfo multi_sample_state_create_info = {};
        multi_sample_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multi_sample_state_create_info.rasterizationSamples = options.rasterizationSamples;
        multi_sample_state_create_info.sampleShadingEnable = VK_TRUE;
        multi_sample_state_create_info.minSampleShading = 0.5f; // It can be in range of 0 to 1. Min fraction for sample shading; closer to one is smooth
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

        VkPipelineLayout pipelineLayout{};
        VK_Check(vkCreatePipelineLayout(device, &layout_create_info, nullptr, &pipelineLayout));

        VkPipelineDynamicStateCreateInfo * dynamicStateCreateInfoRef = nullptr;
        VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo{};
        std::vector<VkDynamicState> dynamicStates{ VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
        if (options.useStaticViewportAndScissor == false)
        {
            if (options.dynamicStateCreateInfo == nullptr)
            {
                dynamicStateCreateInfo = VkPipelineDynamicStateCreateInfo{};
                dynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
                dynamicStateCreateInfo.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
                dynamicStateCreateInfo.pDynamicStates = dynamicStates.data();
                dynamicStateCreateInfoRef = &dynamicStateCreateInfo;
            }
            else
            {
                dynamicStateCreateInfoRef = options.dynamicStateCreateInfo;
            }
        }

        // Create the graphics pipeline
        VkGraphicsPipelineCreateInfo pipelineCreateInfo{};
        pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineCreateInfo.stageCount = static_cast<uint32_t>(shaderStagesCreateInfos.size());
        pipelineCreateInfo.pStages = shaderStagesCreateInfos.data();
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

        VkPipeline pipeline{};
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

        auto const pipelineGroup = std::make_shared<RT::PipelineGroup>(
            pipelineLayout,
            pipeline
        );
        MFA_ASSERT(pipelineGroup->isValid());

        return pipelineGroup;
    }

    //-------------------------------------------------------------------------------------------------

    void AssignViewportAndScissorToCommandBuffer(
        VkExtent2D const & extent2D,
        VkCommandBuffer commandBuffer
    )
    {
        MFA_ASSERT(commandBuffer != nullptr);

        VkViewport const viewport{
            .x = 0.0f,
            .y = 0.0f,
            .width = static_cast<float>(extent2D.width),
            .height = static_cast<float>(extent2D.height),
            .minDepth = 0.0f,
            .maxDepth = 1.0f,
        };

        VkRect2D scissor{
            .offset {
                .x = 0,
                .y = 0
            },
            .extent {
                .width = extent2D.width,
                .height = extent2D.height,
            }
        };

        SetViewport(commandBuffer, viewport);
        SetScissor(commandBuffer, scissor);
    }

    //-------------------------------------------------------------------------------------------------

    void DestroyPipelineGroup(VkDevice device, RT::PipelineGroup & pipelineGroup)
    {
        MFA_ASSERT(device != nullptr);
        MFA_ASSERT(pipelineGroup.isValid());
        vkDestroyPipeline(device, pipelineGroup.pipeline, nullptr);
        vkDestroyPipelineLayout(device, pipelineGroup.pipelineLayout, nullptr);
    }

    //-------------------------------------------------------------------------------------------------

    std::shared_ptr<RT::DescriptorSetLayoutGroup> CreateDescriptorSetLayout(
        VkDevice device,
        uint8_t const bindings_count,
        VkDescriptorSetLayoutBinding * bindings
    )
    {
        VkDescriptorSetLayoutCreateInfo descriptorLayoutCreateInfo = {};
        descriptorLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        descriptorLayoutCreateInfo.bindingCount = static_cast<uint32_t>(bindings_count);
        descriptorLayoutCreateInfo.pBindings = bindings;
        VkDescriptorSetLayout descriptorSetLayout{};
        VK_Check(vkCreateDescriptorSetLayout(
            device,
            &descriptorLayoutCreateInfo,
            nullptr,
            &descriptorSetLayout
        ));
        return std::make_shared<RT::DescriptorSetLayoutGroup>(descriptorSetLayout);
    }

    //-------------------------------------------------------------------------------------------------

    void DestroyDescriptorSetLayout(
        VkDevice device,
        VkDescriptorSetLayout descriptor_set_layout
    )
    {
        vkDestroyDescriptorSetLayout(
            device,
            descriptor_set_layout,
            nullptr
        );
    }

    //-------------------------------------------------------------------------------------------------

    VkShaderStageFlagBits ConvertAssetShaderStageToGpu(AssetSystem::ShaderStage const stage)
    {
        switch (stage)
        {
        case AssetSystem::Shader::Stage::Vertex:
            return VK_SHADER_STAGE_VERTEX_BIT;
        case AssetSystem::Shader::Stage::Fragment:
            return VK_SHADER_STAGE_FRAGMENT_BIT;
        case AssetSystem::Shader::Stage::Geometry:
            return VK_SHADER_STAGE_GEOMETRY_BIT;
        case AssetSystem::Shader::Stage::Invalid:
        default:
            MFA_CRASH("Unhandled shader stage");
        }
    }

    //-------------------------------------------------------------------------------------------------

    void CreateVertexBuffer(
        VkDevice device,
        VkPhysicalDevice physicalDevice,
        VkCommandPool commandPool,
        VkQueue graphicQueue,
        CBlob const & verticesBlob,
        std::shared_ptr<RT::BufferAndMemory> & outVertexBuffer,
        std::shared_ptr<RT::BufferAndMemory> & inOutStagingBuffer
    )
    {
        VkDeviceSize const bufferSize = verticesBlob.len;

        if (inOutStagingBuffer == nullptr)
        {
            inOutStagingBuffer = CreateBuffer(
                device,
                physicalDevice,
                bufferSize,
                VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
            );
        }

        MapDataToBuffer(device, inOutStagingBuffer->memory, verticesBlob);

        outVertexBuffer = CreateBuffer(
            device,
            physicalDevice,
            bufferSize,
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
        );

        CopyBuffer(
            device,
            commandPool,
            graphicQueue,
            inOutStagingBuffer->buffer,
            outVertexBuffer->buffer,
            bufferSize
        );
    }

    //-------------------------------------------------------------------------------------------------

    void UpdateVertexBuffer(
        VkDevice device,
        VkCommandPool commandPool,
        VkQueue graphicQueue,
        CBlob const & verticesBlob,
        RT::BufferAndMemory const & vertexBuffer,
        RT::BufferAndMemory const & stagingBuffer
    )
    {
        VkDeviceSize const bufferSize = verticesBlob.len;

        MapDataToBuffer(device, stagingBuffer.memory, verticesBlob);

        CopyBuffer(
            device,
            commandPool,
            graphicQueue,
            stagingBuffer.buffer,
            vertexBuffer.buffer,
            bufferSize
        );
    }

    //-------------------------------------------------------------------------------------------------

    void CreateIndexBuffer(
        VkDevice device,
        VkPhysicalDevice physicalDevice,
        VkCommandPool commandPool,
        VkQueue graphicQueue,
        CBlob const & indicesBlob,
        std::shared_ptr<RT::BufferAndMemory> & outIndexBuffer,
        std::shared_ptr<RT::BufferAndMemory> & inOutStagingIndexBuffer
    )
    {
        auto const bufferSize = indicesBlob.len;

        if (inOutStagingIndexBuffer == nullptr)
        {
            inOutStagingIndexBuffer = CreateBuffer(
                device,
                physicalDevice,
                bufferSize,
                VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
            );
        }

        MapDataToBuffer(device, inOutStagingIndexBuffer->memory, indicesBlob);

        outIndexBuffer = CreateBuffer(
            device,
            physicalDevice,
            bufferSize,
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
        );

        CopyBuffer(
            device,
            commandPool,
            graphicQueue,
            inOutStagingIndexBuffer->buffer,
            outIndexBuffer->buffer,
            bufferSize
        );
    }

    //-------------------------------------------------------------------------------------------------
    /* About HOST_VISIBLE_BIT
     * It might be useful for things that are only updated by the CPU once per frame,
     * but used by the GPU many times per frame. If something, that is updated by the CPU,
     * is only used once by the GPU, you might as well let the GPU fetch it from system memory.
     * The alternate would be to have a CPU buffer and a GPU buffer and used a transfer command to move data from one to the other.
     */

    void CreateUniformBuffer(
        VkDevice device,
        VkPhysicalDevice physicalDevice,
        uint32_t const buffersCount,
        VkDeviceSize const buffersSize,
        std::shared_ptr<RT::BufferAndMemory> *outBuffers
    )
    {
        for (uint32_t index = 0; index < buffersCount; index++)
        {
            outBuffers[index] = CreateBuffer(
                device,
                physicalDevice,
                buffersSize,
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                // TODO: I believe that we should no use host visible bit for everything,
                // Based on frequency we should decide where to have HOST_VISIBLE_Buffer or staging buffer
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT             
            );
        }
    }

    //-------------------------------------------------------------------------------------------------

    void UpdateBuffer(
        VkDevice device,
        RT::BufferAndMemory const & bufferGroup,
        CBlob const data
    )
    {
        MapDataToBuffer(device, bufferGroup.memory, data);
    }

    //-------------------------------------------------------------------------------------------------

    void CreateStorageBuffer(
        VkDevice device,
        VkPhysicalDevice physicalDevice,
        uint32_t const buffersCount,
        VkDeviceSize const buffersSize,
        std::shared_ptr<RT::BufferAndMemory> *outStorageBuffer
    )
    {
        MFA_ASSERT(outStorageBuffer != nullptr);
        for (uint32_t index = 0; index < buffersCount; index++)
        {
            outStorageBuffer[index] = CreateBuffer(
                device,
                physicalDevice,
                buffersSize,
                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
            );
        }
    }

    //-------------------------------------------------------------------------------------------------

    VkDescriptorPool CreateDescriptorPool(
        VkDevice device,
        uint32_t const maxSets
    )
    {
        std::vector<VkDescriptorPoolSize> poolSizes{};
        // TODO Check if both of these variables must have same value as maxSets // Maybe I have to ask each of separately
        {// Uniform buffers
            VkDescriptorPoolSize poolSize;
            poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            poolSize.descriptorCount = maxSets;
            poolSizes.emplace_back(poolSize);
        }
        {// Combined image sampler
            VkDescriptorPoolSize poolSize;
            poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            poolSize.descriptorCount = maxSets;
            poolSizes.emplace_back(poolSize);
        }
        {// Sampled image
            VkDescriptorPoolSize poolSize;
            poolSize.type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
            poolSize.descriptorCount = maxSets;
            poolSizes.emplace_back(poolSize);
        }
        {// Sampler
            VkDescriptorPoolSize poolSize;
            poolSize.type = VK_DESCRIPTOR_TYPE_SAMPLER;
            poolSize.descriptorCount = maxSets;
            poolSizes.emplace_back(poolSize);
        }
        {// Storage buffer
            VkDescriptorPoolSize poolSize;
            poolSize.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            poolSize.descriptorCount = maxSets;
            poolSizes.emplace_back(poolSize);
        }
        VkDescriptorPoolCreateInfo poolInfo = {};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        poolInfo.pPoolSizes = poolSizes.data();
        poolInfo.maxSets = maxSets;

        VkDescriptorPool descriptor_pool{};

        VK_Check(vkCreateDescriptorPool(
            device,
            &poolInfo,
            nullptr,
            &descriptor_pool
        ));

        return descriptor_pool;
    }

    //-------------------------------------------------------------------------------------------------

    void DestroyDescriptorPool(
        VkDevice device,
        VkDescriptorPool pool
    )
    {
        MFA_ASSERT(device != nullptr);
        MFA_VK_VALID_ASSERT(pool);
        vkDestroyDescriptorPool(device, pool, nullptr);
    }

    //-------------------------------------------------------------------------------------------------

    RT::DescriptorSetGroup CreateDescriptorSet(
        VkDevice device,
        VkDescriptorPool descriptorPool,
        VkDescriptorSetLayout descriptorSetLayout,
        uint32_t const descriptorSetCount,
        uint8_t const schemasCount,
        VkWriteDescriptorSet * schemas
    )
    {
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

        std::vector<VkDescriptorSet> descriptorSets{};
        descriptorSets.resize(allocInfo.descriptorSetCount);
        // Descriptor sets gets destroyed automatically when descriptor pool is destroyed
        VK_Check(vkAllocateDescriptorSets(
            device,
            &allocInfo,
            descriptorSets.data()
        ));
        MFA_LOG_INFO("Create descriptor set is successful");

        if (schemasCount > 0 && schemas != nullptr)
        {
            MFA_ASSERT(descriptorSets.size() < 256);
            UpdateDescriptorSets(
                device,
                static_cast<uint8_t>(descriptorSets.size()),
                descriptorSets.data(),
                schemasCount,
                schemas
            );
        }

        return RT::DescriptorSetGroup{ .descriptorSets = descriptorSets };
    }

    //-------------------------------------------------------------------------------------------------

    void UpdateDescriptorSets(
        VkDevice const device,
        uint8_t const descriptorSetCount,
        VkDescriptorSet * descriptorSets,
        uint8_t const schemasCount,
        VkWriteDescriptorSet * schemas
    )
    {
        for (auto descriptor_set_index = 0; descriptor_set_index < descriptorSetCount; descriptor_set_index++)
        {
            std::vector<VkWriteDescriptorSet> descriptor_writes(schemasCount);
            for (uint8_t schema_index = 0; schema_index < schemasCount; schema_index++)
            {
                descriptor_writes[schema_index] = schemas[schema_index];
                descriptor_writes[schema_index].dstSet = descriptorSets[descriptor_set_index];
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

    //-------------------------------------------------------------------------------------------------

    std::vector<VkCommandBuffer> CreateCommandBuffers(
        VkDevice device,
        uint32_t const count,
        VkCommandPool commandPool
    )
    {
        std::vector<VkCommandBuffer> commandBuffers(count);

        VkCommandBufferAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = commandPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = count;

        VK_Check(vkAllocateCommandBuffers(
            device,
            &allocInfo,
            commandBuffers.data()
        ));

        MFA_LOG_INFO("Allocated graphics command buffers.");

        // Command buffer data gets recorded each time
        return commandBuffers;
    }

    //-------------------------------------------------------------------------------------------------

    void DestroyCommandBuffers(
        VkDevice device,
        VkCommandPool commandPool,
        uint32_t const commandBuffersCount,
        VkCommandBuffer * commandBuffers
    )
    {
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

    //-------------------------------------------------------------------------------------------------

    std::vector<VkSemaphore> CreateSemaphores(VkDevice device, uint32_t count)
    {
        VkSemaphoreCreateInfo const semaphoreInfo{
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
        };

        std::vector<VkSemaphore> semaphores (count);

        for (auto & semaphore : semaphores)
        {
            VK_Check(vkCreateSemaphore(
                device,
                &semaphoreInfo,
                nullptr,
                &semaphore
            ));
        }

        return semaphores;
    }
    
    //-------------------------------------------------------------------------------------------------

    void DestroySemaphored(VkDevice device, std::vector<VkSemaphore> const & semaphores)
    {
        for (auto & semaphore : semaphores)
        {
            vkDestroySemaphore(device, semaphore, nullptr);
        }
    }

    //-------------------------------------------------------------------------------------------------

    std::vector<VkFence> CreateFence(VkDevice device, uint32_t count)
    {
        VkFenceCreateInfo const fenceInfo{
            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
            .flags = VK_FENCE_CREATE_SIGNALED_BIT
        };

        std::vector<VkFence> fences (count);

        for (auto & fence : fences)
        {
            VK_Check(vkCreateFence(
                device,
                &fenceInfo,
                nullptr,
                &fence
            ));
        }

        return fences;
    }

    //-------------------------------------------------------------------------------------------------

    void DestroyFence(VkDevice device, std::vector<VkFence> const & fences)
    {
        for (auto & fence : fences)
        {
            vkDestroyFence(device, fence, nullptr);
        }
    }

    //-------------------------------------------------------------------------------------------------

    void DeviceWaitIdle(VkDevice device)
    {
        MFA_ASSERT(device != nullptr);
        VK_Check(vkDeviceWaitIdle(device));
    }

    //-------------------------------------------------------------------------------------------------

    void WaitForFence(VkDevice device, VkFence inFlightFence)
    {
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

    //-------------------------------------------------------------------------------------------------

    VkResult AcquireNextImage(
        VkDevice device,
        VkSemaphore imageAvailabilitySemaphore,
        RT::SwapChainGroup const & swapChainGroup,
        uint32_t & outImageIndex
    )
    {
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

    //-------------------------------------------------------------------------------------------------

    void BindVertexBuffer(
        VkCommandBuffer commandBuffer,
        RT::BufferAndMemory const & vertexBuffer,
        uint32_t firstBinding,
        VkDeviceSize offset
    )
    {
        vkCmdBindVertexBuffers(
            commandBuffer,
            firstBinding,
            1,
            &vertexBuffer.buffer,
            &offset
        );
    }

    //-------------------------------------------------------------------------------------------------

    void BindIndexBuffer(
        VkCommandBuffer commandBuffer,
        RT::BufferAndMemory const & indexBuffer,
        VkDeviceSize offset,
        VkIndexType indexType
    )
    {
        vkCmdBindIndexBuffer(
            commandBuffer,
            indexBuffer.buffer,
            offset,
            indexType
        );
    }

    //-------------------------------------------------------------------------------------------------

    void DrawIndexed(
        VkCommandBuffer const commandBuffer,
        uint32_t const indicesCount,
        uint32_t const instanceCount,
        uint32_t const firstIndex,
        uint32_t const vertexOffset,
        uint32_t const firstInstance
    )
    {
        vkCmdDrawIndexed(
            commandBuffer,
            indicesCount,
            instanceCount,
            firstIndex,
            vertexOffset,
            firstInstance
        );
    }

    //-------------------------------------------------------------------------------------------------

    void SetScissor(VkCommandBuffer commandBuffer, VkRect2D const & scissor)
    {
        MFA_ASSERT(commandBuffer != nullptr);
        vkCmdSetScissor(
            commandBuffer,
            0,
            1,
            &scissor
        );
    }

    //-------------------------------------------------------------------------------------------------

    void SetViewport(VkCommandBuffer commandBuffer, VkViewport const & viewport)
    {
        MFA_ASSERT(commandBuffer != nullptr);
        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
    }

    //-------------------------------------------------------------------------------------------------

    void PushConstants(
        VkCommandBuffer command_buffer,
        VkPipelineLayout pipeline_layout,
        AssetSystem::ShaderStage const shader_stage,
        uint32_t const offset,
        CBlob const data
    )
    {
        vkCmdPushConstants(
            command_buffer,
            pipeline_layout,
            ConvertAssetShaderStageToGpu(shader_stage),
            offset,
            static_cast<uint32_t>(data.len),
            data.ptr
        );
    }

    //-------------------------------------------------------------------------------------------------

    void PushConstants(
        VkCommandBuffer command_buffer,
        VkPipelineLayout pipeline_layout,
        VkShaderStageFlags const shader_stage,
        uint32_t const offset,
        CBlob const data
    )
    {
        vkCmdPushConstants(
            command_buffer,
            pipeline_layout,
            shader_stage,
            offset,
            static_cast<uint32_t>(data.len),
            data.ptr
        );
    }

    //-------------------------------------------------------------------------------------------------

    void UpdateDescriptorSetsBasic(
        VkDevice device,
        uint8_t const descriptorSetsCount,
        VkDescriptorSet * descriptorSets,
        VkDescriptorBufferInfo const & bufferInfo,
        uint32_t const imageInfoCount,
        VkDescriptorImageInfo const * imageInfos
    )
    {
        MFA_ASSERT(device != nullptr);
        MFA_ASSERT(descriptorSetsCount > 0);
        MFA_ASSERT(descriptorSets != nullptr);
        for (uint8_t i = 0; i < descriptorSetsCount; i++)
        {
            std::vector<VkWriteDescriptorSet> descriptorWrites(2);

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

    //-------------------------------------------------------------------------------------------------

    void UpdateDescriptorSets(
        VkDevice device,
        uint32_t descriptorWritesCount,
        VkWriteDescriptorSet * descriptorWrites
    )
    {
        MFA_ASSERT(device != nullptr);
        MFA_ASSERT(descriptorWritesCount > 0);
        MFA_ASSERT(descriptorWrites != nullptr);

        vkUpdateDescriptorSets(
            device,
            descriptorWritesCount,
            descriptorWrites,
            0,
            nullptr
        );
    }

    //-------------------------------------------------------------------------------------------------

    uint32_t ComputeSwapChainImagesCount(VkSurfaceCapabilitiesKHR surfaceCapabilities)
    {
        uint32_t imageCount = surfaceCapabilities.minImageCount + 1;
        if (surfaceCapabilities.maxImageCount != 0 && imageCount > surfaceCapabilities.maxImageCount)
        {
            imageCount = surfaceCapabilities.maxImageCount;
        }
        return imageCount;
    }

    //-------------------------------------------------------------------------------------------------

    void BeginCommandBuffer(
        VkCommandBuffer commandBuffer,
        VkCommandBufferBeginInfo const & beginInfo
    )
    {
        VK_Check(vkBeginCommandBuffer(commandBuffer, &beginInfo));
    }

    //-------------------------------------------------------------------------------------------------

    void ExecuteCommandBuffer(
        VkCommandBuffer primaryCommandBuffer,
        uint32_t const subCommandBuffersCount,
        const VkCommandBuffer * subCommandBuffers
    )
    {
        vkCmdExecuteCommands(
            primaryCommandBuffer,
            subCommandBuffersCount,
            subCommandBuffers
        );
    }

    //-------------------------------------------------------------------------------------------------

    void PipelineBarrier(
        VkCommandBuffer commandBuffer,
        VkPipelineStageFlags sourceStageMask,
        VkPipelineStageFlags destinationStateMask,
        uint32_t const imageMemoryBarrierCount,
        VkImageMemoryBarrier const * imageMemoryBarriers
    )
    {
        vkCmdPipelineBarrier(
            commandBuffer,
            sourceStageMask,
            destinationStateMask,
            0,
            0, nullptr,
            0, nullptr,
            imageMemoryBarrierCount, imageMemoryBarriers
        );
    }

    //-------------------------------------------------------------------------------------------------

    void PipelineBarrier(
        VkCommandBuffer commandBuffer,
        VkPipelineStageFlags sourceStageMask,
        VkPipelineStageFlags destinationStateMask,
        VkBufferMemoryBarrier const & bufferMemoryBarrier
    )
    {
        vkCmdPipelineBarrier(
            commandBuffer,
            sourceStageMask,
            destinationStateMask,
            0,
            0,
            nullptr,
            1,
            &bufferMemoryBarrier,
            0,
            nullptr
        );
    }

    //-------------------------------------------------------------------------------------------------

    void EndCommandBuffer(VkCommandBuffer commandBuffer)
    {
        if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
        {
            MFA_CRASH("Failed to record command buffer");
        }
    }

    //-------------------------------------------------------------------------------------------------

    void SubmitQueues(
        VkQueue queue,
        uint32_t submitCount,
        VkSubmitInfo * submitInfos,
        VkFence fence
    )
    {
        VK_Check(vkQueueSubmit(queue, submitCount, submitInfos, fence));
    }

    //-------------------------------------------------------------------------------------------------

    void ResetFences(VkDevice device, uint32_t fencesCount, VkFence const * fences)
    {
        VK_Check(vkResetFences(device, fencesCount, fences));
    }

    //-------------------------------------------------------------------------------------------------

    VkSampleCountFlagBits ComputeMaxUsableSampleCount(VkPhysicalDevice physicalDevice)
    {
        MFA_VK_VALID_ASSERT(physicalDevice);

        VkPhysicalDeviceProperties physicalDeviceProperties;
        vkGetPhysicalDeviceProperties(physicalDevice, &physicalDeviceProperties);

        return ComputeMaxUsableSampleCount(physicalDeviceProperties);
    }

    //-------------------------------------------------------------------------------------------------

    VkSampleCountFlagBits ComputeMaxUsableSampleCount(VkPhysicalDeviceProperties deviceProperties)
    {
        auto const counts = deviceProperties.limits.framebufferColorSampleCounts
            & deviceProperties.limits.framebufferDepthSampleCounts;

        if (counts & VK_SAMPLE_COUNT_64_BIT) { return VK_SAMPLE_COUNT_64_BIT; }
        if (counts & VK_SAMPLE_COUNT_32_BIT) { return VK_SAMPLE_COUNT_32_BIT; }
        if (counts & VK_SAMPLE_COUNT_16_BIT) { return VK_SAMPLE_COUNT_16_BIT; }
        if (counts & VK_SAMPLE_COUNT_8_BIT) { return VK_SAMPLE_COUNT_8_BIT; }
        if (counts & VK_SAMPLE_COUNT_4_BIT) { return VK_SAMPLE_COUNT_4_BIT; }
        if (counts & VK_SAMPLE_COUNT_2_BIT) { return VK_SAMPLE_COUNT_2_BIT; }

        return VK_SAMPLE_COUNT_1_BIT;
    }

    //-------------------------------------------------------------------------------------------------

    void CopyImage(
        VkCommandBuffer commandBuffer,
        VkImage sourceImage,
        VkImage destinationImage,
        VkImageCopy const & copyRegion
    )
    {
        // Put image copy into command buffer
        vkCmdCopyImage(
            commandBuffer,
            sourceImage,
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            destinationImage,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1,
            &copyRegion
        );
    }

    //-------------------------------------------------------------------------------------------------

    void WaitForQueue(VkQueue queue)
    {
        VK_Check(vkQueueWaitIdle(queue));
    }

    //-------------------------------------------------------------------------------------------------

    VkQueryPool CreateQueryPool(VkDevice device, VkQueryPoolCreateInfo const & createInfo)
    {
        VkQueryPool queryPool;
        VK_Check(vkCreateQueryPool(device, &createInfo, nullptr, &queryPool));
        return queryPool;
    }

    //-------------------------------------------------------------------------------------------------

    void DestroyQueryPool(VkDevice device, VkQueryPool queryPool)
    {
        vkDestroyQueryPool(device, queryPool, nullptr);
    }

    //-------------------------------------------------------------------------------------------------

    void BeginQuery(VkCommandBuffer commandBuffer, VkQueryPool queryPool, uint32_t const queryId)
    {
        vkCmdBeginQuery(commandBuffer, queryPool, queryId, 0);
    }

    //-------------------------------------------------------------------------------------------------

    void EndQuery(VkCommandBuffer commandBuffer, VkQueryPool queryPool, uint32_t const queryId)
    {
        vkCmdEndQuery(commandBuffer, queryPool, queryId);
    }

    //-------------------------------------------------------------------------------------------------

    void GetQueryPoolResult(
        VkDevice device,
        VkQueryPool queryPool,
        uint32_t const samplesCount,
        uint64_t * outSamplesData,
        uint32_t samplesOffset
    )
    {
        // We use vkGetQueryResults to copy the results into a host visible buffer
        vkGetQueryPoolResults(
            device,
            queryPool,
            samplesOffset,
            samplesCount,
            samplesCount * sizeof(uint64_t),
            outSamplesData,
            sizeof(uint64_t),
            // Store results a 64 bit values and wait until the results have been finished
            // If you don't want to wait, you can use VK_QUERY_RESULT_WITH_AVAILABILITY_BIT
            // which also returns the state of the result (ready) in the result
            VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT
        );
    }

    //-------------------------------------------------------------------------------------------------

    void ResetQueryPool(
        VkCommandBuffer commandBuffer,
        VkQueryPool queryPool,
        uint32_t const queryCount,
        uint32_t const firstQueryIndex
    )
    {
        vkCmdResetQueryPool(commandBuffer, queryPool, firstQueryIndex, queryCount);
    }

    //-------------------------------------------------------------------------------------------------

}
