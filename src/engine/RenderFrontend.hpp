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
    VkDescriptorSetLayout_T * descriptor_set_layout = nullptr;
};
[[nodiscard]]
DrawPipeline CreateBasicDrawPipeline(
    U8 gpu_shaders_count, 
    RB::GpuShader * gpu_shaders
);

// TODO AdvancedDrawPipeline support in future

void DestroyDrawPipeline(DrawPipeline & draw_pipeline);

[[nodiscard]]
std::vector<VkDescriptorSet_T *> CreateDescriptorSetsForDrawPipeline(DrawPipeline & draw_pipeline);

void BindBasicDescriptorSetWriteInfo(
    U8 descriptor_sets_count,
    VkDescriptorSet_T ** descriptor_sets,
    VkDescriptorBufferInfo const & buffer_info,
    VkDescriptorImageInfo const & image_info
);

// TODO AdvanceBindingDescriptorSetWriteInfo

struct UniformBufferGroup {
    std::vector<RB::BufferGroup> buffers;
    size_t buffer_size;
};
UniformBufferGroup CreateUniformBuffer(size_t buffer_size);

void BindDataToUniformBuffer(
    UniformBufferGroup const & uniform_buffer, 
    Blob data,
    U8 current_image_index
);

void DestroyUniformBuffer(UniformBufferGroup & uniform_buffer);

struct MeshBuffers {
    RB::BufferGroup vertices_buffer;
    RB::BufferGroup indices_buffer;
    U32 indices_count;
};
[[nodiscard]]
MeshBuffers CreateMeshBuffers(Asset::MeshAsset const & mesh_asset);

void DestroyMeshBuffers(MeshBuffers & mesh_buffers);

[[nodiscard]]
RB::GpuTexture CreateTexture(Asset::TextureAsset & texture_asset);

void DestroyTexture(RB::GpuTexture & gpu_texture);

struct SamplerGroup {
    VkSampler_T * sampler;
};
// TODO We should ask for options here
[[nodiscard]]
SamplerGroup CreateSampler();

void DestroySampler(SamplerGroup const & sampler_group);

[[nodiscard]]
RB::GpuShader CreateShader(Asset::ShaderAsset const & shader_asset);

void DestroyShader(RB::GpuShader & gpu_shader);

struct DrawPass {
    U8 image_index;
    bool is_valid = false;
};
[[nodiscard]]
DrawPass BeginPass();

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
void DrawTexturedMesh(
    DrawPass const & draw_pass,
    DrawPipeline & draw_pipeline,
    UniformBufferGroup const & uniform_buffer,
    MeshBuffers const & mesh_buffers,
    RB::GpuTexture const & texture,
    SamplerGroup const & sampler,
    U8 current_image_index
);

void EndPass(DrawPass & draw_pass);

}