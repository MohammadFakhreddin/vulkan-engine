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
    U8 bindings_count,
    VkDescriptorSetLayoutBinding * bindings
);

void DestroyDescriptorSetLayout(VkDescriptorSetLayout_T * descriptor_set_layout);

[[nodiscard]]
DrawPipeline CreateBasicDrawPipeline(
    U8 gpu_shaders_count, 
    RB::GpuShader * gpu_shaders,
    VkDescriptorSetLayout_T * descriptor_set_layout

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

void UpdateDescriptorSetsBasic(
    U8 descriptor_sets_count,
    VkDescriptorSet_T ** descriptor_sets,
    VkDescriptorBufferInfo const & buffer_info,
    VkDescriptorImageInfo const & image_info
);

void UpdateDescriptorSets(
    U8 descriptor_sets_count,
    VkDescriptorSet_T ** descriptor_sets,
    U8 write_info_count,
    VkWriteDescriptorSet * write_info
);

// TODO AdvanceBindingDescriptorSetWriteInfo

struct MeshBuffers {
    RB::BufferGroup vertices_buffer;
    RB::BufferGroup indices_buffer;
    U32 indices_count;
    Asset::MeshAsset mesh_asset {};
};

[[nodiscard]]
MeshBuffers CreateMeshBuffers(Asset::MeshAsset const & mesh_asset);

void DestroyMeshBuffers(MeshBuffers & mesh_buffers);

[[nodiscard]]
RB::BufferGroup CreateVertexBuffer(CBlob vertices_blob);

[[nodiscard]]
RB::BufferGroup CreateIndexBuffer(CBlob indices_blob);

[[nodiscard]]
RB::GpuTexture CreateTexture(Asset::TextureAsset & texture_asset);

void DestroyTexture(RB::GpuTexture & gpu_texture);

struct SamplerGroup {
    VkSampler_T * sampler;
};
// TODO We should ask for options here
[[nodiscard]]
SamplerGroup CreateSampler(RB::CreateSamplerParams const & sampler_params = {});

void DestroySampler(SamplerGroup & sampler_group);

void DeviceWaitIdle();

[[nodiscard]]
RB::GpuShader CreateShader(Asset::ShaderAsset const & shader_asset);

void DestroyShader(RB::GpuShader & gpu_shader);

struct DrawPass {
    U8 image_index;
    U8 frame_index;
    bool is_valid = false;
    DrawPipeline * draw_pipeline = nullptr;
};

struct UniformBufferGroup {
    std::vector<RB::BufferGroup> buffers;
    size_t buffer_size;
};
UniformBufferGroup CreateUniformBuffer(size_t buffer_size);

void UpdateUniformBuffer(
    DrawPass const & draw_pass,
    UniformBufferGroup const & uniform_buffer, 
    CBlob data
);

void DestroyUniformBuffer(UniformBufferGroup & uniform_buffer);

[[nodiscard]]
DrawPass BeginPass();

void BindDrawPipeline(
    DrawPass & draw_pass,
    DrawPipeline & draw_pipeline   
);

void UpdateDescriptorSetsBasic(
    DrawPass const & draw_pass,
    VkDescriptorSet_T ** descriptor_sets,
    UniformBufferGroup const & uniform_buffer,
    RB::GpuTexture const & gpu_texture,
    SamplerGroup const & sampler_group
);

void BindDescriptorSets(
    DrawPass const & draw_pass,
    VkDescriptorSet_T ** descriptor_sets
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
    DrawPass draw_pass, 
    RB::BufferGroup vertex_buffer,
    VkDeviceSize offset = 0
);

void BindIndexBuffer(
    DrawPass draw_pass, 
    RB::BufferGroup index_buffer,
    VkDeviceSize offset = 0,
    VkIndexType index_type = VK_INDEX_TYPE_UINT32
);

void DrawIndexed(
    DrawPass draw_pass, 
    U32 indices_count,
    U32 instance_count = 1,
    U32 first_index = 0,
    U32 vertex_offset = 0,
    U32 first_instance = 0
);

void EndPass(DrawPass & draw_pass);

void SetScissor(DrawPass const & draw_pass, VkRect2D const & scissor);

void SetViewport(DrawPass const & draw_pass, VkViewport const & viewport);

void PushConstants(
    DrawPass const & draw_pass, 
    Asset::ShaderStage shader_stage, 
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