#pragma once

#include "BedrockCommon.hpp"
#include "FoundationAsset.hpp"

#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>
#include <vulkan/vulkan.h>
#include <functional>
// TODO Write description for all functions for learning purpose
// Note: Not all functions can be called from outside
// TODO Remove functions that are not usable from outside
namespace MFA::RenderBackend {

using ScreenWidth = Platforms::ScreenSizeType;
using ScreenHeight = Platforms::ScreenSizeType;
using CpuTexture = Asset::TextureAsset;
using CpuShader = Asset::ShaderAsset;

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
    VkInstance_T * vk_instance,
    PFN_vkDebugReportCallbackEXT const & debug_callback
);

void DestroyDebugReportCallback(
    VkInstance_T * instance,
    VkDebugReportCallbackEXT const & report_callback_ext
);

[[nodiscard]]
U32 FindMemoryType (
    VkPhysicalDevice * physical_device,
    U32 type_filter, 
    VkMemoryPropertyFlags property_flags
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
    VkCommandPool const & command_pool, 
    VkQueue const & graphic_queue, 
    VkCommandBuffer const & command_buffer
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
    VkQueue_T * graphic_queue,
    VkCommandPool_T * command_pool,
    VkImage_T * image, 
    VkImageLayout const old_layout, 
    VkImageLayout const new_layout
);

struct BufferGroup {
    VkBuffer_T * buffer = nullptr;
    VkDeviceMemory_T * memory = nullptr;
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
    VkDeviceMemory_T * buffer_memory,
    CBlob data_blob
);

void CopyBuffer(
    VkDevice_T * device,
    VkCommandPool_T * command_pool,
    VkQueue_T * graphic_queue,
    VkBuffer_T * source_buffer,
    VkBuffer_T * destination_buffer,
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
    CpuTexture & cpu_texture,
    VkDevice_T * device,
    VkPhysicalDevice_T * physical_device,
    VkQueue_T * graphic_queue,
    VkCommandPool_T * command_pool
);

bool DestroyTexture(VkDevice_T * device, GpuTexture & gpu_texture);

// TODO It needs handle system // TODO Might be moved to a new class called render_types
class GpuTexture {
friend GpuTexture CreateTexture(
    CpuTexture & cpu_texture,
    VkDevice_T * device,
    VkPhysicalDevice_T * physical_device,
    VkQueue_T * graphic_queue,
    VkCommandPool_T * command_pool
);
friend bool DestroyTexture(VkDevice_T * device, GpuTexture & gpu_texture);
public:
    [[nodiscard]]
    CpuTexture const * cpu_texture() const {return &m_cpu_texture;}
    [[nodiscard]]
    CpuTexture * cpu_texture() {return &m_cpu_texture;}
    [[nodiscard]]
    bool valid () const {return MFA_PTR_VALID(m_image_view);}
    [[nodiscard]]
    VkImage_T const * image() const {return m_image_group.image;}
    [[nodiscard]]
    VkImageView_T * image_view() const {return m_image_view;}
private:
    ImageGroup m_image_group {};
    VkImageView_T * m_image_view = nullptr;
    CpuTexture m_cpu_texture {};
};

[[nodiscard]]
VkFormat ConvertCpuTextureFormatToGpu(Asset::TextureFormat cpu_format);

void CopyBufferToImage(
    VkDevice_T * device,
    VkCommandPool_T * command_pool,
    VkBuffer_T * buffer,
    VkImage_T * image,
    VkQueue_T * graphic_queue,
    CpuTexture const & cpu_texture
);

struct LogicalDevice {
    VkDevice_T * device;
    VkPhysicalDeviceMemoryProperties physical_memory_properties;
};
[[nodiscard]]
LogicalDevice CreateLogicalDevice(
    VkPhysicalDevice_T * physical_device,
    U32 graphics_queue_family,
    U32 present_queue_family,
    VkPhysicalDeviceFeatures const & enabled_physical_device_features
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
    VkPhysicalDevice_T * physical_device = nullptr;
    VkPhysicalDeviceFeatures physical_device_features {};
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
    VkSwapchainKHR_T * swap_chain = nullptr;
    VkFormat swap_chain_format {};
    std::vector<VkImage_T *> swap_chain_images {};
    std::vector<VkImageView_T *> swap_chain_image_views {};
};

[[nodiscard]]
SwapChainGroup CreateSwapChain(
    VkDevice_T * device,
    VkPhysicalDevice_T * physical_device, 
    VkSurfaceKHR_T * window_surface,
    VkExtent2D swap_chain_extend,
    VkSwapchainKHR_T * old_swap_chain = nullptr
);

void DestroySwapChain(VkDevice_T * device, SwapChainGroup const & swap_chain_group);

struct DepthImageGroup {
    ImageGroup image_group {};
    VkImageView_T * image_view = nullptr;
};

[[nodiscard]]
DepthImageGroup CreateDepth(
    VkPhysicalDevice_T * physical_device,
    VkDevice_T * device,
    VkExtent2D swap_chain_extend
);

void DestroyDepth(VkDevice_T * device, DepthImageGroup const & depth_group);

// TODO Ask for options
[[nodiscard]]
VkRenderPass_T * CreateRenderPass(
    VkPhysicalDevice_T * physical_device, 
    VkDevice_T * device, 
    VkFormat swap_chain_format
);

void DestroyRenderPass(VkDevice_T * device, VkRenderPass_T * render_pass);

[[nodiscard]]
std::vector<VkFramebuffer_T *> CreateFrameBuffers(
    VkDevice_T * device,
    VkRenderPass_T * render_pass,
    U32 swap_chain_image_views_count, 
    VkImageView_T ** swap_chain_image_views,
    VkImageView_T * depth_image_view,
    VkExtent2D swap_chain_extent
);

void DestroyFrameBuffers(
    VkDevice_T * device,
    U32 frame_buffers_count,
    VkFramebuffer_T ** frame_buffers
);

class GpuShader;

[[nodiscard]]
GpuShader CreateShader(VkDevice_T * device, CpuShader const & cpu_shader);

bool DestroyShader(VkDevice_T * device, GpuShader & gpu_shader);

class GpuShader {
friend GpuShader CreateShader(VkDevice_T * device, CpuShader const & cpu_shader);
friend bool DestroyShader(VkDevice_T * device, GpuShader & gpu_shader);
public:
    [[nodiscard]]
    CpuShader * cpu_shader() {return &m_cpu_shader;}
    [[nodiscard]]
    CpuShader const * cpu_shader() const {return &m_cpu_shader;}
    [[nodiscard]]
    bool valid () const {return MFA_PTR_VALID(m_shader_module);}
    [[nodiscard]]
    VkShaderModule_T * shader_module() const {return m_shader_module;}
private:
    VkShaderModule_T * m_shader_module = nullptr;
    CpuShader m_cpu_shader {};
};

struct GraphicPipelineGroup {
    VkPipelineLayout_T * pipeline_layout = nullptr;
    VkPipeline_T * graphic_pipeline = nullptr;
};
struct CreateGraphicPipelineOptions {
    VkFrontFace font_face = VK_FRONT_FACE_CLOCKWISE;
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
    U8 push_constants_range_count = 0,
    VkPushConstantRange * push_constant_ranges = nullptr,
    CreateGraphicPipelineOptions const & options = {}
);

void DestroyGraphicPipeline(VkDevice_T * device, GraphicPipelineGroup & graphic_pipeline_group);

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
VkShaderStageFlagBits ConvertAssetShaderStageToGpu(Asset::ShaderStage stage);

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
    U8 swap_chain_images_count,
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
    U8 swap_chain_images_count,
    U8 schemas_count = 0,
    VkWriteDescriptorSet * schemas = nullptr
);

// TODO Consider creating an easier interface
void UpdateDescriptorSet(
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
    VkCommandPool_T * command_pool,
    U8 command_buffers_count,
    VkCommandBuffer_T ** command_buffers
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
    U8 max_frames_in_flight,
    U8 swap_chain_images_count
);

void DestroySyncObjects(VkDevice_T * device, SyncObjects const & sync_objects);

void DeviceWaitIdle(VkDevice_T * device);

void WaitForFence(VkDevice_T * device, VkFence_T * in_flight_fence);

[[nodiscard]]
U8 AcquireNextImage(
    VkDevice_T * device, 
    VkSemaphore_T * image_availability_semaphore, 
    SwapChainGroup const & swap_chain_group
);

void BindVertexBuffer(VkCommandBuffer_T * command_buffer, BufferGroup vertex_buffer);

void BindIndexBuffer(VkCommandBuffer_T * command_buffer, BufferGroup index_buffer);

void DrawIndexed(VkCommandBuffer_T * command_buffer, U32 indices_count);

void SetScissor(VkCommandBuffer_T * command_buffer, VkRect2D scissor);

}
