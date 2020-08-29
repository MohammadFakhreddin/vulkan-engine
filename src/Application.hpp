#ifndef APPLICATION_CLASS
#define APPLICATION_CLASS

#include <vulkan/vulkan.h>
#include <SDL2/SDL.h>
#include <vector>
//#define GLM_FORCE_RADIANS
//#define GLM_FORCE_DEPTH_ZERO_TO_ONE
//#include <glm/ext.hpp>
#include <chrono>
#include <string>

#include "MatrixTemplate.h"

#define DEBUGGING_ENABLED

class Application{
private:
    static constexpr const char* DEBUG_LAYER = "VK_LAYER_LUNARG_standard_validation";
    static constexpr uint32_t SCREEN_WIDTH = 640;
    static constexpr uint32_t SCREEN_HEIGHT = 480;
    static constexpr float RATIO = static_cast<float>(SCREEN_WIDTH) / static_cast<float>(SCREEN_HEIGHT);
    //static constexpr float LEFT = -1.0f * RATIO;
    //static constexpr float RIGHT = 1.0f * RATIO;
    //static constexpr float TOP = 1.0f;
    //static constexpr float BOTTOM = -1.0f;
    static constexpr float Z_NEAR = 1000.0f;
    static constexpr float Z_FAR = 0.0f;
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
    void updateUniformBuffer(uint32_t currentImage);
    void createSwapChain();
    void createRenderPass();
    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
    void createImage(
        uint32_t width, 
        uint32_t height, 
        VkFormat format, 
        VkImageTiling tiling, 
        VkImageUsageFlags usage, 
        VkMemoryPropertyFlags properties, 
        VkImage& image, 
        VkDeviceMemory& imageMemory
    );
    void createImageViews();
    void createImageView(VkImageView * imageView, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);
    void createFramebuffers();
    void createGraphicsPipeline();
    void createDescriptorPool();
    void createDescriptorSet();
    void createCommandBuffers();
    void drawFrame();
    void onWindowSizeChanged();
    VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
    VkFormat findDepthFormat();
    bool hasStencilComponent(VkFormat format);
    void throwErrorAndExit(std::string error);
private:
    struct UniformBufferObject {
        float model[16];
        //float view[16];
        float proj[16];
        float camera[16];
    };
    struct Vertex {
        float pos[3];
        float color[3];
    };
    // Triangle
    // Setup vertices
    /*std::vector<Vertex> vertices = {
        { { -0.5f, -0.5f,  0.0f }, { 1.0f, 0.0f, 0.0f } },
        { { -0.5f,  0.5f,  0.0f }, { 0.0f, 1.0f, 0.0f } },
        { {  0.5f,  0.5f,  0.0f }, { 0.0f, 0.0f, 1.0f } },
        {{ 0.5f, -0.5f, 0.0f }, { 1.0f, 1.0f, 1.0f } },
    };
    std::vector<uint32_t> const indices = { 
        0, 1, 2, 2, 3, 0
    };*/
    float const halfWidth = 0.5f;
    float const cubeFrontZ = 0.5f;
    float const cubeBackZ = -0.5f;
    std::vector<Vertex> const vertices = {
        //// front
        {{-halfWidth, -halfWidth, cubeFrontZ},{1.0f, 0.0f, 0.0f}},
        {{halfWidth, -halfWidth, cubeFrontZ},{0.0f, 1.0f, 0.0f}},
        {{halfWidth,  halfWidth,  cubeFrontZ},{0.0f, 0.0f, 1.0f}},
        {{-halfWidth,  halfWidth,  cubeFrontZ},{1.0f, 1.0f, 1.0f}},
        // back
        {{-halfWidth, -halfWidth,  cubeBackZ},{1.0f, 0.0f, 1.0f}},
        {{halfWidth, -halfWidth,  cubeBackZ},{0.0f, 1.0f, 1.0f}},
        {{halfWidth,  halfWidth,  cubeBackZ},{0.0f, 1.0f, 1.0f}},
        {{-halfWidth,  halfWidth,  cubeBackZ},{1.0f, 0.0f, 1.0f}}
    };
    std::vector<uint32_t> const indices = { 
        // front
		0, 1, 2,
		2, 3, 0,
		// right
		1, 5, 6,
		6, 2, 1,
		// back
		7, 6, 5,
		5, 4, 7,
		// left
		4, 0, 3,
		3, 7, 4,
		// bottom
		4, 5, 1,
		1, 0, 4,
		// top
		3, 2, 6,
		6, 7, 3
    };
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
    // Multiple buffers may be running so we need to have buffer for each image
    std::vector<VkBuffer> uniformBuffers;
    std::vector<VkDeviceMemory> uniformBuffersMemory;
    VkExtent2D swapChainExtent;
    std::vector<VkImage> swapChainImages;
    VkImage depthImage;
    VkDeviceMemory depthImageMemory;
    //VkImage textureImage;
    VkFormat swapChainFormat;
    VkRenderPass renderPass;
    std::vector<VkImageView> swapChainImageViews;
    VkImageView depthImageView;
    //VkImageView textureImageView;
    std::vector<VkFramebuffer> swapChainFramebuffers;
    VkDescriptorSetLayout descriptorSetLayout;
    VkPipelineLayout pipelineLayout;
    VkPipeline graphicsPipeline;
    VkDescriptorPool descriptorPool;
    std::vector<VkDescriptorSet> descriptorSets;
    std::vector<VkCommandBuffer> graphicsCommandBuffers;
    bool windowResized = true;
};

#endif