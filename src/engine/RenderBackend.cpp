#include "RenderBackend.hpp"

#include <vector>

#include "BedrockAssert.hpp"
#include "BedrockLog.hpp"
#include "BedrockPlatforms.hpp"
#include "BedrockMath.hpp"
#include "BedrockMemory.hpp"

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

VkSurfaceKHR_T * CreateWindowSurface(SDL_Window * window, VkInstance_T * instance) {
    VkSurfaceKHR_T * ret = nullptr;
    SDL_Check(SDL_Vulkan_CreateSurface(
        window,
        instance,
        &ret
    ));
    return ret;
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
    createInfo.subresourceRange.baseMipLevel = 0;
    createInfo.subresourceRange.levelCount = 1;
    createInfo.subresourceRange.baseArrayLayer = 0;
    createInfo.subresourceRange.layerCount = 1;

    VK_Check(vkCreateImageView(device, &createInfo, nullptr, &image_view));

    return image_view;
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
    VkDevice_T * device,
    VkQueue const & graphic_queue,
    VkCommandPool const & command_pool,
    VkImage const & image, 
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

    EndAndSubmitSingleTimeCommand(&device, command_pool, graphic_queue, command_buffer);
}

CreateBufferResult CreateBuffer(
    VkDevice_T * device,
    VkPhysicalDevice_T * physical_device,
    VkDeviceSize const size, 
    VkBufferUsageFlags const usage, 
    VkMemoryPropertyFlags const properties 
) {
    MFA_PTR_ASSERT(device);
    MFA_PTR_ASSERT(physical_device);

    CreateBufferResult ret {};

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

    VK_Check(vkAllocateMemory(device, &memory_allocation_info, nullptr, &ret.buffer_memory));
    VK_Check(vkBindBufferMemory(device, ret.buffer, ret.buffer_memory, 0));
    return ret;
}

void MapDataToBuffer(
    VkDevice_T * device,
    VkDeviceMemory_T * buffer_memory,
    CBlob data_blob
) {
    void * temp_buffer_data = nullptr;
    vkMapMemory(device, buffer_memory, 0, data_blob.len, 0, &temp_buffer_data);
    MFA_PTR_ASSERT(temp_buffer_data);
    ::memcpy(temp_buffer_data, data_blob.ptr, static_cast<size_t>(data_blob.len));
    vkUnmapMemory(device, buffer_memory);
}

void DestroyBuffer(
    VkDevice_T * device,
    VkBuffer_T * buffer,
    VkDeviceMemory_T * memory
) {
    vkDestroyBuffer(device, buffer, nullptr);
    vkFreeMemory(device, memory, nullptr);
}

CreateImageResult CreateImage(
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
    CreateImageResult ret {};

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

    VK_Check(vkAllocateMemory(device, &allocate_info, nullptr, &ret.image_memory));
    VK_Check(vkBindImageMemory(device, ret.image, ret.image_memory, 0)); 
}

void DestroyImage(
    VkImage_T * image,
    VkDeviceMemory_T * memory,
    VkDevice_T * device
) {
    vkDestroyImage(device, image, nullptr);
    vkFreeMemory(device, memory, nullptr);
}

GpuTexture CreateTexture(
    CpuTexture & cpu_texture,
    VkDevice_T * device,
    VkPhysicalDevice_T * physical_device,
    VkQueue const & graphic_queue,
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
        auto const create_buffer_result = CreateBuffer(
            device,
            physical_device,
            data_blob.len,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        );
        VkBuffer_T * upload_buffer = create_buffer_result.buffer;
        VkDeviceMemory_T * upload_buffer_memory = create_buffer_result.buffer_memory;
        MFA_DEFER {DestroyBuffer(device, upload_buffer, upload_buffer_memory);};
        // Map texture data to buffer
        MapDataToBuffer(device, upload_buffer_memory, data_blob);

        auto const vulkan_format = ConvertCpuTextureFormatToGpu(format);

        auto const create_image_result = CreateImage(
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
        VkImage_T * image = create_image_result.image;
        VkDeviceMemory_T * image_memory = create_image_result.image_memory;
        
        TransferImageLayout(
            device,
            graphic_queue,
            command_pool,
            image,
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
        );

        CopyBufferToImage(
            device,
            command_pool,
            upload_buffer,
            image,
            graphic_queue,
            cpu_texture
        );

        TransferImageLayout(
            device,
            graphic_queue,
            command_pool,
            image,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        );

        VkImageView_T * image_view = CreateImageView(
            device,
            image,
            vulkan_format,
            VK_IMAGE_ASPECT_COLOR_BIT
        );

        MFA_PTR_ASSERT(image);
        gpu_texture.m_image = image;
        MFA_PTR_ASSERT(image_memory);
        gpu_texture.m_image_memory = image_memory;
        MFA_PTR_ASSERT(image_view);
        gpu_texture.m_image_view = image_view;
        gpu_texture.m_cpu_texture = cpu_texture;
        gpu_texture.m_device = device;
        gpu_texture.m_is_valid = true;
    }
    return gpu_texture;
}

bool DestroyTexture(GpuTexture & gpu_texture) {
    bool success = false;
    if(gpu_texture.valid()) {
        MFA_PTR_ASSERT(gpu_texture.m_device);
        MFA_PTR_ASSERT(gpu_texture.m_image);
        MFA_PTR_ASSERT(gpu_texture.m_image_memory);
        MFA_PTR_ASSERT(gpu_texture.m_image_view);
        DestroyImage(
            gpu_texture.m_image,
            gpu_texture.m_image_memory,
            gpu_texture.m_device
        );
        success = true;
        gpu_texture.m_image = nullptr;
        gpu_texture.m_image_view = nullptr;
        gpu_texture.m_image_memory = nullptr;
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
            // TODO Make sure that importer does not add header to offset
            region.bufferOffset = static_cast<U32>(cpu_texture_header->mip_offset_bytes(mip_level, slice_index)); 
            region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            region.imageSubresource.mipLevel = cpu_texture_header->mip_count - mip_level - 1;
            region.imageSubresource.baseArrayLayer = slice_index;
            region.imageSubresource.layerCount = 1;
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

    EndAndSubmitSingleTimeCommand(&device, command_pool, graphic_queue, command_buffer);

}

CreateLogicalDeviceResult CreateLogicalDevice(
    VkPhysicalDevice_T * physical_device,
    U32 const graphics_queue_family,
    VkQueue_T * graphic_queue,
    U32 const present_queue_family,
    VkQueue_T * present_queue,
    VkPhysicalDeviceFeatures const & enabled_physical_device_features
) {
    CreateLogicalDeviceResult ret {};

    MFA_PTR_ASSERT(physical_device);
    MFA_PTR_ASSERT(graphic_queue);
    MFA_PTR_ASSERT(present_queue);

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

    const char* device_extensions = VK_KHR_SWAPCHAIN_EXTENSION_NAME;
    deviceCreateInfo.enabledExtensionCount = 1;
    deviceCreateInfo.ppEnabledExtensionNames = &device_extensions;
    // Necessary for shader (for some reason)
    deviceCreateInfo.pEnabledFeatures = &enabled_physical_device_features;

    std::vector<char const *> debug_layers {};
    debug_layers.emplace_back(DebugLayer);
    deviceCreateInfo.enabledLayerCount = static_cast<U32>(debug_layers.size());
    deviceCreateInfo.ppEnabledLayerNames = debug_layers.data();

    VK_Check(vkCreateDevice(physical_device, &deviceCreateInfo, nullptr, &ret.device));
    MFA_PTR_ASSERT(ret.device);
    MFA_LOG_INFO("Logical device create was successful");
    // Get graphics and presentation queues (which may be the same)
    vkGetDeviceQueue(ret.device, graphics_queue_family, 0, &graphic_queue);
    vkGetDeviceQueue(ret.device, present_queue_family, 0, &present_queue);

    MFA_LOG_INFO("Acquired graphics and presentation queues");
    vkGetPhysicalDeviceMemoryProperties(physical_device, &ret.physical_memory_properties);

    return ret;
}

// TODO Consider oop and storing data
VkSampler_T * CreateSampler(VkDevice_T * device) {
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
    sampler_info.maxAnisotropy = 16.0f;
    sampler_info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    sampler_info.unnormalizedCoordinates = VK_FALSE;
    sampler_info.compareEnable = VK_FALSE;
    sampler_info.compareOp = VK_COMPARE_OP_ALWAYS;
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

FindPhysicalDeviceResult FindPhysicalDevice(VkInstance_T * vk_instance, uint8_t const retry_count) {
    FindPhysicalDeviceResult ret {};

    uint32_t device_count = 0;
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

}