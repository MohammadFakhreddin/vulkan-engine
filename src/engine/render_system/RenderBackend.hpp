#pragma once

#include "RenderTypesFWD.hpp"
#include "engine/BedrockCommon.hpp"
#include "engine/FoundationAsset.hpp"

#ifdef __ANDROID__
#include "vulkan_wrapper.h"
#include <android_native_app_glue.h>
#else
#include <vulkan/vulkan.h>
#endif
#ifdef CreateWindow
#undef CreateWindow
#endif

#ifdef __DESKTOP__
#include "libs/sdl/SDL.hpp"
#endif

// TODO Write description for all functions for learning purpose
// Note: Not all functions can be called from outside
// TODO Remove functions that are not usable from outside
namespace MFA::RenderBackend
{

    using ScreenWidth = Platforms::ScreenSize;
    using ScreenHeight = Platforms::ScreenSize;

    // Vulkan functions
#ifdef __DESKTOP__
    [[nodiscard]]
    MSDL::SDL_Window * CreateWindow(ScreenWidth screenWidth, ScreenHeight screenHeight);

    void DestroyWindow(MSDL::SDL_Window * window);
#endif

#ifdef __DESKTOP__
    [[nodiscard]]
    VkSurfaceKHR CreateWindowSurface(MSDL::SDL_Window * window, VkInstance_T * instance);
#elif defined(__ANDROID__)
    [[nodiscard]]
    VkSurfaceKHR CreateWindowSurface(ANativeWindow * window, VkInstance_T * instance);
#elif defined(__IOS__)
    [[nodiscard]]
    VkSurfaceKHR CreateWindowSurface(void * view, VkInstance_T * instance);
#else
#error Unhandled os detected
#endif

    void DestroyWindowSurface(VkInstance instance, VkSurfaceKHR surface);


    [[nodiscard]]
    VkExtent2D ChooseSwapChainExtent(
        VkSurfaceCapabilitiesKHR const & surfaceCapabilities,
        ScreenWidth screenWidth,
        ScreenHeight screenHeight
    );

    [[nodiscard]]
    VkPresentModeKHR ChoosePresentMode(uint8_t presentModesCount, VkPresentModeKHR const * present_modes);

    [[nodiscard]]
    VkSurfaceFormatKHR ChooseSurfaceFormat(uint8_t availableFormatsCount, VkSurfaceFormatKHR const * availableFormats);

#ifdef __DESKTOP__
    [[nodiscard]]
    VkInstance CreateInstance(char const * applicationName, MSDL::SDL_Window * window);
#elif defined(__ANDROID__) || defined(__IOS__)
    [[nodiscard]]
    VkInstance CreateInstance(char const * applicationName);
#elif defined(__IOS__)
#else
#error Os not handled
#endif

    void DestroyInstance(VkInstance instance);

    [[nodiscard]]
    VkDebugReportCallbackEXT CreateDebugCallback(
        VkInstance vkInstance,
        PFN_vkDebugReportCallbackEXT const & debugCallback
    );

    void DestroyDebugReportCallback(
        VkInstance instance,
        VkDebugReportCallbackEXT const & reportCallbackExt
    );

    [[nodiscard]]
    uint32_t FindMemoryType(
        VkPhysicalDevice * physicalDevice,
        uint32_t typeFilter,
        VkMemoryPropertyFlags propertyFlags
    );

    [[nodiscard]]
    VkImageView CreateImageView(
        VkDevice device,
        VkImage const & image,
        VkFormat format,
        VkImageAspectFlags aspectFlags,
        uint32_t mipmapCount,
        uint32_t layerCount,
        VkImageViewType viewType
    );

    void DestroyImageView(
        VkDevice device,
        VkImageView imageView
    );

    [[nodiscard]]
    VkCommandBuffer BeginSingleTimeCommand(VkDevice device, VkCommandPool const & command_pool);

    void EndAndSubmitSingleTimeCommand(
        VkDevice device,
        VkCommandPool const & commandPool,
        VkQueue const & graphicQueue,
        VkCommandBuffer const & commandBuffer
    );

    [[nodiscard]]
    VkFormat FindDepthFormat(VkPhysicalDevice physical_device);

    [[nodiscard]]
    VkFormat FindSupportedFormat(
        VkPhysicalDevice physical_device,
        uint8_t candidates_count,
        VkFormat * candidates,
        VkImageTiling tiling,
        VkFormatFeatureFlags features
    );

    void TransferImageLayout(
        VkDevice device,
        VkQueue graphicQueue,
        VkCommandPool commandPool,
        VkImage image,
        VkImageLayout oldLayout,
        VkImageLayout newLayout,
        uint32_t levelCount,
        uint32_t layerCount
    );

    [[nodiscard]]
    RT::BufferGroup CreateBuffer(
        VkDevice device,
        VkPhysicalDevice physicalDevice,
        VkDeviceSize size,
        VkBufferUsageFlags usage,
        VkMemoryPropertyFlags properties
    );

    void MapDataToBuffer(
        VkDevice device,
        VkDeviceMemory bufferMemory,
        CBlob dataBlob
    );

    void CopyBuffer(
        VkDevice device,
        VkCommandPool commandPool,
        VkQueue graphicQueue,
        VkBuffer sourceBuffer,
        VkBuffer destinationBuffer,
        VkDeviceSize size
    );

    void DestroyBuffer(
        VkDevice device,
        RT::BufferGroup & bufferGroup
    );

    [[nodiscard]]
    RT::ImageGroup CreateImage(
        VkDevice device,
        VkPhysicalDevice physical_device,
        uint32_t width,
        uint32_t height,
        uint32_t depth,
        uint8_t mip_levels,
        uint16_t slice_count,
        VkFormat format,
        VkImageTiling tiling,
        VkImageUsageFlags usage,
        VkSampleCountFlagBits samplesCount,
        VkMemoryPropertyFlags properties,
        VkImageCreateFlags imageCreateFlags = 0
    );

    void DestroyImage(
        VkDevice device,
        RT::ImageGroup const & imageGroup
    );

    [[nodiscard]]
    RT::GpuTexture CreateTexture(
        AS::Texture & cpuTexture,
        VkDevice device,
        VkPhysicalDevice physicalDevice,
        VkQueue graphicQueue,
        VkCommandPool commandPool
    );

    bool DestroyTexture(VkDevice device, RT::GpuTexture & gpuTexture);

    [[nodiscard]]
    VkFormat ConvertCpuTextureFormatToGpu(AS::TextureFormat cpuFormat);

    void CopyBufferToImage(
        VkDevice device,
        VkCommandPool commandPool,
        VkBuffer buffer,
        VkImage image,
        VkQueue graphicQueue,
        AS::Texture const & cpuTexture
    );

    [[nodiscard]]
    RT::LogicalDevice CreateLogicalDevice(
        VkPhysicalDevice physicalDevice,
        uint32_t graphicsQueueFamily,
        uint32_t presentQueueFamily,
        VkPhysicalDeviceFeatures const & enabledPhysicalDeviceFeatures
    );

    void DestroyLogicalDevice(RT::LogicalDevice const & logicalDevice);

    [[nodiscard]]
    VkQueue GetQueueByFamilyIndex(
        VkDevice device,
        uint32_t queueFamilyIndex
    );

    [[nodiscard]]
    VkSampler CreateSampler(
        VkDevice device,
        RT::CreateSamplerParams const & params
    );

    void DestroySampler(VkDevice device, VkSampler sampler);

    [[nodiscard]]
    VkDebugReportCallbackEXT SetDebugCallback(
        VkInstance instance,
        PFN_vkDebugReportCallbackEXT const & callback
    );

    // TODO Might need to ask features from outside instead
    struct FindPhysicalDeviceResult
    {
        VkPhysicalDevice physicalDevice = nullptr;
        VkPhysicalDeviceFeatures physicalDeviceFeatures{};
        VkSampleCountFlagBits maxSampleCount{};
        VkPhysicalDeviceProperties physicalDeviceProperties{};
    };
    [[nodiscard]]
    FindPhysicalDeviceResult FindPhysicalDevice(VkInstance instance);

    [[nodiscard]]
    bool CheckSwapChainSupport(VkPhysicalDevice physical_device);

    struct FindPresentAndGraphicQueueFamilyResult
    {
        uint32_t present_queue_family;
        uint32_t graphic_queue_family;
    };
    [[nodiscard]]
    FindPresentAndGraphicQueueFamilyResult FindPresentAndGraphicQueueFamily(
        VkPhysicalDevice physical_device,
        VkSurfaceKHR window_surface
    );

    [[nodiscard]]
    VkCommandPool CreateCommandPool(VkDevice device, uint32_t queue_family_index);

    void DestroyCommandPool(VkDevice device, VkCommandPool commandPool);

    [[nodiscard]]
    VkSurfaceCapabilitiesKHR GetSurfaceCapabilities(
        VkPhysicalDevice physicalDevice,
        VkSurfaceKHR windowSurface
    );



    [[nodiscard]]
    RT::SwapChainGroup CreateSwapChain(
        VkDevice device,
        VkPhysicalDevice physicalDevice,
        VkSurfaceKHR windowSurface,
        VkSurfaceCapabilitiesKHR surfaceCapabilities,
        VkSwapchainKHR oldSwapChain = VkSwapchainKHR{}
    );

    void DestroySwapChain(VkDevice device, RT::SwapChainGroup const & swapChainGroup);

    [[nodiscard]]
    RT::DepthImageGroup CreateDepthImage(
        VkPhysicalDevice physicalDevice,
        VkDevice device,
        VkExtent2D imageExtent,
        RT::CreateDepthImageOptions const & options
    );

    void DestroyDepthImage(VkDevice device, RT::DepthImageGroup const & depthImageGroup);

    [[nodiscard]]
    RT::ColorImageGroup CreateColorImage(
        VkPhysicalDevice physicalDevice,
        VkDevice device,
        VkExtent2D const & imageExtent,
        VkFormat imageFormat,
        RT::CreateColorImageOptions const & options
    );

    void DestroyColorImage(VkDevice device, RT::ColorImageGroup const & colorImageGroup);

    // TODO Ask for options
    [[nodiscard]]
    VkRenderPass CreateRenderPass(
        VkDevice device,
        VkAttachmentDescription * attachments,
        uint32_t attachmentsCount,
        VkSubpassDescription * subPasses,
        uint32_t subPassesCount,
        VkSubpassDependency * dependencies,
        uint32_t dependenciesCount
    );

    void DestroyRenderPass(VkDevice device, VkRenderPass renderPass);

    void BeginRenderPass(VkCommandBuffer commandBuffer, VkRenderPassBeginInfo const & renderPassBeginInfo);

    void EndRenderPass(VkCommandBuffer commandBuffer);

    [[nodiscard]]
    VkFramebuffer CreateFrameBuffers(
        VkDevice device,
        VkRenderPass renderPass,
        VkImageView const * attachments,
        uint32_t attachmentsCount,
        VkExtent2D swapChainExtent,
        uint32_t layersCount
    );

    void DestroyFrameBuffers(
        VkDevice device,
        uint32_t frameBuffersCount,
        VkFramebuffer * frameBuffers
    );

    [[nodiscard]]
    RT::GpuShader CreateShader(VkDevice device, AS::Shader const & cpuShader);

    bool DestroyShader(VkDevice device, RT::GpuShader & gpuShader);

    // Note Shaders can be removed after creating graphic pipeline
    [[nodiscard]]
    RT::PipelineGroup CreatePipelineGroup(
        VkDevice device,
        uint8_t shaderStagesCount,
        RT::GpuShader const ** shaderStages,
        VkVertexInputBindingDescription vertexBindingDescription,
        uint32_t attributeDescriptionCount,
        VkVertexInputAttributeDescription * attributeDescriptionData,
        VkExtent2D swapChainExtent,
        VkRenderPass renderPass,
        uint32_t descriptorSetLayoutCount,
        VkDescriptorSetLayout * descriptorSetLayouts,
        RT::CreateGraphicPipelineOptions const & options
    );

    void AssignViewportAndScissorToCommandBuffer(
        VkExtent2D const & extent2D,
        VkCommandBuffer commandBuffer
    );

    void DestroyPipelineGroup(VkDevice device, RT::PipelineGroup & graphicPipelineGroup);

    [[nodiscard]]
    VkDescriptorSetLayout CreateDescriptorSetLayout(
        VkDevice device,
        uint8_t bindings_count,
        VkDescriptorSetLayoutBinding * bindings
    );

    void DestroyDescriptorSetLayout(
        VkDevice device,
        VkDescriptorSetLayout descriptor_set_layout
    );

    [[nodiscard]]
    VkShaderStageFlagBits ConvertAssetShaderStageToGpu(AssetSystem::ShaderStage stage);

    [[nodiscard]]
    RT::BufferGroup CreateVertexBuffer(
        VkDevice device,
        VkPhysicalDevice physicalDevice,
        VkCommandPool commandPool,
        VkQueue graphicQueue,
        CBlob verticesBlob
    );

    void DestroyVertexBuffer(
        VkDevice device,
        RT::BufferGroup & vertexBufferGroup
    );

    [[nodiscard]]
    RT::BufferGroup CreateIndexBuffer(
        VkDevice device,
        VkPhysicalDevice physicalDevice,
        VkCommandPool commandPool,
        VkQueue graphicQueue,
        CBlob indicesBlob
    );

    void DestroyIndexBuffer(
        VkDevice device,
        RT::BufferGroup & indexBufferGroup
    );

    void CreateUniformBuffer(
        VkDevice device,
        VkPhysicalDevice physicalDevice,
        uint32_t buffersCount,
        VkDeviceSize buffersSize,
        RT::BufferGroup * outUniformBuffers
    );

    void UpdateBufferGroup(
        VkDevice device,
        RT::BufferGroup const & bufferGroup,
        CBlob data
    );

    void DestroyBufferGroup(
        VkDevice device,
        RT::BufferGroup & bufferGroup
    );

    void CreateStorageBuffer(
        VkDevice device,
        VkPhysicalDevice physicalDevice,
        uint32_t buffersCount,
        VkDeviceSize buffersSize,
        RT::BufferGroup * outStorageBuffer
    );


    [[nodiscard]]
    VkDescriptorPool CreateDescriptorPool(
        VkDevice device,
        uint32_t maxSets
    );

    void DestroyDescriptorPool(
        VkDevice device,
        VkDescriptorPool pool
    );

    // Descriptor sets gets destroyed automatically when descriptor pool is destroyed
    [[nodiscard]]
    RT::DescriptorSetGroup CreateDescriptorSet(
        VkDevice device,
        VkDescriptorPool descriptorPool,
        VkDescriptorSetLayout descriptorSetLayout,
        uint32_t descriptorSetCount,
        uint8_t schemasCount = 0,
        VkWriteDescriptorSet * schemas = nullptr
    );

    // TODO Consider creating an easier interface
    void UpdateDescriptorSets(
        VkDevice device,
        uint8_t descriptorSetCount,
        VkDescriptorSet * descriptorSets,
        uint8_t schemasCount,
        VkWriteDescriptorSet * schemas
    );

    /***
     *  CreateCommandBuffer (For pipeline I guess)
     *  Allocate graphics command buffers
     *  There is no need for destroying it
     ***/
    std::vector<VkCommandBuffer> CreateCommandBuffers(
        VkDevice device,
        uint32_t count,
        VkCommandPool commandPool
    );

    void DestroyCommandBuffers(
        VkDevice device,
        VkCommandPool commandPool,
        uint32_t commandBuffersCount,
        VkCommandBuffer * commandBuffers
    );

    [[nodiscard]]
    RT::SyncObjects CreateSyncObjects(
        VkDevice device,
        uint8_t maxFramesInFlight,
        uint32_t swapChainImagesCount
    );

    void DestroySyncObjects(VkDevice device, RT::SyncObjects const & syncObjects);

    void DeviceWaitIdle(VkDevice device);

    void WaitForFence(VkDevice device, VkFence inFlightFence);

    VkResult AcquireNextImage(
        VkDevice device,
        VkSemaphore imageAvailabilitySemaphore,
        RT::SwapChainGroup const & swapChainGroup,
        uint32_t & outImageIndex
    );

    void BindVertexBuffer(
        VkCommandBuffer commandBuffer,
        RT::BufferGroup const & vertexBuffer,
        VkDeviceSize offset = 0
    );

    void BindIndexBuffer(
        VkCommandBuffer commandBuffer,
        RT::BufferGroup const & indexBuffer,
        VkDeviceSize offset = 0,
        VkIndexType indexType = VK_INDEX_TYPE_UINT32
    );

    void DrawIndexed(
        VkCommandBuffer commandBuffer,
        uint32_t indicesCount,
        uint32_t instanceCount = 1,
        uint32_t firstIndex = 0,
        uint32_t vertexOffset = 0,
        uint32_t firstInstance = 0
    );

    void SetScissor(VkCommandBuffer commandBuffer, VkRect2D const & scissor);

    void SetViewport(VkCommandBuffer commandBuffer, VkViewport const & viewport);

    void PushConstants(
        VkCommandBuffer command_buffer,
        VkPipelineLayout pipeline_layout,
        AssetSystem::ShaderStage shader_stage,
        uint32_t offset,
        CBlob data
    );

    void PushConstants(
        VkCommandBuffer command_buffer,
        VkPipelineLayout pipeline_layout,
        VkShaderStageFlags shader_stage,
        uint32_t offset,
        CBlob data
    );

    void UpdateDescriptorSetsBasic(
        VkDevice device,
        uint8_t descriptorSetsCount,
        VkDescriptorSet * descriptorSets,
        VkDescriptorBufferInfo const & bufferInfo,
        uint32_t imageInfoCount,
        VkDescriptorImageInfo const * imageInfos
    );

    void UpdateDescriptorSets(
        VkDevice device,
        uint32_t descriptorWritesCount,
        VkWriteDescriptorSet * descriptorWrites
    );

    uint32_t ComputeSwapChainImagesCount(VkSurfaceCapabilitiesKHR surfaceCapabilities);

    void BeginCommandBuffer(
        VkCommandBuffer commandBuffer,
        VkCommandBufferBeginInfo const & beginInfo
    );

    void PipelineBarrier(
        VkCommandBuffer commandBuffer,
        VkPipelineStageFlags sourceStageMask,
        VkPipelineStageFlags destinationStateMask,
        VkImageMemoryBarrier const & memoryBarrier
    );

    void EndCommandBuffer(VkCommandBuffer commandBuffer);

    void SubmitQueues(
        VkQueue queue,
        uint32_t submitCount,
        VkSubmitInfo * submitInfos,
        VkFence fence
    );

    void ResetFences(VkDevice device, uint32_t fencesCount, VkFence const * fences);

    VkSampleCountFlagBits ComputeMaxUsableSampleCount(VkPhysicalDevice physicalDevice);

    VkSampleCountFlagBits ComputeMaxUsableSampleCount(VkPhysicalDeviceProperties deviceProperties);

    void CopyImage(
        VkCommandBuffer commandBuffer,
        VkImage sourceImage,
        VkImage destinationImage,
        VkImageCopy const & copyRegion
    );

    void WaitForQueue(VkQueue queue);

    VkQueryPool CreateQueryPool(VkDevice device, VkQueryPoolCreateInfo const & createInfo);

    void DestroyQueryPool(VkDevice device, VkQueryPool queryPool);

    void BeginQuery(VkCommandBuffer commandBuffer, VkQueryPool queryPool, uint32_t queryId);

    void EndQuery(VkCommandBuffer commandBuffer, VkQueryPool queryPool, uint32_t queryId);

    void GetQueryPoolResult(
        VkDevice device,
        VkQueryPool queryPool,
        uint32_t samplesCount,
        uint64_t * outSamplesData,
        uint32_t samplesOffset = 0
    );

    void ResetQueryPool(
        VkCommandBuffer commandBuffer,
        VkQueryPool queryPool,
        uint32_t queryCount,
        uint32_t firstQueryIndex = 0
    );

}

namespace MFA
{

    namespace RB = RenderBackend;

}
