#pragma once

#include "RenderTypes.hpp"
#include "RenderTypesFWD.hpp"
#include "engine/BedrockCommon.hpp"
#include "engine/asset_system/AssetTypes.hpp"

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

namespace MFA::AssetSystem
{
    class Texture;
    class Shader;
}

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
    std::shared_ptr<RT::ImageViewGroup> CreateImageView(
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
        RT::ImageViewGroup const & imageView
    );

    [[nodiscard]]
    VkCommandBuffer BeginSingleTimeCommand(VkDevice device, VkCommandPool const & commandPool);

    void EndAndSubmitSingleTimeCommand(
        VkDevice device,
        VkCommandPool const & commandPool,
        VkQueue const & queue,
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
    std::shared_ptr<RT::BufferAndMemory> CreateBuffer(
        VkDevice device,
        VkPhysicalDevice physicalDevice,
        VkDeviceSize size,
        VkBufferUsageFlags usage,
        VkMemoryPropertyFlags properties
    );

    void MapHostVisibleMemory(
        VkDevice device,
        VkDeviceMemory bufferMemory,
        size_t offset,
        size_t size,
        void ** outBufferData
    );

    void CopyDataToHostVisibleBuffer(
        VkDevice device,
        VkDeviceMemory bufferMemory,
        CBlob dataBlob
    );

    void UnMapHostVisibleMemory(
        VkDevice device,
        VkDeviceMemory bufferMemory
    );

    // TODO Too many parameters. Create struct instead
    [[nodiscard]]
    std::shared_ptr<RT::ImageGroup> CreateImage(
        VkDevice device,
        VkPhysicalDevice physical_device,
        uint32_t width,
        uint32_t height,
        uint32_t depth,
        uint8_t mipLevels,
        uint16_t sliceCount,
        VkFormat format,
        VkImageTiling tiling,
        VkImageUsageFlags usage,
        VkSampleCountFlagBits samplesCount,
        VkMemoryPropertyFlags properties,
        VkImageCreateFlags imageCreateFlags = 0,
        VkImageType imageType = VK_IMAGE_TYPE_2D
    );

    void DestroyImage(
        VkDevice device,
        RT::ImageGroup const & imageGroup
    );

    [[nodiscard]]
    std::shared_ptr<RT::GpuTexture> CreateTexture(
        AS::Texture const & cpuTexture,
        VkDevice device,
        VkPhysicalDevice physicalDevice,
        VkQueue graphicQueue,
        VkCommandPool commandPool
    );

    void DestroyTexture(VkDevice device, RT::GpuTexture & gpuTexture);

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
    std::shared_ptr<RT::SamplerGroup> CreateSampler(
        VkDevice device,
        RT::CreateSamplerParams const & params
    );

    void DestroySampler(VkDevice device, RT::SamplerGroup const & sampler);

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

    struct FindQueueFamilyResult
    {
        bool const isPresentQueueValid = false;
        uint32_t const presentQueueFamily = -1;

        bool const isGraphicQueueValid = false;
        uint32_t const graphicQueueFamily = -1;

        bool const isComputeQueueValid = false;
        uint32_t const computeQueueFamily = -1;
    };

    [[nodiscard]]
    FindQueueFamilyResult FindQueueFamilies(
        VkPhysicalDevice physicalDevice,
        VkSurfaceKHR windowSurface
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
    std::shared_ptr<RT::SwapChainGroup> CreateSwapChain(
        VkDevice device,
        VkPhysicalDevice physicalDevice,
        VkSurfaceKHR windowSurface,
        VkSurfaceCapabilitiesKHR surfaceCapabilities,
        VkSwapchainKHR oldSwapChain = VkSwapchainKHR{}
    );

    void DestroySwapChain(VkDevice device, RT::SwapChainGroup const & swapChainGroup);

    [[nodiscard]]
    std::shared_ptr<RT::DepthImageGroup> CreateDepthImage(
        VkPhysicalDevice physicalDevice,
        VkDevice device,
        VkExtent2D imageExtent,
        VkFormat depthFormat,
        RT::CreateDepthImageOptions const & options
    );

    [[nodiscard]]
    std::shared_ptr<RT::ColorImageGroup> CreateColorImage(
        VkPhysicalDevice physicalDevice,
        VkDevice device,
        VkExtent2D const & imageExtent,
        VkFormat imageFormat,
        RT::CreateColorImageOptions const & options
    );

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
    std::shared_ptr<RT::GpuShader> CreateShader(
        VkDevice device,
        std::shared_ptr<AS::Shader> const & cpuShader
    );

    void DestroyShader(VkDevice device, RT::GpuShader const & gpuShader);

    VkPipelineLayout CreatePipelineLayout(
        VkDevice device,
        uint32_t setLayoutCount,
        const VkDescriptorSetLayout * pSetLayouts,
        uint32_t pushConstantRangeCount,
        const VkPushConstantRange * pPushConstantRanges
    );

    [[nodiscard]]
    std::shared_ptr<RT::PipelineGroup> CreateGraphicPipeline(
        VkDevice device,
        uint8_t shaderStagesCount,
        RT::GpuShader const ** shaderStages,
        uint32_t vertexBindingDescriptionCount,
        VkVertexInputBindingDescription const * vertexBindingDescriptionData,
        uint32_t attributeDescriptionCount,
        VkVertexInputAttributeDescription * attributeDescriptionData,
        VkExtent2D swapChainExtent,
        VkRenderPass renderPass,
        VkPipelineLayout pipelineLayout,
        RT::CreateGraphicPipelineOptions const & options
    );

    [[nodiscard]]
    std::shared_ptr<RT::PipelineGroup> CreateComputePipeline(
        VkDevice device,
        RT::GpuShader const & shaderStage,
        VkPipelineLayout pipelineLayout
    );

    void AssignViewportAndScissorToCommandBuffer(
        VkExtent2D const & extent2D,
        VkCommandBuffer commandBuffer
    );

    void DestroyPipelineGroup(VkDevice device, RT::PipelineGroup & pipelineGroup);

    [[nodiscard]]
    VkShaderStageFlagBits ConvertAssetShaderStageToGpu(AssetSystem::ShaderStage stage);

    //-----------------------------------------Buffer-------------------------------------------------

    //std::shared_ptr<RT::BufferAndMemory> CreateVertexBuffer(
    //    VkDevice device,
    //    VkPhysicalDevice physicalDevice,
    //    VkCommandBuffer commandBuffer,
    //    CBlob const & verticesBlob,
    //    RT::BufferAndMemory  const & stageBuffer
    //);

    //std::shared_ptr<RT::BufferAndMemory> CreateIndexBuffer(
    //    VkDevice device,
    //    VkPhysicalDevice physicalDevice,
    //    VkCommandBuffer commandBuffer,
    //    CBlob const & indicesBlob,
    //    RT::BufferAndMemory  const & stageBuffer
    //);

    //std::shared_ptr<RT::BufferAndMemory> CreateStageBuffer(
    //    VkDevice device,
    //    VkPhysicalDevice physicalDevice,
    //    VkDeviceSize bufferSize
    //);

    //[[nodiscard]]
    //std::vector<std::shared_ptr<RT::BufferAndMemory>> CreateMultipleBufferAndMemory(
    //    VkDevice device,
    //    VkPhysicalDevice physicalDevice,
    //    uint32_t buffersCount,
    //    VkDeviceSize buffersSize,
    //    VkBufferUsageFlagBits memoryUsageFlags,
    //    VkMemoryPropertyFlags memoryPropertyFlags
    //);

    void UpdateLocalBuffer(
        VkCommandBuffer commandBuffer,
        RT::BufferAndMemory const & buffer,
        RT::BufferAndMemory const & stagingBuffer
    );

    void UpdateHostVisibleBuffer(
        VkDevice device,
        RT::BufferAndMemory const & bufferGroup,
        CBlob const & data
    );

    void DestroyBuffer(
        VkDevice device,
        const RT::BufferAndMemory & bufferGroup
    );

    //-----------------------------------------Descriptors-------------------------------------------------

    [[nodiscard]]
    std::shared_ptr<RT::DescriptorSetLayoutGroup> CreateDescriptorSetLayout(
        VkDevice device,
        uint8_t bindings_count,
        VkDescriptorSetLayoutBinding * bindings
    );

    void DestroyDescriptorSetLayout(
        VkDevice device,
        VkDescriptorSetLayout descriptor_set_layout
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

    void UpdateDescriptorSets(
        VkDevice device,
        uint8_t descriptorSetCount,
        VkDescriptorSet * descriptorSets,
        uint8_t schemasCount,
        VkWriteDescriptorSet * schemas
    );

    //-----------------------------------------CommandBuffer-------------------------------------------------

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

    //-----------------------------------------Semaphore-------------------------------------------------

    [[nodiscard]]
    std::vector<VkSemaphore> CreateSemaphores(
        VkDevice device,
        uint32_t count
    );

    void DestroySemaphored(
        VkDevice device,
        std::vector<VkSemaphore> const & semaphores
    );

    //------------------------------------------Fence-------------------------------------------------

    [[nodiscard]]
    std::vector<VkFence> CreateFence(
        VkDevice device,
        uint32_t count
    );

    void DestroyFence(
        VkDevice device,
        std::vector<VkFence> const & fences
    );

    //-------------------------------------------------------------------------------------------------

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
        RT::BufferAndMemory const & vertexBuffer,
        uint32_t firstBinding = 0,
        VkDeviceSize offset = 0
    );

    void BindIndexBuffer(
        VkCommandBuffer commandBuffer,
        RT::BufferAndMemory const & indexBuffer,
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

    void ExecuteCommandBuffer(
        VkCommandBuffer primaryCommandBuffer,
        uint32_t subCommandBuffersCount,
        const VkCommandBuffer * subCommandBuffers
    );

    void PipelineBarrier(
        VkCommandBuffer commandBuffer,
        VkPipelineStageFlags sourceStageMask,
        VkPipelineStageFlags destinationStateMask,
        uint32_t imageMemoryBarrierCount,
        VkImageMemoryBarrier const * imageMemoryBarriers
    );

    void PipelineBarrier(
        VkCommandBuffer commandBuffer,
        VkPipelineStageFlags sourceStageMask,
        VkPipelineStageFlags destinationStateMask,
        uint32_t barrierCount,
        VkBufferMemoryBarrier const * bufferMemoryBarrier
    );

    void EndCommandBuffer(VkCommandBuffer commandBuffer);

    void SubmitQueues(
        VkQueue queue,
        uint32_t submitCount,
        const VkSubmitInfo * submitInfos,
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

    void Dispatch(
        VkCommandBuffer commandBuffer,
        uint32_t groupCountX,
        uint32_t groupCountY,
        uint32_t groupCountZ
    );

}

namespace MFA
{

    namespace RB = RenderBackend;

}
