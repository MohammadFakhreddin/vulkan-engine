#pragma once

#include "engine/BedrockCommon.hpp"
#include "engine/FoundationAsset.hpp"

#include <functional>
#ifdef __ANDROID__
#include "vulkan_wrapper.h"
#include <android_native_app_glue.h>
#else
#include <vulkan/vulkan.h>
#endif
#ifdef CreateWindow
#undef CreateWindow
#endif

#if defined(__DESKTOP__) || defined(__IOS__)
#define MFA_VK_VALID(vkVariable) vkVariable != nullptr
#define MFA_VK_INVALID(vkVariable) vkVariable == nullptr
#define MFA_VK_VALID_ASSERT(vkVariable) MFA_ASSERT(MFA_VK_VALID(vkVariable))
#define MFA_VK_INVALID_ASSERT(vkVariable) MFA_ASSERT(MFA_VK_INVALID(vkVariable))
#define MFA_VK_MAKE_NULL(vkVariable) vkVariable = nullptr
#elif defined(__ANDROID__)
#define MFA_VK_VALID(vkVariable) vkVariable != 0
#define MFA_VK_INVALID(vkVariable) vkVariable == 0
#define MFA_VK_VALID_ASSERT(vkVariable) MFA_ASSERT(MFA_VK_VALID(vkVariable))
#define MFA_VK_INVALID_ASSERT(vkVariable) MFA_ASSERT(MFA_VK_INVALID(vkVariable))
#define MFA_VK_MAKE_NULL(vkVariable) vkVariable = 0
#else
#error Unhandled platform
#endif

#ifdef __DESKTOP__
struct SDL_Window;
#endif

// TODO Write description for all functions for learning purpose
// Note: Not all functions can be called from outside
// TODO Remove functions that are not usable from outside
namespace MFA::RenderBackend {

struct CreateGraphicPipelineOptions;

using ScreenWidth = Platforms::ScreenSize;
using ScreenHeight = Platforms::ScreenSize;
using CpuTexture = AssetSystem::Texture;
using CpuShader = AssetSystem::Shader;

// Vulkan functions
#ifdef __DESKTOP__
[[nodiscard]]
SDL_Window * CreateWindow(ScreenWidth screenWidth, ScreenHeight screenHeight);

void DestroyWindow(SDL_Window * window);
#endif

#ifdef __DESKTOP__
[[nodiscard]]
VkSurfaceKHR CreateWindowSurface(SDL_Window * window, VkInstance_T * instance);
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
VkInstance CreateInstance(char const * applicationName, SDL_Window * window);
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
uint32_t FindMemoryType (
    VkPhysicalDevice * physicalDevice,
    uint32_t typeFilter, 
    VkMemoryPropertyFlags propertyFlags
);

[[nodiscard]]
VkImageView CreateImageView (
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

struct BufferGroup {
    VkBuffer buffer {};
    VkDeviceMemory memory {};
    [[nodiscard]]
    bool isValid() const noexcept {
        return MFA_VK_VALID(buffer) && MFA_VK_VALID(memory);
    }
    void revoke() {
        MFA_VK_MAKE_NULL(buffer);
        MFA_VK_MAKE_NULL(memory);
    }
};
[[nodiscard]]
BufferGroup CreateBuffer(
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
    BufferGroup & bufferGroup
);

struct ImageGroup {
    VkImage image {};
    VkDeviceMemory memory {};
    [[nodiscard]]
    bool isValid() const noexcept {
        return MFA_VK_VALID(image) && MFA_VK_VALID(memory);
    }
    void revoke() {
        MFA_VK_MAKE_NULL(image);
        MFA_VK_MAKE_NULL(memory);
    }
};

[[nodiscard]]
ImageGroup CreateImage(
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
    VkMemoryPropertyFlags properties,
    VkImageCreateFlags imageCreateFlags = 0
);

void DestroyImage(
    VkDevice device,
    ImageGroup const & image_group
);

class GpuTexture;

[[nodiscard]]
GpuTexture CreateTexture(
    CpuTexture & cpuTexture,
    VkDevice device,
    VkPhysicalDevice physicalDevice,
    VkQueue graphicQueue,
    VkCommandPool commandPool
);

bool DestroyTexture(VkDevice device, GpuTexture & gpuTexture);

// TODO It needs handle system // TODO Might be moved to a new class called render_types
class GpuTexture {
friend GpuTexture CreateTexture(
    CpuTexture & cpuTexture,
    VkDevice device,
    VkPhysicalDevice physicalDevice,
    VkQueue graphicQueue,
    VkCommandPool commandPool
);
friend bool DestroyTexture(VkDevice device, GpuTexture & gpuTexture);
public:
    [[nodiscard]]
    CpuTexture const * cpuTexture() const {return &mCpuTexture;}
    [[nodiscard]]
    CpuTexture * cpuTexture() {return &mCpuTexture;}
    [[nodiscard]]
    bool isValid () const {
        if (mCpuTexture.isValid() == false) {
            return false;
        }
        if (mImageGroup.isValid() == false) {
            return false;
        }
        return MFA_VK_VALID(mImageView);
    }
    void revoke() {
        mImageGroup.revoke();
        MFA_VK_MAKE_NULL(mImageView);
    }
    [[nodiscard]]
    VkImage const & image() const {return mImageGroup.image;}
    [[nodiscard]]
    VkImageView image_view() const {return mImageView;}
private:
    ImageGroup mImageGroup {};
    VkImageView mImageView {};
    CpuTexture mCpuTexture {};
};

[[nodiscard]]
VkFormat ConvertCpuTextureFormatToGpu(AssetSystem::TextureFormat cpuFormat);

void CopyBufferToImage(
    VkDevice device,
    VkCommandPool commandPool,
    VkBuffer buffer,
    VkImage image,
    VkQueue graphicQueue,
    CpuTexture const & cpuTexture
);

struct LogicalDevice {
    VkDevice device;
    VkPhysicalDeviceMemoryProperties physical_memory_properties;
};
[[nodiscard]]
LogicalDevice CreateLogicalDevice(
    VkPhysicalDevice physicalDevice,
    uint32_t graphicsQueueFamily,
    uint32_t presentQueueFamily,
    VkPhysicalDeviceFeatures const & enabledPhysicalDeviceFeatures
);

void DestroyLogicalDevice(LogicalDevice const & logical_device);

[[nodiscard]]
VkQueue GetQueueByFamilyIndex(
    VkDevice device,
    uint32_t queue_family_index
);

struct CreateSamplerParams {
    float min_lod = 0;  // Level of detail
    float max_lod = 1;
    bool anisotropy_enabled = true;
    float max_anisotropy = 16.0f;
};
[[nodiscard]]
VkSampler CreateSampler(
    VkDevice device, 
    CreateSamplerParams const & params = {}
);

void DestroySampler(VkDevice device, VkSampler sampler);

[[nodiscard]]
VkDebugReportCallbackEXT SetDebugCallback(
    VkInstance instance,
    PFN_vkDebugReportCallbackEXT const & callback
);

// TODO Might need to ask features from outside instead
struct FindPhysicalDeviceResult {
    VkPhysicalDevice physicalDevice = nullptr;
    VkPhysicalDeviceFeatures physicalDeviceFeatures {};
};
[[nodiscard]]
FindPhysicalDeviceResult FindPhysicalDevice(VkInstance vk_instance);

[[nodiscard]]
bool CheckSwapChainSupport(VkPhysicalDevice physical_device);

struct FindPresentAndGraphicQueueFamilyResult {
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

struct SwapChainGroup {
    VkSwapchainKHR swapChain {};
    VkFormat swapChainFormat {};
    std::vector<VkImage> swapChainImages {};
    std::vector<VkImageView> swapChainImageViews {};
};

[[nodiscard]]
SwapChainGroup CreateSwapChain(
    VkDevice device,
    VkPhysicalDevice physicalDevice, 
    VkSurfaceKHR windowSurface,
    VkSurfaceCapabilitiesKHR surfaceCapabilities,
    VkSwapchainKHR oldSwapChain = VkSwapchainKHR {}
);

void DestroySwapChain(VkDevice device, SwapChainGroup const & swapChainGroup);

// TODO Maybe we should move this render frontend
struct DepthImageGroup {
    ImageGroup imageGroup {};
    VkImageView imageView {};
    VkFormat imageFormat;
};

struct CreateDepthImageOptions {
    uint16_t layerCount = 1;
    VkImageUsageFlags usageFlags = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    VkImageViewType viewType = VK_IMAGE_VIEW_TYPE_2D;
    VkImageCreateFlags imageCreateFlags = 0;
};
[[nodiscard]]
DepthImageGroup CreateDepthImage(
    VkPhysicalDevice physicalDevice,
    VkDevice device,
    VkExtent2D imageExtent,
    CreateDepthImageOptions const & options
);

void DestroyDepthImage(VkDevice device, DepthImageGroup const & depthImageGroup);


struct ColorImageGroup {
    ImageGroup imageGroup {};
    VkImageView imageView {};
    VkFormat imageFormat;
};

struct CreateColorImageOptions {
    uint16_t layerCount = 1;
    VkImageUsageFlags usageFlags = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    VkImageViewType viewType = VK_IMAGE_VIEW_TYPE_2D;
    VkImageCreateFlags imageCreateFlags = 0;
};

[[nodiscard]]
ColorImageGroup CreateColorImage(
    VkPhysicalDevice physicalDevice, 
    VkDevice device, 
    VkExtent2D const & imageExtent,
    VkFormat imageFormat,
    CreateColorImageOptions const & options
);

void DestroyColorImage(VkDevice device, ColorImageGroup const & colorImageGroup);


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

class GpuShader;

[[nodiscard]]
GpuShader CreateShader(VkDevice device, CpuShader const & cpuShader);

bool DestroyShader(VkDevice device, GpuShader & gpuShader);

class GpuShader {
friend GpuShader CreateShader(VkDevice device, CpuShader const & cpuShader);
friend bool DestroyShader(VkDevice device, GpuShader & gpuShader);
public:
    [[nodiscard]]
    CpuShader * cpuShader() {return &mCpuShader;}
    [[nodiscard]]
    CpuShader const * cpuShader() const {return &mCpuShader;}
    [[nodiscard]]
    bool valid () const {
        return MFA_VK_VALID(mShaderModule);
    }
    [[nodiscard]]
    VkShaderModule shaderModule() const {return mShaderModule;}
    void revoke() {
        MFA_VK_MAKE_NULL(mShaderModule);
    }
private:
    VkShaderModule mShaderModule {};
    CpuShader mCpuShader {};
};

struct GraphicPipelineGroup {
    friend void DestroyGraphicPipeline(VkDevice device, GraphicPipelineGroup & graphicPipelineGroup);
    // TODO We can make this struct friend of createPipeline as well

    VkPipelineLayout pipelineLayout {};
    VkPipeline graphicPipeline {};

    [[nodiscard]]
    bool isValid() const noexcept {
        return MFA_VK_VALID(pipelineLayout)
            && MFA_VK_VALID(graphicPipeline);
    }

private:

    void revoke() {
        MFA_VK_MAKE_NULL(pipelineLayout);
        MFA_VK_MAKE_NULL(graphicPipeline);
    }
};

struct CreateGraphicPipelineOptions {
    VkFrontFace frontFace = VK_FRONT_FACE_CLOCKWISE;
    VkCullModeFlags cullMode = VK_CULL_MODE_BACK_BIT;           // TODO It should be VK_CULL_MODE_BACK_BIT although somehow it breaks imgui. Why?
    VkPipelineDynamicStateCreateInfo * dynamicStateCreateInfo = nullptr;
    VkPipelineDepthStencilStateCreateInfo depthStencil {};
    VkPipelineColorBlendAttachmentState colorBlendAttachments {};
    uint8_t pushConstantsRangeCount = 0;
    VkPushConstantRange * pushConstantRanges = nullptr;
    bool useStaticViewportAndScissor = false;           // Use of dynamic viewport and scissor is recommended
    VkPrimitiveTopology primitiveTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    // Default params
    explicit CreateGraphicPipelineOptions() {
        depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencil.depthTestEnable = VK_TRUE;
        depthStencil.depthWriteEnable = VK_TRUE;
        depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
        depthStencil.depthBoundsTestEnable = VK_FALSE;
        depthStencil.stencilTestEnable = VK_FALSE;

        colorBlendAttachments.blendEnable = VK_TRUE;
        colorBlendAttachments.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        colorBlendAttachments.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        colorBlendAttachments.colorBlendOp = VK_BLEND_OP_ADD;
        colorBlendAttachments.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        colorBlendAttachments.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        colorBlendAttachments.alphaBlendOp = VK_BLEND_OP_ADD;
        colorBlendAttachments.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    }
};

// Note Shaders can be removed after creating graphic pipeline
[[nodiscard]]
GraphicPipelineGroup CreateGraphicPipeline(
    VkDevice device, 
    uint8_t shaderStagesCount, 
    GpuShader const * shaderStages,
    VkVertexInputBindingDescription vertexBindingDescription,
    uint32_t attributeDescriptionCount,
    VkVertexInputAttributeDescription * attributeDescriptionData,
    VkExtent2D swapChainExtent,
    VkRenderPass renderPass,
    uint32_t descriptorSetLayoutCount,
    VkDescriptorSetLayout* descriptorSetLayouts,
    CreateGraphicPipelineOptions const & options
);

void AssignViewportAndScissorToCommandBuffer(
    VkExtent2D const & extent2D,
    VkCommandBuffer commandBuffer
);

void DestroyGraphicPipeline(VkDevice device, GraphicPipelineGroup & graphicPipelineGroup);

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
BufferGroup CreateVertexBuffer(
    VkDevice device,
    VkPhysicalDevice physical_device,
    VkCommandPool command_pool,
    VkQueue graphic_queue,
    CBlob vertices_blob
);

void DestroyVertexBuffer(
    VkDevice device,
    BufferGroup & vertex_buffer_group
);

[[nodiscard]]
BufferGroup CreateIndexBuffer (
    VkDevice device,
    VkPhysicalDevice physical_device,
    VkCommandPool command_pool,
    VkQueue graphic_queue,
    CBlob indices_blob
);

void DestroyIndexBuffer(
    VkDevice device,
    BufferGroup & index_buffer_group
);

[[nodiscard]]
std::vector<BufferGroup> CreateUniformBuffer(
    VkDevice device,
    VkPhysicalDevice physical_device,
    uint32_t buffersCount,
    VkDeviceSize size 
);

void UpdateUniformBuffer(
    VkDevice device,
    BufferGroup const & uniform_buffer_group,
    CBlob data
);

void DestroyUniformBuffer(
    VkDevice device,
    BufferGroup & buffer_group
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
std::vector<VkDescriptorSet> CreateDescriptorSet(
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
    uint8_t descriptor_set_count,
    VkDescriptorSet * descriptor_sets,
    uint8_t schemas_count,
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
    VkCommandBuffer* commandBuffers
);

// CreateSyncObjects (Fence, Semaphore, ...)
struct SyncObjects {
    std::vector<VkSemaphore> imageAvailabilitySemaphores;
    std::vector<VkSemaphore> renderFinishIndicatorSemaphores;
    std::vector<VkFence> fencesInFlight;
    std::vector<VkFence> imagesInFlight;
};
[[nodiscard]]
SyncObjects CreateSyncObjects(
    VkDevice device,
    uint8_t maxFramesInFlight,
    uint32_t swapChainImagesCount
);

void DestroySyncObjects(VkDevice device, SyncObjects const & syncObjects);

void DeviceWaitIdle(VkDevice device);

void WaitForFence(VkDevice device, VkFence inFlightFence);

VkResult AcquireNextImage(
    VkDevice device, 
    VkSemaphore imageAvailabilitySemaphore, 
    SwapChainGroup const & swapChainGroup,
    uint32_t & outImageIndex
);

void BindVertexBuffer(
    VkCommandBuffer command_buffer,
    BufferGroup vertex_buffer,
    VkDeviceSize offset = 0
);

void BindIndexBuffer(
    VkCommandBuffer commandBuffer,
    BufferGroup indexBuffer,
    VkDeviceSize offset = 0,
    VkIndexType indexType = VK_INDEX_TYPE_UINT32
);

void DrawIndexed(
    VkCommandBuffer command_buffer,
    uint32_t indices_count,
    uint32_t instance_count = 1,
    uint32_t first_index = 0,
    uint32_t vertex_offset = 0,
    uint32_t first_instance = 0
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

void UpdateDescriptorSetsBasic(
    VkDevice device,
    uint8_t descriptorSetsCount,
    VkDescriptorSet* descriptorSets,
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

}
