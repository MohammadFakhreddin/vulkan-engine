#pragma once

#include "RenderBackend.hpp"

namespace MFA::RenderFrontend {

namespace RB = RenderBackend;
using ScreenWidth = RB::ScreenWidth;
using ScreenHeight = RB::ScreenHeight;

struct InitParams {
    ScreenWidth screen_width;
    ScreenHeight screen_height;
    char const * application_name;
};

bool Init(InitParams const & params);
// TODO Implement this function
bool Shutdown();

bool Resize(ScreenWidth screen_width, ScreenHeight screen_height);

// TODO CreateDrawPipeline + asking for descriptor set layout required bindings
struct DrawPipeline {
    RB::GraphicPipelineGroup graphic_pipeline_group {};
};

[[nodiscard]]
VkDescriptorSetLayout_T * CreateBasicDescriptorSetLayout();

[[nodiscard]]
VkDescriptorSetLayout_T * CreateDescriptorSetLayout(
    U8 bindingsCount,
    VkDescriptorSetLayoutBinding * bindings
);

void DestroyDescriptorSetLayout(VkDescriptorSetLayout_T * descriptorSetLayout);

[[nodiscard]]
DrawPipeline CreateBasicDrawPipeline(
    U8 gpuShadersCount, 
    RB::GpuShader * gpuShaders,
    VkDescriptorSetLayout_T * descriptorSetLayout,
    VkVertexInputBindingDescription const & vertexInputBindingDescription,
    U8 vertexInputAttributeDescriptionCount,
    VkVertexInputAttributeDescription * vertexInputAttributeDescriptions
);

[[nodiscard]]
DrawPipeline CreateDrawPipeline(
    U8 gpu_shaders_count, 
    RB::GpuShader * gpu_shaders,
    VkDescriptorSetLayout_T * descriptor_set_layout,
    VkVertexInputBindingDescription vertex_binding_description,
    U32 input_attribute_description_count,
    VkVertexInputAttributeDescription * input_attribute_description_data,
    RB::CreateGraphicPipelineOptions const & options = {}
);

void DestroyDrawPipeline(DrawPipeline & draw_pipeline);

[[nodiscard]]
std::vector<VkDescriptorSet_T *> CreateDescriptorSets(
    VkDescriptorSetLayout_T * descriptor_set_layout
);

[[nodiscard]]
std::vector<VkDescriptorSet_T *> CreateDescriptorSets(
    U32 descriptorSetCount,
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
    U8 imageIndex;
    U8 frame_index;
    bool isValid = false;
    DrawPipeline * drawPipeline = nullptr;
};

struct UniformBufferGroup {
    std::vector<RB::BufferGroup> buffers;
    size_t buffer_size;
};
UniformBufferGroup CreateUniformBuffer(size_t bufferSize, U32 count);

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
    U32 imageInfoCount,
    VkDescriptorImageInfo const * imageInfos
);

void UpdateDescriptorSets(
    U8 writeInfoCount,
    VkWriteDescriptorSet * writeInfo
);

void UpdateDescriptorSets(
    U8 descriptorSetsCount,
    VkDescriptorSet_T ** descriptorSets,
    U8 writeInfoCount,
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
    U32 indicesCount,
    U32 instanceCount = 1,
    U32 firstIndex = 0,
    U32 vertexOffset = 0,
    U32 firstInstance = 0
);

void EndPass(DrawPass & drawPass);

void SetScissor(DrawPass const & draw_pass, VkRect2D const & scissor);

void SetViewport(DrawPass const & draw_pass, VkViewport const & viewport);

void PushConstants(
    DrawPass const & draw_pass, 
    AssetSystem::ShaderStage shader_stage, 
    U32 offset, 
    CBlob data
);

U8 SwapChainImagesCount();

// SDL Functions

void WarpMouseInWindow(I32 x, I32 y);

[[nodiscard]]
U32 GetWindowFlags();

void GetWindowSize(I32 & out_width, I32 & out_height);

void GetDrawableSize(I32 & out_width, I32 & out_height);

}