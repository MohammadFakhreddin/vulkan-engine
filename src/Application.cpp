#include <Application.hpp>

#include <windows.h>
#include <cassert>
#include <iostream>
#include <filesystem>
#include <fstream>

Application::Application() {
    timeStart = std::chrono::high_resolution_clock::now();
}

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
    createUniformBuffer();
    createSwapChain();
    createRenderPass();
    createImageViews();
    createFramebuffers();
    createGraphicsPipeline();
    // createDescriptorPool();
    // createDescriptorSet();
    // createCommandBuffers();
}

void
Application::mainLoop(){

}

void
Application::cleanUp(bool action){

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
        applicationInfo.pEngineName = NULL;
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
            std::cerr << "no extensions supported!" << std::endl;
            exit(1);
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

    std::cerr << "physical device doesn't support swap chains" << std::endl;
    exit(1);
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
            std::cerr << "could not find a valid queue family with present support" << std::endl;
            exit(1);
        }
    }
    else {
        std::cerr << "could not find a valid queue family with graphics support" << std::endl;
        exit(1);
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
        std::cerr << "failed to create logical device" << std::endl;
        exit(1);
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
        std::cerr << "failed to create semaphores" << std::endl;
        exit(1);
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
        std::cerr << "failed to create command queue for graphics queue family" << std::endl;
        exit(1);
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
    struct Vertex {
        float pos[3];
        float color[3];
    };
    // Setup vertices
    std::vector<Vertex> vertices = {
        { { -0.5f, -0.5f,  0.0f }, { 1.0f, 0.0f, 0.0f } },
        { { -0.5f,  0.5f,  0.0f }, { 0.0f, 1.0f, 0.0f } },
        { {  0.5f,  0.5f,  0.0f }, { 0.0f, 0.0f, 1.0f } }
    };
    uint32_t verticesSize = (uint32_t)(vertices.size() * sizeof(vertices[0]));
    // Setup indices
    std::vector<uint32_t> indices = { 0, 1, 2 };
    uint32_t indicesSize = (uint32_t)(indices.size() * sizeof(indices[0]));
    VkMemoryAllocateInfo memAlloc = {};
    memAlloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    VkMemoryRequirements memReqs;
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

    vkGetBufferMemoryRequirements(device, stagingBuffers.vertices.buffer, &memReqs);
    memAlloc.allocationSize = memReqs.size;
    getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, &memAlloc.memoryTypeIndex);
    vkAllocateMemory(device, &memAlloc, nullptr, &stagingBuffers.vertices.memory);

    vkMapMemory(device, stagingBuffers.vertices.memory, 0, verticesSize, 0, &data);
    memcpy(data, vertices.data(), verticesSize);
    vkUnmapMemory(device, stagingBuffers.vertices.memory);
    vkBindBufferMemory(device, stagingBuffers.vertices.buffer, stagingBuffers.vertices.memory, 0);

    // Then allocate a gpu only buffer for vertices
    vertexBufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    vkCreateBuffer(device, &vertexBufferInfo, nullptr, &vertexBuffer);
    vkGetBufferMemoryRequirements(device, vertexBuffer, &memReqs);
    memAlloc.allocationSize = memReqs.size;
    getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &memAlloc.memoryTypeIndex);
    vkAllocateMemory(device, &memAlloc, nullptr, &vertexBufferMemory);
    vkBindBufferMemory(device, vertexBuffer, vertexBufferMemory, 0);

    // Next copy indices to host accessible index buffer memory
    VkBufferCreateInfo indexBufferInfo = {};
    indexBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    indexBufferInfo.size = indicesSize;
    indexBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

    vkCreateBuffer(device, &indexBufferInfo, nullptr, &stagingBuffers.indices.buffer);
    vkGetBufferMemoryRequirements(device, stagingBuffers.indices.buffer, &memReqs);
    memAlloc.allocationSize = memReqs.size;
    getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, &memAlloc.memoryTypeIndex);
    vkAllocateMemory(device, &memAlloc, nullptr, &stagingBuffers.indices.memory);
    vkMapMemory(device, stagingBuffers.indices.memory, 0, indicesSize, 0, &data);
    memcpy(data, indices.data(), indicesSize);
    vkUnmapMemory(device, stagingBuffers.indices.memory);
    vkBindBufferMemory(device, stagingBuffers.indices.buffer, stagingBuffers.indices.memory, 0);

    // And allocate another gpu only buffer for indices
    indexBufferInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    vkCreateBuffer(device, &indexBufferInfo, nullptr, &indexBuffer);
    vkGetBufferMemoryRequirements(device, indexBuffer, &memReqs);
    memAlloc.allocationSize = memReqs.size;
    getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &memAlloc.memoryTypeIndex);
    vkAllocateMemory(device, &memAlloc, nullptr, &indexBufferMemory);
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

    vkEndCommandBuffer(copyCommandBuffer);
    // Submit to queue
    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &copyCommandBuffer;

    vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(graphicsQueue);

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

    // vec2 position
    vertexAttributeDescriptions.resize(2);
    vertexAttributeDescriptions[0].binding = 0;
    vertexAttributeDescriptions[0].location = 0;
    vertexAttributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;

    // vec3 color
    vertexAttributeDescriptions[1].binding = 0;
    vertexAttributeDescriptions[1].location = 1;
    vertexAttributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    vertexAttributeDescriptions[1].offset = sizeof(float) * 3;

}

void
Application::createUniformBuffer() {
    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = sizeof(uniformBufferData);
    bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;

    vkCreateBuffer(device, &bufferInfo, nullptr, &uniformBuffer);

    VkMemoryRequirements memReqs;
    vkGetBufferMemoryRequirements(device, uniformBuffer, &memReqs);

    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memReqs.size;
    getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, &allocInfo.memoryTypeIndex);

    vkAllocateMemory(device, &allocInfo, nullptr, &uniformBufferMemory);
    vkBindBufferMemory(device, uniformBuffer, uniformBufferMemory, 0);

    updateUniformData();
}

void 
Application::updateUniformData() {
    // Rotate based on time
    auto timeNow = std::chrono::high_resolution_clock::now();
    long long millis = std::chrono::duration_cast<std::chrono::milliseconds>(timeStart - timeNow).count();
    float angle = (millis % 4000) / 4000.0f * glm::radians(360.f);

    glm::mat4 modelMatrix;
    modelMatrix = glm::rotate(modelMatrix, angle, glm::vec3(0, 0, 1));
    modelMatrix = glm::translate(modelMatrix, glm::vec3(0.5f / 3.0f, -0.5f / 3.0f, 0.0f));

    // Set up view
    auto viewMatrix = glm::lookAt(glm::vec3(1, 1, 1), glm::vec3(0, 0, 0), glm::vec3(0, 0, -1));

    // Set up projection
    auto projMatrix = glm::perspective(glm::radians(70.f), swapChainExtent.width / (float)swapChainExtent.height, 0.1f, 10.0f);

    uniformBufferData.transformationMatrix = projMatrix * viewMatrix * modelMatrix;

    void* data;
    vkMapMemory(device, uniformBufferMemory, 0, sizeof(uniformBufferData), 0, &data);
    memcpy(data, &uniformBufferData, sizeof(uniformBufferData));
    vkUnmapMemory(device, uniformBufferMemory);
}

void
Application::createSwapChain() {
    // Find surface capabilities
    VkSurfaceCapabilitiesKHR surfaceCapabilities;
    if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, windowSurface, &surfaceCapabilities) != VK_SUCCESS) {
        std::cerr << "failed to acquire presentation surface capabilities" << std::endl;
        exit(1);
    }

    // Find supported surface formats
    uint32_t formatCount;
    if (vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, windowSurface, &formatCount, nullptr) != VK_SUCCESS || formatCount == 0) {
        std::cerr << "failed to get number of supported surface formats" << std::endl;
        exit(1);
    }

    std::vector<VkSurfaceFormatKHR> surfaceFormats(formatCount);
    if (vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, windowSurface, &formatCount, surfaceFormats.data()) != VK_SUCCESS) {
        std::cerr << "failed to get supported surface formats" << std::endl;
        exit(1);
    }

    // Find supported present modes
    uint32_t presentModeCount;
    if (vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, windowSurface, &presentModeCount, nullptr) != VK_SUCCESS || presentModeCount == 0) {
        std::cerr << "failed to get number of supported presentation modes" << std::endl;
        exit(1);
    }

    std::vector<VkPresentModeKHR> presentModes(presentModeCount);
    if (vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, windowSurface, &presentModeCount, presentModes.data()) != VK_SUCCESS) {
        std::cerr << "failed to get supported presentation modes" << std::endl;
        exit(1);
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
        std::cerr << "failed to create swap chain" << std::endl;
        exit(1);
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
    if (vkGetSwapchainImagesKHR(device, swapChain, &actualImageCount, nullptr) != VK_SUCCESS || actualImageCount == 0) {
        std::cerr << "failed to acquire number of swap chain images" << std::endl;
        exit(1);
    }

    swapChainImages.resize(actualImageCount);

    if (vkGetSwapchainImagesKHR(device, swapChain, &actualImageCount, swapChainImages.data()) != VK_SUCCESS) {
        std::cerr << "failed to acquire swap chain images" << std::endl;
        exit(1);
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
        VkExtent2D swapChainExtent = {};

        swapChainExtent.width = fMin(
            fMax(SCREEN_WIDTH, surfaceCapabilities.minImageExtent.width), 
            surfaceCapabilities.maxImageExtent.width
        );
        swapChainExtent.height = fMin(
            fMax(SCREEN_HEIGHT, surfaceCapabilities.minImageExtent.height), 
            surfaceCapabilities.maxImageExtent.height
        );

        return swapChainExtent;
    }
    else {
        return surfaceCapabilities.currentExtent;
    }
}

void 
Application::createRenderPass() {
    VkAttachmentDescription attachmentDescription = {};
    attachmentDescription.format = swapChainFormat;
    attachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;
    attachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachmentDescription.initialLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    attachmentDescription.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    // Note: hardware will automatically transition attachment to the specified layout
    // Note: index refers to attachment descriptions array
    VkAttachmentReference colorAttachmentReference = {};
    colorAttachmentReference.attachment = 0;
    colorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    // Note: this is a description of how the attachments of the render pass will be used in this sub pass
    // e.g. if they will be read in shaders and/or drawn to
    VkSubpassDescription subPassDescription = {};
    subPassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subPassDescription.colorAttachmentCount = 1;
    subPassDescription.pColorAttachments = &colorAttachmentReference;

    // Create the render pass
    VkRenderPassCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    createInfo.attachmentCount = 1;
    createInfo.pAttachments = &attachmentDescription;
    createInfo.subpassCount = 1;
    createInfo.pSubpasses = &subPassDescription;

    if (vkCreateRenderPass(device, &createInfo, nullptr, &renderPass) != VK_SUCCESS) {
        std::cerr << "failed to create render pass" << std::endl;
        exit(1);
    }
    else {
        std::cout << "created render pass" << std::endl;
    }
}

void
Application::createImageViews() {
    swapChainImageViews.resize(swapChainImages.size());

    // Create an image view for every image in the swap chain
    for (size_t i = 0; i < swapChainImages.size(); i++) {
        VkImageViewCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = swapChainImages[i];
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = swapChainFormat;
        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(device, &createInfo, nullptr, &swapChainImageViews[i]) != VK_SUCCESS) {
            std::cerr << "failed to create image view for swap chain image #" << i << std::endl;
            exit(1);
        }
    }

    std::cout << "created image views for swap chain images" << std::endl;

}

void 
Application::createFramebuffers() {
    swapChainFramebuffers.resize(swapChainImages.size());

    // Note: Framebuffer is basically a specific choice of attachments for a render pass
    // That means all attachments must have the same dimensions, interesting restriction
    for (size_t i = 0; i < swapChainImages.size(); i++) {
        VkFramebufferCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        createInfo.renderPass = renderPass;
        createInfo.attachmentCount = 1;
        createInfo.pAttachments = &swapChainImageViews[i];
        createInfo.width = swapChainExtent.width;
        createInfo.height = swapChainExtent.height;
        createInfo.layers = 1;

        if (vkCreateFramebuffer(device, &createInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS) {
            std::cerr << "failed to create framebuffer for swap chain image view #" << i << std::endl;
            exit(1);
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
    vertexInputCreateInfo.vertexAttributeDescriptionCount = 2;
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
    viewport.width = (float)swapChainExtent.width;
    viewport.height = (float)swapChainExtent.height;
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
    rasterizationCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizationCreateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizationCreateInfo.depthBiasEnable = VK_FALSE;
    rasterizationCreateInfo.depthBiasConstantFactor = 0.0f;
    rasterizationCreateInfo.depthBiasClamp = 0.0f;
    rasterizationCreateInfo.depthBiasSlopeFactor = 0.0f;
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
    VkDescriptorSetLayoutBinding layoutBinding = {};
    layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    layoutBinding.descriptorCount = 1;
    layoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkDescriptorSetLayoutCreateInfo descriptorLayoutCreateInfo = {};
    descriptorLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptorLayoutCreateInfo.bindingCount = 1;
    descriptorLayoutCreateInfo.pBindings = &layoutBinding;

    if (vkCreateDescriptorSetLayout(device, &descriptorLayoutCreateInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
        std::cerr << "failed to create descriptor layout" << std::endl;
        exit(1);
    }
    else {
        std::cout << "created descriptor layout" << std::endl;
    }

    VkPipelineLayoutCreateInfo layoutCreateInfo = {};
    layoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutCreateInfo.setLayoutCount = 1;
    layoutCreateInfo.pSetLayouts = &descriptorSetLayout;

    if (vkCreatePipelineLayout(device, &layoutCreateInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
        std::cerr << "failed to create pipeline layout" << std::endl;
        exit(1);
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

    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &graphicsPipeline) != VK_SUCCESS) {
        std::cerr << "failed to create graphics pipeline" << std::endl;
        exit(1);
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
        std::cerr << "failed to create shader module for " << filename << std::endl;
        exit(1);
    }

    std::cout << "created shader module for " << filename << std::endl;

    return shaderModule;
}