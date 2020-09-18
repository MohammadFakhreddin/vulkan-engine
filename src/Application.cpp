#include <Application.hpp>

#include <windows.h>
#include <cassert>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <SDL2\SDL_vulkan.h>

Application::Application(){}

void
Application::run(){
    //Create window
    {
        SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS);
        //Windows only
        int realScreenWidth = (int)GetSystemMetrics(SM_CXSCREEN);
        int realScreenHeight = (int)GetSystemMetrics(SM_CYSCREEN);

        window = SDL_CreateWindow(
            "VULKAN_ENGINE", 
            uint32_t((realScreenWidth / 2.0f) - (SCREEN_WIDTH / 2.0f)), 
            uint32_t((realScreenHeight / 2.0f) - (SCREEN_HEIGHT / 2.0f)),
            SCREEN_WIDTH, SCREEN_HEIGHT,
            SDL_WINDOW_SHOWN /*| SDL_WINDOW_FULLSCREEN */| SDL_WINDOW_VULKAN
        );
        assert(nullptr != window);
    }
    setupVulkan();
    mainLoop();
    cleanUp(true);
}

void
Application::setupVulkan(){
    oldSwapChain = VK_NULL_HANDLE;
    createInstance();
    createDebugCallback();
    createWindowSurface();
    findPhysicalDevice();
    checkSwapChainSupport();
    findQueueFamilies();
    createLogicalDevice();
    createSemaphores();
    createCommandPool();
    createVertexBuffer();
    createSwapChain();
    createRenderPass();
    createImageViews();
    createFramebuffers();
    createGraphicsPipeline();
    createUniformBuffer();
    createDescriptorPool();
    createDescriptorSet();
    createCommandBuffers();
}

void
Application::createInstance(){
    
    // Filling out application description:
    VkApplicationInfo applicationInfo = {};
    {
        // sType is mandatory
        applicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        // pNext is mandatory
        applicationInfo.pNext = NULL;
        // The name of our application
        applicationInfo.pApplicationName = "VULKAN_ENGINE";
        // The name of the engine (e.g: Game engine name)
        applicationInfo.pEngineName = "VULKAN_ENGINE";
        // The version of the engine
        applicationInfo.engineVersion = 1;
        // The version of Vulkan we're using for this application
        applicationInfo.apiVersion = VK_API_VERSION_1_0;
    }
    std::vector<const char*> vkInstanceExtensions;
    {// Filling sdl extentions
        unsigned int sdlExtensionCount = 0;
        sdlCheck(SDL_Vulkan_GetInstanceExtensions(window, &sdlExtensionCount, nullptr));
        vkInstanceExtensions.resize(sdlExtensionCount);
        SDL_Vulkan_GetInstanceExtensions(
            window,
            &sdlExtensionCount,
            vkInstanceExtensions.data()
        );
#ifdef DEBUGGING_ENABLED
        vkInstanceExtensions.emplace_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
#endif // DEBUGGING_ENABLED
    }
    {// Checking for extension support
        uint32_t vulkanSupportedExtensionCount = 0;
        vkCheck(vkEnumerateInstanceExtensionProperties(
            nullptr,
            &vulkanSupportedExtensionCount,
            nullptr
        ));
        if (vulkanSupportedExtensionCount == 0) {
            throwErrorAndExit("no extensions supported!");
        }
    }
    // Filling out instance description:
    VkInstanceCreateInfo instanceInfo = {};
    {
        // sType is mandatory
        instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        // pNext is mandatory
        instanceInfo.pNext = NULL;
        // flags is mandatory
        instanceInfo.flags = 0;
        // The application info structure is then passed through the instance
        instanceInfo.pApplicationInfo = &applicationInfo;
#ifdef DEBUGGING_ENABLED
        instanceInfo.enabledLayerCount = 1;
        instanceInfo.ppEnabledLayerNames = &DEBUG_LAYER;
#endif // DEBUGGING_ENABLED
        instanceInfo.enabledExtensionCount = (uint32_t)vkInstanceExtensions.size();
        instanceInfo.ppEnabledExtensionNames = vkInstanceExtensions.data();
    }
    // Now create the desired instance
    vkCheck(vkCreateInstance(&instanceInfo, NULL, &vkInstance));
}

void
Application::vkCheck(VkResult result) {
    assert(result == VK_SUCCESS);
}

void
Application::sdlCheck(SDL_bool result){
    assert(result == SDL_TRUE);
}

VkBool32
debugCallback(
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

void 
Application::createDebugCallback() {
#ifdef DEBUGGING_ENABLED
    VkDebugReportCallbackCreateInfoEXT debugInfo = {};
    debugInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
    debugInfo.pfnCallback = debugCallback;
    debugInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
    
    PFN_vkCreateDebugReportCallbackEXT createDebugReportCallback = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(
        vkInstance, 
        "vkCreateDebugReportCallbackEXT"
    );

    vkCheck(createDebugReportCallback(vkInstance, &debugInfo, nullptr, &debugReportCallbackExtension));
#endif // DEBUGGING_ENABLED

}

void
Application::createWindowSurface() {
    // but instead of creating a renderer, we can draw directly to the screen
    SDL_Vulkan_CreateSurface(
        window,
        vkInstance,
        &windowSurface
    );

}

void
Application::findPhysicalDevice() {
    uint32_t deviceCount = 0;
    //Getting number of physical devices
    vkCheck(vkEnumeratePhysicalDevices(vkInstance, &deviceCount, nullptr));
    assert(deviceCount > 0);
    deviceCount = 1;
    
    const auto phyDevResult = vkEnumeratePhysicalDevices(vkInstance, &deviceCount, &physicalDevice);
    //TODO Search about incomplete
    assert(phyDevResult == VK_SUCCESS || phyDevResult == VK_INCOMPLETE);
    
    VkPhysicalDeviceProperties deviceProperties;
    VkPhysicalDeviceFeatures deviceFeatures;
    vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);
    vkGetPhysicalDeviceFeatures(physicalDevice, &deviceFeatures);

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

void
Application::checkSwapChainSupport() {
    uint32_t extensionCount = 0;
    vkCheck(vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, nullptr));

    std::vector<VkExtensionProperties> deviceExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, deviceExtensions.data());

    for (const auto& extension : deviceExtensions) {
        if (strcmp(extension.extensionName, VK_KHR_SWAPCHAIN_EXTENSION_NAME) == 0) {
            std::cout << "physical device supports swap chains" << std::endl;
            return;
        }
    }

    throwErrorAndExit("physical device doesn't support swap chains");
}

void
Application::findQueueFamilies() {
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice,&queueFamilyCount,nullptr);
    if (queueFamilyCount == 0) {
        std::cout << "physical device has no queue families!" << std::endl;
        exit(1);
    }
    // Find queue family with graphics support
    // Note: is a transfer queue necessary to copy vertices to the gpu or can a graphics queue handle that?
    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies.data());
    
    std::cout << "physical device has " << queueFamilyCount << " queue families" << std::endl;

    bool foundGraphicsQueueFamily = false;
    bool foundPresentQueueFamily = false;

    for (uint32_t i = 0; i < queueFamilyCount; i++) {
        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, windowSurface, &presentSupport);

        if (queueFamilies[i].queueCount > 0 && queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            graphicsQueueFamily = i;
            foundGraphicsQueueFamily = true;

            if (presentSupport) {
                presentQueueFamily = i;
                foundPresentQueueFamily = true;
                break;
            }
        }

        if (!foundPresentQueueFamily && presentSupport) {
            presentQueueFamily = i;
            foundPresentQueueFamily = true;
        }
    }

    if (foundGraphicsQueueFamily) {
        std::cout << "queue family #" << graphicsQueueFamily << " supports graphics" << std::endl;

        if (foundPresentQueueFamily) {
            std::cout << "queue family #" << presentQueueFamily << " supports presentation" << std::endl;
        }
        else {
            throwErrorAndExit("could not find a valid queue family with present support");
        }
    }
    else {
        throwErrorAndExit("could not find a valid queue family with graphics support");
    }
}

void 
Application::createLogicalDevice() {
    // Greate one graphics queue and optionally a separate presentation queue
    float queuePriority = 1.0f;

    VkDeviceQueueCreateInfo queueCreateInfo[2] = {};

    queueCreateInfo[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo[0].queueFamilyIndex = graphicsQueueFamily;
    queueCreateInfo[0].queueCount = 1;
    queueCreateInfo[0].pQueuePriorities = &queuePriority;

    queueCreateInfo[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo[0].queueFamilyIndex = presentQueueFamily;
    queueCreateInfo[0].queueCount = 1;
    queueCreateInfo[0].pQueuePriorities = &queuePriority;

    // Create logical device from physical device
    // Note: there are separate instance and device extensions!
    VkDeviceCreateInfo deviceCreateInfo = {};
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.pQueueCreateInfos = queueCreateInfo;

    if (graphicsQueueFamily == presentQueueFamily) {
        deviceCreateInfo.queueCreateInfoCount = 1;
    }
    else {
        deviceCreateInfo.queueCreateInfoCount = 2;
    }

    // Necessary for shader (for some reason)
    VkPhysicalDeviceFeatures enabledFeatures = {};
    enabledFeatures.shaderClipDistance = VK_TRUE;
    enabledFeatures.shaderCullDistance = VK_TRUE;

    const char* deviceExtensions = VK_KHR_SWAPCHAIN_EXTENSION_NAME;
    deviceCreateInfo.enabledExtensionCount = 1;
    deviceCreateInfo.ppEnabledExtensionNames = &deviceExtensions;
    deviceCreateInfo.pEnabledFeatures = &enabledFeatures;

#ifdef DEBUGGING_ENABLED
    deviceCreateInfo.enabledLayerCount = 1;
    deviceCreateInfo.ppEnabledLayerNames = &DEBUG_LAYER;
#endif // DEBUGGING_ENABLED

    if (vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &device) != VK_SUCCESS) {
        throwErrorAndExit("Failed to create logical device");
    }

    std::cout << "created logical device" << std::endl;

    // Get graphics and presentation queues (which may be the same)
    vkGetDeviceQueue(device, graphicsQueueFamily, 0, &graphicsQueue);
    vkGetDeviceQueue(device, presentQueueFamily, 0, &presentQueue);

    std::cout << "acquired graphics and presentation queues" << std::endl;

    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &deviceMemoryProperties);
}

void
Application::createSemaphores() {
    VkSemaphoreCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    if (vkCreateSemaphore(device, &createInfo, nullptr, &imageAvailableSemaphore) != VK_SUCCESS ||
        vkCreateSemaphore(device, &createInfo, nullptr, &renderingFinishedSemaphore) != VK_SUCCESS) {
        throwErrorAndExit("Failed to create semaphores");
    }
    else {
        std::cout << "created semaphores" << std::endl;
    }
}

void
Application::createCommandPool() {
    // Create graphics command pool
    VkCommandPoolCreateInfo poolCreateInfo = {};
    poolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolCreateInfo.queueFamilyIndex = graphicsQueueFamily;

    if (vkCreateCommandPool(device, &poolCreateInfo, nullptr, &commandPool) != VK_SUCCESS) {
        throwErrorAndExit("Failed to create command queue for graphics queue family");
    }
    else {
        std::cout << "created command pool for graphics queue family" << std::endl;
    }
}

// Find device memory that is supported by the requirements (typeBits) and meets the desired properties
VkBool32 
Application::getMemoryType(uint32_t typeBits, VkFlags properties, uint32_t* typeIndex) {
    for (uint32_t i = 0; i < 32; i++) {
        if ((typeBits & 1) == 1) {
            if ((deviceMemoryProperties.memoryTypes[i].propertyFlags & properties) == properties) {
                *typeIndex = i;
                return true;
            }
        }
        typeBits >>= 1;
    }
    return false;
}

void
Application::createVertexBuffer() {

    uint32_t const verticesSize = static_cast<uint32_t>(vertices.size() * sizeof(vertices[0]));
    uint32_t const indicesSize = static_cast<uint32_t>(indices.size() * sizeof(indices[0]));
    VkMemoryAllocateInfo memoryAllocateInfo = {};
    memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    VkMemoryRequirements memoryRequirements;
    void* data;

    struct StagingBuffer {
        VkDeviceMemory memory;
        VkBuffer buffer;
    };

    struct {
        StagingBuffer vertices;
        StagingBuffer indices;
    } stagingBuffers;

    // Allocate command buffer for copy operation
    VkCommandBufferAllocateInfo cmdBufInfo = {};
    cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmdBufInfo.commandPool = commandPool;
    cmdBufInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmdBufInfo.commandBufferCount = 1;

    VkCommandBuffer copyCommandBuffer;
    vkAllocateCommandBuffers(device, &cmdBufInfo, &copyCommandBuffer);

    // First copy vertices to host accessible vertex buffer memory
    VkBufferCreateInfo vertexBufferInfo = {};
    vertexBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    vertexBufferInfo.size = verticesSize;
    vertexBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

    vkCreateBuffer(device, &vertexBufferInfo, nullptr, &stagingBuffers.vertices.buffer);

    vkGetBufferMemoryRequirements(device, stagingBuffers.vertices.buffer, &memoryRequirements);
    memoryAllocateInfo.allocationSize = memoryRequirements.size;
    getMemoryType(memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, &memoryAllocateInfo.memoryTypeIndex);
    vkAllocateMemory(device, &memoryAllocateInfo, nullptr, &stagingBuffers.vertices.memory);

    vkMapMemory(device, stagingBuffers.vertices.memory, 0, verticesSize, 0, &data);
    memcpy(data, vertices.data(), verticesSize);
    vkUnmapMemory(device, stagingBuffers.vertices.memory);
    vkBindBufferMemory(device, stagingBuffers.vertices.buffer, stagingBuffers.vertices.memory, 0);

    // Then allocate a gpu only buffer for vertices
    vertexBufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    vkCreateBuffer(device, &vertexBufferInfo, nullptr, &vertexBuffer);
    vkGetBufferMemoryRequirements(device, vertexBuffer, &memoryRequirements);
    memoryAllocateInfo.allocationSize = memoryRequirements.size;
    getMemoryType(memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &memoryAllocateInfo.memoryTypeIndex);
    vkAllocateMemory(device, &memoryAllocateInfo, nullptr, &vertexBufferMemory);
    vkBindBufferMemory(device, vertexBuffer, vertexBufferMemory, 0);

    // Next copy indices to host accessible index buffer memory
    VkBufferCreateInfo indexBufferInfo = {};
    indexBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    indexBufferInfo.size = indicesSize;
    indexBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

    vkCreateBuffer(device, &indexBufferInfo, nullptr, &stagingBuffers.indices.buffer);
    vkGetBufferMemoryRequirements(device, stagingBuffers.indices.buffer, &memoryRequirements);
    memoryAllocateInfo.allocationSize = memoryRequirements.size;
    getMemoryType(memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, &memoryAllocateInfo.memoryTypeIndex);
    vkAllocateMemory(device, &memoryAllocateInfo, nullptr, &stagingBuffers.indices.memory);
    vkMapMemory(device, stagingBuffers.indices.memory, 0, indicesSize, 0, &data);
    memcpy(data, indices.data(), indicesSize);
    vkUnmapMemory(device, stagingBuffers.indices.memory);
    vkBindBufferMemory(device, stagingBuffers.indices.buffer, stagingBuffers.indices.memory, 0);

    // And allocate another gpu only buffer for indices
    indexBufferInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    vkCreateBuffer(device, &indexBufferInfo, nullptr, &indexBuffer);
    vkGetBufferMemoryRequirements(device, indexBuffer, &memoryRequirements);
    memoryAllocateInfo.allocationSize = memoryRequirements.size;
    getMemoryType(memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &memoryAllocateInfo.memoryTypeIndex);
    vkAllocateMemory(device, &memoryAllocateInfo, nullptr, &indexBufferMemory);
    vkBindBufferMemory(device, indexBuffer, indexBufferMemory, 0);

    // Now copy data from host visible buffer to gpu only buffer
    VkCommandBufferBeginInfo bufferBeginInfo = {};
    bufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    bufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(copyCommandBuffer, &bufferBeginInfo);

    VkBufferCopy copyRegion = {};
    copyRegion.size = verticesSize;
    vkCmdCopyBuffer(copyCommandBuffer, stagingBuffers.vertices.buffer, vertexBuffer, 1, &copyRegion);
    copyRegion.size = indicesSize;
    vkCmdCopyBuffer(copyCommandBuffer, stagingBuffers.indices.buffer, indexBuffer, 1, &copyRegion);

    {
        auto const result = vkEndCommandBuffer(copyCommandBuffer);
        if(VK_SUCCESS != result)
        {
            throwErrorAndExit("vkEndCommandBuffer failed");
        }
    }
    // Submit to queue
    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &copyCommandBuffer;

    {
        auto const result = vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
        if(VK_SUCCESS != result)
        {
            throwErrorAndExit("vkEndCommandBuffer failed");
        }
    }
    {
        auto const result = vkQueueWaitIdle(graphicsQueue);
        if(VK_SUCCESS != result)
        {
            throwErrorAndExit("vkQueueWaitIdle failed");
        }
    }
    vkFreeCommandBuffers(device, commandPool, 1, &copyCommandBuffer);

    vkDestroyBuffer(device, stagingBuffers.vertices.buffer, nullptr);
    vkFreeMemory(device, stagingBuffers.vertices.memory, nullptr);
    vkDestroyBuffer(device, stagingBuffers.indices.buffer, nullptr);
    vkFreeMemory(device, stagingBuffers.indices.memory, nullptr);

    std::cout << "set up vertex and index buffers" << std::endl;

    // Binding and attribute descriptions
    vertexBindingDescription.binding = 0;
    vertexBindingDescription.stride = sizeof(vertices[0]);
    vertexBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    // vec3 position
    vertexAttributeDescriptions.resize(2);
    vertexAttributeDescriptions[0].binding = 0;
    vertexAttributeDescriptions[0].location = 0;
    vertexAttributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    vertexAttributeDescriptions[0].offset = offsetof(Vertex,pos);

    // vec3 color
    vertexAttributeDescriptions[1].binding = 0;
    vertexAttributeDescriptions[1].location = 1;
    vertexAttributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    vertexAttributeDescriptions[1].offset = offsetof(Vertex, color);

}

void
Application::createUniformBuffer() {

    uniformBuffers.resize(swapChainImages.size());
    uniformBuffersMemory.resize(swapChainImages.size());

    for(int i=0; i<swapChainImages.size(); i++){
        VkBufferCreateInfo bufferInfo = {};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = sizeof(UniformBufferObject);
        bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;

        vkCreateBuffer(device, &bufferInfo, nullptr, &uniformBuffers[i]);

        VkMemoryRequirements memReqs;
        vkGetBufferMemoryRequirements(device, uniformBuffers[i], &memReqs);

        VkMemoryAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memReqs.size;
        getMemoryType(
            memReqs.memoryTypeBits, 
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, 
            &allocInfo.memoryTypeIndex
        );

        vkAllocateMemory(device, &allocInfo, nullptr, &uniformBuffersMemory[i]);
        vkBindBufferMemory(device, uniformBuffers[i], uniformBuffersMemory[i], 0);
    }
}

float degree = 0;

void 
Application::updateUniformBuffer(uint32_t currentImage) {

    UniformBufferObject ubo{};
    
    degree++;
    if(degree >= 360.0f)
    {
        degree = 0.0f;
    }

    {// Model
        Matrix4X4Float rotation;
        Matrix4X4Float::assignRotationXYZ(rotation,Math::deg2Rad(degree),Math::deg2Rad(degree),Math::deg2Rad(0.0f));
        ::memcpy(ubo.rotation,rotation.cells,sizeof(ubo.rotation));
    }
    {// Transformation
        Matrix4X4Float transformation;
        Matrix4X4Float::assignTransformation(transformation,0,0,-300);
        ::memcpy(ubo.transformation,transformation.cells,sizeof(ubo.transformation));
    }
    {// Perspective
        Matrix4X4Float perspective;
        Matrix4X4Float::PreparePerspectiveProjectionMatrix(
            perspective,
            RATIO,
            40,
            Z_NEAR,
            Z_FAR
        );
        ::memcpy(ubo.perspective,perspective.cells,sizeof(ubo.perspective));
    }
    
    void* data;
    {
        const auto result = vkMapMemory(
            device, 
            uniformBuffersMemory[currentImage], 
            0, 
            sizeof(UniformBufferObject), 
            0, 
            &data
        );
        if(VK_SUCCESS != result)
        {
            throwErrorAndExit("vkMapMemory for uniform buffer failed");
        }
    }

    memcpy(data, &ubo, sizeof(UniformBufferObject));

    vkUnmapMemory(device, uniformBuffersMemory[currentImage]);
}

void
Application::createSwapChain() {
    // Find surface capabilities
    VkSurfaceCapabilitiesKHR surfaceCapabilities;
    if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, windowSurface, &surfaceCapabilities) != VK_SUCCESS) {
        throwErrorAndExit("Failed to acquire presentation surface capabilities");
    }

    // Find supported surface formats
    uint32_t formatCount;
    if (vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, windowSurface, &formatCount, nullptr) != VK_SUCCESS || formatCount == 0) {
        throwErrorAndExit("Failed to get number of supported surface formats");
    }

    std::vector<VkSurfaceFormatKHR> surfaceFormats(formatCount);
    if (vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, windowSurface, &formatCount, surfaceFormats.data()) != VK_SUCCESS) {
        throwErrorAndExit("Failed to get supported surface formats");
    }

    // Find supported present modes
    uint32_t presentModeCount;
    if (vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, windowSurface, &presentModeCount, nullptr) != VK_SUCCESS || presentModeCount == 0) {
        throwErrorAndExit("Failed to get number of supported presentation modes");
    }

    std::vector<VkPresentModeKHR> presentModes(presentModeCount);
    if (vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, windowSurface, &presentModeCount, presentModes.data()) != VK_SUCCESS) {
        throwErrorAndExit("Failed to get supported presentation modes");
    }

    // Determine number of images for swap chain
    uint32_t imageCount = surfaceCapabilities.minImageCount + 1;
    if (surfaceCapabilities.maxImageCount != 0 && imageCount > surfaceCapabilities.maxImageCount) {
        imageCount = surfaceCapabilities.maxImageCount;
    }

    std::cout << "using " << imageCount << " images for swap chain" << std::endl;

    // Select a surface format
    VkSurfaceFormatKHR surfaceFormat = chooseSurfaceFormat(surfaceFormats);

    // Select swap chain size
    swapChainExtent = chooseSwapExtent(surfaceCapabilities);

    // Determine transformation to use (preferring no transform)
    VkSurfaceTransformFlagBitsKHR surfaceTransform;
    if (surfaceCapabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR) {
        surfaceTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    }
    else {
        surfaceTransform = surfaceCapabilities.currentTransform;
    }

    // Choose presentation mode (preferring MAILBOX ~= triple buffering)
    VkPresentModeKHR presentMode = choosePresentMode(presentModes);

    // Finally, create the swap chain
    VkSwapchainCreateInfoKHR createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = windowSurface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = swapChainExtent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    createInfo.queueFamilyIndexCount = 0;
    createInfo.pQueueFamilyIndices = nullptr;
    createInfo.preTransform = surfaceTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = oldSwapChain;

    if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain) != VK_SUCCESS) {
        throwErrorAndExit("Failed to create swap chain");
    }
    else {
        std::cout << "created swap chain" << std::endl;
    }

    if (oldSwapChain != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(device, oldSwapChain, nullptr);
    }
    oldSwapChain = swapChain;

    swapChainFormat = surfaceFormat.format;

// Store the images used by the swap chain
    // Note: these are the images that swap chain image indices refer to
    // Note: actual number of images may differ from requested number, since it's a lower bound
    uint32_t actualImageCount = 0;
    if (
        vkGetSwapchainImagesKHR(device, swapChain, &actualImageCount, nullptr) != VK_SUCCESS || 
        actualImageCount == 0
    ) {
        throwErrorAndExit("Failed to acquire number of swap chain images");
    }

    swapChainImages.resize(actualImageCount);

    if (vkGetSwapchainImagesKHR(device, swapChain, &actualImageCount, swapChainImages.data()) != VK_SUCCESS) {
        throwErrorAndExit("Failed to acquire swap chain images");

    }

    std::cout << "acquired swap chain images" << std::endl;
}

VkSurfaceFormatKHR 
Application::chooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
    // We can either choose any format
    if (availableFormats.size() == 1 && availableFormats[0].format == VK_FORMAT_UNDEFINED) {
        return{ VK_FORMAT_R8G8B8A8_UNORM, VK_COLORSPACE_SRGB_NONLINEAR_KHR };
    }

    // Or go with the standard format - if available
    for (const auto& availableSurfaceFormat : availableFormats) {
        if (availableSurfaceFormat.format == VK_FORMAT_R8G8B8A8_UNORM) {
            return availableSurfaceFormat;
        }
    }

    // Or fall back to the first available one
    return availableFormats[0];
}

VkPresentModeKHR 
Application::choosePresentMode(const std::vector<VkPresentModeKHR> presentModes) {
    for (const auto& presentMode : presentModes) {
        if (presentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return presentMode;
        }
    }

    // If mailbox is unavailable, fall back to FIFO (guaranteed to be available)
    return VK_PRESENT_MODE_FIFO_KHR;
}

uint32_t fMin(uint32_t a,uint32_t b) {
    return a < b ? a : b;
}

uint32_t fMax(uint32_t a, uint32_t b) {
    return a > b ? a : b;
}

VkExtent2D 
Application::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& surfaceCapabilities) {
    if (surfaceCapabilities.currentExtent.width == -1) {
        VkExtent2D const swapChainExtent = {
            fMin(
            fMax(SCREEN_WIDTH, surfaceCapabilities.minImageExtent.width), 
            surfaceCapabilities.maxImageExtent.width
            ),
            fMin(
            fMax(SCREEN_HEIGHT, surfaceCapabilities.minImageExtent.height), 
            surfaceCapabilities.maxImageExtent.height
            )
        };
        return swapChainExtent;
    }
    return surfaceCapabilities.currentExtent;
}

void 
Application::createRenderPass() {
    VkAttachmentDescription colorAttachment = {};
    colorAttachment.format = swapChainFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentDescription depthAttachment{};
    depthAttachment.format = findDepthFormat();
    depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
  
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

    if (vkCreateRenderPass(device, &createInfo, nullptr, &renderPass) != VK_SUCCESS) {
        throwErrorAndExit("Failed to create render pass");
    }
    else {
        std::cout << "created render pass" << std::endl;
    }
}

uint32_t
Application::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    throwErrorAndExit("failed to find suitable memory type!");
    return 0;
}

void
Application::createImage(
    uint32_t width, 
    uint32_t height, 
    VkFormat format, 
    VkImageTiling tiling, 
    VkImageUsageFlags usage, 
    VkMemoryPropertyFlags properties, 
    VkImage& image, 
    VkDeviceMemory& imageMemory
) {
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = tiling;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = usage;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateImage(device, &imageInfo, nullptr, &image) != VK_SUCCESS) {
        throw std::runtime_error("failed to create image!");
    }

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(device, image, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

    if (vkAllocateMemory(device, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate image memory!");
    }

    vkBindImageMemory(device, image, imageMemory, 0);
}

void
Application::createImageViews() {
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

void
Application::createImageView(VkImageView * imageView, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags)
{
    VkImageViewCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    createInfo.image = image;
    createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    createInfo.format = format;
    createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.subresourceRange.aspectMask = aspectFlags;
    createInfo.subresourceRange.baseMipLevel = 0;
    createInfo.subresourceRange.levelCount = 1;
    createInfo.subresourceRange.baseArrayLayer = 0;
    createInfo.subresourceRange.layerCount = 1;
    if (vkCreateImageView(device, &createInfo, nullptr, imageView) != VK_SUCCESS) {
        throwErrorAndExit("failed to create image view for swap chain image #");
    }
}

void 
Application::createFramebuffers() {
    swapChainFramebuffers.resize(swapChainImages.size());

    // Note: FrameBuffer is basically a specific choice of attachments for a render pass
    // That means all attachments must have the same dimensions, interesting restriction
    for (size_t i = 0; i < swapChainImages.size(); i++) {

        std::vector<VkImageView> attachments = {
            swapChainImageViews[i],
            depthImageView
        };

        VkFramebufferCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        createInfo.renderPass = renderPass;
        createInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        createInfo.pAttachments = attachments.data();
        createInfo.width = swapChainExtent.width;
        createInfo.height = swapChainExtent.height;
        createInfo.layers = 1;

        if (vkCreateFramebuffer(device, &createInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS) {
            throwErrorAndExit("failed to create framebuffer for swap chain image view #");
        }
    }

    std::cout << "created framebuffers for swap chain image views" << std::endl;
}

void 
Application::createGraphicsPipeline() {
    VkShaderModule vertexShaderModule = createShaderModule("./assets/shaders/vert.spv");
    VkShaderModule fragmentShaderModule = createShaderModule("./assets/shaders/frag.spv");

    // Set up shader stage info
    VkPipelineShaderStageCreateInfo vertexShaderCreateInfo = {};
    vertexShaderCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertexShaderCreateInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertexShaderCreateInfo.module = vertexShaderModule;
    vertexShaderCreateInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragmentShaderCreateInfo = {};
    fragmentShaderCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragmentShaderCreateInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragmentShaderCreateInfo.module = fragmentShaderModule;
    fragmentShaderCreateInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = { vertexShaderCreateInfo, fragmentShaderCreateInfo };

    // Describe vertex input
    VkPipelineVertexInputStateCreateInfo vertexInputCreateInfo = {};
    vertexInputCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputCreateInfo.vertexBindingDescriptionCount = 1;
    vertexInputCreateInfo.pVertexBindingDescriptions = &vertexBindingDescription;
    vertexInputCreateInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexAttributeDescriptions.size());
    vertexInputCreateInfo.pVertexAttributeDescriptions = vertexAttributeDescriptions.data();

    // Describe input assembly
    VkPipelineInputAssemblyStateCreateInfo inputAssemblyCreateInfo = {};
    inputAssemblyCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssemblyCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssemblyCreateInfo.primitiveRestartEnable = VK_FALSE;

    // Describe viewport and scissor
    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(swapChainExtent.width);
    viewport.height = static_cast<float>(swapChainExtent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor = {};
    scissor.offset.x = 0;
    scissor.offset.y = 0;
    scissor.extent.width = swapChainExtent.width;
    scissor.extent.height = swapChainExtent.height;

    // Note: scissor test is always enabled (although dynamic scissor is possible)
    // Number of viewports must match number of scissors
    VkPipelineViewportStateCreateInfo viewportCreateInfo = {};
    viewportCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportCreateInfo.viewportCount = 1;
    viewportCreateInfo.pViewports = &viewport;
    viewportCreateInfo.scissorCount = 1;
    viewportCreateInfo.pScissors = &scissor;

    // Describe rasterization
    // Note: depth bias and using polygon modes other than fill require changes to logical device creation (device features)
    VkPipelineRasterizationStateCreateInfo rasterizationCreateInfo = {};
    rasterizationCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizationCreateInfo.depthClampEnable = VK_FALSE;
    rasterizationCreateInfo.rasterizerDiscardEnable = VK_FALSE;
    rasterizationCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizationCreateInfo.cullMode = VK_CULL_MODE_NONE;
    rasterizationCreateInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizationCreateInfo.depthBiasEnable = VK_FALSE;
    //rasterizationCreateInfo.depthBiasConstantFactor = 0.0f;
    //rasterizationCreateInfo.depthBiasClamp = 0.0f;
    //rasterizationCreateInfo.depthBiasSlopeFactor = 0.0f;
    rasterizationCreateInfo.lineWidth = 1.0f;
    
    // Describe multisampling
    // Note: using multisampling also requires turning on device features
    VkPipelineMultisampleStateCreateInfo multisampleCreateInfo = {};
    multisampleCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampleCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampleCreateInfo.sampleShadingEnable = VK_FALSE;
    multisampleCreateInfo.minSampleShading = 1.0f;
    multisampleCreateInfo.alphaToCoverageEnable = VK_FALSE;
    multisampleCreateInfo.alphaToOneEnable = VK_FALSE;

    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_TRUE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.stencilTestEnable = VK_FALSE;

    // Describing color blending
    // Note: all paramaters except blendEnable and colorWriteMask are irrelevant here
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
    VkPipelineColorBlendStateCreateInfo colorBlendCreateInfo = {};
    colorBlendCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlendCreateInfo.logicOpEnable = VK_FALSE;
    colorBlendCreateInfo.logicOp = VK_LOGIC_OP_COPY;
    colorBlendCreateInfo.attachmentCount = 1;
    colorBlendCreateInfo.pAttachments = &colorBlendAttachmentState;
    colorBlendCreateInfo.blendConstants[0] = 0.0f;
    colorBlendCreateInfo.blendConstants[1] = 0.0f;
    colorBlendCreateInfo.blendConstants[2] = 0.0f;
    colorBlendCreateInfo.blendConstants[3] = 0.0f;

    // Describe pipeline layout
    // Note: this describes the mapping between memory and shader resources (descriptor sets)
    // This is for uniform buffers and samplers
    VkDescriptorSetLayoutBinding uboLayoutBinding {};
    uboLayoutBinding.binding = 0;
    // Our object type is uniformBuffer
    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    // Number of values in buffer
    uboLayoutBinding.descriptorCount = 1;
    // Which shader stage we are going to reffer to // In our case we only need vertex shader to access to it
    uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    // For Image sampler, Currently we do not need this
    uboLayoutBinding.pImmutableSamplers = nullptr; // Optional

    VkDescriptorSetLayoutCreateInfo descriptorLayoutCreateInfo = {};
    descriptorLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptorLayoutCreateInfo.bindingCount = 1;
    descriptorLayoutCreateInfo.pBindings = &uboLayoutBinding;

    if (vkCreateDescriptorSetLayout(device, &descriptorLayoutCreateInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
        throwErrorAndExit("Failed to create descriptor layout");
    }
    else {
        std::cout << "created descriptor layout" << std::endl;
    }

    VkPipelineLayoutCreateInfo layoutCreateInfo = {};
    layoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutCreateInfo.setLayoutCount = 1;
    layoutCreateInfo.pSetLayouts = &descriptorSetLayout; // Array of descriptor set layout, Order matter when more than 1

    if (vkCreatePipelineLayout(device, &layoutCreateInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
        throwErrorAndExit("Failed to create pipeline layout");
    }
    else {
        std::cout << "created pipeline layout" << std::endl;
    }

    // Create the graphics pipeline
    VkGraphicsPipelineCreateInfo pipelineCreateInfo = {};
    pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineCreateInfo.stageCount = 2;
    pipelineCreateInfo.pStages = shaderStages;
    pipelineCreateInfo.pVertexInputState = &vertexInputCreateInfo;
    pipelineCreateInfo.pInputAssemblyState = &inputAssemblyCreateInfo;
    pipelineCreateInfo.pViewportState = &viewportCreateInfo;
    pipelineCreateInfo.pRasterizationState = &rasterizationCreateInfo;
    pipelineCreateInfo.pMultisampleState = &multisampleCreateInfo;
    pipelineCreateInfo.pColorBlendState = &colorBlendCreateInfo;
    pipelineCreateInfo.layout = pipelineLayout;
    pipelineCreateInfo.renderPass = renderPass;
    pipelineCreateInfo.subpass = 0;
    pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineCreateInfo.basePipelineIndex = -1;
    pipelineCreateInfo.pDepthStencilState = &depthStencil;
        

    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &graphicsPipeline) != VK_SUCCESS) {
        throwErrorAndExit("Failed to create graphics pipeline");
    }
    else {
        std::cout << "created graphics pipeline" << std::endl;
    }

    // No longer necessary
    vkDestroyShaderModule(device, vertexShaderModule, nullptr);
    vkDestroyShaderModule(device, fragmentShaderModule, nullptr);
}

VkShaderModule 
Application::createShaderModule(const char * filename) {
    assert(std::filesystem::exists(filename));

    std::ifstream file(filename, std::ios::ate | std::ios::binary);
    std::vector<char> fileBytes(file.tellg());
    file.seekg(0, std::ios::beg);
    file.read(fileBytes.data(), fileBytes.size());
    file.close();

    VkShaderModuleCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = fileBytes.size();
    createInfo.pCode = (uint32_t*)fileBytes.data();

    VkShaderModule shaderModule;
    if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        throwErrorAndExit("Failed to create shader module for " + std::string(filename) + '\n');
    }

    std::cout << "created shader module for " << filename << std::endl;

    return shaderModule;
}

void
Application::createDescriptorPool() {
    // This describes how many descriptor sets we'll create from this pool for each type
    VkDescriptorPoolSize poolSize {};
    poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSize.descriptorCount = static_cast<uint32_t>(swapChainImages.size());;

    VkDescriptorPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;
    poolInfo.maxSets = static_cast<uint32_t>(swapChainImages.size());;

    if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
        throwErrorAndExit("failed to create descriptor pool");
    }
    else {
        std::cout << "created descriptor pool" << std::endl;
    }
}

void 
Application::createDescriptorSet() {
    std::vector<VkDescriptorSetLayout> layouts(swapChainImages.size(), descriptorSetLayout);
    // There needs to be one descriptor set per binding point in the shader
    VkDescriptorSetAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = static_cast<uint32_t>(swapChainImages.size());
    allocInfo.pSetLayouts = layouts.data();

    descriptorSets.resize(swapChainImages.size());
    // Descriptor sets gets desctoroyed automaticly when descriptor pool is destroyed
    if (vkAllocateDescriptorSets(device, &allocInfo, descriptorSets.data()) != VK_SUCCESS) {
        throwErrorAndExit("Failed to create descriptor set");
    }
    else {
        std::cout << "created descriptor set" << std::endl;
    }

    for(size_t i=0;i<swapChainImages.size();i++){
        // Update descriptor set with uniform binding
        VkDescriptorBufferInfo descriptorBufferInfo = {};
        descriptorBufferInfo.buffer = uniformBuffers[i];
        descriptorBufferInfo.offset = 0;
        descriptorBufferInfo.range = sizeof(UniformBufferObject);

        VkWriteDescriptorSet writeDescriptorSet = {};
        writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeDescriptorSet.dstSet = descriptorSets[i];
        // How many array elements are we going to update at same time
        writeDescriptorSet.descriptorCount = 1;
        writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        writeDescriptorSet.pBufferInfo = &descriptorBufferInfo;
        writeDescriptorSet.dstBinding = 0;
        writeDescriptorSet.pImageInfo = nullptr; // Optional
        writeDescriptorSet.pTexelBufferView = nullptr; // Optional
        vkUpdateDescriptorSets(device, 1, &writeDescriptorSet, 0, nullptr);
    }

}

void 
Application::createCommandBuffers() {
    // Allocate graphics command buffers
    graphicsCommandBuffers.resize(swapChainImages.size());

    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = static_cast<uint32_t>(swapChainImages.size());

    if (vkAllocateCommandBuffers(device, &allocInfo, graphicsCommandBuffers.data()) != VK_SUCCESS) {
        throwErrorAndExit("Failed to allocate graphics command buffers");
    }
    else {
        std::cout << "allocated graphics command buffers" << std::endl;
    }

    // Prepare data for recording command buffers
    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

    VkImageSubresourceRange subResourceRange = {};
    subResourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    subResourceRange.baseMipLevel = 0;
    subResourceRange.levelCount = 1;
    subResourceRange.baseArrayLayer = 0;
    subResourceRange.layerCount = 1;

    //VkClearValue clearColor = {
    //    { 0.1f, 0.1f, 0.1f, 1.0f } // R, G, B, A
    //};

    std::vector<VkClearValue> clearValues{};
    clearValues.resize(2);
    clearValues[0].color = { 0.1f, 0.1f, 0.1f, 1.0f };
    clearValues[1].depthStencil = {1.0f, 0};

    // Record command buffer for each swap image
    for (size_t i = 0; i < swapChainImages.size(); i++) {
        vkBeginCommandBuffer(graphicsCommandBuffers[i], &beginInfo);

        // If present queue family and graphics queue family are different, then a barrier is necessary
        // The barrier is also needed initially to transition the image to the present layout
        VkImageMemoryBarrier presentToDrawBarrier = {};
        presentToDrawBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        presentToDrawBarrier.srcAccessMask = 0;
        presentToDrawBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        presentToDrawBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        presentToDrawBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        if (presentQueueFamily != graphicsQueueFamily) {
            presentToDrawBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            presentToDrawBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        }
        else {
            presentToDrawBarrier.srcQueueFamilyIndex = presentQueueFamily;
            presentToDrawBarrier.dstQueueFamilyIndex = graphicsQueueFamily;
        }

        presentToDrawBarrier.image = swapChainImages[i];
        presentToDrawBarrier.subresourceRange = subResourceRange;

        vkCmdPipelineBarrier(
            graphicsCommandBuffers[i], 
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 
            0, 0, nullptr, 0, nullptr, 1, 
            &presentToDrawBarrier
        );

        VkRenderPassBeginInfo renderPassBeginInfo = {};
        renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassBeginInfo.renderPass = renderPass;
        renderPassBeginInfo.framebuffer = swapChainFramebuffers[i];
        renderPassBeginInfo.renderArea.offset.x = 0;
        renderPassBeginInfo.renderArea.offset.y = 0;
        renderPassBeginInfo.renderArea.extent = swapChainExtent;
        renderPassBeginInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
        renderPassBeginInfo.pClearValues = clearValues.data();


        vkCmdBeginRenderPass(graphicsCommandBuffers[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

        vkCmdBindDescriptorSets(
            graphicsCommandBuffers[i], 
            VK_PIPELINE_BIND_POINT_GRAPHICS, 
            pipelineLayout, 0, 1, 
            &descriptorSets[i], 0, nullptr);

        vkCmdBindPipeline(graphicsCommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

        VkDeviceSize offset = 0;
        vkCmdBindVertexBuffers(graphicsCommandBuffers[i], 0, 1, &vertexBuffer, &offset);

        vkCmdBindIndexBuffer(graphicsCommandBuffers[i], indexBuffer, 0, VK_INDEX_TYPE_UINT32);

        vkCmdDrawIndexed(graphicsCommandBuffers[i], static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
        
        vkCmdEndRenderPass(graphicsCommandBuffers[i]);

        // If present and graphics queue families differ, then another barrier is required
        if (presentQueueFamily != graphicsQueueFamily) {
            VkImageMemoryBarrier drawToPresentBarrier = {};
            drawToPresentBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            drawToPresentBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            drawToPresentBarrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
            drawToPresentBarrier.oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
            drawToPresentBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
            drawToPresentBarrier.srcQueueFamilyIndex = graphicsQueueFamily;
            drawToPresentBarrier.dstQueueFamilyIndex = presentQueueFamily;
            drawToPresentBarrier.image = swapChainImages[i];
            drawToPresentBarrier.subresourceRange = subResourceRange;

            vkCmdPipelineBarrier(
                graphicsCommandBuffers[i], 
                VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 
                VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 
                0, 0, nullptr, 
                0, nullptr, 1, 
                &drawToPresentBarrier
            );
        }

        if (vkEndCommandBuffer(graphicsCommandBuffers[i]) != VK_SUCCESS) {
            throwErrorAndExit("Failed to record command buffer");
        }
    }

    std::cout << "recorded command buffers" << std::endl;

    // No longer needed
    vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
}

void 
Application::mainLoop() {
    bool quit = false;
    //Event handler
    SDL_Event e;
    //While application is running
    uint32_t targeFps = 1000 / 60;
    uint32_t startTime;
    uint32_t deltaTime;
    while (!quit)
    {
        startTime = SDL_GetTicks();
        drawFrame();
        //Handle events on queue
        if (SDL_PollEvent(&e) != 0)
        {
            //User requests quit
            if (e.type == SDL_QUIT)
            {
                quit = true;
            }
        }
        deltaTime = SDL_GetTicks() - startTime;
        //std::cout << "TargetFps:" << targeFps << " DeltaTime:" << deltaTime << std::endl;
        if(targeFps > deltaTime){
            SDL_Delay( targeFps - deltaTime );
        }
    }
}

void 
Application::drawFrame() {
    // Acquire image
    uint32_t imageIndex;
    VkResult res = vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);

    // Unless surface is out of date right now, defer swap chain recreation until end of this frame
    if (res == VK_ERROR_OUT_OF_DATE_KHR) {
        onWindowSizeChanged();
        return;
    }
    if (res != VK_SUCCESS) {
        throwErrorAndExit("failed to acquire image");
    }

    updateUniformBuffer(imageIndex);

    // Wait for image to be available and draw
    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &imageAvailableSemaphore;

    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &renderingFinishedSemaphore;

    // This is the stage where the queue should wait on the semaphore
    VkPipelineStageFlags waitDstStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    submitInfo.pWaitDstStageMask = &waitDstStageMask;

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &graphicsCommandBuffers[imageIndex];

    const auto submitQueueResult = vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    if (submitQueueResult != VK_SUCCESS) {
        throwErrorAndExit("failed to submit draw command buffer");
    }

    // Present drawn image
    // Note: semaphore here is not strictly necessary, because commands are processed in submission order within a single queue
    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &renderingFinishedSemaphore;

    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &swapChain;
    presentInfo.pImageIndices = &imageIndex;

    res = vkQueuePresentKHR(presentQueue, &presentInfo);

    if (res == VK_SUBOPTIMAL_KHR || res == VK_ERROR_OUT_OF_DATE_KHR || windowResized) {
        onWindowSizeChanged();
    }
    else if (res != VK_SUCCESS) {
        throwErrorAndExit("failed to submit present command buffer");
    }
}

void 
Application::onWindowSizeChanged() {
    windowResized = false;

    // Only recreate objects that are affected by frame-buffer size changes
    cleanUp(false);

    createSwapChain();
    createRenderPass();
    createImageViews();
    createFramebuffers();
    createGraphicsPipeline();
    createCommandBuffers();
}

void 
Application::cleanUp(bool fullClean) {
    vkDeviceWaitIdle(device);

    vkFreeCommandBuffers(device, commandPool, (uint32_t)graphicsCommandBuffers.size(), graphicsCommandBuffers.data());

    vkDestroyPipeline(device, graphicsPipeline, nullptr);
    vkDestroyRenderPass(device, renderPass, nullptr);

    for (size_t i = 0; i < swapChainImages.size(); i++) {
        vkDestroyFramebuffer(device, swapChainFramebuffers[i], nullptr);
        vkDestroyImageView(device, swapChainImageViews[i], nullptr);
    }

    vkDestroyImageView(device,depthImageView,nullptr);
    //vkDestroyImageView(device,textureImageView,nullptr);

    vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);

    if (fullClean) {
        vkDestroySemaphore(device, imageAvailableSemaphore, nullptr);
        vkDestroySemaphore(device, renderingFinishedSemaphore, nullptr);

        vkDestroyCommandPool(device, commandPool, nullptr);

        // Clean up uniform buffer related objects
        vkDestroyDescriptorPool(device, descriptorPool, nullptr);
        for(int i=0 ; i<swapChainImages.size(); i++){
            vkDestroyBuffer(device, uniformBuffers[i], nullptr);
            vkFreeMemory(device, uniformBuffersMemory[i], nullptr);
        }
        // Buffers must be destroyed after no command buffers are referring to them anymore
        vkDestroyBuffer(device, vertexBuffer, nullptr);
        vkFreeMemory(device, vertexBufferMemory, nullptr);
        vkDestroyBuffer(device, indexBuffer, nullptr);
        vkFreeMemory(device, indexBufferMemory, nullptr);
        vkFreeMemory(device, depthImageMemory, nullptr);

        // Note: implicitly destroys images (in fact, we're not allowed to do that explicitly)
        vkDestroySwapchainKHR(device, swapChain, nullptr);

        vkDestroyDevice(device, nullptr);

        vkDestroySurfaceKHR(vkInstance, windowSurface, nullptr);

#ifdef DEBUGGING_ENABLED
        PFN_vkDestroyDebugReportCallbackEXT DestroyDebugReportCallback = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(vkInstance, "vkDestroyDebugReportCallbackEXT");
        DestroyDebugReportCallback(vkInstance, debugReportCallbackExtension, nullptr);
#endif // DEBUGGING_ENABLED

        vkDestroyInstance(vkInstance, nullptr);
    }
}

VkFormat 
Application::findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features)
{
    for (VkFormat format : candidates) {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);

        if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
            return format;
        }
        if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
            return format;
        }
    }

    throwErrorAndExit("failed to find supported format!");
    VkFormat result {};
    return result;
}

VkFormat
Application::findDepthFormat()
{
    return findSupportedFormat(
        {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
        VK_IMAGE_TILING_OPTIMAL,
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
    );
}

bool
Application::hasStencilComponent(VkFormat format) {
    return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}

void
Application::throwErrorAndExit(std::string const error){
    throw std::runtime_error(error);
}