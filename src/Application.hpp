#ifndef APPLICATION_CLASS
#define APPLICATION_CLASS

#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>
#include <vulkan/vulkan.h>
#include <vector>
#include <chrono>
#include <string>

#include "RenderTypes.hpp"
#include "FileSystem.h"
#define DEBUGGING_ENABLED

class Application{
private:
    static constexpr uint32_t SCREEN_WIDTH = 640;
    static constexpr uint32_t SCREEN_HEIGHT = 480;
    static constexpr float HALF_WIDTH = SCREEN_WIDTH / 2.0f;
    static constexpr float HALF_HEIGHT = SCREEN_HEIGHT / 2.0f;
    static constexpr float RATIO = static_cast<float>(SCREEN_WIDTH) / static_cast<float>(SCREEN_HEIGHT);
    static constexpr float LEFT = 0.0f;
    static constexpr float RIGHT = 640.0f;
    static constexpr float TOP = 480.0f;
    static constexpr float BOTTOM = 0.0f;
    static constexpr float Z_NEAR = 0.1f;
    static constexpr float Z_FAR = 1000.0f;
    static constexpr int MAX_FRAMES_IN_FLIGHT = 2;
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
    void createCommandPool();
    void createVertexBuffer();
    void createIndexBuffer();
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
    void createTextureImage();
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
    void createBuffer(
        VkDeviceSize size, 
        VkBufferUsageFlags usage, 
        VkMemoryPropertyFlags properties, 
        VkBuffer & buffer, 
        VkDeviceMemory & bufferMemory
    );
    void endSingleTimeCommands(VkCommandBuffer commandBuffer) const;
    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) const;
    VkCommandBuffer beginSingleTimeCommands() const;
    void transitionImageLayout(
        VkImage image, 
        VkFormat format, 
        VkImageLayout oldLayout, 
        VkImageLayout newLayout
    ) const;
    void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) const;
    void createSyncObjects();
    void createTextureSampler();
private:
    struct UniformBufferObject {
        float rotation[16];
        float transformation[16];
        float perspective[16];
    };

    //MTypes::Mesh m_viking_house_mesh;
    //MTypes::Mesh m_cube_mesh;
    std::vector<RenderTypes::Mesh> m_mesh_list;
    //float const halfWidth = 50.0f;
    //float const cubeFrontZ = 50.0f;
    //float const cubeBackZ = -50.0f;
  //  std::vector<MTypes::Vertex> const vertices = {
  //      //// front
  //      {{-halfWidth, -halfWidth, cubeFrontZ},{1.0f, 0.0f, 0.0f},{0.0f, 0.0f}},
  //      {{halfWidth, -halfWidth, cubeFrontZ},{0.0f, 1.0f, 0.0f},{1.0f, 0.0f}},
  //      {{halfWidth,  halfWidth,  cubeFrontZ},{0.0f, 0.0f, 1.0f},{1.0f, 1.0f}},
  //      {{-halfWidth,  halfWidth,  cubeFrontZ},{1.0f, 1.0f, 1.0f},{0.0f, 1.0f}},
  //      //// back
  //      {{-halfWidth, -halfWidth,  cubeBackZ},{0.5f, 0.0f, 0.5f},{1.0f, 0.0f}},
  //      {{halfWidth, -halfWidth,  cubeBackZ},{0.0f, 0.5f, 0.5f},{0.0f, 0.0f}},
  //      {{halfWidth,  halfWidth,  cubeBackZ},{0.0f, 0.5f, 0.5f},{0.0f, 1.0f}},
  //      {{-halfWidth,  halfWidth,  cubeBackZ},{0.5f, 0.0f, 0.5f},{1.0f, 1.0f}}
  //  };
  //  std::vector<uint32_t> const indices = { 
  //      // front
		//0, 1, 2,
		//2, 3, 0,
		//// right
		//1, 5, 6,
		//6, 2, 1,
		//// back
		//7, 6, 5,
		//5, 4, 7,
		//// left
		//4, 0, 3,
		//3, 7, 4,
		//// bottom
		//4, 5, 1,
		//1, 0, 4,
		//// top
		//3, 2, 6,
		//6, 7, 3
  //  };
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
    VkCommandPool commandPool;
    //VkVertexInputBindingDescription vertexBindingDescription;
    VkBuffer indexBuffer;
    VkDeviceMemory indexBufferMemory;
    VkDeviceMemory vertexBufferMemory;
    VkBuffer vertexBuffer;
    //std::vector<VkVertexInputAttributeDescription> vertexAttributeDescriptions;
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
    std::vector<VkFramebuffer> swapChainFrameBuffers;
    VkDescriptorSetLayout descriptorSetLayout;
    VkPipelineLayout pipelineLayout;
    VkPipeline graphicsPipeline;
    VkDescriptorPool descriptorPool;
    std::vector<VkDescriptorSet> descriptorSets;
    std::vector<VkCommandBuffer> graphicsCommandBuffers;
    bool windowResized = false;
    VkImage textureImage;
    VkDeviceMemory textureImageMemory;
    VkImageView textureImageView;
    VkSampler textureSampler;
    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;
    std::vector<VkFence> imagesInFlight;
    size_t currentFrame = 0;
    VkPhysicalDeviceFeatures deviceFeatures;
    bool framebufferResized = false;
};

#endif