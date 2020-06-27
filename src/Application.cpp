#include <Application.hpp>

#include <windows.h>
#include <cassert>
#include <vector>
#include <iostream>

void
Application::run(){
    //Create window
    {
        SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS);
        //Windows only
        int realScreenWidth = (int)GetSystemMetrics(SM_CXSCREEN);
        int realScreenHeight = (int)GetSystemMetrics(SM_CYSCREEN);

        int windowWidth = 800;
        int windowHeight = 600;

        window = SDL_CreateWindow(
            "VULKAN_ENGINE", 0, 0,
            800, 600,
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
    // createDebugCallback();
    // createWindowSurface();
    // findPhysicalDevice();
    // checkSwapChainSupport();
    // findQueueFamilies();
    // createLogicalDevice();
    // createSemaphores();
    // createCommandPool();
    // createVertexBuffer();
    // createUniformBuffer();
    // createSwapChain();
    // createRenderPass();
    // createImageViews();
    // createFramebuffers();
    // createGraphicsPipeline();
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
            vkInstanceExtensions.data() + 0
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