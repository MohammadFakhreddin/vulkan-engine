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

namespace MFA::RenderFrontend {

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
    uint8_t frame_index;
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

[[nodiscard]]
DrawPass BeginPass();

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
    DrawPass drawPass, 
    RB::BufferGroup vertexBuffer,
    VkDeviceSize offset = 0
);

void BindIndexBuffer(
    DrawPass drawPass, 
    RB::BufferGroup indexBuffer,
    VkDeviceSize offset = 0,
    VkIndexType indexType = VK_INDEX_TYPE_UINT32
);

void DrawIndexed(
    DrawPass drawPass, 
    uint32_t indicesCount,
    uint32_t instanceCount = 1,
    uint32_t firstIndex = 0,
    uint32_t vertexOffset = 0,
    uint32_t firstInstance = 0
);

void EndPass(DrawPass & drawPass);

void OnNewFrame(float deltaTimeInSec);

void SetScissor(DrawPass const & draw_pass, VkRect2D const & scissor);

void SetViewport(DrawPass const & draw_pass, VkViewport const & viewport);

void PushConstants(
    DrawPass const & draw_pass, 
    AssetSystem::ShaderStage shader_stage, 
    uint32_t offset, 
    CBlob data
);

uint8_t SwapChainImagesCount();

void GetDrawableSize(int32_t & outWidth, int32_t & outHeight);

#ifdef __DESKTOP__
// SDL Functions

void WarpMouseInWindow(int32_t x, int32_t y);

uint32_t GetMouseState(int32_t * x, int32_t * y);

uint8_t const * GetKeyboardState(int * numKeys = nullptr);

[[nodiscard]]
uint32_t GetWindowFlags();

#endif

void AssignViewportAndScissorToCommandBuffer(
    VkCommandBuffer commandBuffer
);

#ifdef __DESKTOP__
using EventWatchId = int;
using EventWatch = std::function<int(void* data, MSDL::SDL_Event* event)>;
EventWatchId AddEventWatch(EventWatch const & eventWatch);

void RemoveEventWatch(EventWatchId watchId);
#endif

// Currently only used for android (Probably ios in future)
void NotifyDeviceResized();

[[nodiscard]]
VkSurfaceCapabilitiesKHR GetSurfaceCapabilities();

}
