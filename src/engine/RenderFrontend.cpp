#include "RenderFrontend.hpp"

#include <string>

#include "BedrockAssert.hpp"
#include "RenderBackend.hpp"
#include "BedrockLog.hpp"

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
};

static State state {};

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
    state.vk_debug_report_callback_ext = RB::CreateDebugCallback(
        state.vk_instance,
        DebugCallback
    );
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
        static_cast<U8>(state.swap_chain_group.swap_chain_images.size())
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

DrawPipeline CreateBasicDrawPipeline(
    U8 const gpu_shaders_count, 
    RB::GpuShader * gpu_shaders
) {
    VkVertexInputBindingDescription const vertex_binding_description {
        .binding = 0,
        .stride = sizeof(Asset::MeshVertices::Vertex),
        .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
    };
    std::vector<VkVertexInputAttributeDescription> input_attribute_descriptions {4};
    input_attribute_descriptions[0] = VkVertexInputAttributeDescription {
        .location = 0,
        .binding = 0,
        .format = VK_FORMAT_R32G32B32_SFLOAT,
        .offset = offsetof(Asset::MeshVertices::Vertex, position),
    };
    input_attribute_descriptions[1] = VkVertexInputAttributeDescription {
        .location = 1,
        .binding = 0,
        .format = VK_FORMAT_R32G32B32_SFLOAT,
        .offset = offsetof(Asset::MeshVertices::Vertex, normal)
    };
    input_attribute_descriptions[2] = VkVertexInputAttributeDescription {
        .location = 2,
        .binding = 0,
        .format = VK_FORMAT_R32G32_SFLOAT,
        .offset = offsetof(Asset::MeshVertices::Vertex, uv)
    };
    input_attribute_descriptions[3] = VkVertexInputAttributeDescription {
        .location = 3,
        .binding = 0,
        .format = VK_FORMAT_R32G32_SFLOAT,
        .offset = offsetof(Asset::MeshVertices::Vertex, color)
    };
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

    auto * descriptor_set_layout = RB::CreateDescriptorSetLayout(
        state.logical_device.device,
        static_cast<U8>(descriptor_set_layout_bindings.size()),
        descriptor_set_layout_bindings.data()
    );

    auto const graphic_pipeline_group = RB::CreateGraphicPipeline(
        state.logical_device.device,
        gpu_shaders_count,
        gpu_shaders,
        vertex_binding_description,
        static_cast<U32>(input_attribute_descriptions.size()),
        input_attribute_descriptions.data(),
        VkExtent2D {.width = state.screen_width, .height = state.screen_height},
        state.render_pass,
        descriptor_set_layout
    );

    return DrawPipeline {
        .graphic_pipeline_group = graphic_pipeline_group,
        .descriptor_set_layout = descriptor_set_layout,
    };
}

void DestroyDrawPipeline(DrawPipeline & draw_pipeline) {
    RB::DestroyGraphicPipeline(
        state.logical_device.device, 
        draw_pipeline.graphic_pipeline_group
    );
    RB::DestroyDescriptorSetLayout(
        state.logical_device.device, 
        draw_pipeline.descriptor_set_layout
    );
}

std::vector<VkDescriptorSet_T *> CreateDescriptorSetsForDrawPipeline(DrawPipeline & draw_pipeline) {
    return RB::CreateDescriptorSet(
        state.logical_device.device,
        state.descriptor_pool,
        draw_pipeline.descriptor_set_layout,
        static_cast<U8>(state.swap_chain_group.swap_chain_images.size())
    );
}

// TODO I think we should keep these information for each mesh individually
void BindBasicDescriptorSetWriteInfo(
    U8 const descriptor_sets_count,
    VkDescriptorSet_T ** descriptor_sets,
    VkDescriptorBufferInfo const & buffer_info,
    VkDescriptorImageInfo const & image_info
) {
    MFA_ASSERT(descriptor_sets_count > 0);
    MFA_PTR_ASSERT(descriptor_sets);
    for(U8 i = 0; i < descriptor_sets_count; i++) {
        std::vector<VkWriteDescriptorSet> descriptorWrites{};
        
        descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].dstSet = descriptor_sets[i];
        descriptorWrites[0].dstBinding = 0;
        descriptorWrites[0].dstArrayElement = 0;
        descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].pBufferInfo = &buffer_info;

        descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[1].dstSet = descriptor_sets[i];
        descriptorWrites[1].dstBinding = 1;
        descriptorWrites[1].dstArrayElement = 0;
        descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrites[1].descriptorCount = 1;
        descriptorWrites[1].pImageInfo = &image_info;

        vkUpdateDescriptorSets(
            state.logical_device.device, 
            static_cast<uint32_t>(descriptorWrites.size()), 
            descriptorWrites.data(), 
            0, 
            nullptr
        );
    }
}

UniformBuffer CreateUniformBuffer(size_t const buffer_size) {
    auto const buffers = RB::CreateUniformBuffer(
        state.logical_device.device,
        state.physical_device,
        static_cast<U8>(state.swap_chain_group.swap_chain_images.size()),
        buffer_size
    );
    return {
        .buffers = buffers,
        .buffer_size = buffer_size
    };
}

void BindDataToUniformBuffer(
    UniformBuffer const & uniform_buffer, 
    Blob const data,
    U8 const current_image_index
) {
    RB::UpdateUniformBuffer(
        state.logical_device.device, 
        uniform_buffer.buffers[current_image_index], 
        data
    );
}

void DestroyUniformBuffer(UniformBuffer & uniform_buffer) {
    for(auto & buffer_group : uniform_buffer.buffers) {
        RB::DestroyUniformBuffer(
            state.logical_device.device,
            buffer_group
        );
    }
}

MeshBuffers CreateMeshBuffers(Asset::MeshAsset const & mesh_asset) {
    auto vertices_buffer = RB::CreateVertexBuffer(
        state.logical_device.device,
        state.physical_device,
        state.graphic_command_pool,
        state.graphic_queue,
        mesh_asset.vertices_blob()
    );
    auto indices_buffer = RB::CreateIndexBuffer(
        state.logical_device.device,
        state.physical_device,
        state.graphic_command_pool,
        state.graphic_queue,
        mesh_asset.indices_blob()
    );
    return MeshBuffers {
        .vertices_buffer = vertices_buffer,
        .indices_buffer = indices_buffer,
        .indices_count = mesh_asset.header_object()->index_count
    };
}

void DestroyMeshBuffers(MeshBuffers const & mesh_buffers) {
    RB::DestroyVertexBuffer(
        state.logical_device.device,
        mesh_buffers.vertices_buffer
    );
    RB::DestroyIndexBuffer(
        state.logical_device.device,
        mesh_buffers.indices_buffer
    );
}

RB::GpuTexture CreateTexture(Asset::TextureAsset & texture_asset) {
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
    RB::DestroyTexture(gpu_texture);
}

// TODO Ask for options
SamplerGroup CreateSampler() {
    auto * sampler = RB::CreateSampler(state.logical_device.device);
    return {
        .sampler = sampler
    };
}

void DestroySampler(SamplerGroup const & sampler_group) {
    RB::DestroySampler(state.logical_device.device, sampler_group.sampler);
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

    VkImageSubresourceRange subResourceRange = {};
    subResourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    subResourceRange.baseMipLevel = 0;
    subResourceRange.levelCount = 1;
    subResourceRange.baseArrayLayer = 0;
    subResourceRange.layerCount = 1;

    vkBeginCommandBuffer(state.graphic_command_buffers[draw_pass.image_index], &beginInfo);

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

void DrawBasicTexturedMesh(
    DrawPass const & draw_pass,
    DrawPipeline & draw_pipeline,
    VkDescriptorSet_T ** descriptor_sets,
    UniformBuffer const & uniform_buffer,
    MeshBuffers const & mesh_buffers,
    RB::GpuTexture const & gpu_texture,
    SamplerGroup const & sampler_group
) {
    MFA_ASSERT(draw_pass.is_valid);
    {
        VkDescriptorBufferInfo buffer_info {};
        buffer_info.buffer = uniform_buffer.buffers[draw_pass.image_index].buffer;
        buffer_info.offset = 0;
        buffer_info.range = uniform_buffer.buffer_size;

        VkDescriptorImageInfo image_info {};
        image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        image_info.imageView = gpu_texture.image_view();
        image_info.sampler = sampler_group.sampler;
        RenderFrontend::BindBasicDescriptorSetWriteInfo(
            1,
            &descriptor_sets[draw_pass.image_index],
            buffer_info,
            image_info
        );
    }

    // We should bind specific descriptor set with different texture for each mesh
    vkCmdBindDescriptorSets(
        state.graphic_command_buffers[draw_pass.image_index], 
        VK_PIPELINE_BIND_POINT_GRAPHICS, 
        draw_pipeline.graphic_pipeline_group.pipeline_layout, 
        0, 
        1, 
        &descriptor_sets[draw_pass.image_index], 
        0, 
        nullptr
    );
    
    // We can bind command buffer to multiple pipeline
    vkCmdBindPipeline(
        state.graphic_command_buffers[draw_pass.image_index], 
        VK_PIPELINE_BIND_POINT_GRAPHICS, 
        draw_pipeline.graphic_pipeline_group.graphic_pipeline
    );

    VkDeviceSize offset = 0;
    // TODO Check again for numbers
    vkCmdBindVertexBuffers(
        state.graphic_command_buffers[draw_pass.image_index], 
        0, 
        1, 
        &mesh_buffers.vertices_buffer.buffer, 
        &offset
    );

    vkCmdBindIndexBuffer(
        state.graphic_command_buffers[draw_pass.image_index], 
        mesh_buffers.indices_buffer.buffer, 
        0, 
        VK_INDEX_TYPE_UINT32
    );

    vkCmdDrawIndexed(
        state.graphic_command_buffers[draw_pass.image_index], 
        mesh_buffers.indices_count, 
        1, 0, 0, 0
    );
    
}

void EndPass(DrawPass & draw_pass) {
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
}


}
