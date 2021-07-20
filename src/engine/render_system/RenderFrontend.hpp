#pragma once

#include "RenderBackend.hpp"

#ifdef __DESKTOP__
#include "libs/sdl/SDL.hpp"
#elif defined(__ANDROID__)
#include <android_native_app_glue.h>
#elif defined(__IOS__)
#else
#error Os is not supported
#endif

namespace MFA {
    class DisplayRenderPass;
}

namespace MFA::RenderFrontend {

inline static constexpr int MAX_FRAMES_IN_FLIGHT = 2;

namespace RB = RenderBackend;
using ScreenWidth = RB::ScreenWidth;
using ScreenHeight = RB::ScreenHeight;

using ResizeEventListener = std::function<void()>;

struct InitParams {
#ifdef __DESKTOP__
    ScreenWidth screenWidth = 0;
    ScreenHeight screenHeight = 0;
    bool resizable = true;
#elif defined(__ANDROID__)
    android_app * app = nullptr;
#elif defined(__IOS__)
    void * view = nullptr;
#else
    #error Os is not supported
#endif
    char const * applicationName = nullptr;
};

bool Init(InitParams const & params);

bool Shutdown();

void SetResizeEventListener(ResizeEventListener const & eventListener);

using DrawPipeline = RB::GraphicPipelineGroup;

[[nodiscard]]
VkDescriptorSetLayout CreateBasicDescriptorSetLayout();

[[nodiscard]]
VkDescriptorSetLayout CreateDescriptorSetLayout(
    uint8_t bindingsCount,
    VkDescriptorSetLayoutBinding * bindings
);

void DestroyDescriptorSetLayout(VkDescriptorSetLayout descriptorSetLayout);

[[nodiscard]]
DrawPipeline CreateBasicDrawPipeline(
    VkRenderPass renderPass,
    uint8_t gpuShadersCount, 
    RB::GpuShader * gpuShaders,
    uint32_t descriptorSetLayoutCount,
    VkDescriptorSetLayout * descriptorSetLayouts,
    VkVertexInputBindingDescription const & vertexInputBindingDescription,
    uint8_t vertexInputAttributeDescriptionCount,
    VkVertexInputAttributeDescription * vertexInputAttributeDescriptions
);

[[nodiscard]]
DrawPipeline CreateDrawPipeline(
    VkRenderPass renderPass,
    uint8_t gpuShadersCount, 
    RB::GpuShader * gpuShaders,
    uint32_t descriptorLayoutsCount,
    VkDescriptorSetLayout * descriptorSetLayouts,
    VkVertexInputBindingDescription vertexBindingDescription,
    uint32_t inputAttributeDescriptionCount,
    VkVertexInputAttributeDescription * inputAttributeDescriptionData,
    RB::CreateGraphicPipelineOptions const & options = RB::CreateGraphicPipelineOptions {}
);

void DestroyDrawPipeline(DrawPipeline & draw_pipeline);

[[nodiscard]]
std::vector<VkDescriptorSet> CreateDescriptorSets(
    VkDescriptorSetLayout descriptorSetLayout
);

[[nodiscard]]
std::vector<VkDescriptorSet> CreateDescriptorSets(
    uint32_t descriptorSetCount,
    VkDescriptorSetLayout descriptorSetLayout
);

struct MeshBuffers {
    RB::BufferGroup verticesBuffer {};
    RB::BufferGroup indicesBuffer {};
};

void DestroyMeshBuffers(MeshBuffers & meshBuffers);

[[nodiscard]]
RB::BufferGroup CreateVertexBuffer(CBlob vertices_blob);

[[nodiscard]]
RB::BufferGroup CreateIndexBuffer(CBlob indices_blob);

[[nodiscard]]
RB::GpuTexture CreateTexture(AssetSystem::Texture & texture);

void DestroyTexture(RB::GpuTexture & gpu_texture);

struct SamplerGroup {
    VkSampler sampler;
    [[nodiscard]]
    bool isValid() const noexcept{
        return MFA_VK_VALID(sampler);
    }
    void revoke() {
        MFA_ASSERT(isValid());
        MFA_VK_MAKE_NULL(sampler);
    }
};
// TODO We should ask for options here
[[nodiscard]]
SamplerGroup CreateSampler(RB::CreateSamplerParams const & samplerParams = {});

void DestroySampler(SamplerGroup & sampler_group);

struct GpuModel {
    bool valid = false;
    MeshBuffers meshBuffers {};
    std::vector<RB::GpuTexture> textures {};
    AssetSystem::Model model {};
    // TODO Samplers
};
[[nodiscard]]
GpuModel CreateGpuModel(AssetSystem::Model & model_asset);

void DestroyGpuModel(GpuModel & gpu_model);

void DeviceWaitIdle();

[[nodiscard]]
RB::GpuShader CreateShader(AssetSystem::Shader const & shader);

void DestroyShader(RB::GpuShader & gpu_shader);

struct DrawPass {
    uint32_t imageIndex;
    uint8_t frameIndex;
    bool isValid = false;
    DrawPipeline * drawPipeline = nullptr;
};

struct UniformBufferGroup {
    std::vector<RB::BufferGroup> buffers;
    size_t bufferSize;
    [[nodiscard]]
    bool isValid() const noexcept;
};

UniformBufferGroup CreateUniformBuffer(size_t bufferSize, uint32_t count);

void UpdateUniformBuffer(
    RB::BufferGroup const & uniform_buffer, 
    CBlob data
);

void DestroyUniformBuffer(UniformBufferGroup & uniform_buffer);

//[[nodiscard]]
//DrawPass BeginDrawPass();

void BindDrawPipeline(
    DrawPass & drawPass,
    DrawPipeline & drawPipeline   
);

void UpdateDescriptorSetBasic(
    DrawPass const & drawPass,
    VkDescriptorSet descriptorSet,
    UniformBufferGroup const & uniformBuffer,
    RB::GpuTexture const & gpuTexture,
    SamplerGroup const & samplerGroup
);

void UpdateDescriptorSetBasic(
    DrawPass const & drawPass,
    VkDescriptorSet descriptorSet,
    UniformBufferGroup const & uniformBuffer,
    uint32_t imageInfoCount,
    VkDescriptorImageInfo const * imageInfos
);

void UpdateDescriptorSets(
    uint8_t writeInfoCount,
    VkWriteDescriptorSet * writeInfo
);

void UpdateDescriptorSets(
    uint8_t descriptorSetsCount,
    VkDescriptorSet* descriptorSets,
    uint8_t writeInfoCount,
    VkWriteDescriptorSet * writeInfo
);

void BindDescriptorSet(
    DrawPass const & drawPass,
    VkDescriptorSet descriptorSet
);

//: loop through each mesh instance in the mesh instances list to render
//
//  - get the mesh from the asset manager
//  - populate the push constants with the transformation matrix of the mesh
//  - bind the pipeline to the command buffer
//  - bind the vertex buffer for the mesh to the command buffer
//  - bind the index buffer for the mesh to the command buffer
//  - get the texture instance to apply to the mesh from the asset manager
//  - get the descriptor set for the texture that defines how it maps to the pipeline's descriptor set layout
//  - bind the descriptor set for the texture into the pipeline's descriptor set layout
//  - tell the command buffer to draw the mesh
//
//: end loop

void BindVertexBuffer(
    DrawPass const & drawPass, 
    RB::BufferGroup vertexBuffer,
    VkDeviceSize offset = 0
);

void BindIndexBuffer(
    DrawPass const & drawPass, 
    RB::BufferGroup indexBuffer,
    VkDeviceSize offset = 0,
    VkIndexType indexType = VK_INDEX_TYPE_UINT32
);

void DrawIndexed(
    DrawPass const & drawPass, 
    uint32_t indicesCount,
    uint32_t instanceCount = 1,
    uint32_t firstIndex = 0,
    uint32_t vertexOffset = 0,
    uint32_t firstInstance = 0
);

//void BeginDisplayRenderPass(DrawPass const & drawPass);
//
//void EndDisplayRenderPass(DrawPass const & drawPass);

void BeginRenderPass(DrawPass const & drawPass, VkRenderPass renderPass, VkFramebuffer * frameBuffers);

void EndRenderPass(DrawPass const & drawPass);

//void EndDrawPass(DrawPass & drawPass);

void OnNewFrame(float deltaTimeInSec);

void SetScissor(DrawPass const & draw_pass, VkRect2D const & scissor);

void SetViewport(DrawPass const & draw_pass, VkViewport const & viewport);

void PushConstants(
    DrawPass const & draw_pass, 
    AssetSystem::ShaderStage shader_stage, 
    uint32_t offset, 
    CBlob data
);

[[nodiscard]]
uint32_t GetSwapChainImagesCount();

void GetDrawableSize(int32_t & outWidth, int32_t & outHeight);

#ifdef __DESKTOP__
// SDL Functions

void WarpMouseInWindow(int32_t x, int32_t y);

uint32_t GetMouseState(int32_t * x, int32_t * y);

uint8_t const * GetKeyboardState(int * numKeys = nullptr);

[[nodiscard]]
uint32_t GetWindowFlags();

#endif

void AssignViewportAndScissorToCommandBuffer(uint32_t imageIndex);

#ifdef __DESKTOP__
using EventWatchId = int;
using EventWatch = std::function<int(void* data, MSDL::SDL_Event* event)>;
EventWatchId AddEventWatch(EventWatch const & eventWatch);

void RemoveEventWatch(EventWatchId watchId);
#endif

void NotifyDeviceResized();

[[nodiscard]]
VkSurfaceCapabilitiesKHR GetSurfaceCapabilities();

[[nodiscard]]
RB::SwapChainGroup CreateSwapChain(VkSwapchainKHR oldSwapChain = VkSwapchainKHR {});

void DestroySwapChain(RB::SwapChainGroup const & swapChainGroup);


[[nodiscard]]
VkRenderPass CreateRenderPass(VkFormat imageFormat);

void DestroyRenderPass(VkRenderPass renderPass);


RB::DepthImageGroup CreateDepthImage(VkExtent2D const & imageSize);

void DestroyDepthImage(RB::DepthImageGroup const & depthImage);

// TODO We can gather all renderTypes in one file

std::vector<VkFramebuffer> CreateFrameBuffers(
    VkRenderPass renderPass,
    uint32_t swapChainImageViewsCount, 
    VkImageView* swapChainImageViews,
    VkImageView depthImageView,
    VkExtent2D swapChainExtent
);

void DestroyFrameBuffers(
    uint32_t frameBuffersCount,
    VkFramebuffer * frameBuffers
);


bool IsWindowVisible();

bool IsWindowResized();


void WaitForFence(uint8_t frameIndex);

void AcquireNextImage(
    uint8_t frameIndex,
    RB::SwapChainGroup const & swapChainGroup,
    uint32_t & outImageIndex
);


void BeginCommandBuffer(
    uint32_t imageIndex,
    VkCommandBufferBeginInfo const & beginInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT
    }
);

uint32_t GetPresentQueueFamily();

uint32_t GetGraphicQueueFamily();

void PipelineBarrier(
    uint32_t imageIndex,
    VkPipelineStageFlags sourceStageMask,
    VkPipelineStageFlags destinationStateMask,
    VkImageMemoryBarrier const & memoryBarrier
);

void EndCommandBuffer(uint32_t imageIndex);

void SubmitQueue(uint32_t imageIndex, uint8_t frameIndex);

void PresentQueue(
    uint32_t imageIndex, 
    uint8_t frameIndex, 
    VkSwapchainKHR swapChain
);

DisplayRenderPass * GetDisplayRenderPass();

}
