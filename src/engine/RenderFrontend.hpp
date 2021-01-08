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

struct UniformBuffer {
    std::vector<RB::BufferGroup> buffers;
};
UniformBuffer CreateUniformBuffer(size_t buffer_size);

// TODO UpdateUniformBuffer

// TODO DestroyUniformBuffer

// TODO UpdateDescriptorSet

}