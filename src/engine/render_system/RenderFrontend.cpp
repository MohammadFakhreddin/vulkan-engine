#include "RenderFrontend.hpp"

#include "engine/BedrockAssert.hpp"
#include "RenderBackend.hpp"
#include "engine/BedrockLog.hpp"
#include "render_passes/DisplayRenderPass.hpp"

#include "libs/imgui/imgui.h"

#include <string>

namespace MFA::RenderFrontend {

#ifdef __DESKTOP__
struct SDLEventWatchGroup {
    SDLEventWatchId id = 0;
    SDLEventWatch watch = nullptr;
};
#endif

struct ResizeEventWatchGroup {
    ResizeEventListenerId id = 0;
    ResizeEventListener watch = nullptr;
};

struct State {
    // CreateWindow
    ScreenWidth screenWidth = 0;
    ScreenHeight screenHeight = 0;
    // CreateInstance
    std::string application_name {};
    VkInstance vk_instance = nullptr;
    // CreateDebugCallback
    VkDebugReportCallbackEXT vkDebugReportCallbackExt {};
    VkSurfaceKHR surface {};
    VkPhysicalDevice physicalDevice {};
    VkPhysicalDeviceFeatures physicalDeviceFeatures {};
    uint32_t graphicQueueFamily = 0;
    uint32_t presentQueueFamily = 0;
    VkQueue graphicQueue {};
    VkQueue presentQueue {};
    RB::LogicalDevice logicalDevice {};
    VkCommandPool graphicCommandPool {};
    //RB::SwapChainGroup swapChainGroup {};
    //VkRenderPass displayRenderPass {};
    //RB::DepthImageGroup depthImageGroup {};
    //std::vector<VkFramebuffer> frameBuffers {};
    VkDescriptorPool descriptorPool {};
    // Each renderPass should have its own commandBuffer and syncedObject
    //std::vector<VkCommandBuffer> graphicCommandBuffers {};
    //RB::SyncObjects syncObjects {};
    //uint8_t currentFrame = 0;
    // Resize
    bool isWindowResizable = false;
    bool windowResized = false;
    std::vector<ResizeEventWatchGroup> resizeEventListeners {};
    VkSurfaceCapabilitiesKHR surfaceCapabilities {};
    uint32_t swapChainImageCount = 0;
    DisplayRenderPass displayRenderPass {};
#ifdef __DESKTOP__
    // CreateWindow
    MSDL::SDL_Window * window = nullptr;
    // Event watches
    int nextEventListenerId = 0;
    std::vector<SDLEventWatchGroup> sdlEventListeners {};
#elif defined(__ANDROID__)
    ANativeWindow * window = nullptr;
#elif defined(__IOS__)
    void * window = nullptr;
#else
#error Os is not handled
#endif
    bool isWindowVisible = true;                        // Currently only minimize can cause this to be false
} static * state = nullptr;

static VkBool32 VKAPI_PTR DebugCallback(
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
        MFA_LOG_ERROR("Message code: %d\nMessage: %s\nLocation: %llu\n", message_code, message, static_cast<uint64_t>(location));
    } else if(flags & VK_DEBUG_REPORT_WARNING_BIT_EXT) {
        MFA_LOG_WARN("Message code: %d\nMessage: %s\nLocation: %llu\n", message_code, message, static_cast<uint64_t>(location));
    } else {
        MFA_LOG_INFO("Message code: %d\nMessage: %s\nLocation: %llu\n", message_code, message, static_cast<uint64_t>(location));
    }
    MFA_ASSERT(false);
    return true;
}

#ifdef __DESKTOP__
static bool IsResizeEvent(uint8_t const sdlEvent) {
#if defined(__PLATFORM_WIN__) || defined(__PLATFORM_LINUX__)
    return sdlEvent == MSDL::SDL_WINDOWEVENT_RESIZED;
#elif defined(__PLATFORM_MAC__)
    return sdlEvent == MSDL::SDL_WINDOWEVENT_RESIZED || 
        sdlEvent == MSDL::SDL_WINDOWEVENT_SIZE_CHANGED ||
        sdlEvent == MSDL::SDL_WINDOWEVENT_MAXIMIZED ||
        sdlEvent == MSDL::SDL_WINDOWEVENT_MINIMIZED ||
        sdlEvent == MSDL::SDL_WINDOWEVENT_EXPOSED;;
#else
    #error Unhandled platform
#endif
}

static int SDLEventWatcher(void* data, MSDL::SDL_Event* event) {
    if (
        state->isWindowResizable == true &&
        event->type == MSDL::SDL_WINDOWEVENT &&
        IsResizeEvent(event->window.event)
    ) {
        MSDL::SDL_Window * sdlWindow = MSDL::SDL_GetWindowFromID(event->window.windowID);
        if (sdlWindow == static_cast<MSDL::SDL_Window *>(data)) {
            state->windowResized = true;
        }
    }
    for (auto & eventListener : state->sdlEventListeners) {
        MFA_ASSERT(eventListener.watch != nullptr);
        eventListener.watch(data, event);
    }
    return 0;
}
#endif

[[nodiscard]]
static VkSurfaceCapabilitiesKHR computeSurfaceCapabilities() {
    return RB::GetSurfaceCapabilities(state->physicalDevice, state->surface);    
}

bool Init(InitParams const & params) {
    state = new State();
    state->application_name = params.applicationName;
#ifdef __DESKTOP__
    state->window = RB::CreateWindow(
        params.screenWidth, 
        params.screenHeight
    );
    state->isWindowResizable = params.resizable;

    if (params.resizable) {
        // Make window resizable
        MSDL::SDL_SetWindowResizable(state->window, MSDL::SDL_TRUE);
    }

    MSDL::SDL_SetWindowMinimumSize(state->window, 100, 100);

    MSDL::SDL_AddEventWatch(SDLEventWatcher, state->window);

#elif defined(__ANDROID__)
    state->window = params.app->window;
#elif defined(__IOS__)
    state->window = params.view;
#else
    #error "Os is not supported"
#endif

#ifdef __DESKTOP__
    state->vk_instance = RB::CreateInstance(
        state->application_name.c_str(),
        state->window
    );
#elif defined(__ANDROID__) || defined(__IOS__)
    state->vk_instance = RB::CreateInstance(state->application_name.c_str());
#else
    #error "Os is not supported"
#endif
    MFA_VK_VALID_ASSERT(state->vk_instance);

#if defined(MFA_DEBUG)  // TODO Fix support for android
    state->vkDebugReportCallbackExt = RB::CreateDebugCallback(
        state->vk_instance,
        DebugCallback
    );
#endif

    state->surface = RB::CreateWindowSurface(state->window, state->vk_instance);

    {// FindPhysicalDevice
        auto const findPhysicalDeviceResult = RB::FindPhysicalDevice(state->vk_instance); // TODO Check again for retry count number
        state->physicalDevice = findPhysicalDeviceResult.physicalDevice;
        state->physicalDeviceFeatures = findPhysicalDeviceResult.physicalDeviceFeatures;
    }

    // Find surface capabilities
    state->surfaceCapabilities = computeSurfaceCapabilities();

    state->swapChainImageCount = RB::ComputeSwapChainImagesCount(state->surfaceCapabilities);

    state->screenWidth = state->surfaceCapabilities.currentExtent.width;
    state->screenHeight = state->surfaceCapabilities.currentExtent.height;
    MFA_LOG_INFO("ScreenWidth: %f \nScreenHeight: %f", static_cast<float>(state->screenWidth), static_cast<float>(state->screenHeight));

    if(RB::CheckSwapChainSupport(state->physicalDevice) == false) {
        MFA_LOG_ERROR("Swapchain is not supported on this device");
        return false;
    }
    {// Trying to find queue family
        auto const result = RB::FindPresentAndGraphicQueueFamily(state->physicalDevice, state->surface);
        state->graphicQueueFamily = result.graphic_queue_family;
        state->presentQueueFamily = result.present_queue_family;
    }
    state->logicalDevice = RB::CreateLogicalDevice(
        state->physicalDevice,
        state->graphicQueueFamily,
        state->presentQueueFamily,
        state->physicalDeviceFeatures
    );
    // Get graphics and presentation queues (which may be the same)
    state->graphicQueue = RB::GetQueueByFamilyIndex(
        state->logicalDevice.device, 
        state->graphicQueueFamily
    );
    state->presentQueue = RB::GetQueueByFamilyIndex(
        state->logicalDevice.device, 
        state->presentQueueFamily
    );
    MFA_LOG_INFO("Acquired graphics and presentation queues");
    state->graphicCommandPool = RB::CreateCommandPool(state->logicalDevice.device, state->graphicQueueFamily);
    
    state->descriptorPool = RB::CreateDescriptorPool(
        state->logicalDevice.device, 
        1000 // TODO We might need to ask this from user
    );

    state->displayRenderPass.Init();

    return true;
}

static void OnResize() {
    RB::DeviceWaitIdle(state->logicalDevice.device);

    state->surfaceCapabilities = computeSurfaceCapabilities();

    state->screenWidth = state->surfaceCapabilities.currentExtent.width;
    state->screenHeight = state->surfaceCapabilities.currentExtent.height;

    MFA_ASSERT(state->screenWidth > 0);
    MFA_ASSERT(state->screenHeight > 0);

    state->windowResized = false;

    state->displayRenderPass.OnResize();

    for (auto & eventWatchGroup : state->resizeEventListeners) {
        MFA_ASSERT(eventWatchGroup.watch != nullptr);
        eventWatchGroup.watch();
    }

}

bool Shutdown() {
    // Common part with resize
    RB::DeviceWaitIdle(state->logicalDevice.device);

    MFA_ASSERT(state->resizeEventListeners.empty());

#ifdef __DESKTOP__
    MFA_ASSERT(state->sdlEventListeners.empty());
    MSDL::SDL_DelEventWatch(SDLEventWatcher, state->window);
#endif

    state->displayRenderPass.Shutdown();

    // DestroyPipeline in application // TODO We should have reference to what user creates + params for re-creation
    // GraphicPipeline, UniformBuffer, PipelineLayout
    // Shutdown only procedure
    
    RB::DestroyDescriptorPool(
        state->logicalDevice.device,
        state->descriptorPool
    );

    RB::DestroyCommandPool(
        state->logicalDevice.device,
        state->graphicCommandPool
    );

    RB::DestroyLogicalDevice(state->logicalDevice);
    
    RB::DestroyWindowSurface(state->vk_instance, state->surface);

#ifdef MFA_DEBUG
    RB::DestroyDebugReportCallback(state->vk_instance, state->vkDebugReportCallbackExt);
#endif

    RB::DestroyInstance(state->vk_instance);

    delete state;

    return true;
}

int AddResizeEventListener(ResizeEventListener const & eventListener) {
    MFA_ASSERT(eventListener != nullptr);
    state->resizeEventListeners.emplace_back(ResizeEventWatchGroup {
        .id = state->nextEventListenerId++,
        .watch = eventListener
    });
    return state->resizeEventListeners.back().id;
}

bool RemoveResizeEventListener(ResizeEventListenerId listenerId) {
    bool success = false;
    for (int i = static_cast<int>(state->resizeEventListeners.size()) - 1; i >= 0; --i) {
        auto & listener = state->resizeEventListeners[i];
        if (listener.id == listenerId) {
            success = true;
            state->resizeEventListeners.erase(state->resizeEventListeners.begin() + i);
            break;
        }
    }
    return success;
}

VkDescriptorSetLayout CreateBasicDescriptorSetLayout() {
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
        static_cast<uint8_t>(descriptor_set_layout_bindings.size()),
        descriptor_set_layout_bindings.data()
    );
}

VkDescriptorSetLayout CreateDescriptorSetLayout(
    uint8_t const bindingsCount, 
    VkDescriptorSetLayoutBinding * bindings
) {
    auto descriptorSetLayout = RB::CreateDescriptorSetLayout(
        state->logicalDevice.device,
        bindingsCount,
        bindings
    );
    MFA_VK_VALID_ASSERT(descriptorSetLayout);
    return descriptorSetLayout;
}


void DestroyDescriptorSetLayout(VkDescriptorSetLayout descriptorSetLayout) {
    MFA_ASSERT(descriptorSetLayout);
    RB::DestroyDescriptorSetLayout(state->logicalDevice.device, descriptorSetLayout);
}

DrawPipeline CreateBasicDrawPipeline(
    VkRenderPass renderPass,
    uint8_t const gpuShadersCount, 
    RB::GpuShader * gpuShaders,
    uint32_t const descriptorSetLayoutCount,
    VkDescriptorSetLayout* descriptorSetLayouts,
    VkVertexInputBindingDescription const & vertexInputBindingDescription,
    uint8_t const vertexInputAttributeDescriptionCount,
    VkVertexInputAttributeDescription * vertexInputAttributeDescriptions
) {
    MFA_ASSERT(gpuShadersCount > 0);
    MFA_ASSERT(gpuShaders);
    MFA_ASSERT(descriptorSetLayouts);

    RB::CreateGraphicPipelineOptions pipelineOptions {};
    pipelineOptions.depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    pipelineOptions.depthStencil.depthTestEnable = VK_TRUE;
    pipelineOptions.depthStencil.depthWriteEnable = VK_TRUE;
    pipelineOptions.depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
    pipelineOptions.depthStencil.depthBoundsTestEnable = VK_FALSE;
    pipelineOptions.depthStencil.stencilTestEnable = VK_FALSE;

    pipelineOptions.colorBlendAttachments.blendEnable = VK_TRUE;
    pipelineOptions.colorBlendAttachments.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    pipelineOptions.colorBlendAttachments.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    pipelineOptions.colorBlendAttachments.colorBlendOp = VK_BLEND_OP_ADD;
    pipelineOptions.colorBlendAttachments.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    pipelineOptions.colorBlendAttachments.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    pipelineOptions.colorBlendAttachments.alphaBlendOp = VK_BLEND_OP_ADD;
    pipelineOptions.colorBlendAttachments.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    pipelineOptions.useStaticViewportAndScissor = false;

    return CreateDrawPipeline(
        renderPass,
        gpuShadersCount,
        gpuShaders,
        descriptorSetLayoutCount,
        descriptorSetLayouts,
        vertexInputBindingDescription,
        vertexInputAttributeDescriptionCount,
        vertexInputAttributeDescriptions,
        pipelineOptions
    );
}

[[nodiscard]]
DrawPipeline CreateDrawPipeline(
    VkRenderPass renderPass,
    uint8_t const gpuShadersCount, 
    RB::GpuShader * gpuShaders,
    uint32_t const descriptorLayoutsCount,
    VkDescriptorSetLayout * descriptorSetLayouts,
    VkVertexInputBindingDescription const vertexBindingDescription,
    uint32_t const inputAttributeDescriptionCount,
    VkVertexInputAttributeDescription * inputAttributeDescriptionData,
    RB::CreateGraphicPipelineOptions const & options
) {

    VkExtent2D extent2D;
    extent2D.width = static_cast<uint32_t>(state->screenWidth);
    extent2D.height = static_cast<uint32_t>(state->screenHeight);

    auto const graphicPipelineGroup = RB::CreateGraphicPipeline(
        state->logicalDevice.device,
        gpuShadersCount,
        gpuShaders,
        vertexBindingDescription,
        static_cast<uint32_t>(inputAttributeDescriptionCount),
        inputAttributeDescriptionData,
        extent2D,
        renderPass,
        descriptorLayoutsCount,
        descriptorSetLayouts,
        options
    );

    return graphicPipelineGroup;
}

void DestroyDrawPipeline(DrawPipeline & draw_pipeline) {
    RB::DestroyGraphicPipeline(
        state->logicalDevice.device, 
        draw_pipeline
    );
}

std::vector<VkDescriptorSet> CreateDescriptorSets(VkDescriptorSetLayout descriptorSetLayout) {
    return RB::CreateDescriptorSet(
        state->logicalDevice.device,
        state->descriptorPool,
        descriptorSetLayout,
        state->swapChainImageCount
    );
}

std::vector<VkDescriptorSet> CreateDescriptorSets(
    uint32_t const descriptorSetCount,
    VkDescriptorSetLayout descriptorSetLayout
) {
    MFA_VK_VALID_ASSERT(descriptorSetLayout);
    return RB::CreateDescriptorSet(
        state->logicalDevice.device,
        state->descriptorPool,
        descriptorSetLayout,
        descriptorSetCount
    );
}

bool UniformBufferGroup::isValid() const noexcept {
    if(bufferSize <= 0 || buffers.empty() == true) {
        return false;
    }
    for(auto const & buffer : buffers) {
        if(buffer.isValid() == false) {
            return false;
        }
    }
    return true;
}

UniformBufferGroup CreateUniformBuffer(size_t const bufferSize, uint32_t const count) {
    auto const buffers = RB::CreateUniformBuffer(
        state->logicalDevice.device,
        state->physicalDevice,
        count,
        bufferSize
    );
    UniformBufferGroup group;
    group.buffers = buffers;
    group.bufferSize = bufferSize;
    return group;
}

void UpdateUniformBuffer(
    RB::BufferGroup const & uniform_buffer, 
    CBlob const data
) {
    RB::UpdateUniformBuffer(
        state->logicalDevice.device, 
        uniform_buffer, 
        data
    );
}

void DestroyUniformBuffer(UniformBufferGroup & uniform_buffer) {
    for(auto & buffer_group : uniform_buffer.buffers) {
        RB::DestroyUniformBuffer(
            state->logicalDevice.device,
            buffer_group
        );
    }
}

RB::BufferGroup CreateVertexBuffer(CBlob const vertices_blob) {
    return RB::CreateVertexBuffer(
        state->logicalDevice.device,
        state->physicalDevice,
        state->graphicCommandPool,
        state->graphicQueue,
        vertices_blob
    );
}

RB::BufferGroup CreateIndexBuffer(CBlob const indices_blob) {
    return RB::CreateIndexBuffer(
        state->logicalDevice.device,
        state->physicalDevice,
        state->graphicCommandPool,
        state->graphicQueue,
        indices_blob
    );
}

MeshBuffers CreateMeshBuffers(AssetSystem::Mesh const & mesh) {
    MFA_ASSERT(mesh.IsValid());
    MeshBuffers buffers {};
    buffers.verticesBuffer = CreateVertexBuffer(mesh.GetVerticesBuffer());
    buffers.indicesBuffer = CreateIndexBuffer(mesh.GetIndicesBuffer());
    return buffers;
}

void DestroyMeshBuffers(MeshBuffers & meshBuffers) {
    RB::DestroyVertexBuffer(state->logicalDevice.device, meshBuffers.verticesBuffer);
    RB::DestroyIndexBuffer(state->logicalDevice.device, meshBuffers.indicesBuffer);
}

RB::GpuTexture CreateTexture(AssetSystem::Texture & texture) {
    auto gpuTexture = RB::CreateTexture(
        texture,
        state->logicalDevice.device,
        state->physicalDevice,
        state->graphicQueue,
        state->graphicCommandPool
    );
    MFA_ASSERT(gpuTexture.isValid());
    return gpuTexture;
}

void DestroyTexture(RB::GpuTexture & gpu_texture) {
    RB::DestroyTexture(state->logicalDevice.device, gpu_texture);
}

// TODO Ask for options
SamplerGroup CreateSampler(RB::CreateSamplerParams const & samplerParams) {
    auto sampler = RB::CreateSampler(state->logicalDevice.device, samplerParams);
    MFA_VK_VALID_ASSERT(sampler);
    SamplerGroup samplerGroup {};
    samplerGroup.sampler = sampler;
    MFA_ASSERT(samplerGroup.isValid());
    return samplerGroup;
}

void DestroySampler(SamplerGroup & sampler_group) {
    RB::DestroySampler(state->logicalDevice.device, sampler_group.sampler);
    sampler_group.revoke();
}

GpuModel CreateGpuModel(AssetSystem::Model & model_asset) {
    GpuModel gpuModel {};
    gpuModel.valid = true;
    gpuModel.meshBuffers = CreateMeshBuffers(model_asset.mesh);
    gpuModel.model = model_asset;
    if(false == model_asset.textures.empty()) {
        for (auto & texture_asset : model_asset.textures) {
            gpuModel.textures.emplace_back(CreateTexture(texture_asset));
            MFA_ASSERT(gpuModel.textures.back().isValid());
        }
    }
    return gpuModel;
}

void DestroyGpuModel(GpuModel & gpu_model) {
    MFA_ASSERT(gpu_model.valid);
    gpu_model.valid = false;
    DestroyMeshBuffers(gpu_model.meshBuffers);
    if(false == gpu_model.textures.empty()) {
        for (auto & gpu_texture : gpu_model.textures) {
            DestroyTexture(gpu_texture);
        }
    }
}

void DeviceWaitIdle() {
    RB::DeviceWaitIdle(state->logicalDevice.device);
}

void BindDrawPipeline(
    DrawPass & drawPass,
    DrawPipeline & drawPipeline   
) {
    MFA_ASSERT(drawPass.isValid);
    drawPass.drawPipeline = &drawPipeline;
    // We can bind command buffer to multiple pipeline
    vkCmdBindPipeline(
        drawPass.renderPass->GetCommandBuffer(drawPass),
        VK_PIPELINE_BIND_POINT_GRAPHICS, 
        drawPipeline.graphicPipeline
    );
}
// TODO Remove all basic functions
void UpdateDescriptorSetBasic(
    DrawPass const & drawPass,
    VkDescriptorSet descriptorSet,
    UniformBufferGroup const & uniformBuffer,
    RB::GpuTexture const & gpuTexture,
    SamplerGroup const & samplerGroup
) {
    VkDescriptorImageInfo imageInfo {};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView = gpuTexture.image_view();
    imageInfo.sampler = samplerGroup.sampler;

    UpdateDescriptorSetBasic(
        drawPass,
        descriptorSet,
        uniformBuffer,
        1,
        &imageInfo
    );
}

void UpdateDescriptorSetBasic(
    DrawPass const & drawPass,
    VkDescriptorSet descriptorSet,
    UniformBufferGroup const & uniformBuffer,
    uint32_t const imageInfoCount,
    VkDescriptorImageInfo const * imageInfos
) {
    VkDescriptorBufferInfo bufferInfo {};
    bufferInfo.buffer = uniformBuffer.buffers[drawPass.imageIndex].buffer;
    bufferInfo.offset = 0;
    bufferInfo.range = uniformBuffer.bufferSize;

    RB::UpdateDescriptorSetsBasic(
        state->logicalDevice.device,
        1,
        &descriptorSet,
        bufferInfo,
        imageInfoCount,
        imageInfos
    );
}

void UpdateDescriptorSets(
    uint8_t const writeInfoCount,
    VkWriteDescriptorSet * writeInfo
) {
    RB::UpdateDescriptorSets(
        state->logicalDevice.device,
        writeInfoCount,
        writeInfo
    );
}

void UpdateDescriptorSets(
    uint8_t const descriptorSetsCount,
    VkDescriptorSet* descriptorSets,
    uint8_t const writeInfoCount,
    VkWriteDescriptorSet * writeInfo
) {
    RB::UpdateDescriptorSets(
        state->logicalDevice.device,
        descriptorSetsCount,
        descriptorSets,
        writeInfoCount,
        writeInfo
    );
}

void BindDescriptorSet(
    DrawPass const & drawPass,
    VkDescriptorSet descriptorSet
) {
    MFA_ASSERT(drawPass.isValid);
    MFA_ASSERT(drawPass.drawPipeline);
    MFA_ASSERT(descriptorSet);
    // We should bind specific descriptor set with different texture for each mesh
    vkCmdBindDescriptorSets(
        drawPass.renderPass->GetCommandBuffer(drawPass),
        VK_PIPELINE_BIND_POINT_GRAPHICS, 
        drawPass.drawPipeline->pipelineLayout, 
        0, 
        1, 
        &descriptorSet, 
        0, 
        nullptr
    );
}

void BindVertexBuffer(
    DrawPass const & drawPass, 
    RB::BufferGroup const vertexBuffer,
    VkDeviceSize const offset
) {
    MFA_ASSERT(drawPass.isValid);
    RB::BindVertexBuffer(
        drawPass.renderPass->GetCommandBuffer(drawPass),
        vertexBuffer,
        offset
    );
}

void BindIndexBuffer(
    DrawPass const & drawPass,
    RB::BufferGroup const indexBuffer,
    VkDeviceSize const offset,
    VkIndexType const indexType
) {
    MFA_ASSERT(drawPass.isValid);
    RB::BindIndexBuffer(
        drawPass.renderPass->GetCommandBuffer(drawPass),
        indexBuffer,
        offset,
        indexType
    );
}

void DrawIndexed(
    DrawPass const & drawPass, 
    uint32_t const indicesCount,
    uint32_t const instanceCount,
    uint32_t const firstIndex,
    uint32_t const vertexOffset,
    uint32_t const firstInstance
) {
    MFA_ASSERT(drawPass.isValid);
    RB::DrawIndexed(
        drawPass.renderPass->GetCommandBuffer(drawPass),
        indicesCount,
        instanceCount,
        firstIndex,
        vertexOffset,
        firstInstance
    );
}

void BeginRenderPass(
    VkCommandBuffer commandBuffer, 
    VkRenderPass renderPass, 
    VkFramebuffer frameBuffer
) {
    std::vector<VkClearValue> clearValues {};
    clearValues.resize(2);
    clearValues[0].color = VkClearColorValue { .float32 = {0.1f, 0.1f, 0.1f, 1.0f }};
    clearValues[1].depthStencil = { .depth = 1.0f, .stencil = 0};

    VkRenderPassBeginInfo renderPassBeginInfo = {};
    renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassBeginInfo.renderPass = renderPass;
    renderPassBeginInfo.framebuffer = frameBuffer;
    renderPassBeginInfo.renderArea.offset.x = 0;
    renderPassBeginInfo.renderArea.offset.y = 0;
    renderPassBeginInfo.renderArea.extent.width = static_cast<uint32_t>(state->screenWidth);
    renderPassBeginInfo.renderArea.extent.height = static_cast<uint32_t>(state->screenHeight);
    renderPassBeginInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassBeginInfo.pClearValues = clearValues.data();

    RB::BeginRenderPass(commandBuffer, renderPassBeginInfo);
}

void EndRenderPass(VkCommandBuffer commandBuffer) {
    RB::EndRenderPass(commandBuffer);
}

void OnNewFrame(float deltaTimeInSec) {
    if (state->windowResized) {
        OnResize();
    }
#ifdef __DESKTOP__
    state->isWindowVisible = (GetWindowFlags() & MSDL::SDL_WINDOW_MINIMIZED) > 0 ? false : true;
#endif
}

[[nodiscard]]
RB::GpuShader CreateShader(AssetSystem::Shader const & shader) {
    return RB::CreateShader(
        state->logicalDevice.device,
        shader
    );
}

void DestroyShader(RB::GpuShader & gpu_shader) {
    RB::DestroyShader(state->logicalDevice.device, gpu_shader);
}

void SetScissor(DrawPass const & drawPass, VkRect2D const & scissor) {
    MFA_ASSERT(drawPass.isValid);
    MFA_ASSERT(drawPass.renderPass != nullptr);
    RB::SetScissor(drawPass.renderPass->GetCommandBuffer(drawPass), scissor);
}

void SetViewport(DrawPass const & drawPass, VkViewport const & viewport) {
    MFA_ASSERT(drawPass.isValid);
    MFA_ASSERT(drawPass.renderPass != nullptr);
    RB::SetViewport(drawPass.renderPass->GetCommandBuffer(drawPass), viewport);
}

void PushConstants(
    DrawPass const & drawPass,
    AssetSystem::ShaderStage const shaderStage, 
    uint32_t const offset, 
    CBlob const data
) {
    MFA_ASSERT(drawPass.isValid);
    MFA_ASSERT(drawPass.renderPass != nullptr);
    RB::PushConstants(
        drawPass.renderPass->GetCommandBuffer(drawPass),
        drawPass.drawPipeline->pipelineLayout,
        shaderStage,
        offset,
        data
    );
}

uint32_t GetSwapChainImagesCount() {
    return state->swapChainImageCount;
}

void GetDrawableSize(int32_t & outWidth, int32_t & outHeight) {
    outWidth = state->screenWidth;
    outHeight = state->screenHeight;
}

#ifdef __DESKTOP__
// SDL functions

void WarpMouseInWindow(int32_t const x, int32_t const y) {
    MSDL::SDL_WarpMouseInWindow(state->window, x, y);
}

uint32_t GetMouseState(int32_t * x, int32_t * y) {
    return MSDL::SDL_GetMouseState(x, y);
}

uint8_t const * GetKeyboardState(int * numKeys) {
    return MSDL::SDL_GetKeyboardState(numKeys);
}

uint32_t GetWindowFlags() {
    return MSDL::SDL_GetWindowFlags(state->window); 
}

#endif

void AssignViewportAndScissorToCommandBuffer(VkCommandBuffer commandBuffer, VkExtent2D const & extent2D) {
    RB::AssignViewportAndScissorToCommandBuffer(
        extent2D,
        commandBuffer
    );
}

#ifdef __DESKTOP__
// TODO We might need separate SDL class
int AddEventWatch(SDLEventWatch const & eventWatch) {
    MFA_ASSERT(eventWatch != nullptr);

    SDLEventWatchGroup const group {
        .id = state->nextEventListenerId,
        .watch = eventWatch,
    };

    ++state->nextEventListenerId;
    state->sdlEventListeners.emplace_back(group);
    return group.id;
}

void RemoveEventWatch(SDLEventWatchId const watchId) {
    for (int i = static_cast<int>(state->sdlEventListeners.size()) - 1; i >= 0; --i) {
        if (state->sdlEventListeners[i].id == watchId) {
            state->sdlEventListeners.erase(state->sdlEventListeners.begin() + i);
        }
    }
}

#endif

void NotifyDeviceResized() {
    state->windowResized = true;
}

VkSurfaceCapabilitiesKHR GetSurfaceCapabilities() {
    return state->surfaceCapabilities;    
}

RB::SwapChainGroup CreateSwapChain(VkSwapchainKHR oldSwapChain) {
    return RB::CreateSwapChain(
        state->logicalDevice.device,
        state->physicalDevice,
        state->surface,
        state->surfaceCapabilities,
        oldSwapChain
    );
}

void DestroySwapChain(RB::SwapChainGroup const & swapChainGroup) {
    RB::DestroySwapChain(state->logicalDevice.device, swapChainGroup);
}


VkRenderPass CreateRenderPass(
    VkAttachmentDescription * attachments,
    uint32_t const attachmentsCount,
    VkSubpassDescription * subPasses,
    uint32_t const subPassesCount,
    VkSubpassDependency * dependencies,
    uint32_t const dependenciesCount
) {
    return RB::CreateRenderPass(
        state->logicalDevice.device,
        attachments,
        attachmentsCount,
        subPasses,
        subPassesCount,
        dependencies,
        dependenciesCount
    );
}

void DestroyRenderPass(VkRenderPass renderPass) {
    RB::DestroyRenderPass(state->logicalDevice.device, renderPass);
}

[[nodiscard]]
RB::DepthImageGroup CreateDepthImage(
    VkExtent2D const & imageSize, 
    RB::CreateDepthImageOptions const & options
) {
    return RB::CreateDepthImage(
        state->physicalDevice,
        state->logicalDevice.device,
        imageSize,
        options
    );
}

void DestroyDepthImage(RB::DepthImageGroup const & depthImage) {
    RB::DestroyDepthImage(state->logicalDevice.device, depthImage);
}

VkFramebuffer CreateFrameBuffer(
    VkRenderPass renderPass,
    VkImageView const * attachments,
    uint32_t attachmentsCount,
    VkExtent2D frameBufferExtent,
    uint32_t layersCount
) {
    return RB::CreateFrameBuffers(
        state->logicalDevice.device,
        renderPass,
        attachments,
        attachmentsCount,
        frameBufferExtent,
        layersCount
    );
}

void DestroyFrameBuffers(
    uint32_t const frameBuffersCount,
    VkFramebuffer * frameBuffers
) {
    RB::DestroyFrameBuffers(
        state->logicalDevice.device,
        frameBuffersCount,
        frameBuffers
    );
}

bool IsWindowVisible() {
    return state->isWindowVisible;
}

bool IsWindowResized() {
    return state->windowResized;
}

void WaitForFence(VkFence fence) {
    RB::WaitForFence(
        state->logicalDevice.device, 
        fence
    );
}

void AcquireNextImage(
    VkSemaphore imageAvailabilitySemaphore,
    RB::SwapChainGroup const & swapChainGroup,
    uint32_t & outImageIndex
) {
    RB::AcquireNextImage(
        state->logicalDevice.device,
        imageAvailabilitySemaphore,
        swapChainGroup,
        outImageIndex
    );
}

std::vector<VkCommandBuffer> CreateGraphicCommandBuffers(uint32_t const count) {
    return RB::CreateCommandBuffers(
        state->logicalDevice.device,
        count,
        state->graphicCommandPool
    );   
}

void BeginCommandBuffer(
    VkCommandBuffer commandBuffer, 
    VkCommandBufferBeginInfo const & beginInfo
) {
    RB::BeginCommandBuffer(
        commandBuffer, 
        beginInfo
    );
}

void EndCommandBuffer(VkCommandBuffer commandBuffer) {
    RB::EndCommandBuffer(commandBuffer);
}

void DestroyGraphicCommandBuffer(VkCommandBuffer * commandBuffers, uint32_t const commandBuffersCount) {
    RB::DestroyCommandBuffers(
        state->logicalDevice.device,
        state->graphicCommandPool,
        commandBuffersCount,
        commandBuffers
    );
}

uint32_t GetPresentQueueFamily() {
    return state->presentQueueFamily;
}

uint32_t GetGraphicQueueFamily() {
    return state->graphicQueueFamily;
}

void PipelineBarrier(
    VkCommandBuffer commandBuffer,
    VkPipelineStageFlags sourceStageMask,
    VkPipelineStageFlags destinationStateMask,
    VkImageMemoryBarrier const & memoryBarrier
) {
    RB::PipelineBarrier(
        commandBuffer,
        sourceStageMask,
        destinationStateMask,
        memoryBarrier
    );
}

void SubmitQueue(
    VkCommandBuffer commandBuffer, 
    VkSemaphore imageAvailabilitySemaphore,  
    VkSemaphore renderFinishIndicatorSemaphore,
    VkFence inFlightFence
) {
    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkPipelineStageFlags wait_stages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &imageAvailabilitySemaphore;
    submitInfo.pWaitDstStageMask = wait_stages;

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &renderFinishIndicatorSemaphore;

    RB::ResetFences(
        state->logicalDevice.device, 
        1, 
        &inFlightFence
    );
    
    RB::SubmitQueues(
        state->graphicQueue, 
        1, 
        &submitInfo, 
        inFlightFence
    );
}

void PresentQueue(
    uint32_t const imageIndex, 
    VkSemaphore renderFinishIndicatorSemaphore, 
    VkSwapchainKHR swapChain
) {
    // Present drawn image
    // Note: semaphore here is not strictly necessary, because commands are processed in submission order within a single queue
    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &renderFinishIndicatorSemaphore;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &swapChain;
    presentInfo.pImageIndices = &imageIndex;

    // TODO Move to renderBackend
    auto const res = vkQueuePresentKHR(state->presentQueue, &presentInfo);
    if (res == VK_SUBOPTIMAL_KHR || res == VK_ERROR_OUT_OF_DATE_KHR || state->windowResized == true) {
        OnResize();
    } else if (res != VK_SUCCESS) {
        MFA_CRASH("Failed to submit present command buffer");
    }  
}

DisplayRenderPass * GetDisplayRenderPass() {
    return &state->displayRenderPass;
}

RB::SyncObjects createSyncObjects(
    uint8_t const maxFramesInFlight, 
    uint32_t const swapChainImagesCount
) {
    return RB::CreateSyncObjects(
        state->logicalDevice.device, 
        maxFramesInFlight, 
        swapChainImagesCount
    );
}

void DestroySyncObjects(RB::SyncObjects & syncObjects) {
    RB::DestroySyncObjects(state->logicalDevice.device, syncObjects);
}

}
