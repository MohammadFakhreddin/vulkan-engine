#ifndef APPLICATION_CLASS
#define APPLICATION_CLASS

#include <vulkan/vulkan.h>
#include <SDL2/SDL.h>
#include <SDL2\SDL_vulkan.h>

#define DEBUGGING_ENABLED

class Application{
public:
    void run();
private:
    void setupVulkan();
    void mainLoop();
    void cleanUp(bool);
    void createInstance();
    void checkVKResult(VkResult result);
private:
    VkSwapchainKHR oldSwapChain;
    VkSwapchainKHR swapChain;
    SDL_Window * window;
    VkInstance vkInstance;
};

#endif