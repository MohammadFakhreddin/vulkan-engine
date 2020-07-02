#ifndef APPLICATION_CLASS
#define APPLICATION_CLASS

#include <vulkan/vulkan.h>
#include <SDL2/SDL.h>
#include <SDL2\SDL_vulkan.h>
#include <vector>
#include <glm/ext.hpp>
#include <chrono>

#define DEBUGGING_ENABLED

class Application{
private:
    static constexpr const char* DEBUG_LAYER = "VK_LAYER_LUNARG_standard_validation";
    static constexpr uint32_t SCREEN_WIDTH = 640;
    static constexpr uint32_t SCREEN_HEIGHT = 480;
public:
    Application();
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
    void createLogicalDevice();
    void createSemaphores();
    void createCommandPool();
    void createVertexBuffer();
    void createUniformBuffer();
    void updateUniformData();
    void createSwapChain();
    void createRenderPass();
    void createImageViews();
    void createFramebuffers();
    void createGraphicsPipeline();
    void createDescriptorPool();
    void createDescriptorSet();
    void createCommandBuffers();
    void draw();
    void onWindowSizeChanged();
private:
    VkBool32 getMemoryType(uint32_t typeBits, VkFlags properties, uint32_t* typeIndex);
    VkSurfaceFormatKHR chooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
    VkPresentModeKHR choosePresentMode(const std::vector<VkPresentModeKHR> presentModes);
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& surfaceCapabilities);
    VkShaderModule createShaderModule(const char * filename);
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
    VkQueue graphicsQueue;
    VkQueue presentQueue;
    VkPhysicalDeviceMemoryProperties deviceMemoryProperties;
    VkDevice device;
    VkSemaphore imageAvailableSemaphore;
    VkSemaphore renderingFinishedSemaphore;
    VkCommandPool commandPool;
    VkVertexInputBindingDescription vertexBindingDescription;
    VkBuffer indexBuffer;
    VkDeviceMemory indexBufferMemory;
    VkDeviceMemory vertexBufferMemory;
    VkBuffer vertexBuffer;
    std::vector<VkVertexInputAttributeDescription> vertexAttributeDescriptions;
    struct {
        glm::mat4 transformationMatrix;
    } uniformBufferData;
    VkBuffer uniformBuffer;
    VkDeviceMemory uniformBufferMemory;
    std::chrono::high_resolution_clock::time_point timeStart;
    VkExtent2D swapChainExtent;
    std::vector<VkImage> swapChainImages;
    VkFormat swapChainFormat;
    VkRenderPass renderPass;
    std::vector<VkImageView> swapChainImageViews;
    std::vector<VkFramebuffer> swapChainFramebuffers;
    VkDescriptorSetLayout descriptorSetLayout;
    VkPipelineLayout pipelineLayout;
    VkPipeline graphicsPipeline;
    VkDescriptorPool descriptorPool;
    VkDescriptorSet descriptorSet;
    std::vector<VkCommandBuffer> graphicsCommandBuffers;
    bool windowResized = true;
};

#endif