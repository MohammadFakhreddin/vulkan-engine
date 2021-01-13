#include "RenderBackend.hpp"

#include <vector>
#include <cstring>

#include "BedrockAssert.hpp"
#include "BedrockLog.hpp"
#include "BedrockPlatforms.hpp"
#include "BedrockMath.hpp"
#include "BedrockMemory.hpp"

namespace MFA::RenderBackend {

inline static constexpr char EngineName[256] = "MFA";
inline static constexpr int EngineVersion = 1;
std::string ValidationLayer = "VK_LAYER_KHRONOS_validation";
std::vector<char const *> DebugLayers = {
    ValidationLayer.c_str()
};

static void VK_Check(VkResult const result) {
  if(result != VK_SUCCESS) {\
      MFA_CRASH("Vulkan command failed with code:" + std::to_string(result));
  }
}

static void SDL_Check(SDL_bool const result) {
  if(result != SDL_TRUE) {\
      MFA_CRASH("SDL command failed");
  }
}

SDL_Window * CreateWindow(ScreenWidth const screen_width, ScreenHeight const screen_height) {
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS);
    auto const screen_info = MFA::Platforms::ComputeScreenSize();
    return SDL_CreateWindow(
        "VULKAN_ENGINE", 
        static_cast<U32>((screen_info.screen_width / 2.0f) - (screen_width / 2.0f)), 
        static_cast<U32>((screen_info.screen_height / 2.0f) - (screen_height / 2.0f)),
        screen_width, screen_height,
        SDL_WINDOW_SHOWN /*| SDL_WINDOW_FULLSCREEN */| SDL_WINDOW_VULKAN
    );
}

void DestroyWindow(SDL_Window * window) {
    MFA_PTR_ASSERT(window);
    SDL_DestroyWindow(window);
}

VkSurfaceKHR_T * CreateWindowSurface(SDL_Window * window, VkInstance_T * instance) {
    VkSurfaceKHR_T * ret = nullptr;
    SDL_Check(SDL_Vulkan_CreateSurface(
        window,
        instance,
        &ret
    ));
    return ret;
}

void DestroyWindowSurface(VkInstance_T * instance, VkSurfaceKHR_T * surface) {
    MFA_PTR_ASSERT(instance);
    MFA_PTR_ASSERT(surface);
    vkDestroySurfaceKHR(instance, surface, nullptr);
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
    U8 const present_modes_count, 
    VkPresentModeKHR const * present_modes
) {
    for(U8 index = 0; index < present_modes_count; index ++) {
        if (present_modes[index] == VK_PRESENT_MODE_MAILBOX_KHR) {
            return present_modes[index];
        } 
    }
    // If mailbox is unavailable, fall back to FIFO (guaranteed to be available)
    return VK_PRESENT_MODE_FIFO_KHR;
}

VkSurfaceFormatKHR ChooseSurfaceFormat(
    U8 const available_formats_count,
    VkSurfaceFormatKHR const * available_formats
) {
    // We can either choose any format
    if (available_formats_count == 1 && available_formats[0].format == VK_FORMAT_UNDEFINED) {
        return { VK_FORMAT_R8G8B8A8_UNORM, VK_COLORSPACE_SRGB_NONLINEAR_KHR };
    }

    // Or go with the standard format - if available
    for(U8 index = 0; index < available_formats_count; index ++) {
        if (available_formats[index].format == VK_FORMAT_R8G8B8A8_UNORM) {
            return available_formats[index];
        }
    }

    // Or fall back to the first available one
    return available_formats[0];
}

VkInstance_T * CreateInstance(char const * application_name, SDL_Window * window) {
    // Filling out application description:
    VkApplicationInfo application_info = {};
    {
        // sType is mandatory
        application_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        // pNext is mandatory
        application_info.pNext = nullptr;
        // The name of our application
        application_info.pApplicationName = application_name;
        application_info.applicationVersion = 1;    // TODO Ask this as parameter
        // The name of the engine (e.g: Game engine name)
        application_info.pEngineName = EngineName;
        // The version of the engine
        application_info.engineVersion = EngineVersion;
        // The version of Vulkan we're using for this application
        application_info.apiVersion = VK_API_VERSION_1_1;
    }
    std::vector<char const *> instance_extensions {};
    {// Filling sdl extensions
        unsigned int sdl_extenstion_count = 0;
        SDL_Check(SDL_Vulkan_GetInstanceExtensions(window, &sdl_extenstion_count, nullptr));
        instance_extensions.resize(sdl_extenstion_count);
        SDL_Check(SDL_Vulkan_GetInstanceExtensions(
            window,
            &sdl_extenstion_count,
            instance_extensions.data()
        ));
        instance_extensions.emplace_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
    }
    {// Checking for extension support
        U32 vk_supported_extension_count = 0;
        VK_Check(vkEnumerateInstanceExtensionProperties(
            nullptr,
            &vk_supported_extension_count,
            nullptr
        ));
        if (vk_supported_extension_count == 0) {
            MFA_CRASH("no extensions supported!");
        }
    }
    // TODO We should enumarateLayers before using them
    // Filling out instance description:
    VkInstanceCreateInfo instanceInfo = {};
    {
        // sType is mandatory
        instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        // pNext is mandatory
        instanceInfo.pNext = nullptr;
        // flags is mandatory
        instanceInfo.flags = 0;
        // The application info structure is then passed through the instance
        instanceInfo.pApplicationInfo = &application_info;
#ifdef MFA_DEBUG
        instanceInfo.enabledLayerCount = static_cast<U32>(DebugLayers.size());
        instanceInfo.ppEnabledLayerNames = DebugLayers.data();
#else
        instanceInfo.enabledLayerCount = 0;
        instanceInfo.ppEnabledLayerNames = nullptr;
#endif
        instanceInfo.enabledExtensionCount = static_cast<U32>(instance_extensions.size());
        instanceInfo.ppEnabledExtensionNames = instance_extensions.data();
    }
    VkInstance vk_instance;
    VK_Check(vkCreateInstance(&instanceInfo, nullptr, &vk_instance));
    return vk_instance;
}

void DestroyInstance(VkInstance_T * instance) {
    MFA_PTR_ASSERT(instance);
    vkDestroyInstance(instance, nullptr);
}

VkDebugReportCallbackEXT CreateDebugCallback(
    VkInstance_T * vk_instance,
    PFN_vkDebugReportCallbackEXT const & debug_callback
) {
    MFA_PTR_ASSERT(vk_instance);
    VkDebugReportCallbackCreateInfoEXT debug_info = {};
    debug_info.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
    debug_info.pfnCallback = debug_callback;
    debug_info.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
    auto const debug_report_callback = reinterpret_cast<PFN_vkCreateDebugReportCallbackEXT>(vkGetInstanceProcAddr(
        vk_instance,
        "vkCreateDebugReportCallbackEXT"
    ));
    VkDebugReportCallbackEXT debug_report_callback_ext;
    VK_Check(debug_report_callback(
        vk_instance, 
        &debug_info, 
        nullptr, 
        &debug_report_callback_ext
    ));
    return debug_report_callback_ext;
}

void DestroyDebugReportCallback(
    VkInstance_T * instance,
    VkDebugReportCallbackEXT const & report_callback_ext
) {
    MFA_PTR_ASSERT(instance);
    auto const DestroyDebugReportCallback = reinterpret_cast<PFN_vkDestroyDebugReportCallbackEXT>(vkGetInstanceProcAddr(
        instance, 
        "vkDestroyDebugReportCallbackEXT"
    ));
    DestroyDebugReportCallback(instance, report_callback_ext, nullptr);
}

U32 FindMemoryType (
    VkPhysicalDevice * physical_device,
    U32 const type_filter, 
    VkMemoryPropertyFlags const property_flags
) {
    MFA_PTR_ASSERT(physical_device);
    VkPhysicalDeviceMemoryProperties memory_properties;
    vkGetPhysicalDeviceMemoryProperties(*physical_device, &memory_properties);

    for (U32 memory_type_index = 0; memory_type_index < memory_properties.memoryTypeCount; memory_type_index++) {
        if ((type_filter & (1 << memory_type_index)) && (memory_properties.memoryTypes[memory_type_index].propertyFlags & property_flags) == property_flags) {
            return memory_type_index;
        }
    }

    MFA_CRASH("failed to find suitable memory type!");
}

VkImageView_T * CreateImageView (
    VkDevice_T * device,
    VkImage const & image, 
    VkFormat const format, 
    VkImageAspectFlags const aspect_flags
) {
    MFA_PTR_ASSERT(device);

    VkImageView_T * image_view = nullptr;

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
    createInfo.subresourceRange.levelCount = 1;
    createInfo.subresourceRange.baseArrayLayer = 0;
    createInfo.subresourceRange.layerCount = 1;

    VK_Check(vkCreateImageView(device, &createInfo, nullptr, &image_view));

    return image_view;
}

void DestroyImageView(
    VkDevice_T * device,
    VkImageView_T * image_view
) {
    vkDestroyImageView(device, image_view, nullptr);
}

VkCommandBuffer BeginSingleTimeCommand(VkDevice_T * device, VkCommandPool const & command_pool) {
    MFA_PTR_ASSERT(device);
    
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
    VkDevice_T * device, 
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

    vkFreeCommandBuffers(device, command_pool, 1, &command_buffer);
}

[[nodiscard]]
VkFormat FindDepthFormat(VkPhysicalDevice_T * physical_device) {
    std::vector<VkFormat> candidate_formats {
        VK_FORMAT_D32_SFLOAT,
        VK_FORMAT_D32_SFLOAT_S8_UINT,
        VK_FORMAT_D24_UNORM_S8_UINT
    };
    return FindSupportedFormat (
        physical_device,
        static_cast<U8>(candidate_formats.size()),
        candidate_formats.data(),
        VK_IMAGE_TILING_OPTIMAL,
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
    );
}

[[nodiscard]]
VkFormat FindSupportedFormat(
    VkPhysicalDevice_T * physical_device,
    U8 const candidates_count, 
    VkFormat * candidates,
    VkImageTiling const tiling, 
    VkFormatFeatureFlags const features
) {
    for(U8 index = 0; index < candidates_count; index ++) {
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
    VkDevice_T * device,
    VkQueue_T * graphic_queue,
    VkCommandPool_T * command_pool,
    VkImage_T * image, 
    VkImageLayout const old_layout, 
    VkImageLayout const new_layout
) {
    MFA_PTR_ASSERT(device);
    VkCommandBuffer_T * command_buffer = BeginSingleTimeCommand(device, command_pool);

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = old_layout;
    barrier.newLayout = new_layout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    VkPipelineStageFlags source_stage;
    VkPipelineStageFlags destination_stage;

    if (
        old_layout == VK_IMAGE_LAYOUT_UNDEFINED && 
        new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
    ) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        source_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destination_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } else if (
        old_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && 
        new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    ) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        source_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destination_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    } else {
        MFA_CRASH("unsupported layout transition!");
    }

    vkCmdPipelineBarrier(
        command_buffer,
        source_stage, destination_stage,
        0,
        0, nullptr,
        0, nullptr,
        1, &barrier
    );

    EndAndSubmitSingleTimeCommand(device, command_pool, graphic_queue, command_buffer);
}

BufferGroup CreateBuffer(
    VkDevice_T * device,
    VkPhysicalDevice_T * physical_device,
    VkDeviceSize const size, 
    VkBufferUsageFlags const usage, 
    VkMemoryPropertyFlags const properties 
) {
    MFA_PTR_ASSERT(device);
    MFA_PTR_ASSERT(physical_device);

    BufferGroup ret {};

    VkBufferCreateInfo buffer_info {};
    buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_info.size = size;
    buffer_info.usage = usage;
    buffer_info.sharingMode = VkSharingMode::VK_SHARING_MODE_EXCLUSIVE;

    VK_Check(vkCreateBuffer(device, &buffer_info, nullptr, &ret.buffer));
    MFA_PTR_ASSERT(ret.buffer);
    VkMemoryRequirements memory_requirements {};
    vkGetBufferMemoryRequirements(device, ret.buffer, &memory_requirements);

    VkMemoryAllocateInfo memory_allocation_info {};
    memory_allocation_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memory_allocation_info.allocationSize = memory_requirements.size;
    memory_allocation_info.memoryTypeIndex = FindMemoryType(
        &physical_device, 
        memory_requirements.memoryTypeBits, 
        properties
    );

    VK_Check(vkAllocateMemory(device, &memory_allocation_info, nullptr, &ret.memory));
    VK_Check(vkBindBufferMemory(device, ret.buffer, ret.memory, 0));
    return ret;
}

void MapDataToBuffer(
    VkDevice_T * device,
    VkDeviceMemory_T * buffer_memory,
    CBlob const data_blob
) {
    void * temp_buffer_data = nullptr;
    vkMapMemory(device, buffer_memory, 0, data_blob.len, 0, &temp_buffer_data);
    MFA_PTR_ASSERT(temp_buffer_data);
    ::memcpy(temp_buffer_data, data_blob.ptr, static_cast<size_t>(data_blob.len));
    vkUnmapMemory(device, buffer_memory);
}

void CopyBuffer(
    VkDevice_T * device,
    VkCommandPool_T * command_pool,
    VkQueue_T * graphic_queue,
    VkBuffer_T * source_buffer,
    VkBuffer_T * destination_buffer,
    VkDeviceSize const size
) {
    auto * command_buffer = BeginSingleTimeCommand(device, command_pool);

    VkBufferCopy copyRegion{};
    copyRegion.size = size;
    vkCmdCopyBuffer(
        command_buffer,
        source_buffer,
        destination_buffer,
        1, 
        &copyRegion
    );

    EndAndSubmitSingleTimeCommand(device, command_pool, graphic_queue, command_buffer); 
}

void DestroyBuffer(
    VkDevice_T * device,
    BufferGroup & buffer_group
) {
    MFA_PTR_ASSERT(buffer_group.memory);
    MFA_PTR_ASSERT(buffer_group.buffer);
    vkFreeMemory(device, buffer_group.memory, nullptr);
    vkDestroyBuffer(device, buffer_group.buffer, nullptr);
    buffer_group.memory = nullptr;
    buffer_group.buffer = nullptr;
}

ImageGroup CreateImage(
    VkDevice_T * device,
    VkPhysicalDevice_T * physical_device,
    U32 const width, 
    U32 const height,
    U32 const depth,
    U8 const mip_levels,
    U16 const slice_count,
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
    VkDevice_T * device,
    ImageGroup const & image_group
) {
    vkDestroyImage(device, image_group.image, nullptr);
    vkFreeMemory(device, image_group.memory, nullptr);
}

GpuTexture CreateTexture(
    CpuTexture & cpu_texture,
    VkDevice_T * device,
    VkPhysicalDevice_T * physical_device,
    VkQueue_T * graphic_queue,
    VkCommandPool_T * command_pool
) {
    MFA_PTR_ASSERT(device);
    MFA_PTR_ASSERT(physical_device);
    MFA_PTR_ASSERT(command_pool);
    GpuTexture gpu_texture {};
    if(cpu_texture.valid()) {
        auto const * cpu_texture_header = cpu_texture.header_object();
        auto const format = cpu_texture_header->format;
        auto const mip_count = cpu_texture_header->mip_count;
        auto const slice_count = cpu_texture_header->slices;
        auto const largest_mipmap_info = cpu_texture_header->mipmap_infos[0];
        auto const data_blob = cpu_texture.data();
        MFA_BLOB_ASSERT(data_blob);
        // Create upload buffer
        auto upload_buffer_group = CreateBuffer(
            device,
            physical_device,
            data_blob.len,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        );
        MFA_DEFER {DestroyBuffer(device, upload_buffer_group);};
        // Map texture data to buffer
        MapDataToBuffer(device, upload_buffer_group.memory, data_blob);

        auto const vulkan_format = ConvertCpuTextureFormatToGpu(format);

        auto const image_group = CreateImage(
            device, 
            physical_device, 
            largest_mipmap_info.dims.width,
            largest_mipmap_info.dims.height,
            largest_mipmap_info.dims.depth,
            mip_count,
            slice_count,
            vulkan_format,
            VK_IMAGE_TILING_OPTIMAL, 
            VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, 
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
        );
        
        TransferImageLayout(
            device,
            graphic_queue,
            command_pool,
            image_group.image,
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
        );

        CopyBufferToImage(
            device,
            command_pool,
            upload_buffer_group.buffer,
            image_group.image,
            graphic_queue,
            cpu_texture
        );

        TransferImageLayout(
            device,
            graphic_queue,
            command_pool,
            image_group.image,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        );

        VkImageView_T * image_view = CreateImageView(
            device,
            image_group.image,
            vulkan_format,
            VK_IMAGE_ASPECT_COLOR_BIT
        );

        MFA_PTR_ASSERT(image_group.image);
        MFA_PTR_ASSERT(image_group.memory);
        gpu_texture.m_image_group = image_group;
        MFA_PTR_ASSERT(image_view);
        gpu_texture.m_image_view = image_view;
        gpu_texture.m_cpu_texture = cpu_texture;
    }
    return gpu_texture;
}

bool DestroyTexture(VkDevice_T * device, GpuTexture & gpu_texture) {
    MFA_PTR_ASSERT(device);
    bool success = false;
    if(gpu_texture.valid()) {
        MFA_PTR_ASSERT(device);
        MFA_PTR_ASSERT(gpu_texture.m_image_group.image);
        MFA_PTR_ASSERT(gpu_texture.m_image_group.memory);
        MFA_PTR_ASSERT(gpu_texture.m_image_view);
        DestroyImage(
            device,
            gpu_texture.m_image_group
        );
        DestroyImageView(
            device,
            gpu_texture.m_image_view
        );
        success = true;
        gpu_texture.m_image_group.image = nullptr;
        gpu_texture.m_image_group.memory = nullptr;
        gpu_texture.m_image_view = nullptr;
    }
    return success;
}

VkFormat ConvertCpuTextureFormatToGpu(Asset::TextureFormat const cpu_format) {
    using namespace Asset;
    switch (cpu_format)
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
    VkDevice_T * device,
    VkCommandPool_T * command_pool,
    VkBuffer_T * buffer,
    VkImage_T * image,
    VkQueue_T * graphic_queue,
    CpuTexture const & cpu_texture
) {
    MFA_PTR_ASSERT(device);
    MFA_PTR_ASSERT(command_pool);
    MFA_PTR_ASSERT(buffer);
    MFA_PTR_ASSERT(image);
    MFA_ASSERT(cpu_texture.valid());

    VkCommandBuffer_T * command_buffer = BeginSingleTimeCommand(device, command_pool);

    auto const * cpu_texture_header = cpu_texture.header_object();
    auto const regions_count = cpu_texture_header->mip_count * cpu_texture_header->slices;
    // TODO Maybe add a function allocate from heap
    auto const regions_blob = Memory::Alloc(regions_count * sizeof(VkBufferImageCopy));
    MFA_DEFER {Memory::Free(regions_blob);};
    auto * regions_array = regions_blob.as<VkBufferImageCopy>();
    for(U8 slice_index = 0; slice_index < cpu_texture_header->slices; slice_index++) {
        for(U8 mip_level = 0; mip_level < cpu_texture_header->mip_count; mip_level++) {
            auto const & mip_info = cpu_texture_header->mipmap_infos[mip_level];
            auto & region = regions_array[mip_level];
            region.imageExtent.width = mip_info.dims.width; 
            region.imageExtent.height = mip_info.dims.height; 
            region.imageExtent.depth = mip_info.dims.depth;
            region.imageOffset.x = 0;
            region.imageOffset.y = 0;
            region.imageOffset.z = 0;
            region.bufferOffset = static_cast<U32>(cpu_texture_header->mip_offset_bytes(mip_level, slice_index)); 
            region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            region.imageSubresource.mipLevel = cpu_texture_header->mip_count - mip_level - 1;
            region.imageSubresource.baseArrayLayer = slice_index;
            region.imageSubresource.layerCount = 1;
            region.bufferRowLength = 0;
            region.bufferImageHeight = 0;
        }
    }

    vkCmdCopyBufferToImage(
        command_buffer,
        buffer,
        image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        regions_count,
        regions_array
    );

    EndAndSubmitSingleTimeCommand(device, command_pool, graphic_queue, command_buffer);

}

LogicalDevice CreateLogicalDevice(
    VkPhysicalDevice_T * physical_device,
    U32 const graphics_queue_family,
    U32 const present_queue_family,
    VkPhysicalDeviceFeatures const & enabled_physical_device_features
) {
    LogicalDevice ret {};

    MFA_PTR_ASSERT(physical_device);
    
    // Create one graphics queue and optionally a separate presentation queue
    float queue_priority = 1.0f;

    VkDeviceQueueCreateInfo queue_create_info[2] = {};

    queue_create_info[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_create_info[0].queueFamilyIndex = graphics_queue_family;
    queue_create_info[0].queueCount = 1;
    queue_create_info[0].pQueuePriorities = &queue_priority;

    queue_create_info[1].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_create_info[1].queueFamilyIndex = present_queue_family;
    queue_create_info[1].queueCount = 1;
    queue_create_info[1].pQueuePriorities = &queue_priority;

    // Create logical device from physical device
    // Note: there are separate instance and device extensions!
    VkDeviceCreateInfo deviceCreateInfo = {};
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.pQueueCreateInfos = queue_create_info;

    if (graphics_queue_family == present_queue_family) {
        deviceCreateInfo.queueCreateInfoCount = 1;
    } else {
        deviceCreateInfo.queueCreateInfoCount = 2;
    }

    const char * device_extensions = VK_KHR_SWAPCHAIN_EXTENSION_NAME;
    deviceCreateInfo.enabledExtensionCount = 1;
    deviceCreateInfo.ppEnabledExtensionNames = &device_extensions;
    // Necessary for shader (for some reason)
    deviceCreateInfo.pEnabledFeatures = &enabled_physical_device_features;
#ifdef MFA_DEBUG
    deviceCreateInfo.enabledLayerCount = static_cast<U32>(DebugLayers.size());
    deviceCreateInfo.ppEnabledLayerNames = DebugLayers.data();
#else
    deviceCreateInfo.enabledLayerCount = 0;
    deviceCreateInfo.ppEnabledLayerNames = nullptr;
#endif
    VK_Check(vkCreateDevice(physical_device, &deviceCreateInfo, nullptr, &ret.device));
    MFA_PTR_ASSERT(ret.device);
    MFA_LOG_INFO("Logical device create was successful");    
    vkGetPhysicalDeviceMemoryProperties(physical_device, &ret.physical_memory_properties);

    return ret;
}

void DestroyLogicalDevice(LogicalDevice const & logical_device) {
    vkDestroyDevice(logical_device.device, nullptr);
}

VkQueue_T * GetQueueByFamilyIndex(
    VkDevice_T * device,
    U32 const queue_family_index
) {
    VkQueue_T * graphic_queue = nullptr;
    vkGetDeviceQueue(
        device, 
        queue_family_index, 
        0, 
        &graphic_queue
    );
    return graphic_queue;
}

// TODO Consider oop and storing data
VkSampler_T * CreateSampler(
    VkDevice_T * device, 
    CreateSamplerParams const & params
) {
    MFA_PTR_ASSERT(device);
    VkSampler sampler = nullptr;

    VkSamplerCreateInfo sampler_info{};
    sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sampler_info.magFilter = VK_FILTER_LINEAR;
    sampler_info.minFilter = VK_FILTER_LINEAR;
    sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_info.anisotropyEnable = VK_TRUE;
    sampler_info.maxAnisotropy = params.max_anisotropy;
    sampler_info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    sampler_info.unnormalizedCoordinates = VK_FALSE;
    sampler_info.compareEnable = VK_FALSE;
    sampler_info.compareOp = VK_COMPARE_OP_ALWAYS;
    sampler_info.minLod = params.min_lod;
    sampler_info.maxLod = params.max_lod;
    sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

    VK_Check(vkCreateSampler(device, &sampler_info, nullptr, &sampler));
    
    return sampler;
}

void DestroySampler(VkDevice_T * device, VkSampler_T * sampler) {
    MFA_PTR_ASSERT(device);
    MFA_PTR_ASSERT(sampler);
    vkDestroySampler(device, sampler, nullptr);
}

VkDebugReportCallbackEXT_T * SetDebugCallback(
    VkInstance_T * instance, 
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
    VkDebugReportCallbackEXT_T * ret = nullptr;
    VK_Check(debug_report_callback(
        instance, 
        &debug_info, 
        nullptr, 
        &ret
    ));
    return ret;
}

static FindPhysicalDeviceResult FindPhysicalDevice(VkInstance_T * vk_instance, U8 const retry_count) {
    FindPhysicalDeviceResult ret {};

    U32 device_count = 0;
    //Getting number of physical devices
    VK_Check(vkEnumeratePhysicalDevices(
        vk_instance, 
        &device_count, 
        nullptr
    ));
    MFA_ASSERT(device_count > 0);

    std::vector<VkPhysicalDevice> devices (device_count);
    const auto phyDevResult = vkEnumeratePhysicalDevices(
        vk_instance, 
        &device_count, 
        devices.data()
    );
    //TODO Search about incomplete
    MFA_ASSERT(phyDevResult == VK_SUCCESS || phyDevResult == VK_INCOMPLETE);
    // TODO We need to choose physical device based on features that we need
    if(retry_count >= devices.size()) {
        MFA_CRASH("No suitable physical device exists");
    }
    ret.physical_device = devices[retry_count];
    MFA_PTR_ASSERT(ret.physical_device);
    ret.physical_device_features.samplerAnisotropy = VK_TRUE;
    ret.physical_device_features.shaderClipDistance = VK_TRUE;
    ret.physical_device_features.shaderCullDistance = VK_TRUE;
    
    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(ret.physical_device, &deviceProperties);
    vkGetPhysicalDeviceFeatures(ret.physical_device, &ret.physical_device_features);

    U32 supportedVersion[] = {
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

FindPhysicalDeviceResult FindPhysicalDevice(VkInstance_T * vk_instance) {
    return FindPhysicalDevice(vk_instance, 0);
}

bool CheckSwapChainSupport(VkPhysicalDevice_T * physical_device) {
    bool ret = false;
    U32 extension_count = 0;
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
    VkPhysicalDevice_T * physical_device, 
    VkSurfaceKHR_T * window_surface
) {
    FindPresentAndGraphicQueueFamilyResult ret {};

    U32 queue_family_count = 0;
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
    for (U32 i = 0; i < queue_family_count; i++) {
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

VkCommandPool_T * CreateCommandPool(VkDevice_T * device, U32 const queue_family_index) {
    MFA_PTR_ASSERT(device);
    // Create graphics command pool
    VkCommandPoolCreateInfo pool_create_info = {};
    pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pool_create_info.queueFamilyIndex = queue_family_index;
    pool_create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    VkCommandPool_T * command_pool = nullptr;
    VK_Check(vkCreateCommandPool(device, &pool_create_info, nullptr, &command_pool));

    return command_pool;
}

void DestroyCommandPool(VkDevice_T * device, VkCommandPool_T * command_pool) {
    MFA_ASSERT(device);
    vkDestroyCommandPool(device, command_pool, nullptr);
}

SwapChainGroup CreateSwapChain(
    VkDevice_T * device,
    VkPhysicalDevice_T * physical_device, 
    VkSurfaceKHR_T * window_surface,
    VkExtent2D swap_chain_extend,
    VkSwapchainKHR_T * old_swap_chain
) {
    // Find surface capabilities
    VkSurfaceCapabilitiesKHR surface_capabilities;
    VK_Check (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, window_surface, &surface_capabilities));

    // Find supported surface formats
    uint32_t formatCount;
    if (
        vkGetPhysicalDeviceSurfaceFormatsKHR(
            physical_device, 
            window_surface, 
            &formatCount, 
            nullptr
        ) != VK_SUCCESS || 
        formatCount == 0
    ) {
        MFA_CRASH("Failed to get number of supported surface formats");
    }
    
    std::vector<VkSurfaceFormatKHR> surface_formats(formatCount);
    VK_Check (vkGetPhysicalDeviceSurfaceFormatsKHR(
        physical_device, 
        window_surface, 
        &formatCount, 
        surface_formats.data()
    ));

    // Find supported present modes
    uint32_t presentModeCount;
    if (vkGetPhysicalDeviceSurfacePresentModesKHR(
        physical_device, 
        window_surface, 
        &presentModeCount, 
        nullptr
    ) != VK_SUCCESS || presentModeCount == 0) {
        MFA_CRASH("Failed to get number of supported presentation modes");
    }

    std::vector<VkPresentModeKHR> present_modes(presentModeCount);
    VK_Check (vkGetPhysicalDeviceSurfacePresentModesKHR(
        physical_device, 
        window_surface, 
        &presentModeCount, 
        present_modes.data())
    );

    // Determine number of images for swap chain
    uint32_t imageCount = surface_capabilities.minImageCount + 1;
    if (surface_capabilities.maxImageCount != 0 && imageCount > surface_capabilities.maxImageCount) {
        imageCount = surface_capabilities.maxImageCount;
    }

    MFA_LOG_INFO("Using %d images for swap chain.", imageCount);
    MFA_ASSERT(surface_formats.size() <= 255);
    // Select a surface format
    auto const selected_surface_format = ChooseSurfaceFormat(
        static_cast<uint8_t>(surface_formats.size()),
        surface_formats.data()
    );

    // Select swap chain size
    auto const selected_swap_chain_extent = ChooseSwapChainExtent(
        surface_capabilities, 
        swap_chain_extend.width, 
        swap_chain_extend.height
    );

    // Determine transformation to use (preferring no transform)
    VkSurfaceTransformFlagBitsKHR surfaceTransform {};
    if (surface_capabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR) {
        surfaceTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    }
    else {
        surfaceTransform = surface_capabilities.currentTransform;
    }

    MFA_ASSERT(present_modes.size() <= 255);
    // Choose presentation mode (preferring MAILBOX ~= triple buffering)
    auto const selected_present_mode = ChoosePresentMode(static_cast<uint8_t>(present_modes.size()), present_modes.data());

    // Finally, create the swap chain
    VkSwapchainCreateInfoKHR createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = window_surface;
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
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = selected_present_mode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = old_swap_chain;

    SwapChainGroup ret {};

    VK_Check (vkCreateSwapchainKHR(device, &createInfo, nullptr, &ret.swap_chain));

    MFA_LOG_INFO("Created swap chain");
    
    if (MFA_PTR_VALID(old_swap_chain)) {
        vkDestroySwapchainKHR(device, old_swap_chain, nullptr);
    }
    // TODO This should be inside Frontend layer
    //m_state.old_swap_chain = m_state.swap_chain;
    ret.swap_chain_format = selected_surface_format.format;
    
    // Store the images used by the swap chain
    // Note: these are the images that swap chain image indices refer to
    // Note: actual number of images may differ from requested number, since it's a lower bound
    uint32_t actual_image_count = 0;
    if (
        vkGetSwapchainImagesKHR(device, ret.swap_chain, &actual_image_count, nullptr) != VK_SUCCESS || 
        actual_image_count == 0
    ) {
        MFA_CRASH("Failed to acquire number of swap chain images");
    }

    ret.swap_chain_images.resize(actual_image_count);
    VK_Check (vkGetSwapchainImagesKHR(
        device,
        ret.swap_chain,
        &actual_image_count,
        ret.swap_chain_images.data()
    ));

    ret.swap_chain_image_views.resize(actual_image_count);
    for(uint32_t image_index = 0; image_index < actual_image_count; image_index++) {
        MFA_PTR_VALID(ret.swap_chain_images[image_index]);
        ret.swap_chain_image_views[image_index] = CreateImageView(
            device,
            ret.swap_chain_images[image_index],
            ret.swap_chain_format,
            VK_IMAGE_ASPECT_COLOR_BIT
        );
        MFA_PTR_VALID(ret.swap_chain_image_views[image_index]);
    }

    MFA_LOG_INFO("Acquired swap chain images");
    return ret;
}

void DestroySwapChain(VkDevice_T * device, SwapChainGroup const & swap_chain_group) {
    MFA_PTR_ASSERT(device);
    MFA_PTR_ASSERT(swap_chain_group.swap_chain);
    MFA_ASSERT(swap_chain_group.swap_chain_image_views.size() > 0);
    MFA_ASSERT(swap_chain_group.swap_chain_images.size() > 0);
    MFA_ASSERT(swap_chain_group.swap_chain_images.size() == swap_chain_group.swap_chain_image_views.size());
    vkDestroySwapchainKHR(device, swap_chain_group.swap_chain, nullptr);
    for(auto const & swap_chain_image_view : swap_chain_group.swap_chain_image_views) {
        vkDestroyImageView(device, swap_chain_image_view, nullptr);
    }
    //for(auto const & swap_chain_image : swap_chain_group.swap_chain_images) {
    //    vkDestroyImage(device, swap_chain_image, nullptr);
    //}
}

DepthImageGroup CreateDepth(
    VkPhysicalDevice_T * physical_device,
    VkDevice_T * device,
    VkExtent2D const swap_chain_extend
) {
    DepthImageGroup ret {};
    auto const depth_format = FindDepthFormat(physical_device);
    ret.image_group = CreateImage(
        device,
        physical_device,
        swap_chain_extend.width,
        swap_chain_extend.height,
        1,
        1,
        1,
        depth_format,
        VK_IMAGE_TILING_OPTIMAL, 
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, 
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
    );
    MFA_PTR_ASSERT(ret.image_group.image);
    MFA_PTR_ASSERT(ret.image_group.memory);
    ret.image_view = CreateImageView(
        device,
        ret.image_group.image,
        depth_format,
        VK_IMAGE_ASPECT_DEPTH_BIT
    );
    MFA_PTR_ASSERT(ret.image_view);
    return ret;
}

void DestroyDepth(VkDevice_T * device, DepthImageGroup const & depth_group) {
    MFA_PTR_ASSERT(device);
    DestroyImageView(device, depth_group.image_view);
    DestroyImage(device, depth_group.image_group);
 }

VkRenderPass_T * CreateRenderPass(
    VkPhysicalDevice_T * physical_device, 
    VkDevice_T * device, 
    VkFormat const swap_chain_format
) {
    VkAttachmentDescription color_attachment = {};
    color_attachment.format = swap_chain_format;
    color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    color_attachment.initialLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentDescription depth_attachment{};
    depth_attachment.format = FindDepthFormat(physical_device);
    depth_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depth_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depth_attachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depth_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depth_attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    
    // Note: hardware will automatically transition attachment to the specified layout
    // Note: index refers to attachment descriptions array
    VkAttachmentReference colorAttachmentReference = {};
    colorAttachmentReference.attachment = 0;
    colorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthAttachmentRef{};
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

    std::vector<VkAttachmentDescription> attachments = {color_attachment, depth_attachment};
    // Create the render pass
    VkRenderPassCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    createInfo.attachmentCount = static_cast<uint32_t>(attachments.size());;
    createInfo.pAttachments = attachments.data();
    createInfo.subpassCount = 1;
    createInfo.pSubpasses = &subPassDescription;
    createInfo.dependencyCount = 1;
    createInfo.pDependencies = &dependency;

    VkRenderPass_T * render_pass = nullptr;
    VK_Check (vkCreateRenderPass(device, &createInfo, nullptr, &render_pass));

    MFA_LOG_INFO("Created render pass.");

    return render_pass;
}

void DestroyRenderPass(VkDevice_T * device, VkRenderPass_T * render_pass) {
    MFA_PTR_ASSERT(device);
    MFA_PTR_ASSERT(render_pass);
    vkDestroyRenderPass(device, render_pass, nullptr);
}

std::vector<VkFramebuffer_T *> CreateFrameBuffers(
    VkDevice_T * device,
    VkRenderPass_T * render_pass,
    U32 const swap_chain_image_views_count, 
    VkImageView_T ** swap_chain_image_views,
    VkImageView_T * depth_image_view,
    VkExtent2D const swap_chain_extent
) {
    MFA_PTR_ASSERT(device);
    MFA_PTR_ASSERT(render_pass);
    MFA_ASSERT(swap_chain_image_views_count > 0);
    MFA_PTR_ASSERT(swap_chain_image_views);
    std::vector<VkFramebuffer_T *> swap_chain_frame_buffers {swap_chain_image_views_count};

    // Note: FrameBuffer is basically a specific choice of attachments for a render pass
    // That means all attachments must have the same dimensions, interesting restriction
    for (size_t i = 0; i < swap_chain_image_views_count; i++) {

        std::vector<VkImageView> attachments = {
            swap_chain_image_views[i],
            depth_image_view
        };

        VkFramebufferCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        create_info.renderPass = render_pass;
        create_info.attachmentCount = static_cast<uint32_t>(attachments.size());
        create_info.pAttachments = attachments.data();
        create_info.width = swap_chain_extent.width;
        create_info.height = swap_chain_extent.height;
        create_info.layers = 1;

        VK_Check(vkCreateFramebuffer(device, &create_info, nullptr, &swap_chain_frame_buffers[i]));
    }

    MFA_LOG_INFO("Created frame-buffers for swap chain image views");

    return swap_chain_frame_buffers;
}

void DestroyFrameBuffers(
    VkDevice_T * device,
    U32 const frame_buffers_count,
    VkFramebuffer_T ** frame_buffers
) {
    for(U32 index = 0; index < frame_buffers_count; index++) {
        vkDestroyFramebuffer(device, frame_buffers[index], nullptr);
    }
}

GpuShader CreateShader(VkDevice_T * device, CpuShader const & cpu_shader) {
    MFA_PTR_VALID(device);
    GpuShader gpu_shader {};
    if(cpu_shader.valid()) {
        gpu_shader.m_cpu_shader = cpu_shader;
        auto const shader_code = cpu_shader.data();
        VkShaderModuleCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        create_info.codeSize = shader_code.len;
        create_info.pCode = reinterpret_cast<uint32_t const *>(shader_code.ptr);
        VK_Check(vkCreateShaderModule(device, &create_info, nullptr, &gpu_shader.m_shader_module));
        MFA_LOG_INFO("Creating shader module was successful");
        MFA_ASSERT(gpu_shader.valid());
    }
    return gpu_shader;
}

bool DestroyShader(VkDevice_T * device, GpuShader & gpu_shader) {
    MFA_PTR_ASSERT(device);
    bool ret = false;
    if(gpu_shader.valid()) {
        vkDestroyShaderModule(device, gpu_shader.m_shader_module, nullptr);
        gpu_shader.m_shader_module = nullptr;
        ret = true;
    }
    return ret;
}

GraphicPipelineGroup CreateGraphicPipeline(
    VkDevice_T * device, 
    U8 const shader_stages_count, 
    GpuShader const * shader_stages,
    VkVertexInputBindingDescription vertex_binding_description,
    U32 const attribute_description_count,
    VkVertexInputAttributeDescription * attribute_description_data,
    VkExtent2D const swap_chain_extent,
    VkRenderPass_T * render_pass,
    VkDescriptorSetLayout_T * descriptor_set_layout,
    U8 const push_constants_range_count,
    VkPushConstantRange * push_constant_ranges,
    CreateGraphicPipelineOptions const & options
) {
    MFA_PTR_ASSERT(shader_stages);
    MFA_PTR_ASSERT(render_pass);
    MFA_PTR_ASSERT(descriptor_set_layout);
    // Set up shader stage info
    std::vector<VkPipelineShaderStageCreateInfo> shader_stages_create_infos {shader_stages_count};
    for(U8 i = 0; i < shader_stages_count; i++) {
        VkPipelineShaderStageCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        create_info.stage = ConvertAssetShaderStageToGpu(shader_stages[i].cpu_shader()->header_object()->stage);
        create_info.module = shader_stages[i].shader_module();
        create_info.pName = shader_stages[i].cpu_shader()->header_object()->entry_point;
        shader_stages_create_infos[i] = create_info;
    }

    // Describe vertex input
    VkPipelineVertexInputStateCreateInfo vertex_input_state_create_info = {};
    vertex_input_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_input_state_create_info.vertexBindingDescriptionCount = 1;
    vertex_input_state_create_info.pVertexBindingDescriptions = &vertex_binding_description;
    vertex_input_state_create_info.vertexAttributeDescriptionCount = attribute_description_count;
    vertex_input_state_create_info.pVertexAttributeDescriptions = attribute_description_data;

    // Describe input assembly
    VkPipelineInputAssemblyStateCreateInfo input_assembly_create_info = {};
    input_assembly_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_assembly_create_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;  // TODO We might need to ask this from outside
    input_assembly_create_info.primitiveRestartEnable = VK_FALSE;

    // Describe viewport and scissor
    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(swap_chain_extent.width);
    viewport.height = static_cast<float>(swap_chain_extent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor = {};
    scissor.offset.x = 0;
    scissor.offset.y = 0;
    scissor.extent.width = swap_chain_extent.width;
    scissor.extent.height = swap_chain_extent.height;

    // Note: scissor test is always enabled (although dynamic scissor is possible)
    // Number of viewports must match number of scissors
    VkPipelineViewportStateCreateInfo viewport_create_info = {};
    viewport_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_create_info.viewportCount = 1;
    viewport_create_info.pViewports = &viewport;
    viewport_create_info.scissorCount = 1;
    viewport_create_info.pScissors = &scissor;

    // Describe rasterization
    // Note: depth bias and using polygon modes other than fill require changes to logical device creation (device features)
    VkPipelineRasterizationStateCreateInfo rasterization_create_info = {};
    rasterization_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterization_create_info.depthClampEnable = VK_FALSE;
    rasterization_create_info.rasterizerDiscardEnable = VK_FALSE;
    // TODO Might need to ask some of them from outside
    rasterization_create_info.polygonMode = VK_POLYGON_MODE_FILL;
    rasterization_create_info.cullMode = VK_CULL_MODE_NONE;
    rasterization_create_info.frontFace = options.font_face;
    rasterization_create_info.depthBiasEnable = VK_FALSE;
    rasterization_create_info.lineWidth = 1.0f;

    // Describe multi-sampling
    // Note: using multisampling also requires turning on device features
    VkPipelineMultisampleStateCreateInfo multi_sample_state_create_info = {};
    multi_sample_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multi_sample_state_create_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multi_sample_state_create_info.sampleShadingEnable = VK_FALSE;
    multi_sample_state_create_info.minSampleShading = 1.0f;
    multi_sample_state_create_info.alphaToCoverageEnable = VK_FALSE;
    multi_sample_state_create_info.alphaToOneEnable = VK_FALSE;

    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_TRUE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.stencilTestEnable = VK_FALSE;

    // Describing color blending
    // Note: all parameters except blendEnable and colorWriteMask are irrelevant here
    VkPipelineColorBlendAttachmentState colorBlendAttachmentState = {};
    colorBlendAttachmentState.blendEnable = VK_FALSE;
    colorBlendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    // Note: all attachments must have the same values unless a device feature is enabled
    VkPipelineColorBlendStateCreateInfo color_blend_create_info = {};
    color_blend_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    color_blend_create_info.logicOpEnable = VK_FALSE;
    color_blend_create_info.logicOp = VK_LOGIC_OP_COPY;
    color_blend_create_info.attachmentCount = 1;
    color_blend_create_info.pAttachments = &colorBlendAttachmentState;
    color_blend_create_info.blendConstants[0] = 0.0f;
    color_blend_create_info.blendConstants[1] = 0.0f;
    color_blend_create_info.blendConstants[2] = 0.0f;
    color_blend_create_info.blendConstants[3] = 0.0f;

    MFA_LOG_INFO("Created descriptor layout");

    VkPipelineLayoutCreateInfo layout_create_info = {};
    layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layout_create_info.setLayoutCount = 1;
    layout_create_info.pSetLayouts = &descriptor_set_layout; // Array of descriptor set layout, Order matter when more than 1
    layout_create_info.pushConstantRangeCount = push_constants_range_count;
    layout_create_info.pPushConstantRanges = push_constant_ranges;
    
    VkPipelineLayout_T * pipeline_layout = nullptr;
    VK_Check(vkCreatePipelineLayout(device, &layout_create_info, nullptr, &pipeline_layout));

    // Create the graphics pipeline
    VkGraphicsPipelineCreateInfo pipelineCreateInfo = {};
    pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineCreateInfo.stageCount = static_cast<Uint32>(shader_stages_create_infos.size());
    pipelineCreateInfo.pStages = shader_stages_create_infos.data();
    pipelineCreateInfo.pVertexInputState = &vertex_input_state_create_info;
    pipelineCreateInfo.pInputAssemblyState = &input_assembly_create_info;
    pipelineCreateInfo.pViewportState = &viewport_create_info;
    pipelineCreateInfo.pRasterizationState = &rasterization_create_info;
    pipelineCreateInfo.pMultisampleState = &multi_sample_state_create_info;
    pipelineCreateInfo.pColorBlendState = &color_blend_create_info;
    pipelineCreateInfo.layout = pipeline_layout;
    pipelineCreateInfo.renderPass = render_pass;
    pipelineCreateInfo.subpass = 0;
    pipelineCreateInfo.basePipelineHandle = nullptr;
    pipelineCreateInfo.basePipelineIndex = -1;
    pipelineCreateInfo.pDepthStencilState = &depthStencil;
    pipelineCreateInfo.pDynamicState = options.dynamic_state_create_info;
    
    VkPipeline_T * pipeline = nullptr;
    VK_Check(vkCreateGraphicsPipelines(
        device, 
        nullptr, 
        1, 
        &pipelineCreateInfo, 
        nullptr, 
        &pipeline
    ));

    MFA_LOG_INFO("Created graphics pipeline");

    GraphicPipelineGroup group {};
    MFA_PTR_ASSERT(pipeline);
    group.graphic_pipeline = pipeline;
    MFA_PTR_ASSERT(pipeline_layout);
    group.pipeline_layout = pipeline_layout;

    return group;
}

void DestroyGraphicPipeline(VkDevice_T * device, GraphicPipelineGroup & graphic_pipeline_group) {
    MFA_PTR_ASSERT(device);
    MFA_PTR_ASSERT(graphic_pipeline_group.graphic_pipeline);
    MFA_PTR_ASSERT(graphic_pipeline_group.pipeline_layout);
    vkDestroyPipeline(device, graphic_pipeline_group.graphic_pipeline, nullptr);
    vkDestroyPipelineLayout(device, graphic_pipeline_group.pipeline_layout, nullptr);
    graphic_pipeline_group.graphic_pipeline = nullptr;
    graphic_pipeline_group.pipeline_layout = nullptr;
}

[[nodiscard]]
VkDescriptorSetLayout_T * CreateDescriptorSetLayout(
    VkDevice_T * device,
    U8 const bindings_count,
    VkDescriptorSetLayoutBinding * bindings
) {
    VkDescriptorSetLayoutCreateInfo descriptor_layout_create_info = {};
    descriptor_layout_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptor_layout_create_info.bindingCount = static_cast<uint32_t>(bindings_count);
    descriptor_layout_create_info.pBindings = bindings;
    VkDescriptorSetLayout_T * descriptor_set_layout = nullptr;
    VK_Check (vkCreateDescriptorSetLayout(
        device,
        &descriptor_layout_create_info,
        nullptr,
        &descriptor_set_layout
    ));
    return descriptor_set_layout;
}

void DestroyDescriptorSetLayout(
    VkDevice_T * device,
    VkDescriptorSetLayout_T * descriptor_set_layout
) {
    vkDestroyDescriptorSetLayout(
        device,
        descriptor_set_layout,
        nullptr
    );
}

VkShaderStageFlagBits ConvertAssetShaderStageToGpu(Asset::ShaderStage const stage) {
    switch(stage) {
        case Asset::Shader::Stage::Vertex:
        return VK_SHADER_STAGE_VERTEX_BIT;
        case Asset::Shader::Stage::Fragment:
        return VK_SHADER_STAGE_FRAGMENT_BIT;
        case Asset::Shader::Stage::Invalid:
        default:
        MFA_CRASH("Unhandled shader stage");
    }
}

BufferGroup CreateVertexBuffer(
    VkDevice_T * device,
    VkPhysicalDevice_T * physical_device,
    VkCommandPool_T * command_pool,
    VkQueue_T * graphic_queue,
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

    MFA_PTR_ASSERT(vertex_buffer_group.memory);
    MFA_PTR_ASSERT(vertex_buffer_group.buffer);

    return vertex_buffer_group;
}

void DestroyVertexBuffer(
    VkDevice_T * device,
    BufferGroup & vertex_buffer_group
) {
    DestroyBuffer(device, vertex_buffer_group);
}

BufferGroup CreateIndexBuffer (
    VkDevice_T * device,
    VkPhysicalDevice_T * physical_device,
    VkCommandPool_T * command_pool,
    VkQueue_T * graphic_queue,
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
    
    auto const indices_buffer_group = CreateBuffer(
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
        indices_buffer_group.buffer, 
        bufferSize
    );

    DestroyBuffer(device, staging_buffer_group);

    MFA_PTR_ASSERT(indices_buffer_group.memory);
    MFA_PTR_ASSERT(indices_buffer_group.buffer);

    return indices_buffer_group;

}

void DestroyIndexBuffer(
    VkDevice_T * device,
    BufferGroup & index_buffer_group
) {
    DestroyBuffer(device, index_buffer_group);
}

std::vector<BufferGroup> CreateUniformBuffer(
    VkDevice_T * device,
    VkPhysicalDevice_T * physical_device,
    U8 const swap_chain_images_count,
    VkDeviceSize const size 
) {
    std::vector<BufferGroup> uniform_buffers_group {swap_chain_images_count};
    for(U8 index = 0; index < swap_chain_images_count; index++) {
        uniform_buffers_group[index] = CreateBuffer(
            device,
            physical_device,
            size,
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
        );
    }
    return uniform_buffers_group;
}

void UpdateUniformBuffer(
    VkDevice_T * device,
    BufferGroup const & uniform_buffer_group,
    CBlob const data
) {
    MapDataToBuffer(device, uniform_buffer_group.memory, data);
}

void DestroyUniformBuffer(
    VkDevice_T * device,
    BufferGroup & buffer_group
) {
    DestroyBuffer(device, buffer_group);
}

VkDescriptorPool_T * CreateDescriptorPool(
    VkDevice_T * device,
    U32 const swap_chain_images_count
) {
    // TODO Check if both of these variables must have same value as swap_chain_images_count
    VkDescriptorPoolSize poolSize {};
    poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSize.descriptorCount = swap_chain_images_count;

    VkDescriptorPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;
    poolInfo.maxSets = swap_chain_images_count;

    VkDescriptorPool_T * descriptor_pool = nullptr;

    VK_Check(vkCreateDescriptorPool(
        device, 
        &poolInfo, 
        nullptr, 
        &descriptor_pool
    ));

    return descriptor_pool;
}

void DestroyDescriptorPool(
    VkDevice_T * device, 
    VkDescriptorPool_T * pool
) {
    MFA_PTR_ASSERT(device);
    MFA_PTR_ASSERT(pool);
    vkDestroyDescriptorPool(device, pool, nullptr);
}

std::vector<VkDescriptorSet_T *> CreateDescriptorSet(
    VkDevice_T * device,
    VkDescriptorPool_T * descriptor_pool,
    VkDescriptorSetLayout_T * descriptor_set_layout,
    U8 const swap_chain_images_count,
    U8 const schemas_count,
    VkWriteDescriptorSet * schemas
) {
    MFA_PTR_ASSERT(device);
    MFA_PTR_ASSERT(descriptor_pool);
    MFA_PTR_ASSERT(descriptor_set_layout);
    std::vector<VkDescriptorSetLayout> layouts(swap_chain_images_count, descriptor_set_layout);
    // There needs to be one descriptor set per binding point in the shader
    VkDescriptorSetAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptor_pool;
    allocInfo.descriptorSetCount = swap_chain_images_count;
    allocInfo.pSetLayouts = layouts.data();

    std::vector<VkDescriptorSet_T *> descriptor_sets {allocInfo.descriptorSetCount};
    // Descriptor sets gets destroyed automatically when descriptor pool is destroyed
    VK_Check(vkAllocateDescriptorSets (
        device, 
        &allocInfo, 
        descriptor_sets.data()
    ));
    MFA_LOG_INFO("Create descriptor set is successful");

    if(schemas_count > 0 && MFA_PTR_VALID(schemas)) {
        MFA_ASSERT(descriptor_sets.size() < 256);
        UpdateDescriptorSets(
            device,
            static_cast<U8>(descriptor_sets.size()),
            descriptor_sets.data(),
            schemas_count,
            schemas
        );
    }

    return descriptor_sets;
}

void UpdateDescriptorSets(
    VkDevice_T * device,
    U8 const descriptor_set_count,
    VkDescriptorSet_T ** descriptor_sets,
    U8 const schemas_count,
    VkWriteDescriptorSet * schemas
) {
    for(auto descriptor_set_index = 0; descriptor_set_index < descriptor_set_count; descriptor_set_index++){
        std::vector<VkWriteDescriptorSet> descriptor_writes {schemas_count};
        for(U8 schema_index = 0; schema_index < schemas_count; schema_index++) {
            descriptor_writes[schema_index] = schemas[schema_index];
            descriptor_writes[schema_index].dstSet = descriptor_sets[descriptor_set_index];
        }

        vkUpdateDescriptorSets(
            device,
            static_cast<U32>(descriptor_writes.size()), 
            descriptor_writes.data(), 
            0, 
            nullptr
        );
    }
}

std::vector<VkCommandBuffer_T *> CreateCommandBuffers(
    VkDevice_T * device,
    U8 const swap_chain_images_count,
    VkCommandPool_T * command_pool
) {
    std::vector<VkCommandBuffer_T *> command_buffers {swap_chain_images_count};
    
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
    VkDevice_T * device,
    VkCommandPool_T * command_pool,
    U8 const command_buffers_count,
    VkCommandBuffer_T ** command_buffers
) {
    MFA_PTR_ASSERT(device);
    MFA_PTR_ASSERT(command_pool);
    MFA_ASSERT(command_buffers_count > 0);
    MFA_PTR_ASSERT(command_buffers);
    vkFreeCommandBuffers(
        device,
        command_pool,
        command_buffers_count,
        command_buffers
    );
}

SyncObjects CreateSyncObjects(
    VkDevice_T * device,
    U8 const max_frames_in_flight,
    U8 const swap_chain_images_count
) {
    SyncObjects sync_objects {};
    sync_objects.image_availability_semaphores.resize(max_frames_in_flight);
    sync_objects.render_finish_indicator_semaphores.resize(max_frames_in_flight);
    sync_objects.fences_in_flight.resize(max_frames_in_flight);
    sync_objects.images_in_flight.resize(swap_chain_images_count, nullptr);

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (U8 i = 0; i < max_frames_in_flight; i++) {
        VK_Check(vkCreateSemaphore(
            device, 
            &semaphoreInfo, 
            nullptr, 
            &sync_objects.image_availability_semaphores[i]
        ));
        VK_Check(vkCreateSemaphore(
            device, 
            &semaphoreInfo,
            nullptr,
            &sync_objects.render_finish_indicator_semaphores[i]
        ));
        VK_Check(vkCreateFence(
            device, 
            &fenceInfo, 
            nullptr, 
            &sync_objects.fences_in_flight[i]
        ));
    }
    return sync_objects;
}

void DestroySyncObjects(VkDevice_T * device, SyncObjects const & sync_objects) {
    MFA_PTR_VALID(device);
    MFA_ASSERT(sync_objects.fences_in_flight.size() == sync_objects.image_availability_semaphores.size());
    MFA_ASSERT(sync_objects.fences_in_flight.size() == sync_objects.render_finish_indicator_semaphores.size());
    for(U8 i = 0; i < sync_objects.fences_in_flight.size(); i++) {
        vkDestroySemaphore(device, sync_objects.render_finish_indicator_semaphores[i], nullptr);
        vkDestroySemaphore(device, sync_objects.image_availability_semaphores[i], nullptr);
        vkDestroyFence(device, sync_objects.fences_in_flight[i], nullptr);
    }
}

void DeviceWaitIdle(VkDevice_T * device) {
    MFA_PTR_VALID(device);
    VK_Check(vkDeviceWaitIdle(device));
}

void WaitForFence(VkDevice_T * device, VkFence_T * in_flight_fence) {
    MFA_PTR_ASSERT(device);
    MFA_PTR_ASSERT(in_flight_fence);
    VK_Check(vkWaitForFences(
        device, 
        1, 
        &in_flight_fence, 
        VK_TRUE, 
        UINT64_MAX
    ));
}

[[nodiscard]]
U8 AcquireNextImage(
    VkDevice_T * device, 
    VkSemaphore_T * image_availability_semaphore, 
    SwapChainGroup const & swap_chain_group
) {
    MFA_PTR_ASSERT(device);
    MFA_PTR_ASSERT(image_availability_semaphore);
    U32 image_index;
    // TODO What if acquiring fail ?
    VK_Check(vkAcquireNextImageKHR(
        device,
        swap_chain_group.swap_chain,
        UINT64_MAX,
        image_availability_semaphore,
        nullptr,
        &image_index
    ));
    return static_cast<U8>(image_index);
}

void BindVertexBuffer(
    VkCommandBuffer_T * command_buffer,
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
    VkCommandBuffer_T * command_buffer,
    BufferGroup const index_buffer,
    VkDeviceSize const offset,
    VkIndexType const index_type
) {
    vkCmdBindIndexBuffer(
        command_buffer,
        index_buffer.buffer,
        offset, 
        index_type
    );
}

void DrawIndexed(
    VkCommandBuffer_T * command_buffer,
    U32 const indices_count,
    U32 const instance_count,
    U32 const first_index,
    U32 const vertex_offset,
    U32 const first_instance
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

void SetScissor(VkCommandBuffer_T * command_buffer, VkRect2D const & scissor) {
    MFA_PTR_ASSERT(command_buffer);
    vkCmdSetScissor(
        command_buffer, 
        0, 
        1, 
        &scissor
    );
}

void SetViewport(VkCommandBuffer_T * command_buffer, VkViewport const & viewport) {
    MFA_PTR_ASSERT(command_buffer);
    vkCmdSetViewport(command_buffer, 0, 1, &viewport);
}

void PushConstants(
    VkCommandBuffer_T * command_buffer,
    VkPipelineLayout_T * pipeline_layout, 
    Asset::ShaderStage const shader_stage, 
    U32 const offset, 
    CBlob const data
) {
    vkCmdPushConstants(
        command_buffer,
        pipeline_layout,
        ConvertAssetShaderStageToGpu(shader_stage),
        offset,
        static_cast<U32>(data.len), 
        data.ptr
    );
}

}
