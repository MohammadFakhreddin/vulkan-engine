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
        VkDebugReportFlagsEXT flags,
        VkDebugReportObjectTypeEXT objType,
        uint64_t srcObject, size_t location,
        int32_t msgCode,
        const char* pLayerPrefix,
        const char* pMsg,
        void* pUserData
    ) {
        if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT) {
            std::cerr << "ERROR: [" << pLayerPrefix << "] Code " << msgCode << " : " << pMsg << std::endl;
        }
        else if (flags & VK_DEBUG_REPORT_WARNING_BIT_EXT) {
            std::cerr << "WARNING: [" << pLayerPrefix << "] Code " << msgCode << " : " << pMsg << std::endl;
        }
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
    void createBuffer(
        VkDeviceSize const size, 
        VkBufferUsageFlags const usage, 
        VkMemoryPropertyFlags const properties, 
        VkBuffer & buffer, 
        VkDeviceMemory & buffer_memory
    ) const {
        assert(nullptr != m_device);
        assert(nullptr != m_physical_device);

        VkBufferCreateInfo buffer_info{};
        buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        buffer_info.size = size;
        buffer_info.usage = usage;
        buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateBuffer(*m_device, &buffer_info, nullptr, &buffer) != VK_SUCCESS) {
            throw std::runtime_error("Application::createBuffer::vkCreateBuffer failed to create buffer!");
        }

        VkMemoryRequirements memory_requirements;
        vkGetBufferMemoryRequirements(*m_device, buffer, &memory_requirements);

        VkMemoryAllocateInfo memory_allocation_info {};
        memory_allocation_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        memory_allocation_info.allocationSize = memory_requirements.size;
        memory_allocation_info.memoryTypeIndex = findMemoryType(memory_requirements.memoryTypeBits, properties);

        if (vkAllocateMemory(*m_device, &memory_allocation_info, nullptr, &buffer_memory) != VK_SUCCESS) {
            throw std::runtime_error("Application::createBuffer::vkAllocateMemory failed to allocate buffer memory!");
        }

        vkBindBufferMemory(*m_device, buffer, buffer_memory, 0);
    }
    bool createTextureImage(FileSystem::RawTexture cpu_texture)
    {
        //FileSystem::DDSTexture cpu_texture("./assets/images/bc7/nvidia/02_-_Default_baseColor.dds");
        //FileSystem::DDSTexture cpu_texture("./assets/images/bc7/nvidia/02_-_Default_emissive.dds");
        //FileSystem::DDSTexture cpu_texture("./assets/images/bc7/nvidia/02_-_Default_metallicRoughness.dds");
        //FileSystem::DDSTexture cpu_texture("./assets/images/bc7/nvidia/02_-_Default_normal.dds");
        //FileSystem::DDSTexture cpu_texture("./assets/images/bc7/nvidia/04_-_Default_baseColor.dds");
        //FileSystem::DDSTexture cpu_texture("./assets/images/bc7/nvidia/04_-_Default_metallicRoughness.dds");
        //FileSystem::DDSTexture cpu_texture("./assets/images/bc7/nvidia/04_-_Default_normal.dds");
        //FileSystem::DDSTexture cpu_texture("./assets/images/bc7/amd/02_-_Default_baseColor_png_BC7.dds");
        //FileSystem::DDSTexture cpu_texture("./assets/images/bc7/amd/02_-_Default_emissive_png_BC7.dds");
        //FileSystem::DDSTexture cpu_texture("./assets/images/bc7/amd/02_-_Default_metallicRoughness_png_BC7.dds");
        //FileSystem::DDSTexture cpu_texture("./assets/images/bc7/amd/02_-_Default_normal_png_BC7.dds");
        //FileSystem::DDSTexture cpu_texture("./assets/images/bc7/amd/04_-_Default_baseColor_png_BC7.dds");
        //FileSystem::DDSTexture cpu_texture("./assets/images/bc7/amd/04_-_Default_metallicRoughness_png_BC7.dds");
        //FileSystem::DDSTexture cpu_texture("./assets/images/bc7/amd/04_-_Default_normal_png_BC7.dds");
        //FileSystem::RawTexture cpu_texture;
        //FileSystem::LoadTexture(cpu_texture,"./assets/images/texture.png");
        FileSystem::RawTexture cpu_texture;
        FileSystem::LoadTexture(cpu_texture, "./assets/viking/viking.png");
        assert(cpu_texture.isValid());

        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;

        //auto const format = cpu_texture.format();
        //auto const mip_level = cpu_texture.mipmap_count() - 1;
        //auto const mipmap = cpu_texture.pixels(mip_level);
        auto const format = VK_FORMAT_R8G8B8A8_UNORM;
        auto const image_size = cpu_texture.image_size();
        auto const pixels = cpu_texture.pixels;
        auto const width = cpu_texture.width;
        auto const height = cpu_texture.height;

        createBuffer(
            image_size, 
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
            stagingBuffer, 
            stagingBufferMemory
        );

        void * data = nullptr;
        vkMapMemory(device, stagingBufferMemory, 0, image_size, 0, &data);
        assert(nullptr != data);
        ::memcpy(data, pixels, static_cast<size_t>(image_size));
        vkUnmapMemory(device, stagingBufferMemory);

        createImage(
            width, 
            height, 
            format,//VK_FORMAT_R8G8B8A8_UNORM, 
            VK_IMAGE_TILING_OPTIMAL, 
            VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, 
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
            textureImage, 
            textureImageMemory
        );
        transitionImageLayout(
            textureImage, 
            format,//VK_FORMAT_R8G8B8A8_UNORM, 
            VK_IMAGE_LAYOUT_UNDEFINED, 
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
        );
        copyBufferToImage(
            stagingBuffer, 
            textureImage, 
            static_cast<uint32_t>(width), 
            static_cast<uint32_t>(height)
        );
        transitionImageLayout(
            textureImage,
            format, 
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        );
        vkDestroyBuffer(device, stagingBuffer, nullptr);
        vkFreeMemory(device, stagingBufferMemory, nullptr);
        createImageView(&textureImageView, textureImage, format, VK_IMAGE_ASPECT_COLOR_BIT);
    }
    bool create_image_view(
        VkImageView * out_image_view, 
        VkImage const image, 
        VkFormat format, 
        VkImageAspectFlags aspect_flags
    ) {
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
    void create_swap_chain_image_views() {
        swapChainImageViews.resize(swapChainImages.size());

        // Create an image view for every image in the swap chain
        for (size_t i = 0; i < swapChainImages.size(); i++) {
            createImageView(&swapChainImageViews[i],swapChainImages[i],swapChainFormat,VK_IMAGE_ASPECT_COLOR_BIT);
        }

        VkFormat depthFormat = findDepthFormat();

        createImage(
            swapChainExtent.width, 
            swapChainExtent.height, 
            depthFormat, 
            VK_IMAGE_TILING_OPTIMAL, 
            VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, 
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
            depthImage, 
            depthImageMemory
        );
        createImageView(&depthImageView, depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);

        //createImage(&textureImage);
        //createImageView(&textureImageView,textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT);

        std::cout << "created image views for swap chain images" << std::endl;
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
    VkCommandPool m_command_pool;
    
};

#endif
