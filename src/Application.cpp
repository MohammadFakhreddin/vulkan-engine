#include <Application.hpp>
#include <windows.h>

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
            SDL_WINDOW_SHOWN | SDL_WINDOW_FULLSCREEN | SDL_WINDOW_VULKAN
        );
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
    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "VULKAN_ENGINE";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "VULKAN_ENGINE";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;
}