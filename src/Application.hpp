#ifndef APPLICATION_CLASS
#define APPLICATION_CLASS

#include <vulkan/vulkan.h>
#include <SDL2/SDL.h>

#define DEBUGGING_ENABLED

class Application{
public:
    void run();
private:
    void setupVulkan();
    void mainLoop();
    void cleanUp(bool);
    void createInstance();
private:
    VkSwapchainKHR oldSwapChain;
    VkSwapchainKHR swapChain;
};

#endif