#ifndef APPLICATION_CLASS
#define APPLICATION_CLASS

#include <vulkan/vulkan.h>
#include <SDL2/SDL.h>
#include <SDL2\SDL_vulkan.h>

#define DEBUGGING_ENABLED

class Application{
private:
    static constexpr const char* DEBUG_LAYER = "VK_LAYER_LUNARG_standard_validation";
public:
    void run();
private:
    void setupVulkan();
    void mainLoop();
    void cleanUp(bool);
    void createInstance();
    void vkCheck(VkResult result);
    void sdlCheck(SDL_bool result);
    void createDebugCallback();
    void createWindowSurface();
    void findPhysicalDevice();
    void checkSwapChainSupport();
    void findQueueFamilies();
private:
    VkSwapchainKHR oldSwapChain;
    VkSwapchainKHR swapChain;
    SDL_Window* window;
    VkInstance vkInstance;
    VkDebugReportCallbackEXT debugReportCallbackExtension;
    VkPhysicalDevice physicalDevice;
    uint32_t graphicsQueueFamily;
    uint32_t presentQueueFamily;
    VkSurfaceKHR windowSurface;
};

#endif