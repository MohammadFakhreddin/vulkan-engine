#include <Application.hpp>

#include <windows.h>
#include <cassert>

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

        auto window = SDL_CreateWindow(
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

    VkApplicationInfo applicationInfo;
    VkInstanceCreateInfo instanceInfo;
    
    // Filling out application description:
    // sType is mandatory
    applicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    // pNext is mandatory
    applicationInfo.pNext = NULL;
    // The name of our application
    applicationInfo.pApplicationName = "Tutorial 1";
    // The name of the engine (e.g: Game engine name)
    applicationInfo.pEngineName = NULL;
    // The version of the engine
    applicationInfo.engineVersion = 1;
    // The version of Vulkan we're using for this application
    applicationInfo.apiVersion = VK_API_VERSION_1_0;

    // Filling out instance description:
    // sType is mandatory
    instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    // pNext is mandatory
    instanceInfo.pNext = NULL;
    // flags is mandatory
    instanceInfo.flags = 0;
    // The application info structure is then passed through the instance
    instanceInfo.pApplicationInfo = &applicationInfo;
    // Don't enable and layer
    instanceInfo.enabledLayerCount = 0;
    instanceInfo.ppEnabledLayerNames = NULL;
    // Don't enable any extensions
    instanceInfo.enabledExtensionCount = 0;
    instanceInfo.ppEnabledExtensionNames = NULL;

    // Now create the desired instance
    checkVKResult(vkCreateInstance(&instanceInfo, NULL, &vkInstance));
}

void
Application::checkVKResult(VkResult result) {
    assert(result == VK_SUCCESS);
}