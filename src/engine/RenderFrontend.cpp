#include "RenderFrontend.hpp"

#include <string>

#include "BedrockAssert.hpp"
#include "RenderBackend.hpp"
#include "BedrockLog.hpp"
#include "../libs/imgui/imgui.h"

#include <string>

namespace MFA::RenderFrontend {

static constexpr int MAX_FRAMES_IN_FLIGHT = 2;

struct State {
    // CreateWindow
    ScreenWidth screen_width = 0;
    ScreenHeight screen_height = 0;
    // CreateInstance
    std::string application_name;
    VkInstance vk_instance {};
    // CreateWindow
    SDL_Window * window {};
    // CreateDebugCallback
    VkDebugReportCallbackEXT vk_debug_report_callback_ext = nullptr;
    VkSurfaceKHR_T * surface = nullptr;
    VkPhysicalDevice_T * physical_device = nullptr;
    VkPhysicalDeviceFeatures physical_device_features {};
    U32 graphic_queue_family;
    U32 present_queue_family;
    VkQueue_T * graphic_queue = nullptr;
    VkQueue_T * present_queue = nullptr;
    RB::LogicalDevice logical_device {};
    VkCommandPool_T * graphic_command_pool = nullptr;
    RB::SwapChainGroup swap_chain_group {};
    VkRenderPass_T * render_pass = nullptr;
    RB::DepthImageGroup depth_image_group {};
    std::vector<VkFramebuffer_T *> frame_buffers {};
    VkDescriptorPool_T * descriptor_pool {};
    std::vector<VkCommandBuffer_T *> graphic_command_buffers {};
    RB::SyncObjects sync_objects {};
    U8 current_frame = 0;
} static state {};

static VkBool32 DebugCallback(
  VkDebugReportFlagsEXT const flags,
  VkDebugReportObjectTypeEXT object_type,
  uint64_t src_object, 
  size_t location,
  int32_t const message_code,
  char const * player_prefix,
  char const * message,
  void * user_data
) {
    if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT) {
        MFA_LOG_ERROR("Message code: %d\nMessage: %s\n", message_code, message);
    } else if(flags & VK_DEBUG_REPORT_WARNING_BIT_EXT) {
        MFA_LOG_WARN("Message code: %d\nMessage: %s\n", message_code, message);
    } else {
        MFA_LOG_INFO("Message code: %d\nMessage: %s\n", message_code, message);
    }
    MFA_DEBUG_CRASH("Vulkan debug event");
    return true;
}

bool Init(InitParams const & params) {
    {
        state.application_name = params.application_name;
        state.screen_width = params.screen_width;
        state.screen_height = params.screen_height;
    }
    state.window = RB::CreateWindow(
        state.screen_width, 
        state.screen_height
    );
    state.vk_instance = RB::CreateInstance(
        state.application_name.c_str(), 
        state.window
    );
#ifdef MFA_DEBUG
    state.vk_debug_report_callback_ext = RB::CreateDebugCallback(
        state.vk_instance,
        DebugCallback
    );
#endif
    state.surface = RB::CreateWindowSurface(state.window, state.vk_instance);
    {
        auto const find_physical_device_result = RB::FindPhysicalDevice(state.vk_instance); // TODO Check again for retry count number
        state.physical_device = find_physical_device_result.physical_device;
        state.physical_device_features = find_physical_device_result.physical_device_features;
    }
    if(false == RB::CheckSwapChainSupport(state.physical_device)) {
        MFA_LOG_ERROR("Swapchain is not supported on this device");
        return false;
    }
    {
        auto const find_queue_family_result = RB::FindPresentAndGraphicQueueFamily(state.physical_device, state.surface);
        state.graphic_queue_family = find_queue_family_result.graphic_queue_family;
        state.present_queue_family = find_queue_family_result.present_queue_family;
    }
    state.logical_device = RB::CreateLogicalDevice(
        state.physical_device,
        state.graphic_queue_family,
        state.present_queue_family,
        state.physical_device_features
    );
    // Get graphics and presentation queues (which may be the same)
    state.graphic_queue = RB::GetQueueByFamilyIndex(
        state.logical_device.device, 
        state.graphic_queue_family
    );
    state.present_queue = RB::GetQueueByFamilyIndex(
        state.logical_device.device, 
        state.present_queue_family
    );
    MFA_LOG_INFO("Acquired graphics and presentation queues");
    state.graphic_command_pool = RB::CreateCommandPool(state.logical_device.device, state.graphic_queue_family);
    VkExtent2D const swap_chain_extent = {
        .width = state.screen_width,
        .height = state.screen_height
    };
    // Creating sampler/TextureImage/VertexBuffer and IndexBuffer + graphic_pipeline is on application level
    state.swap_chain_group = RB::CreateSwapChain(
        state.logical_device.device,
        state.physical_device,
        state.surface,
        swap_chain_extent
    );
    //// TODO We might need special/multiple render passes in future
    state.render_pass = RB::CreateRenderPass(
        state.physical_device,
        state.logical_device.device,
        state.swap_chain_group.swap_chain_format
    );
    state.depth_image_group = RB::CreateDepth(
        state.physical_device,
        state.logical_device.device,
        swap_chain_extent
    );
    //// TODO Q1: Where do we need frame-buffers ? Q2: Maybe we can create it same time as render pass
    state.frame_buffers = RB::CreateFrameBuffers(
        state.logical_device.device,
        state.render_pass,
        static_cast<U8>(state.swap_chain_group.swap_chain_image_views.size()),
        state.swap_chain_group.swap_chain_image_views.data(),
        state.depth_image_group.image_view,
        swap_chain_extent
    );
    state.descriptor_pool = RB::CreateDescriptorPool(
        state.logical_device.device, 
        1000//static_cast<U8>(state.swap_chain_group.swap_chain_images.size())  // TODO We might need to ask this from user
    );
    state.graphic_command_buffers = RB::CreateCommandBuffers(
        state.logical_device.device,
        static_cast<U8>(state.swap_chain_group.swap_chain_images.size()),
        state.graphic_command_pool
    );
    state.sync_objects = RB::CreateSyncObjects(
        state.logical_device.device,
        MAX_FRAMES_IN_FLIGHT,
        static_cast<U8>(state.swap_chain_group.swap_chain_images.size())
    );
    return true;
}

bool Shutdown() {
    // Common part with resize
    RB::DeviceWaitIdle(state.logical_device.device);
    // DestroyPipeline in application // TODO We should have reference to what user creates + params for re-creation
    // GraphicPipeline, UniformBuffer, PipelineLayout
    // Shutdown only procedure
    RB::DestroySyncObjects(state.logical_device.device, state.sync_objects);
    RB::DestroyCommandBuffers(
        state.logical_device.device, 
        state.graphic_command_pool,
        static_cast<U8>(state.graphic_command_buffers.size()),
        state.graphic_command_buffers.data()
    );
    RB::DestroyDescriptorPool(
        state.logical_device.device,
        state.descriptor_pool
    );
    RB::DestroyFrameBuffers(
        state.logical_device.device,
        static_cast<U8>(state.frame_buffers.size()),
        state.frame_buffers.data()
    );
    RB::DestroyDepth(
        state.logical_device.device,
        state.depth_image_group
    );
    RB::DestroyRenderPass(state.logical_device.device, state.render_pass);
    RB::DestroySwapChain(
        state.logical_device.device,
        state.swap_chain_group
    );
    RB::DestroyCommandPool(
        state.logical_device.device,
        state.graphic_command_pool
    );
    RB::DestroyLogicalDevice(state.logical_device);
    RB::DestroyWindowSurface(state.vk_instance, state.surface);
#ifdef MFA_DEBUG
    RB::DestroyDebugReportCallback(state.vk_instance, state.vk_debug_report_callback_ext);
#endif
    RB::DestroyInstance(state.vk_instance);
    return true;
}

bool Resize(ScreenWidth screen_width, ScreenHeight screen_height) {
    // TODO
    MFA_NOT_IMPLEMENTED_YET("Mohammad Fakhreddin");
}

VkDescriptorSetLayout_T * CreateBasicDescriptorSetLayout() {
    // Describe pipeline layout
    // Note: this describes the mapping between memory and shader resources (descriptor sets)
    // This is for uniform buffers and samplers
    VkDescriptorSetLayoutBinding uboLayoutBinding {};
    uboLayoutBinding.binding = 0;
    // Our object type is uniformBuffer
    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    // Number of values in buffer
    uboLayoutBinding.descriptorCount = 1;
    // Which shader stage we are going to reffer to // In our case we only need vertex shader to access to it
    uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    // For Image sampler, Currently we do not need this
    uboLayoutBinding.pImmutableSamplers = nullptr; // Optional

    VkDescriptorSetLayoutBinding sampler_layout_binding{};
    sampler_layout_binding.binding = 1;
    sampler_layout_binding.descriptorCount = 1;
    sampler_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    sampler_layout_binding.pImmutableSamplers = nullptr;
    sampler_layout_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    std::vector<VkDescriptorSetLayoutBinding> descriptor_set_layout_bindings {
        uboLayoutBinding,
        sampler_layout_binding
    };

    return CreateDescriptorSetLayout(
        static_cast<U8>(descriptor_set_layout_bindings.size()),
        descriptor_set_layout_bindings.data()
    );
}

VkDescriptorSetLayout_T * CreateDescriptorSetLayout(
    U8 const bindings_count, 
    VkDescriptorSetLayoutBinding * bindings
) {
    auto * descriptor_set_layout = RB::CreateDescriptorSetLayout(
        state.logical_device.device,
        bindings_count,
        bindings
    );
    MFA_PTR_ASSERT(descriptor_set_layout);

    return descriptor_set_layout;
}


void DestroyDescriptorSetLayout(VkDescriptorSetLayout_T * descriptor_set_layout) {
    MFA_PTR_ASSERT(descriptor_set_layout);
    RB::DestroyDescriptorSetLayout(state.logical_device.device, descriptor_set_layout);
}

DrawPipeline CreateBasicDrawPipeline(
    U8 const gpu_shaders_count, 
    RB::GpuShader * gpu_shaders,
    VkDescriptorSetLayout_T * descriptor_set_layout,
    VkVertexInputBindingDescription const & vertex_input_binding_description,
    U8 vertex_input_attribute_description_count,
    VkVertexInputAttributeDescription * vertex_input_attribute_descriptions
) {
    MFA_ASSERT(gpu_shaders_count > 0);
    MFA_PTR_VALID(gpu_shaders);
    MFA_PTR_VALID(descriptor_set_layout);
    //VkVertexInputBindingDescription const vertex_binding_description {
    //    .binding = 0,
    //    .stride = sizeof(AssetSystem::MeshVertex),
    //    .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
    //};
    //std::vector<VkVertexInputAttributeDescription> input_attribute_descriptions {4};
    //input_attribute_descriptions[0] = VkVertexInputAttributeDescription {
    //    .location = 0,
    //    .binding = 0,
    //    .format = VK_FORMAT_R32G32B32_SFLOAT,
    //    .offset = offsetof(AssetSystem::MeshVertex, position),
    //};
    //input_attribute_descriptions[1] = VkVertexInputAttributeDescription {
    //    .location = 1,
    //    .binding = 0,
    //    .format = VK_FORMAT_R32G32B32_SFLOAT,
    //    .offset = offsetof(AssetSystem::MeshVertex, normal_map_uv)
    //};
    //input_attribute_descriptions[2] = VkVertexInputAttributeDescription {
    //    .location = 2,
    //    .binding = 0,
    //    .format = VK_FORMAT_R32G32_SFLOAT,
    //    .offset = offsetof(AssetSystem::MeshVertex, base_color_uv)
    //};
    //input_attribute_descriptions[3] = VkVertexInputAttributeDescription {
    //    .location = 3,
    //    .binding = 0,
    //    .format = VK_FORMAT_R32G32_SFLOAT,
    //    .offset = offsetof(AssetSystem::MeshVertex, color)
    //};

    return CreateDrawPipeline(
        gpu_shaders_count,
        gpu_shaders,
        descriptor_set_layout,
        vertex_input_binding_description,
        vertex_input_attribute_description_count,
        vertex_input_attribute_descriptions,
        RB::CreateGraphicPipelineOptions {
            .depth_stencil {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
                .depthTestEnable = VK_TRUE,
                .depthWriteEnable = VK_TRUE,
                .depthCompareOp = VK_COMPARE_OP_LESS,
                .depthBoundsTestEnable = VK_FALSE,
                .stencilTestEnable = VK_FALSE
            },
            .color_blend_attachments {
                .blendEnable = VK_TRUE,
                .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
                .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
                .colorBlendOp = VK_BLEND_OP_ADD,
                .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
                .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
                .alphaBlendOp = VK_BLEND_OP_ADD,
                .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
            },
            .use_static_viewport_and_scissor = true
        }
    );
}

[[nodiscard]]
DrawPipeline CreateDrawPipeline(
    U8 const gpu_shaders_count, 
    RB::GpuShader * gpu_shaders,
    VkDescriptorSetLayout_T * descriptor_set_layout,
    VkVertexInputBindingDescription const vertex_binding_description,
    U32 const input_attribute_description_count,
    VkVertexInputAttributeDescription * input_attribute_description_data,
    RB::CreateGraphicPipelineOptions const & options
) {
    auto const graphic_pipeline_group = RB::CreateGraphicPipeline(
        state.logical_device.device,
        gpu_shaders_count,
        gpu_shaders,
        vertex_binding_description,
        static_cast<U32>(input_attribute_description_count),
        input_attribute_description_data,
        VkExtent2D {.width = state.screen_width, .height = state.screen_height},
        state.render_pass,
        descriptor_set_layout,
        options
    );

    return DrawPipeline {
        .graphic_pipeline_group = graphic_pipeline_group,
    };
}

void DestroyDrawPipeline(DrawPipeline & draw_pipeline) {
    RB::DestroyGraphicPipeline(
        state.logical_device.device, 
        draw_pipeline.graphic_pipeline_group
    );
}

std::vector<VkDescriptorSet_T *> CreateDescriptorSets(
    VkDescriptorSetLayout_T * descriptor_set_layout
) {
    return RB::CreateDescriptorSet(
        state.logical_device.device,
        state.descriptor_pool,
        descriptor_set_layout,
        static_cast<U8>(state.swap_chain_group.swap_chain_images.size())
    );
}

std::vector<VkDescriptorSet_T *> CreateDescriptorSets(
    U32 const descriptor_set_count,
    VkDescriptorSetLayout_T * descriptor_set_layout
) {
    MFA_PTR_ASSERT(descriptor_set_layout);
    return RB::CreateDescriptorSet(
        state.logical_device.device,
        state.descriptor_pool,
        descriptor_set_layout,
        descriptor_set_count
    );
}

UniformBufferGroup CreateUniformBuffer(size_t const buffer_size, U8 const count) {
    auto const buffers = RB::CreateUniformBuffer(
        state.logical_device.device,
        state.physical_device,
        count,
        buffer_size
    );
    return {
        .buffers = buffers,
        .buffer_size = buffer_size
    };
}

void UpdateUniformBuffer(
    DrawPass const & draw_pass,
    UniformBufferGroup const & uniform_buffer, 
    CBlob const data
) {
    RB::UpdateUniformBuffer(
        state.logical_device.device, 
        uniform_buffer.buffers[draw_pass.image_index], 
        data
    );
}

void UpdateUniformBuffer(
    RB::BufferGroup const & uniform_buffer, 
    CBlob const data
) {
    RB::UpdateUniformBuffer(
        state.logical_device.device, 
        uniform_buffer, 
        data
    );
}

void DestroyUniformBuffer(UniformBufferGroup & uniform_buffer) {
    for(auto & buffer_group : uniform_buffer.buffers) {
        RB::DestroyUniformBuffer(
            state.logical_device.device,
            buffer_group
        );
    }
}

RB::BufferGroup CreateVertexBuffer(CBlob const vertices_blob) {
    return RB::CreateVertexBuffer(
        state.logical_device.device,
        state.physical_device,
        state.graphic_command_pool,
        state.graphic_queue,
        vertices_blob
    );
}

RB::BufferGroup CreateIndexBuffer(CBlob const indices_blob) {
    return RB::CreateIndexBuffer(
        state.logical_device.device,
        state.physical_device,
        state.graphic_command_pool,
        state.graphic_queue,
        indices_blob
    );
}

MeshBuffers CreateMeshBuffers(AssetSystem::MeshAsset const & mesh_asset) {
    MFA_ASSERT(mesh_asset.valid());
    MeshBuffers buffers {.sub_mesh_buffers {}};
    auto const header_blob = mesh_asset.header_blob();
    auto const * header_object = header_blob.as<AssetSystem::MeshHeader>();
    auto const header_size = mesh_asset.compute_header_size();
    buffers.indices_buffer = CreateIndexBuffer(mesh_asset.indices_cblob());
    buffers.vertices_buffer = CreateVertexBuffer(mesh_asset.vertices_cblob());
    for (U32 i = 0; i < header_object->sub_mesh_count; i++) {
        auto const & current_sub_mesh = header_object->sub_meshes[i];
        buffers.sub_mesh_buffers.emplace_back(MeshBuffers::DrawCallData {
            .vertex_offset = static_cast<U64>(current_sub_mesh.vertices_offset - header_size),
            .index_offset = static_cast<U64>(current_sub_mesh.indices_offset - header_size),
            .index_count = header_object->sub_meshes[i].index_count,
        });
    }
    return buffers;
}

void DestroyMeshBuffers(MeshBuffers & mesh_buffers) {
    RB::DestroyVertexBuffer(state.logical_device.device, mesh_buffers.vertices_buffer);
    RB::DestroyIndexBuffer(state.logical_device.device, mesh_buffers.indices_buffer);
    mesh_buffers.sub_mesh_buffers.resize(0);
}

RB::GpuTexture CreateTexture(AssetSystem::TextureAsset & texture_asset) {
    auto const gpu_texture = RB::CreateTexture(
        texture_asset,
        state.logical_device.device,
        state.physical_device,
        state.graphic_queue,
        state.graphic_command_pool
    );
    MFA_ASSERT(gpu_texture.valid());
    return gpu_texture;
}

void DestroyTexture(RB::GpuTexture & gpu_texture) {
    RB::DestroyTexture(state.logical_device.device, gpu_texture);
}

// TODO Ask for options
SamplerGroup CreateSampler(RB::CreateSamplerParams const & sampler_params) {
    auto * sampler = RB::CreateSampler(state.logical_device.device, sampler_params);
    MFA_PTR_ASSERT(sampler);
    return {
        .sampler = sampler
    };
}

void DestroySampler(SamplerGroup & sampler_group) {
    RB::DestroySampler(state.logical_device.device, sampler_group.sampler);
    sampler_group.sampler = nullptr;
}

GpuModel CreateGpuModel(AssetSystem::ModelAsset & model_asset) {
    GpuModel gpu_model {
        .valid = true,
        .mesh_buffers = CreateMeshBuffers(model_asset.mesh),
        .textures {},
        .model_asset = model_asset
    };
    if(false == model_asset.textures.empty()) {
        for (auto & texture_asset : model_asset.textures) {
            gpu_model.textures.emplace_back(CreateTexture(texture_asset));
            MFA_ASSERT(gpu_model.textures.back().valid());
        }
    }
    return gpu_model;
}

void DestroyGpuModel(GpuModel & gpu_model) {
    MFA_ASSERT(gpu_model.valid);
    gpu_model.valid = false;
    DestroyMeshBuffers(gpu_model.mesh_buffers);
    if(false == gpu_model.textures.empty()) {
        for (auto & gpu_texture : gpu_model.textures) {
            DestroyTexture(gpu_texture);
        }
    }
}

void DeviceWaitIdle() {
    RB::DeviceWaitIdle(state.logical_device.device);
}

[[nodiscard]]
DrawPass BeginPass() {
    MFA_ASSERT(MAX_FRAMES_IN_FLIGHT > state.current_frame);
    DrawPass draw_pass {};
    RB::WaitForFence(
        state.logical_device.device, 
        state.sync_objects.fences_in_flight[state.current_frame]
    );
    draw_pass.image_index = RB::AcquireNextImage(
        state.logical_device.device,
        state.sync_objects.image_availability_semaphores[state.current_frame],
        state.swap_chain_group
    );
    draw_pass.frame_index = state.current_frame;
    draw_pass.is_valid = true;
    state.current_frame ++;
    if(state.current_frame >= MAX_FRAMES_IN_FLIGHT) {
        state.current_frame = 0;
    }
    // Recording command buffer data at each render frame
    // We need 1 renderPass and multiple command buffer recording
    // Each pipeline has its own set of shader, But we can reuse a pipeline for multiple shaders.
    // For each model we need to record command buffer with our desired pipeline (For example light and objects have different fragment shader)
    // Prepare data for recording command buffers
    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
    {
        auto result = vkBeginCommandBuffer(state.graphic_command_buffers[draw_pass.image_index], &beginInfo);
        if(VK_SUCCESS != result) {
            MFA_CRASH("vkBeginCommandBuffer failed with error code %d,", result);
        }
    }
    // If present queue family and graphics queue family are different, then a barrier is necessary
    // The barrier is also needed initially to transition the image to the present layout
    VkImageMemoryBarrier presentToDrawBarrier = {};
    presentToDrawBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    presentToDrawBarrier.srcAccessMask = 0;
    presentToDrawBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    presentToDrawBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    presentToDrawBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    if (state.present_queue_family != state.graphic_queue_family) {
        presentToDrawBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        presentToDrawBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    }
    else {
        presentToDrawBarrier.srcQueueFamilyIndex = state.present_queue_family;
        presentToDrawBarrier.dstQueueFamilyIndex = state.graphic_queue_family;
    }

    presentToDrawBarrier.image = state.swap_chain_group.swap_chain_images[draw_pass.image_index];

    VkImageSubresourceRange subResourceRange = {};
    subResourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    subResourceRange.baseMipLevel = 0;
    subResourceRange.levelCount = 1;
    subResourceRange.baseArrayLayer = 0;
    subResourceRange.layerCount = 1;
    presentToDrawBarrier.subresourceRange = subResourceRange;

    vkCmdPipelineBarrier(
        state.graphic_command_buffers[draw_pass.image_index], 
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 
        0, 0, nullptr, 0, nullptr, 1, 
        &presentToDrawBarrier
    );

    std::vector<VkClearValue> clearValues{};
    clearValues.resize(2);
    clearValues[0].color = { 0.1f, 0.1f, 0.1f, 1.0f };
    clearValues[1].depthStencil = {1.0f, 0};

    VkRenderPassBeginInfo renderPassBeginInfo = {};
    renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassBeginInfo.renderPass = state.render_pass;
    renderPassBeginInfo.framebuffer = state.frame_buffers[draw_pass.image_index];
    renderPassBeginInfo.renderArea.offset.x = 0;
    renderPassBeginInfo.renderArea.offset.y = 0;
    renderPassBeginInfo.renderArea.extent = VkExtent2D {.width = state.screen_width, .height = state.screen_height};
    renderPassBeginInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassBeginInfo.pClearValues = clearValues.data();

    vkCmdBeginRenderPass(state.graphic_command_buffers[draw_pass.image_index], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

    return draw_pass;
}

void BindDrawPipeline(
    DrawPass & draw_pass,
    DrawPipeline & draw_pipeline   
) {
    MFA_ASSERT(draw_pass.is_valid);
    draw_pass.draw_pipeline = &draw_pipeline;
    // We can bind command buffer to multiple pipeline
    vkCmdBindPipeline(
        state.graphic_command_buffers[draw_pass.image_index], 
        VK_PIPELINE_BIND_POINT_GRAPHICS, 
        draw_pipeline.graphic_pipeline_group.graphic_pipeline
    );
}
// TODO Remove all basic functions
void UpdateDescriptorSetBasic(
    DrawPass const & draw_pass,
    VkDescriptorSet_T * descriptor_set,
    UniformBufferGroup const & uniform_buffer,
    RB::GpuTexture const & gpu_texture,
    SamplerGroup const & sampler_group
) {
    VkDescriptorImageInfo image_info {};
    image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    image_info.imageView = gpu_texture.image_view();
    image_info.sampler = sampler_group.sampler;

    UpdateDescriptorSetBasic(
        draw_pass,
        descriptor_set,
        uniform_buffer,
        1,
        &image_info
    );
}

void UpdateDescriptorSetBasic(
    DrawPass const & draw_pass,
    VkDescriptorSet_T * descriptor_set,
    UniformBufferGroup const & uniform_buffer,
    U32 const image_info_count,
    VkDescriptorImageInfo const * image_infos
) {
    VkDescriptorBufferInfo buffer_info {};
    buffer_info.buffer = uniform_buffer.buffers[draw_pass.image_index].buffer;
    buffer_info.offset = 0;
    buffer_info.range = uniform_buffer.buffer_size;

    RB::UpdateDescriptorSetsBasic(
        state.logical_device.device,
        1,
        &descriptor_set,
        buffer_info,
        image_info_count,
        image_infos
    );
}

void UpdateDescriptorSets(
    U8 const write_info_count,
    VkWriteDescriptorSet * write_info
) {
    RB::UpdateDescriptorSets(
        state.logical_device.device,
        write_info_count,
        write_info
    );
}

void UpdateDescriptorSets(
    U8 const descriptor_sets_count,
    VkDescriptorSet_T ** descriptor_sets,
    U8 const write_info_count,
    VkWriteDescriptorSet * write_info
) {
    RB::UpdateDescriptorSets(
        state.logical_device.device,
        descriptor_sets_count,
        descriptor_sets,
        write_info_count,
        write_info
    );
}

void BindDescriptorSet(
    DrawPass const & draw_pass,
    VkDescriptorSet_T * descriptor_set
) {
    MFA_ASSERT(draw_pass.is_valid);
    MFA_PTR_ASSERT(draw_pass.draw_pipeline);
    MFA_PTR_ASSERT(descriptor_set);
    // We should bind specific descriptor set with different texture for each mesh
    vkCmdBindDescriptorSets(
        state.graphic_command_buffers[draw_pass.image_index], 
        VK_PIPELINE_BIND_POINT_GRAPHICS, 
        draw_pass.draw_pipeline->graphic_pipeline_group.pipeline_layout, 
        0, 
        1, 
        &descriptor_set, 
        0, 
        nullptr
    );
}

void BindVertexBuffer(
    DrawPass const draw_pass, 
    RB::BufferGroup const vertex_buffer,
    VkDeviceSize const offset
) {
    MFA_ASSERT(draw_pass.is_valid);
    RB::BindVertexBuffer(
        state.graphic_command_buffers[draw_pass.image_index], 
        vertex_buffer,
        offset
    );
}

void BindIndexBuffer(
    DrawPass const draw_pass,
    RB::BufferGroup const index_buffer,
    VkDeviceSize const offset,
    VkIndexType const index_type
) {
    MFA_ASSERT(draw_pass.is_valid);
    RB::BindIndexBuffer(
        state.graphic_command_buffers[draw_pass.image_index], 
        index_buffer,
        offset,
        index_type
    );
}

void DrawIndexed(
    DrawPass const draw_pass, 
    U32 const indices_count,
    U32 const instance_count,
    U32 const first_index,
    U32 const vertex_offset,
    U32 const first_instance
) {
    RB::DrawIndexed(
        state.graphic_command_buffers[draw_pass.image_index], 
        indices_count,
        instance_count,
        first_index,
        vertex_offset,
        first_instance
    );
}

void EndPass(DrawPass & draw_pass) {
    // TODO Move these functions to renderBackend, RenderFrontend should not know about backend
    MFA_ASSERT(draw_pass.is_valid);
    draw_pass.is_valid = false;
    vkCmdEndRenderPass(state.graphic_command_buffers[draw_pass.image_index]);

    // If present and graphics queue families differ, then another barrier is required
    if (state.present_queue_family != state.graphic_queue_family) {
        // TODO Check that WTF is this ?
        VkImageSubresourceRange subResourceRange = {};
        subResourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        subResourceRange.baseMipLevel = 0;
        subResourceRange.levelCount = 1;
        subResourceRange.baseArrayLayer = 0;
        subResourceRange.layerCount = 1;

        VkImageMemoryBarrier drawToPresentBarrier = {};
        drawToPresentBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        drawToPresentBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        drawToPresentBarrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
        drawToPresentBarrier.oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        drawToPresentBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        drawToPresentBarrier.srcQueueFamilyIndex = state.graphic_queue_family;
        drawToPresentBarrier.dstQueueFamilyIndex = state.present_queue_family;
        drawToPresentBarrier.image = state.swap_chain_group.swap_chain_images[draw_pass.image_index];
        drawToPresentBarrier.subresourceRange = subResourceRange;
        // TODO Move to RB
        vkCmdPipelineBarrier(
            state.graphic_command_buffers[draw_pass.image_index], 
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 
            VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 
            0, 0, nullptr, 
            0, nullptr, 1, 
            &drawToPresentBarrier
        );
    }

    if (vkEndCommandBuffer(state.graphic_command_buffers[draw_pass.image_index]) != VK_SUCCESS) {
        MFA_CRASH("Failed to record command buffer");
    }

        // Wait for image to be available and draw
    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore wait_semaphores[] = {state.sync_objects.image_availability_semaphores[draw_pass.frame_index]};
    VkPipelineStageFlags wait_stages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = wait_semaphores;
    submitInfo.pWaitDstStageMask = wait_stages;

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &state.graphic_command_buffers[draw_pass.image_index];

    VkSemaphore signal_semaphores[] = {state.sync_objects.render_finish_indicator_semaphores[draw_pass.frame_index]};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signal_semaphores;

    vkResetFences(state.logical_device.device, 1, &state.sync_objects.fences_in_flight[draw_pass.frame_index]);

    {
        auto const result = vkQueueSubmit(
            state.graphic_queue, 
            1, &submitInfo, 
            state.sync_objects.fences_in_flight[draw_pass.frame_index]
        );
        if (result != VK_SUCCESS) {
            MFA_CRASH("Failed to submit draw command buffer");
        }
    }
    // Present drawn image
    // Note: semaphore here is not strictly necessary, because commands are processed in submission order within a single queue
    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signal_semaphores;
    VkSwapchainKHR swapChains[] = {state.swap_chain_group.swap_chain};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    U32 imageIndices = draw_pass.image_index;
    presentInfo.pImageIndices = &imageIndices;

    {
        auto const res = vkQueuePresentKHR(state.present_queue, &presentInfo);
        // TODO Handle resize event
        if (res == VK_SUBOPTIMAL_KHR || res == VK_ERROR_OUT_OF_DATE_KHR){// || windowResized) {
            //onWindowSizeChanged();
        } else if (res != VK_SUCCESS) {
            MFA_CRASH("Failed to submit present command buffer");
        }
    }
}

[[nodiscard]]
RB::GpuShader CreateShader(AssetSystem::ShaderAsset const & shader_asset) {
    return RB::CreateShader(
        state.logical_device.device,
        shader_asset
    );
}

void DestroyShader(RB::GpuShader & gpu_shader) {
    RB::DestroyShader(state.logical_device.device, gpu_shader);
}

void SetScissor(DrawPass const & draw_pass, VkRect2D const & scissor) {
    RB::SetScissor(state.graphic_command_buffers[draw_pass.image_index], scissor);
}

void SetViewport(DrawPass const & draw_pass, VkViewport const & viewport) {
    RB::SetViewport(state.graphic_command_buffers[draw_pass.image_index], viewport);
}

void PushConstants(
    DrawPass const & draw_pass, 
    AssetSystem::ShaderStage const shader_stage, 
    U32 const offset, 
    CBlob const data
) {
    RB::PushConstants(
        state.graphic_command_buffers[draw_pass.image_index],
        draw_pass.draw_pipeline->graphic_pipeline_group.pipeline_layout,
        shader_stage,
        offset,
        data
    );
}

U8 SwapChainImagesCount() {
    return static_cast<U8>(state.swap_chain_group.swap_chain_images.size());
}

// SDL functions

void WarpMouseInWindow(I32 const x, I32 const y) {
    SDL_WarpMouseInWindow(state.window, x, y);
}

U32 GetWindowFlags() {
    return SDL_GetWindowFlags(state.window); 
}

void GetWindowSize(I32 & out_width, I32 & out_height) {
    SDL_GetWindowSize(state.window, &out_width, &out_height);
}

void GetDrawableSize(I32 & out_width, I32 & out_height) {
    SDL_GL_GetDrawableSize(state.window, &out_width, &out_height);
}

}
