#ifndef RENDERER_CLASS
#define RENDERER_CLASS

#include <SDL2/SDL_vulkan.h>
#include <windows.h>

// TODO Move entire application state into this
class Renderer {
private:
    static constexpr const char * DEBUG_LAYER = "VK_LAYER_LUNARG_standard_validation";
private:
    static void VK_Check(VkResult const result) {
        if(result != VK_SUCCESS) {
            throw std::runtime_error("Vulkan command failed");
        }
    }
    static void SDL_Check(SDL_bool const result){
        if(result != SDL_TRUE) {
            throw std::runtime_error("SDL command failed");
        }
    }
    static VkBool32 DebugCallback(
        VkDebugReportFlagsEXT const flags,
        VkDebugReportObjectTypeEXT object_type,
        uint64_t src_object, 
        size_t location,
        int32_t const message_code,
        char const * player_prefix,
        char const * message,
        void * user_data
    ) {
        // TODO Display other parameters as well
        if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT) {
            std::cerr << "ERROR: [" << player_prefix << "] Code " << message_code << " : " << message << std::endl;
        }
        else if (flags & VK_DEBUG_REPORT_WARNING_BIT_EXT) {
            std::cerr << "WARNING: [" << player_prefix << "] Code " << message_code << " : " << message << std::endl;
        }
        assert(false);
        return VK_FALSE;
    }
public:
    struct InitParams {
        uint32_t screen_width;
        uint32_t screen_height;
    };
    bool init(InitParams const & params) {
        {
            m_screen_width = params.screen_width;
            m_screen_height = params.screen_height;
        }
        create_window();
        create_instance();
        create_debug_callback();
        create_window_surface();
        {// Trying to find suitable physical device that supports our required features
            uint8_t retry_count = 0;
            while(true){
                find_physical_device(retry_count);
                retry_count++;
                if(true == check_swap_chain_support() && true == find_queue_families()) {
                    break;
                }
            }
        }
        create_logical_device();
        create_command_pool();
        return true;
    }
    bool shutdown() {
        return true;
    }
    void create_buffer(
        VkDeviceSize const size, 
        VkBufferUsageFlags const usage, 
        VkMemoryPropertyFlags const properties, 
        VkBuffer & out_buffer, 
        VkDeviceMemory & out_buffer_memory
    ) const {
        assert(nullptr != m_device);
        assert(nullptr != m_physical_device);

        VkBufferCreateInfo buffer_info{};
        buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        buffer_info.size = size;
        buffer_info.usage = usage;
        buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateBuffer(*m_device, &buffer_info, nullptr, &out_buffer) != VK_SUCCESS) {
            throw std::runtime_error("Application::createBuffer::vkCreateBuffer failed to create buffer!");
        }

        VkMemoryRequirements memory_requirements;
        vkGetBufferMemoryRequirements(*m_device, out_buffer, &memory_requirements);

        VkMemoryAllocateInfo memory_allocation_info {};
        memory_allocation_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        memory_allocation_info.allocationSize = memory_requirements.size;
        memory_allocation_info.memoryTypeIndex = findMemoryType(memory_requirements.memoryTypeBits, properties);

        if (vkAllocateMemory(*m_device, &memory_allocation_info, nullptr, &out_buffer_memory) != VK_SUCCESS) {
            throw std::runtime_error("Application::createBuffer::vkAllocateMemory failed to allocate buffer memory!");
        }

        if(VK_SUCCESS != vkBindBufferMemory(*m_device, out_buffer, out_buffer_memory, 0)) {
            throw std::runtime_error("Application::create_buffer::vkBindBufferMemory failed to bind buffer!");
        }
    }
    bool create_texture_image(
        VkImage & out_vk_texture,
        VkDeviceMemory & out_vk_memory,
        VkImageView & out_vk_image_view,
        FileSystem::RawTexture const & cpu_texture
    ) const {
        bool success = false;
        if(cpu_texture.isValid()) {

            VkBuffer staging_buffer;
            VkDeviceMemory staging_buffer_memory;

            auto const format = VK_FORMAT_R8G8B8A8_UNORM;
            auto const image_size = cpu_texture.image_size();
            auto const pixels = cpu_texture.pixels;
            auto const width = cpu_texture.width;
            auto const height = cpu_texture.height;

            create_buffer(
                image_size, 
                VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
                staging_buffer, 
                staging_buffer_memory
            );

            void * data = nullptr;
            vkMapMemory(*m_device, staging_buffer_memory, 0, image_size, 0, &data);
            assert(nullptr != data);
            ::memcpy(data, pixels, static_cast<size_t>(image_size));
            vkUnmapMemory(*m_device, staging_buffer_memory);

            create_image(
                out_vk_texture, 
                out_vk_memory,
                width, 
                height, 
                format,//VK_FORMAT_R8G8B8A8_UNORM, 
                VK_IMAGE_TILING_OPTIMAL, 
                VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, 
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
            );

            transition_image_layout(
                out_vk_texture, 
                format,//VK_FORMAT_R8G8B8A8_UNORM, 
                VK_IMAGE_LAYOUT_UNDEFINED, 
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
            );

            copy_buffer_to_image(
                staging_buffer, 
                out_vk_texture, 
                static_cast<uint32_t>(width), 
                static_cast<uint32_t>(height)
            );

            transition_image_layout(
                out_vk_texture,
                format, 
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
            );

            vkDestroyBuffer(*m_device, staging_buffer, nullptr);

            vkFreeMemory(*m_device, staging_buffer_memory, nullptr);

            create_image_view(&out_vk_image_view, out_vk_texture, format, VK_IMAGE_ASPECT_COLOR_BIT);

            success = true;
        }
        return success;
    } 
    void destroy_texture_image(
        VkImage & texture_image, 
        VkDeviceMemory & texture_memory
    ) const {
        vkDestroyImage(*m_device, texture_image, nullptr);
        vkFreeMemory(*m_device, texture_memory, nullptr);
    }
        VkSampler create_texture_sampler() const {
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

        if (vkCreateSampler(*m_device, &sampler_info, nullptr, &sampler) != VK_SUCCESS) {
            throw std::runtime_error("createTextureSampler::vkCreateSampler Failed to create texture sampler!");
        }

        return sampler;
    }
    template <typename T>
    void createVertexBuffer(
        VkBuffer & out_vertex_buffer,
        VkDeviceMemory & out_vertex_buffer_memory, 
        std::vector<T> vertices
    ) {
        VkDeviceSize const buffer_size = sizeof(vertices[0]) * vertices.size();

        VkBuffer staging_buffer;
        VkDeviceMemory staging_buffer_memory;
        create_buffer(
            buffer_size, 
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
            staging_buffer, staging_buffer_memory
        );

        void * data;
        vkMapMemory(*m_device, staging_buffer_memory, 0, buffer_size, 0, &data);
        ::memcpy(data, vertices.data(), static_cast<size_t>(buffer_size));
        vkUnmapMemory(*m_device, staging_buffer_memory);

        create_buffer(
            buffer_size, 
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
            out_vertex_buffer, 
            out_vertex_buffer_memory
        );

        copy_buffer(staging_buffer, out_vertex_buffer, buffer_size);

        vkDestroyBuffer(*m_device, staging_buffer, nullptr);
        vkFreeMemory(*m_device, staging_buffer_memory, nullptr);
    }
    template <typename T>
    void createIndexBuffer(
        VkBuffer & out_index_buffer,
        VkDeviceMemory & out_index_buffer_memory, 
        std::vector<T> indices
    ) {
        VkDeviceSize const buffer_size = sizeof(indices[0]) * indices.size();

        VkBuffer staging_buffer;
        VkDeviceMemory staging_buffer_memory;
        create_buffer(
            buffer_size, 
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
            staging_buffer, 
            staging_buffer_memory
        );

        void* data;
        vkMapMemory(*m_device, staging_buffer_memory, 0, buffer_size, 0, &data);
        ::memcpy(data, indices.data(), (size_t) buffer_size);
        vkUnmapMemory(*m_device, staging_buffer_memory);

        create_buffer(
            buffer_size,
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            out_index_buffer, 
            out_index_buffer_memory
        );

        copy_buffer(staging_buffer, out_index_buffer, buffer_size);

        vkDestroyBuffer(*m_device, staging_buffer, nullptr);
        vkFreeMemory(*m_device, staging_buffer_memory, nullptr);
    }
private:
    void create_window() {
        SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS);
        //Windows only
        uint32_t const real_screen_width = static_cast<int>(GetSystemMetrics(SM_CXSCREEN));
        uint32_t const real_screen_height = static_cast<int>(GetSystemMetrics(SM_CYSCREEN));

        m_window = SDL_CreateWindow(
            "VULKAN_ENGINE", 
            uint32_t((real_screen_width / 2.0f) - (m_screen_width / 2.0f)), 
            uint32_t((real_screen_height / 2.0f) - (m_screen_height / 2.0f)),
            m_screen_width, m_screen_height,
            SDL_WINDOW_SHOWN /*| SDL_WINDOW_FULLSCREEN */| SDL_WINDOW_VULKAN
        );
        assert(nullptr != m_window);
    }
    void create_instance() const {
        // Filling out application description:
        VkApplicationInfo application_info = {};
        {
            // sType is mandatory
            application_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
            // pNext is mandatory
            application_info.pNext = NULL;
            // The name of our application
            application_info.pApplicationName = "VULKAN_ENGINE";
            // The name of the engine (e.g: Game engine name)
            application_info.pEngineName = "VULKAN_ENGINE";
            // The version of the engine
            application_info.engineVersion = 1;
            // The version of Vulkan we're using for this application
            application_info.apiVersion = VK_API_VERSION_1_0;
        }
        std::vector<const char*> instance_extensions;
        {// Filling sdl extensions
            unsigned int sdl_extenstion_count = 0;
            SDL_Check(SDL_Vulkan_GetInstanceExtensions(m_window, &sdl_extenstion_count, nullptr));
            instance_extensions.resize(sdl_extenstion_count);
            SDL_Vulkan_GetInstanceExtensions(
                m_window,
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
                throw std::runtime_error("no extensions supported!");
            }
        }
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
            instanceInfo.enabledLayerCount = 1;
            instanceInfo.ppEnabledLayerNames = &DEBUG_LAYER;
            instanceInfo.enabledExtensionCount = static_cast<uint32_t>(instance_extensions.size());
            instanceInfo.ppEnabledExtensionNames = instance_extensions.data();
        }
        // Now create the desired instance
        VK_Check(vkCreateInstance(&instanceInfo, NULL, m_vk_instance));
    }
    void create_debug_callback() {
        VkDebugReportCallbackCreateInfoEXT debugInfo = {};
        debugInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
        debugInfo.pfnCallback = DebugCallback;
        debugInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
        auto const debug_report_callback = reinterpret_cast<PFN_vkCreateDebugReportCallbackEXT>(vkGetInstanceProcAddr(
            *m_vk_instance,
            "vkCreateDebugReportCallbackEXT"
        ));
        VK_Check(debug_report_callback(
            *m_vk_instance, 
            &debugInfo, 
            nullptr, 
            &m_vk_debug_report_callback_ext
        ));
    }
    void create_window_surface() {
        // but instead of creating a renderer, we can draw directly to the screen
        SDL_Vulkan_CreateSurface(
            m_window,
            *m_vk_instance,
            &m_window_surface
        );
    }
    void find_physical_device(uint8_t const retry_count) {
        uint32_t device_count = 0;
        //Getting number of physical devices
        VK_Check(vkEnumeratePhysicalDevices(
            *m_vk_instance, 
            &device_count, 
            nullptr
        ));
        assert(device_count > 0);
        std::vector<VkPhysicalDevice> devices (device_count);
        const auto phyDevResult = vkEnumeratePhysicalDevices(
            *m_vk_instance, 
            &device_count, 
            devices.data()
        );
        //TODO Search about incomplete
        assert(phyDevResult == VK_SUCCESS || phyDevResult == VK_INCOMPLETE);
        // TODO We need to choose physical device based on features that we need
        if(retry_count >= devices.size()) {
            return throw std::runtime_error("No suitable physical device exists");
        }
        m_physical_device = devices[retry_count];
        m_device_features.samplerAnisotropy = VK_TRUE;
        m_device_features.shaderClipDistance = VK_TRUE;
        m_device_features.shaderCullDistance = VK_TRUE;

        VkPhysicalDeviceProperties deviceProperties;
        vkGetPhysicalDeviceProperties(m_physical_device, &deviceProperties);
        vkGetPhysicalDeviceFeatures(m_physical_device, &m_device_features);

        uint32_t supportedVersion[] = {
            VK_VERSION_MAJOR(deviceProperties.apiVersion),
            VK_VERSION_MINOR(deviceProperties.apiVersion),
            VK_VERSION_PATCH(deviceProperties.apiVersion)
        };

        std::cout << "physical device supports version " << 
            supportedVersion[0] << "." << 
            supportedVersion[1] << "." << 
            supportedVersion[2] << std::endl;
    }
    bool check_swap_chain_support() const {
        bool ret = false;
        uint32_t extension_count = 0;
        VK_Check(vkEnumerateDeviceExtensionProperties(m_physical_device, nullptr, &extension_count, nullptr));
        std::vector<VkExtensionProperties> device_extensions(extension_count);
        vkEnumerateDeviceExtensionProperties(m_physical_device, nullptr, &extension_count, device_extensions.data());
        for (const auto& extension : device_extensions) {
            if (strcmp(extension.extensionName, VK_KHR_SWAPCHAIN_EXTENSION_NAME) == 0) {
                std::cout << "physical device supports swap chains" << std::endl;
                ret = true;
                break;
            }
        }
        return ret;
    }
    bool find_queue_families() {
        bool ret = false;

        uint32_t queue_family_count = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(m_physical_device ,&queue_family_count,nullptr);
        if (queue_family_count == 0) {
            std::cout << "physical device has no queue families!" << std::endl;
            exit(1);
        }
        // Find queue family with graphics support
        // Note: is a transfer queue necessary to copy vertices to the gpu or can a graphics queue handle that?
        std::vector<VkQueueFamilyProperties> queueFamilies(queue_family_count);
        vkGetPhysicalDeviceQueueFamilyProperties(m_physical_device , &queue_family_count, queueFamilies.data());
        
        std::cout << "physical device has " << queue_family_count << " queue families" << std::endl;

        bool found_graphic_queue_family = false;
        bool found_present_queue_family = false;
        for (uint32_t i = 0; i < queue_family_count; i++) {
            VkBool32 present_is_supported = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(m_physical_device , i, m_window_surface, &present_is_supported);
            if (queueFamilies[i].queueCount > 0 && queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                m_graphics_queue_family = i;
                found_graphic_queue_family = true;
                if (present_is_supported) {
                    m_present_queue_family = i;
                    found_present_queue_family = true;
                    break;
                }
            }
            if (!found_present_queue_family && present_is_supported) {
                m_present_queue_family = i;
                found_present_queue_family = true;
            }
        }
        if (found_graphic_queue_family) {
            std::cout << "queue family #" << m_graphics_queue_family << " supports graphics" << std::endl;

            if (found_present_queue_family) {
                std::cout << "queue family #" << m_present_queue_family << " supports presentation" << std::endl;
                ret = true;
            } else {
                std::cerr << "could not find a valid queue family with present support" << std::endl;
            }
        } else {
            std::cerr << "could not find a valid queue family with graphics support" << std::endl;
        }

        return ret;
    }
    void create_logical_device() {
        // Create one graphics queue and optionally a separate presentation queue
        float queue_priority = 1.0f;

        VkDeviceQueueCreateInfo queue_create_info[2] = {};

        queue_create_info[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_create_info[0].queueFamilyIndex = m_graphics_queue_family;
        queue_create_info[0].queueCount = 1;
        queue_create_info[0].pQueuePriorities = &queue_priority;

        queue_create_info[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_create_info[0].queueFamilyIndex = m_present_queue_family;
        queue_create_info[0].queueCount = 1;
        queue_create_info[0].pQueuePriorities = &queue_priority;

        // Create logical device from physical device
        // Note: there are separate instance and device extensions!
        VkDeviceCreateInfo deviceCreateInfo = {};
        deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        deviceCreateInfo.pQueueCreateInfos = queue_create_info;

        if (m_graphics_queue_family == m_present_queue_family) {
            deviceCreateInfo.queueCreateInfoCount = 1;
        } else {
            deviceCreateInfo.queueCreateInfoCount = 2;
        }

        const char* device_extensions = VK_KHR_SWAPCHAIN_EXTENSION_NAME;
        deviceCreateInfo.enabledExtensionCount = 1;
        deviceCreateInfo.ppEnabledExtensionNames = &device_extensions;
        // Necessary for shader (for some reason)
        deviceCreateInfo.pEnabledFeatures = &m_device_features;

        deviceCreateInfo.enabledLayerCount = 1;
        deviceCreateInfo.ppEnabledLayerNames = &DEBUG_LAYER;
    
        if (vkCreateDevice(m_physical_device, &deviceCreateInfo, nullptr, m_device) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create logical device");
        }

        std::cout << "created logical device" << std::endl;
        // Get graphics and presentation queues (which may be the same)
        vkGetDeviceQueue(*m_device, m_graphics_queue_family, 0, &m_graphic_queue);
        vkGetDeviceQueue(*m_device, m_present_queue_family, 0, &m_present_queue);

        std::cout << "acquired graphics and presentation queues" << std::endl;
        vkGetPhysicalDeviceMemoryProperties(m_physical_device, &m_device_memory_properties);
    }
    void create_command_pool() {
        // Create graphics command pool
        VkCommandPoolCreateInfo pool_create_info = {};
        pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        pool_create_info.queueFamilyIndex = m_graphics_queue_family;

        if (vkCreateCommandPool(*m_device, &pool_create_info, nullptr, &m_command_pool) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create command queue for graphics queue family");
        }
        std::cout << "created command pool for graphics queue family" << std::endl;
    }
    uint32_t findMemoryType (
        uint32_t const type_filter, 
        VkMemoryPropertyFlags const properties
    ) const {
        assert(nullptr != m_physical_device);

        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(m_physical_device, &memProperties);

        for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
            if ((type_filter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
                return i;
            }
        }

        throw std::runtime_error("failed to find suitable memory type!");
    }
    bool create_image_view(
        VkImageView * out_image_view, 
        VkImage const & image, 
        VkFormat const format, 
        VkImageAspectFlags const aspect_flags
    ) const {
        bool ret = true;
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
        if (vkCreateImageView(*m_device, &createInfo, nullptr, out_image_view) != VK_SUCCESS) {
            ret = false;
        }
        return ret;
    }
    void transition_image_layout(
        VkImage const & image, 
        VkFormat format, 
        VkImageLayout const old_layout, 
        VkImageLayout const new_layout
    ) const {
        VkCommandBuffer const command_buffer = begin_single_time_commands();

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
            throw std::invalid_argument("unsupported layout transition!");
        }

        vkCmdPipelineBarrier(
            command_buffer,
            source_stage, destination_stage,
            0,
            0, nullptr,
            0, nullptr,
            1, &barrier
        );

        end_single_time_commands(command_buffer);
    }
    void copy_buffer_to_image(
        VkBuffer const & buffer, 
        VkImage const & image, 
        uint32_t const width, 
        uint32_t const height
    ) const {
        VkCommandBuffer const command_buffer = begin_single_time_commands();

        VkBufferImageCopy region{};
        region.bufferOffset = 0;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;
        region.imageOffset = {0, 0, 0};
        region.imageExtent = {
            width,
            height,
            1
        };

        vkCmdCopyBufferToImage(command_buffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

        end_single_time_commands(command_buffer);
    }
    void create_swap_chain_image_views() {
        m_swap_chain_image_views.resize(m_swap_chain_images.size());

        // Create an image view for every image in the swap chain
        for (size_t i = 0; i < m_swap_chain_images.size(); i++) {
            create_image_view(
                &m_swap_chain_image_views[i],
                m_swap_chain_images[i], 
                m_swap_chain_format,
                VK_IMAGE_ASPECT_COLOR_BIT
            );
        }

        VkFormat const depth_format = find_depth_format();

        create_image(
            m_depth_image, 
            m_depth_image_memory,
            m_swap_chain_extent.width, 
            m_swap_chain_extent.height, 
            depth_format, 
            VK_IMAGE_TILING_OPTIMAL, 
            VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, 
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
        );
        create_image_view(&m_depth_image_view, m_depth_image, depth_format, VK_IMAGE_ASPECT_DEPTH_BIT);

        //createImage(&textureImage);
        //createImageView(&textureImageView,textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT);

        std::cout << "created image views for swap chain images" << std::endl;
    }
    // TODO MipLevels and depth
    void create_image(
        VkImage & out_image, 
        VkDeviceMemory & out_image_memory,
        uint32_t const width, 
        uint32_t const height, 
        VkFormat const format, 
        VkImageTiling const tiling, 
        VkImageUsageFlags const usage, 
        VkMemoryPropertyFlags const properties
    ) const {
        VkImageCreateInfo image_info{};
        image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        image_info.imageType = VK_IMAGE_TYPE_2D;
        image_info.extent.width = width;
        image_info.extent.height = height;
        image_info.extent.depth = 1;
        image_info.mipLevels = 1;
        image_info.arrayLayers = 1;
        image_info.format = format;
        image_info.tiling = tiling;
        image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        image_info.usage = usage;
        image_info.samples = VK_SAMPLE_COUNT_1_BIT;
        image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateImage(*m_device, &image_info, nullptr, &out_image) != VK_SUCCESS) {
            throw std::runtime_error("failed to create image!");
        }

        VkMemoryRequirements memory_requirements;
        vkGetImageMemoryRequirements(*m_device, out_image, &memory_requirements);

        VkMemoryAllocateInfo allocate_info{};
        allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocate_info.allocationSize = memory_requirements.size;
        allocate_info.memoryTypeIndex = findMemoryType(memory_requirements.memoryTypeBits, properties);

        if (vkAllocateMemory(*m_device, &allocate_info, nullptr, &out_image_memory) != VK_SUCCESS) {
            throw std::runtime_error("Failed to allocate image memory!");
        }

        vkBindImageMemory(*m_device, out_image, out_image_memory, 0);
    }
    VkCommandBuffer begin_single_time_commands() const {
        VkCommandBufferAllocateInfo allocate_info{};
        allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocate_info.commandPool = m_command_pool;
        allocate_info.commandBufferCount = 1;

        VkCommandBuffer commandBuffer;
        vkAllocateCommandBuffers(*m_device, &allocate_info, &commandBuffer);

        VkCommandBufferBeginInfo begin_info{};
        begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        vkBeginCommandBuffer(commandBuffer, &begin_info);

        return commandBuffer;
    }
    void end_single_time_commands(VkCommandBuffer command_buffer) const {
        vkEndCommandBuffer(command_buffer);

        VkSubmitInfo submit_info {};
        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers = &command_buffer;

        vkQueueSubmit(m_graphic_queue, 1, &submit_info, VK_NULL_HANDLE);
        vkQueueWaitIdle(m_graphic_queue);

        vkFreeCommandBuffers(*m_device, m_command_pool, 1, &command_buffer);
    }
    VkFormat find_depth_format() const {
        return find_supported_format (
            {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
            VK_IMAGE_TILING_OPTIMAL,
            VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
        );
    }
    VkFormat find_supported_format (
        std::vector<VkFormat> const & candidates, 
        VkImageTiling const tiling, 
        VkFormatFeatureFlags const features
    ) const {
        for (VkFormat format : candidates) {
            VkFormatProperties props;
            vkGetPhysicalDeviceFormatProperties(m_physical_device, format, &props);

            if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
                return format;
            }
            if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
                return format;
            }
        }

        throw std::runtime_error("failed to find supported format!");
    }
    void copy_buffer(VkBuffer const & source_buffer, VkBuffer const & destination_buffer, VkDeviceSize const size) const {
        auto const command_buffer = begin_single_time_commands();

        VkBufferCopy copy_region{};
        copy_region.size = size;
        vkCmdCopyBuffer(command_buffer, source_buffer, destination_buffer, 1, &copy_region);

        end_single_time_commands(command_buffer);
    }
    void createSwapChain() {
        // Find surface capabilities
        VkSurfaceCapabilitiesKHR surface_capabilities;
        if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_physical_device, m_window_surface, &surface_capabilities) != VK_SUCCESS) {
            throw std::runtime_error("Failed to acquire presentation surface capabilities");
        }

        // Find supported surface formats
        uint32_t formatCount;
        if (vkGetPhysicalDeviceSurfaceFormatsKHR(m_physical_device, m_window_surface, &formatCount, nullptr) != VK_SUCCESS || formatCount == 0) {
            throw std::runtime_error("Failed to get number of supported surface formats");
        }

        std::vector<VkSurfaceFormatKHR> surface_formats(formatCount);
        if (vkGetPhysicalDeviceSurfaceFormatsKHR(m_physical_device, m_window_surface, &formatCount, surface_formats.data()) != VK_SUCCESS) {
            throw std::runtime_error("Failed to get supported surface formats");
        }

        // Find supported present modes
        uint32_t presentModeCount;
        if (vkGetPhysicalDeviceSurfacePresentModesKHR(m_physical_device, m_window_surface, &presentModeCount, nullptr) != VK_SUCCESS || presentModeCount == 0) {
            throw std::runtime_error("Failed to get number of supported presentation modes");
        }

        std::vector<VkPresentModeKHR> present_modes(presentModeCount);
        if (vkGetPhysicalDeviceSurfacePresentModesKHR(m_physical_device, m_window_surface, &presentModeCount, present_modes.data()) != VK_SUCCESS) {
            throw std::runtime_error("Failed to get supported presentation modes");
        }

        // Determine number of images for swap chain
        uint32_t imageCount = surface_capabilities.minImageCount + 1;
        if (surface_capabilities.maxImageCount != 0 && imageCount > surface_capabilities.maxImageCount) {
            imageCount = surface_capabilities.maxImageCount;
        }

        std::cout << "using " << imageCount << " images for swap chain" << std::endl;

        // Select a surface format
        VkSurfaceFormatKHR const surface_format = choose_surface_format(surface_formats);

        // Select swap chain size
        m_swap_chain_extent = choose_swap_extent(surface_capabilities);

        // Determine transformation to use (preferring no transform)
        VkSurfaceTransformFlagBitsKHR surfaceTransform;
        if (surface_capabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR) {
            surfaceTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
        }
        else {
            surfaceTransform = surface_capabilities.currentTransform;
        }

        // Choose presentation mode (preferring MAILBOX ~= triple buffering)
        VkPresentModeKHR presentMode = choose_present_mode(present_modes);

        // Finally, create the swap chain
        VkSwapchainCreateInfoKHR createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.surface = m_window_surface;
        createInfo.minImageCount = imageCount;
        createInfo.imageFormat = surface_format.format;
        createInfo.imageColorSpace = surface_format.colorSpace;
        createInfo.imageExtent = m_swap_chain_extent;
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0;
        createInfo.pQueueFamilyIndices = nullptr;
        createInfo.preTransform = surfaceTransform;
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        createInfo.presentMode = presentMode;
        createInfo.clipped = VK_TRUE;
        createInfo.oldSwapchain = m_old_swap_chain;

        if (vkCreateSwapchainKHR(*m_device, &createInfo, nullptr, &m_swap_chain) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create swap chain");
        }
        else {
            std::cout << "created swap chain" << std::endl;
        }

        if (m_old_swap_chain != nullptr) {
            vkDestroySwapchainKHR(*m_device, m_old_swap_chain, nullptr);
        }
        m_old_swap_chain = m_swap_chain;

        m_swap_chain_format = surface_format.format;

    // Store the images used by the swap chain
        // Note: these are the images that swap chain image indices refer to
        // Note: actual number of images may differ from requested number, since it's a lower bound
        uint32_t actualImageCount = 0;
        if (
            vkGetSwapchainImagesKHR(*m_device, m_swap_chain, &actualImageCount, nullptr) != VK_SUCCESS || 
            actualImageCount == 0
        ) {
            throw std::runtime_error("Failed to acquire number of swap chain images");
        }

        m_swap_chain_images.resize(actualImageCount);

        if (vkGetSwapchainImagesKHR(*m_device, m_swap_chain, &actualImageCount, m_swap_chain_images.data()) != VK_SUCCESS) {
            throw std::runtime_error("Failed to acquire swap chain images");

        }

        std::cout << "acquired swap chain images" << std::endl;
    }
private:
    //
    uint32_t m_screen_width = 0;
    uint32_t m_screen_height = 0;
    // 
    VkPhysicalDevice m_physical_device = nullptr;
    VkDevice * m_device = nullptr;
    SDL_Window * m_window = nullptr;
    VkInstance * m_vk_instance = nullptr;
    VkDebugReportCallbackEXT m_vk_debug_report_callback_ext = nullptr;
    VkSurfaceKHR m_window_surface = nullptr;
    VkPhysicalDeviceFeatures m_device_features {};
    VkSwapchainKHR m_old_swap_chain = nullptr;
    VkSwapchainKHR m_swap_chain = nullptr;
    uint32_t m_graphics_queue_family {};
    uint32_t m_present_queue_family {};
    VkQueue m_graphic_queue = nullptr;
    VkQueue m_present_queue = nullptr;

    VkPhysicalDeviceMemoryProperties m_device_memory_properties {};

    VkCommandPool m_command_pool = nullptr;

    std::vector<VkImageView> m_swap_chain_image_views;
    std::vector<VkImage> m_swap_chain_images;
    VkExtent2D m_swap_chain_extent {};
    VkFormat m_swap_chain_format {};

    VkImage m_depth_image = nullptr;
    VkDeviceMemory m_depth_image_memory = nullptr;
    VkImageView m_depth_image_view = nullptr;

};

#endif
