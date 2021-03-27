#pragma once

#include "BedrockCommon.hpp"
#include "FoundationAsset.hpp"

#include <functional>

#include <vulkan/vulkan.h>

#ifdef CreateWindow
#undef CreateWindow
#endif

struct SDL_Window;

// TODO Write description for all functions for learning purpose
// Note: Not all functions can be called from outside
// TODO Remove functions that are not usable from outside
namespace MFA::RenderBackend {

using ScreenWidth = Platforms::ScreenSize;
using ScreenHeight = Platforms::ScreenSize;
using CpuTexture = AssetSystem::Texture;
using CpuShader = AssetSystem::Shader;

// Vulkan functions

[[nodiscard]]
SDL_Window * CreateWindow(ScreenWidth screen_width, ScreenHeight screen_height);

void DestroyWindow(SDL_Window * window);

[[nodiscard]]
VkSurfaceKHR_T * CreateWindowSurface(SDL_Window * window, VkInstance_T * instance);

void DestroyWindowSurface(VkInstance_T * instance, VkSurfaceKHR_T * surface);

[[nodiscard]]
VkExtent2D ChooseSwapChainExtent(
    VkSurfaceCapabilitiesKHR const & surface_capabilities, 
    ScreenWidth screen_width, 
    ScreenHeight screen_height
);

[[nodiscard]]
VkPresentModeKHR ChoosePresentMode(uint8_t present_modes_count, VkPresentModeKHR const * present_modes);

[[nodiscard]]
VkSurfaceFormatKHR ChooseSurfaceFormat(uint8_t available_formats_count, VkSurfaceFormatKHR const * available_formats);

[[nodiscard]]
VkInstance_T * CreateInstance(char const * application_name, SDL_Window * window);

void DestroyInstance(VkInstance_T * instance);

[[nodiscard]]
VkDebugReportCallbackEXT CreateDebugCallback(
    VkInstance_T * vkInstance,
    PFN_vkDebugReportCallbackEXT const & debug_callback
);

void DestroyDebugReportCallback(
    VkInstance_T * instance,
    VkDebugReportCallbackEXT const & report_callback_ext
);

[[nodiscard]]
U32 FindMemoryType (
    VkPhysicalDevice * physicalDevice,
    U32 typeFilter, 
    VkMemoryPropertyFlags propertyFlags
);

[[nodiscard]]
VkImageView_T * CreateImageView (
    VkDevice_T * device,
    VkImage_T const & image, 
    VkFormat format, 
    VkImageAspectFlags aspect_flags
);

void DestroyImageView(
    VkDevice_T * device,
    VkImageView_T * image_view
);

[[nodiscard]]
VkCommandBuffer BeginSingleTimeCommand(VkDevice_T * device, VkCommandPool const & command_pool);

void EndAndSubmitSingleTimeCommand(
    VkDevice_T * device, 
    VkCommandPool const & commandPool, 
    VkQueue const & graphicQueue, 
    VkCommandBuffer const & commandBuffer
);

[[nodiscard]]
VkFormat FindDepthFormat(VkPhysicalDevice_T * physical_device);

[[nodiscard]]
VkFormat FindSupportedFormat(
    VkPhysicalDevice_T * physical_device,
    uint8_t candidates_count, 
    VkFormat * candidates,
    VkImageTiling tiling, 
    VkFormatFeatureFlags features
);

void TransferImageLayout(
    VkDevice_T * device,
    VkQueue_T * graphicQueue,
    VkCommandPool_T * commandPool,
    VkImage_T * image, 
    VkImageLayout const oldLayout, 
    VkImageLayout const newLayout
);

struct BufferGroup {
    VkBuffer_T * buffer = nullptr;
    VkDeviceMemory_T * memory = nullptr;
    [[nodiscard]]
    bool isValid() const noexcept {
        return buffer != nullptr && memory != nullptr;
    }
};
[[nodiscard]]
BufferGroup CreateBuffer(
    VkDevice_T * device,
    VkPhysicalDevice_T * physical_device,
    VkDeviceSize size, 
    VkBufferUsageFlags usage, 
    VkMemoryPropertyFlags properties 
);

void MapDataToBuffer(
    VkDevice_T * device,
    VkDeviceMemory_T * bufferMemory,
    CBlob dataBlob
);

void CopyBuffer(
    VkDevice_T * device,
    VkCommandPool_T * commandPool,
    VkQueue_T * graphicQueue,
    VkBuffer_T * sourceBuffer,
    VkBuffer_T * destinationBuffer,
    VkDeviceSize size
);

void DestroyBuffer(
    VkDevice_T * device,
    BufferGroup & buffer_group
);

struct ImageGroup {
    VkImage_T * image = nullptr;
    VkDeviceMemory_T * memory = nullptr;
};

[[nodiscard]]
ImageGroup CreateImage(
    VkDevice_T * device,
    VkPhysicalDevice_T * physical_device,
    U32 width, 
    U32 height,
    U32 depth,
    U8 mip_levels,
    U16 slice_count,
    VkFormat format, 
    VkImageTiling tiling, 
    VkImageUsageFlags usage, 
    VkMemoryPropertyFlags properties
);

void DestroyImage(
    VkDevice_T * device,
    ImageGroup const & image_group
);

class GpuTexture;

[[nodiscard]]
GpuTexture CreateTexture(
    CpuTexture & cpuTexture,
    VkDevice_T * device,
    VkPhysicalDevice_T * physicalDevice,
    VkQueue_T * graphicQueue,
    VkCommandPool_T * commandPool
);

bool DestroyTexture(VkDevice_T * device, GpuTexture & gpuTexture);

// TODO It needs handle system // TODO Might be moved to a new class called render_types
class GpuTexture {
friend GpuTexture CreateTexture(
    CpuTexture & cpuTexture,
    VkDevice_T * device,
    VkPhysicalDevice_T * physicalDevice,
    VkQueue_T * graphicQueue,
    VkCommandPool_T * commandPool
);
friend bool DestroyTexture(VkDevice_T * device, GpuTexture & gpuTexture);
public:
    [[nodiscard]]
    CpuTexture const * cpu_texture() const {return &mCpuTexture;}
    [[nodiscard]]
    CpuTexture * cpu_texture() {return &mCpuTexture;}
    [[nodiscard]]
    bool valid () const {return mImageView != nullptr;}
    [[nodiscard]]
    VkImage_T const * image() const {return mImageGroup.image;}
    [[nodiscard]]
    VkImageView_T * image_view() const {return mImageView;}
private:
    ImageGroup mImageGroup {};
    VkImageView_T * mImageView = nullptr;
    CpuTexture mCpuTexture {};
};

[[nodiscard]]
VkFormat ConvertCpuTextureFormatToGpu(AssetSystem::TextureFormat cpuFormat);

void CopyBufferToImage(
    VkDevice_T * device,
    VkCommandPool_T * commandPool,
    VkBuffer_T * buffer,
    VkImage_T * image,
    VkQueue_T * graphicQueue,
    CpuTexture const & cpuTexture
);

struct LogicalDevice {
    VkDevice_T * device;
    VkPhysicalDeviceMemoryProperties physical_memory_properties;
};
[[nodiscard]]
LogicalDevice CreateLogicalDevice(
    VkPhysicalDevice_T * physicalDevice,
    U32 graphicsQueueFamily,
    U32 presentQueueFamily,
    VkPhysicalDeviceFeatures const & enabledPhysicalDeviceFeatures
);

void DestroyLogicalDevice(LogicalDevice const & logical_device);

[[nodiscard]]
VkQueue_T * GetQueueByFamilyIndex(
    VkDevice_T * device,
    U32 queue_family_index
);

struct CreateSamplerParams {
    float min_lod = 0;  // Level of detail
    float max_lod = 1;
    float max_anisotropy = 16.0f;
};
[[nodiscard]]
VkSampler_T * CreateSampler(
    VkDevice_T * device, 
    CreateSamplerParams const & params = {}
);

void DestroySampler(VkDevice_T * device, VkSampler_T * sampler);

[[nodiscard]]
VkDebugReportCallbackEXT_T * SetDebugCallback(
    VkInstance_T * instance,
    PFN_vkDebugReportCallbackEXT const & callback
);

// TODO Might need to ask features from outside instead
struct FindPhysicalDeviceResult {
    VkPhysicalDevice_T * physicalDevice = nullptr;
    VkPhysicalDeviceFeatures physicalDeviceFeatures {};
};
[[nodiscard]]
FindPhysicalDeviceResult FindPhysicalDevice(VkInstance_T * vk_instance);

[[nodiscard]]
bool CheckSwapChainSupport(VkPhysicalDevice_T * physical_device);

struct FindPresentAndGraphicQueueFamilyResult {
    U32 present_queue_family;
    U32 graphic_queue_family;
};
[[nodiscard]]
FindPresentAndGraphicQueueFamilyResult FindPresentAndGraphicQueueFamily(
    VkPhysicalDevice_T * physical_device, 
    VkSurfaceKHR_T * window_surface
);

[[nodiscard]]
VkCommandPool_T * CreateCommandPool(VkDevice_T * device, U32 queue_family_index);

void DestroyCommandPool(VkDevice_T * device, VkCommandPool_T * command_pool);

struct SwapChainGroup {
    VkSwapchainKHR_T * swapChain = nullptr;
    VkFormat swapChainFormat {};
    std::vector<VkImage_T *> swapChainImages {};
    std::vector<VkImageView_T *> swapChainImageViews {};
};

[[nodiscard]]
SwapChainGroup CreateSwapChain(
    VkDevice_T * device,
    VkPhysicalDevice_T * physical_device, 
    VkSurfaceKHR_T * window_surface,
    VkExtent2D swap_chain_extend,
    VkSwapchainKHR_T * oldSwapChain = nullptr
);

void DestroySwapChain(VkDevice_T * device, SwapChainGroup const & swapChainGroup);

struct DepthImageGroup {
    ImageGroup imageGroup {};
    VkImageView_T * imageView = nullptr;
};

[[nodiscard]]
DepthImageGroup CreateDepth(
    VkPhysicalDevice_T * physical_device,
    VkDevice_T * device,
    VkExtent2D swap_chain_extend
);

void DestroyDepth(VkDevice_T * device, DepthImageGroup const & depthGroup);

// TODO Ask for options
[[nodiscard]]
VkRenderPass_T * CreateRenderPass(
    VkPhysicalDevice_T * physicalDevice, 
    VkDevice_T * device, 
    VkFormat swapChainFormat
);

void DestroyRenderPass(VkDevice_T * device, VkRenderPass_T * renderPass);

[[nodiscard]]
std::vector<VkFramebuffer_T *> CreateFrameBuffers(
    VkDevice_T * device,
    VkRenderPass_T * renderPass,
    U32 swapChainImageViewsCount, 
    VkImageView_T ** swapChainImageViews,
    VkImageView_T * depthImageView,
    VkExtent2D swapChainExtent
);

void DestroyFrameBuffers(
    VkDevice_T * device,
    U32 frameBuffersCount,
    VkFramebuffer_T ** frameBuffers
);

class GpuShader;

[[nodiscard]]
GpuShader CreateShader(VkDevice_T * device, CpuShader const & cpuShader);

bool DestroyShader(VkDevice_T * device, GpuShader & gpuShader);

class GpuShader {
friend GpuShader CreateShader(VkDevice_T * device, CpuShader const & cpuShader);
friend bool DestroyShader(VkDevice_T * device, GpuShader & gpuShader);
public:
    [[nodiscard]]
    CpuShader * cpuShader() {return &mCpuShader;}
    [[nodiscard]]
    CpuShader const * cpuShader() const {return &mCpuShader;}
    [[nodiscard]]
    bool valid () const {return mShaderModule != nullptr;}
    [[nodiscard]]
    VkShaderModule_T * shaderModule() const {return mShaderModule;}
private:
    VkShaderModule_T * mShaderModule = nullptr;
    CpuShader mCpuShader {};
};

struct GraphicPipelineGroup {
    VkPipelineLayout_T * pipelineLayout = nullptr;
    VkPipeline_T * graphicPipeline = nullptr;
};
struct CreateGraphicPipelineOptions {
    VkFrontFace font_face = VK_FRONT_FACE_CLOCKWISE;
    VkPipelineDynamicStateCreateInfo * dynamic_state_create_info = nullptr;
    VkPipelineDepthStencilStateCreateInfo depth_stencil {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .depthTestEnable = VK_TRUE,
        .depthWriteEnable = VK_TRUE,
        .depthCompareOp = VK_COMPARE_OP_LESS,
        .depthBoundsTestEnable = VK_FALSE,
        .stencilTestEnable = VK_FALSE
    };
    VkPipelineColorBlendAttachmentState color_blend_attachments {
        .blendEnable = VK_TRUE,
        .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
        .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
        .colorBlendOp = VK_BLEND_OP_ADD,
        .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
        .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
        .alphaBlendOp = VK_BLEND_OP_ADD,
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
    };
    U8 push_constants_range_count = 0;
    VkPushConstantRange * push_constant_ranges = nullptr;
    bool use_static_viewport_and_scissor = true;
    VkPrimitiveTopology primitive_topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
};
// Note Shaders can be removed after creating graphic pipeline
[[nodiscard]]
GraphicPipelineGroup CreateGraphicPipeline(
    VkDevice_T * device, 
    U8 shader_stages_count, 
    GpuShader const * shader_stages,
    VkVertexInputBindingDescription vertex_binding_description,
    U32 attribute_description_count,
    VkVertexInputAttributeDescription * attribute_description_data,
    VkExtent2D swap_chain_extent,
    VkRenderPass_T * render_pass,
    VkDescriptorSetLayout_T * descriptor_set_layout,
    CreateGraphicPipelineOptions const & options
);

void DestroyGraphicPipeline(VkDevice_T * device, GraphicPipelineGroup & graphicPipelineGroup);

[[nodiscard]]
VkDescriptorSetLayout_T * CreateDescriptorSetLayout(
    VkDevice_T * device,
    U8 bindings_count,
    VkDescriptorSetLayoutBinding * bindings
);

void DestroyDescriptorSetLayout(
    VkDevice_T * device,
    VkDescriptorSetLayout_T * descriptor_set_layout
);

[[nodiscard]]
VkShaderStageFlagBits ConvertAssetShaderStageToGpu(AssetSystem::ShaderStage stage);

[[nodiscard]]
BufferGroup CreateVertexBuffer(
    VkDevice_T * device,
    VkPhysicalDevice_T * physical_device,
    VkCommandPool_T * command_pool,
    VkQueue_T * graphic_queue,
    CBlob vertices_blob
);

void DestroyVertexBuffer(
    VkDevice_T * device,
    BufferGroup & vertex_buffer_group
);

[[nodiscard]]
BufferGroup CreateIndexBuffer (
    VkDevice_T * device,
    VkPhysicalDevice_T * physical_device,
    VkCommandPool_T * command_pool,
    VkQueue_T * graphic_queue,
    CBlob indices_blob
);

void DestroyIndexBuffer(
    VkDevice_T * device,
    BufferGroup & index_buffer_group
);

[[nodiscard]]
std::vector<BufferGroup> CreateUniformBuffer(
    VkDevice_T * device,
    VkPhysicalDevice_T * physical_device,
    U32 buffersCount,
    VkDeviceSize size 
);

void UpdateUniformBuffer(
    VkDevice_T * device,
    BufferGroup const & uniform_buffer_group,
    CBlob data
);

void DestroyUniformBuffer(
    VkDevice_T * device,
    BufferGroup & buffer_group
);

[[nodiscard]]
VkDescriptorPool_T * CreateDescriptorPool(
    VkDevice_T * device,
    U32 swap_chain_images_count
);

void DestroyDescriptorPool(
    VkDevice_T * device, 
    VkDescriptorPool_T * pool
);

// Descriptor sets gets destroyed automatically when descriptor pool is destroyed
[[nodiscard]]
std::vector<VkDescriptorSet_T *> CreateDescriptorSet(
    VkDevice_T * device,
    VkDescriptorPool_T * descriptor_pool,
    VkDescriptorSetLayout_T * descriptor_set_layout,
    U32 descriptor_set_count,
    U8 schemas_count = 0,
    VkWriteDescriptorSet * schemas = nullptr
);

// TODO Consider creating an easier interface
void UpdateDescriptorSets(
    VkDevice_T * device,
    U8 descriptor_set_count,
    VkDescriptorSet_T ** descriptor_sets,
    U8 schemas_count,
    VkWriteDescriptorSet * schemas
);

/***
 *  CreateCommandBuffer (For pipeline I guess)
 *  Allocate graphics command buffers
 *  There is no need for destroying it
 ***/
std::vector<VkCommandBuffer_T *> CreateCommandBuffers(
    VkDevice_T * device,
    U8 swap_chain_images_count,
    VkCommandPool_T * command_pool
);

void DestroyCommandBuffers(
    VkDevice_T * device,
    VkCommandPool_T * commandPool,
    U8 commandBuffersCount,
    VkCommandBuffer_T ** commandBuffers
);

// CreateSyncObjects (Fence, Semaphore, ...)
struct SyncObjects {
    std::vector<VkSemaphore_T *> image_availability_semaphores;
    std::vector<VkSemaphore_T *> render_finish_indicator_semaphores;
    std::vector<VkFence_T *> fences_in_flight;
    std::vector<VkFence_T *> images_in_flight;
};
[[nodiscard]]
SyncObjects CreateSyncObjects(
    VkDevice_T * device,
    U8 maxFramesInFlight,
    U8 swapChainImagesCount
);

void DestroySyncObjects(VkDevice_T * device, SyncObjects const & syncObjects);

void DeviceWaitIdle(VkDevice_T * device);

void WaitForFence(VkDevice_T * device, VkFence_T * inFlightFence);

[[nodiscard]]
U8 AcquireNextImage(
    VkDevice_T * device, 
    VkSemaphore_T * imageAvailabilitySemaphore, 
    SwapChainGroup const & swapChainGroup
);

void BindVertexBuffer(
    VkCommandBuffer_T * command_buffer,
    BufferGroup vertex_buffer,
    VkDeviceSize offset = 0
);

void BindIndexBuffer(
    VkCommandBuffer_T * commandBuffer,
    BufferGroup indexBuffer,
    VkDeviceSize offset = 0,
    VkIndexType indexType = VK_INDEX_TYPE_UINT32
);

void DrawIndexed(
    VkCommandBuffer_T * command_buffer,
    U32 indices_count,
    U32 instance_count = 1,
    U32 first_index = 0,
    U32 vertex_offset = 0,
    U32 first_instance = 0
);

void SetScissor(VkCommandBuffer_T * commandBuffer, VkRect2D const & scissor);

void SetViewport(VkCommandBuffer_T * commandBuffer, VkViewport const & viewport);

void PushConstants(
    VkCommandBuffer_T * command_buffer,
    VkPipelineLayout_T * pipeline_layout, 
    AssetSystem::ShaderStage shader_stage, 
    U32 offset, 
    CBlob data
);

void UpdateDescriptorSetsBasic(
    VkDevice_T * device,
    U8 descriptor_sets_count,
    VkDescriptorSet_T ** descriptor_sets,
    VkDescriptorBufferInfo const & buffer_info,
    U32 image_info_count,
    VkDescriptorImageInfo const * image_infos
);

void UpdateDescriptorSets(
    VkDevice_T * device,
    U8 descriptorWritesCount,
    VkWriteDescriptorSet * descriptorWrites
);

}
