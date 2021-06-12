#pragma once

#include "RenderBackend.hpp"

#include "libs/sdl/SDL.hpp"

namespace MFA::RenderFrontend {

namespace RB = RenderBackend;
using ScreenWidth = RB::ScreenWidth;
using ScreenHeight = RB::ScreenHeight;

using ResizeEventListener = std::function<void()>;

struct InitParams {
    ScreenWidth screen_width;
    ScreenHeight screen_height;
    char const * application_name;
    bool resizable = true;
};

bool Init(InitParams const & params);
bool Shutdown();

void SetResizeEventListener(ResizeEventListener const & eventListener);

using DrawPipeline = RB::GraphicPipelineGroup;

[[nodiscard]]
VkDescriptorSetLayout_T * CreateBasicDescriptorSetLayout();

[[nodiscard]]
VkDescriptorSetLayout_T * CreateDescriptorSetLayout(
    uint8_t bindingsCount,
    VkDescriptorSetLayoutBinding * bindings
);

void DestroyDescriptorSetLayout(VkDescriptorSetLayout_T * descriptorSetLayout);

[[nodiscard]]
DrawPipeline CreateBasicDrawPipeline(
    uint8_t gpuShadersCount, 
    RB::GpuShader * gpuShaders,
    uint32_t descriptorSetLayoutCount,
    VkDescriptorSetLayout_T ** descriptorSetLayouts,
    VkVertexInputBindingDescription const & vertexInputBindingDescription,
    uint8_t vertexInputAttributeDescriptionCount,
    VkVertexInputAttributeDescription * vertexInputAttributeDescriptions
);

[[nodiscard]]
DrawPipeline CreateDrawPipeline(
    uint8_t gpu_shaders_count, 
    RB::GpuShader * gpu_shaders,
    uint32_t descriptor_layouts_count,
    VkDescriptorSetLayout_T ** descriptor_set_layouts,
    VkVertexInputBindingDescription vertex_binding_description,
    uint32_t input_attribute_description_count,
    VkVertexInputAttributeDescription * input_attribute_description_data,
    RB::CreateGraphicPipelineOptions const & options = RB::CreateGraphicPipelineOptions {}
);

void DestroyDrawPipeline(DrawPipeline & draw_pipeline);

[[nodiscard]]
std::vector<VkDescriptorSet_T *> CreateDescriptorSets(
    VkDescriptorSetLayout_T * descriptor_set_layout
);

[[nodiscard]]
std::vector<VkDescriptorSet_T *> CreateDescriptorSets(
    uint32_t descriptorSetCount,
    VkDescriptorSetLayout_T * descriptorSetLayout
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
    VkSampler_T * sampler;
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
    bool isValid() const noexcept {
        if (bufferSize <= 0 || buffers.empty() == true) {
            return false;    
        }
        for (auto const & buffer : buffers) {
            if (buffer.isValid() == false) {
                return false;
            }
        }
        return true;
    }
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
    VkDescriptorSet_T * descriptorSet,
    UniformBufferGroup const & uniformBuffer,
    RB::GpuTexture const & gpuTexture,
    SamplerGroup const & samplerGroup
);

void UpdateDescriptorSetBasic(
    DrawPass const & drawPass,
    VkDescriptorSet_T * descriptorSet,
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
    VkDescriptorSet_T ** descriptorSets,
    uint8_t writeInfoCount,
    VkWriteDescriptorSet * writeInfo
);

void BindDescriptorSet(
    DrawPass const & drawPass,
    VkDescriptorSet_T * descriptorSet
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

// SDL Functions

void WarpMouseInWindow(int32_t x, int32_t y);

uint32_t GetMouseState(int32_t * x, int32_t * y);

uint8_t const * GetKeyboardState(int * numKeys = nullptr);

[[nodiscard]]
uint32_t GetWindowFlags();

void GetWindowSize(int32_t & out_width, int32_t & out_height);

void GetDrawableSize(int32_t & out_width, int32_t & out_height);

void AssignViewportAndScissorToCommandBuffer(
    VkCommandBuffer_T * commandBuffer
);

using EventWatchId = int;
using EventWatch = std::function<int(void* data, MSDL::SDL_Event* event)>;

EventWatchId AddEventWatch(EventWatch const & eventWatch);

void RemoveEventWatch(EventWatchId watchId);

}