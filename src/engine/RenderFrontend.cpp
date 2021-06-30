#include "RenderFrontend.hpp"

#include "BedrockAssert.hpp"
#include "RenderBackend.hpp"
#include "BedrockLog.hpp"

#include "../libs/imgui/imgui.h"

#include <string>

namespace MFA::RenderFrontend {

static constexpr int MAX_FRAMES_IN_FLIGHT = 2;
//#ifdef __DESKTOP__
//static constexpr ScreenWidth RESOLUTION_MAX_WIDTH = 1920;
//static constexpr ScreenHeight RESOLUTION_MAX_HEIGHT = 1080;
//#elif defined(__ANDROID__)
//static constexpr ScreenWidth RESOLUTION_MAX_WIDTH = 800;
//static constexpr ScreenHeight RESOLUTION_MAX_HEIGHT = 600;
//#else
//#error Os is not handled
//#endif

#ifdef __DESKTOP__
struct EventWatchGroup {
    int id = 0;
    EventWatch watch = nullptr;
};
#endif

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
    RB::SwapChainGroup swapChainGroup {};
    VkRenderPass renderPass {};
    RB::DepthImageGroup depthImageGroup {};
    std::vector<VkFramebuffer> frameBuffers {};
    VkDescriptorPool descriptorPool {};
    std::vector<VkCommandBuffer> graphicCommandBuffers {};
    RB::SyncObjects syncObjects {};
    uint8_t currentFrame = 0;
    // Resize
    bool isWindowResizable = false;
    bool windowResized = false;
    ResizeEventListener resizeEventListener = nullptr;
#ifdef __DESKTOP__
    // CreateWindow
    MSDL::SDL_Window * window = nullptr;
    // Event watches
    int nextEventListenerId = 0;
    std::vector<EventWatchGroup> eventListeners {};
#elif defined(__ANDROID__)
    ANativeWindow * window = nullptr;
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
        MFA_LOG_ERROR("Message code: %d\nMessage: %s\nLocation: %llu\n", message_code, message, location);
    } else if(flags & VK_DEBUG_REPORT_WARNING_BIT_EXT) {
        MFA_LOG_WARN("Message code: %d\nMessage: %s\nLocation: %llu\n", message_code, message, location);
    } else {
        MFA_LOG_INFO("Message code: %d\nMessage: %s\nLocation: %llu\n", message_code, message, location);
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
    for (auto & eventListener : state->eventListeners) {
        MFA_ASSERT(eventListener.watch != nullptr);
        eventListener.watch(data, event);
    }
    return 0;
}
#endif

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
#else
    #error Os is not supported
#endif

    state->vk_instance = RB::CreateInstance(
        state->application_name.c_str(),
        state->window
    );
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
    auto const surfaceCapabilities = RB::GetSurfaceCapabilities(state->physicalDevice, state->surface);

    state->screenWidth = surfaceCapabilities.currentExtent.width;
    state->screenHeight = surfaceCapabilities.currentExtent.height;

   /* if (state->screenWidth > RESOLUTION_MAX_WIDTH || state->screenHeight > RESOLUTION_MAX_WIDTH) {
        auto const downScaleResolution = [](ScreenWidth & screenWidth, ScreenHeight & screenHeight)->void{
            screenHeight = static_cast<ScreenWidth>((static_cast<float>(RESOLUTION_MAX_WIDTH) / static_cast<float>(screenWidth)) * static_cast<float>(screenHeight));
            screenWidth = RESOLUTION_MAX_WIDTH;
        };
        if (state->screenWidth > state->screenHeight) {
            downScaleResolution(state->screenWidth, state->screenHeight);
        } else {
            downScaleResolution(state->screenHeight, state->screenWidth);
        }
    }*/

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

    auto const swapChainExtend = VkExtent2D {
        .width = static_cast<uint32_t>(state->screenWidth),
        .height = static_cast<uint32_t>(state->screenHeight)
    };

    // Creating sampler/TextureImage/VertexBuffer and IndexBuffer + graphic_pipeline is on application level
    state->swapChainGroup = RB::CreateSwapChain(
        state->logicalDevice.device,
        state->physicalDevice,
        state->surface,
        surfaceCapabilities
    );
    state->renderPass = RB::CreateRenderPass(
        state->physicalDevice,
        state->logicalDevice.device,
        state->swapChainGroup.swapChainFormat
    );
    state->depthImageGroup = RB::CreateDepth(
        state->physicalDevice,
        state->logicalDevice.device,
        swapChainExtend
    );
    state->frameBuffers = RB::CreateFrameBuffers(
        state->logicalDevice.device,
        state->renderPass,
        static_cast<uint8_t>(state->swapChainGroup.swapChainImageViews.size()),
        state->swapChainGroup.swapChainImageViews.data(),
        state->depthImageGroup.imageView,
        swapChainExtend
    );
    state->descriptorPool = RB::CreateDescriptorPool(
        state->logicalDevice.device, 
        1000 // TODO We might need to ask this from user
    );
    state->graphicCommandBuffers = RB::CreateCommandBuffers(
        state->logicalDevice.device,
        static_cast<uint8_t>(state->swapChainGroup.swapChainImages.size()),
        state->graphicCommandPool
    );

    state->syncObjects = RB::CreateSyncObjects(
        state->logicalDevice.device,
        MAX_FRAMES_IN_FLIGHT,
        static_cast<uint8_t>(state->swapChainGroup.swapChainImages.size())
    );
    return true;
}

void OnWindowResized() {
    RB::DeviceWaitIdle(state->logicalDevice.device);

    auto const surfaceCapabilities = RB::GetSurfaceCapabilities(state->physicalDevice, state->surface);

    state->screenWidth = surfaceCapabilities.currentExtent.width;
    state->screenHeight = surfaceCapabilities.currentExtent.height;

    MFA_ASSERT(state->screenWidth > 0);
    MFA_ASSERT(state->screenHeight > 0);

  /*  if (state->screenWidth > RESOLUTION_MAX_WIDTH || state->screenHeight > RESOLUTION_MAX_WIDTH) {
        auto const downScaleResolution = [](ScreenWidth & screenWidth, ScreenHeight & screenHeight)->void{
            screenHeight = static_cast<ScreenWidth>((static_cast<float>(RESOLUTION_MAX_WIDTH) / static_cast<float>(screenWidth)) * static_cast<float>(screenHeight));
            screenWidth = RESOLUTION_MAX_WIDTH;
        };
        if (state->screenWidth > state->screenHeight) {
            downScaleResolution(state->screenWidth, state->screenHeight);
        } else {
            downScaleResolution(state->screenHeight, state->screenWidth);
        }
    }*/

    auto const extent2D = VkExtent2D {
        .width = static_cast<uint32_t>(state->screenWidth),
        .height = static_cast<uint32_t>(state->screenHeight)
    };

    state->windowResized = false;

    // Depth image
    RB::DestroyDepth(
        state->logicalDevice.device,
        state->depthImageGroup
    );
    state->depthImageGroup = RB::CreateDepth(
        state->physicalDevice,
        state->logicalDevice.device,
        extent2D
    );

    // Swap-chain
    auto const oldSwapChainGroup = state->swapChainGroup;
    state->swapChainGroup = RB::CreateSwapChain(
        state->logicalDevice.device,
        state->physicalDevice,
        state->surface,
        surfaceCapabilities,
        oldSwapChainGroup.swapChain
    );
    RB::DestroySwapChain(
        state->logicalDevice.device,
        oldSwapChainGroup
    );

    // Frame-buffer
    RB::DestroyFrameBuffers(
        state->logicalDevice.device, 
        static_cast<uint32_t>(state->frameBuffers.size()), 
        state->frameBuffers.data()
    );
    state->frameBuffers = RB::CreateFrameBuffers(
        state->logicalDevice.device,
        state->renderPass,
        static_cast<uint32_t>(state->swapChainGroup.swapChainImageViews.size()),
        state->swapChainGroup.swapChainImageViews.data(),
        state->depthImageGroup.imageView,
        extent2D
    );

    // Command buffer
    RB::DestroyCommandBuffers(
        state->logicalDevice.device,
        state->graphicCommandPool,
        static_cast<uint32_t>(state->graphicCommandBuffers.size()),
        state->graphicCommandBuffers.data()
    );   

    state->graphicCommandBuffers = RB::CreateCommandBuffers(
        state->logicalDevice.device,
        static_cast<uint32_t>(state->swapChainGroup.swapChainImages.size()),
        state->graphicCommandPool
    );

    if (state->resizeEventListener != nullptr) {
        // Responsible for scenes
        state->resizeEventListener();
    }
}

bool Shutdown() {
    // Common part with resize
    RB::DeviceWaitIdle(state->logicalDevice.device);

#ifdef __DESKTOP__
    MFA_ASSERT(state->eventListeners.empty());
    MSDL::SDL_DelEventWatch(SDLEventWatcher, state->window);
#endif

    // DestroyPipeline in application // TODO We should have reference to what user creates + params for re-creation
    // GraphicPipeline, UniformBuffer, PipelineLayout
    // Shutdown only procedure
    RB::DestroySyncObjects(state->logicalDevice.device, state->syncObjects);
    RB::DestroyCommandBuffers(
        state->logicalDevice.device, 
        state->graphicCommandPool,
        static_cast<uint8_t>(state->graphicCommandBuffers.size()),
        state->graphicCommandBuffers.data()
    );
    RB::DestroyDescriptorPool(
        state->logicalDevice.device,
        state->descriptorPool
    );
    RB::DestroyFrameBuffers(
        state->logicalDevice.device,
        static_cast<uint8_t>(state->frameBuffers.size()),
        state->frameBuffers.data()
    );
    RB::DestroyDepth(
        state->logicalDevice.device,
        state->depthImageGroup
    );
    RB::DestroyRenderPass(state->logicalDevice.device, state->renderPass);
    RB::DestroySwapChain(
        state->logicalDevice.device,
        state->swapChainGroup
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

void SetResizeEventListener(ResizeEventListener const & eventListener) {
    state->resizeEventListener = eventListener;
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
        state->renderPass,
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

std::vector<VkDescriptorSet> CreateDescriptorSets(
    VkDescriptorSetLayout descriptorSetLayout
) {
    return RB::CreateDescriptorSet(
        state->logicalDevice.device,
        state->descriptorPool,
        descriptorSetLayout,
        static_cast<uint8_t>(state->swapChainGroup.swapChainImages.size())
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
    MFA_ASSERT(mesh.isValid());
    MeshBuffers buffers {};
    buffers.verticesBuffer = CreateVertexBuffer(mesh.getVerticesBuffer());
    buffers.indicesBuffer = CreateIndexBuffer(mesh.getIndicesBuffer());
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

[[nodiscard]]
DrawPass BeginPass() {
    MFA_ASSERT(MAX_FRAMES_IN_FLIGHT > state->currentFrame);
    DrawPass draw_pass {};
    if (state->isWindowVisible == false || state->windowResized == true) {
        DrawPass drawPass {};
        drawPass.isValid = false;
        return drawPass;
    }

    RB::WaitForFence(
        state->logicalDevice.device, 
        state->syncObjects.fences_in_flight[state->currentFrame]
    );
    // We ignore failed acquire of image because a resize will be triggered at end of pass
    RB::AcquireNextImage(
        state->logicalDevice.device,
        state->syncObjects.image_availability_semaphores[state->currentFrame],
        state->swapChainGroup,
        draw_pass.imageIndex
    );

    draw_pass.frame_index = state->currentFrame;
    draw_pass.isValid = true;
    state->currentFrame ++;
    if(state->currentFrame >= MAX_FRAMES_IN_FLIGHT) {
        state->currentFrame = 0;
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
        auto result = vkBeginCommandBuffer(state->graphicCommandBuffers[draw_pass.imageIndex], &beginInfo);
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

    if (state->presentQueueFamily != state->graphicQueueFamily) {
        presentToDrawBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        presentToDrawBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    }
    else {
        presentToDrawBarrier.srcQueueFamilyIndex = state->presentQueueFamily;
        presentToDrawBarrier.dstQueueFamilyIndex = state->graphicQueueFamily;
    }

    presentToDrawBarrier.image = state->swapChainGroup.swapChainImages[draw_pass.imageIndex];

    VkImageSubresourceRange subResourceRange = {};
    subResourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    subResourceRange.baseMipLevel = 0;
    subResourceRange.levelCount = 1;
    subResourceRange.baseArrayLayer = 0;
    subResourceRange.layerCount = 1;
    presentToDrawBarrier.subresourceRange = subResourceRange;

    vkCmdPipelineBarrier(
        state->graphicCommandBuffers[draw_pass.imageIndex], 
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 
        0, 0, nullptr, 0, nullptr, 1, 
        &presentToDrawBarrier
    );

    std::vector<VkClearValue> clearValues{};
    clearValues.resize(2);
    clearValues[0].color = VkClearColorValue { 0.1f, 0.1f, 0.1f, 1.0f };
    clearValues[1].depthStencil = {1.0f, 0};

    VkRenderPassBeginInfo renderPassBeginInfo = {};
    renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassBeginInfo.renderPass = state->renderPass;
    renderPassBeginInfo.framebuffer = state->frameBuffers[draw_pass.imageIndex];
    renderPassBeginInfo.renderArea.offset.x = 0;
    renderPassBeginInfo.renderArea.offset.y = 0;
    renderPassBeginInfo.renderArea.extent.width = static_cast<uint32_t>(state->screenWidth);
    renderPassBeginInfo.renderArea.extent.height = static_cast<uint32_t>(state->screenHeight);
    renderPassBeginInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassBeginInfo.pClearValues = clearValues.data();

    vkCmdBeginRenderPass(state->graphicCommandBuffers[draw_pass.imageIndex], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

    AssignViewportAndScissorToCommandBuffer(state->graphicCommandBuffers[draw_pass.imageIndex]);

    return draw_pass;
}

void BindDrawPipeline(
    DrawPass & drawPass,
    DrawPipeline & drawPipeline   
) {
    MFA_ASSERT(drawPass.isValid);
    drawPass.drawPipeline = &drawPipeline;
    // We can bind command buffer to multiple pipeline
    vkCmdBindPipeline(
        state->graphicCommandBuffers[drawPass.imageIndex], 
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
        state->graphicCommandBuffers[drawPass.imageIndex], 
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
    DrawPass const drawPass, 
    RB::BufferGroup const vertexBuffer,
    VkDeviceSize const offset
) {
    MFA_ASSERT(drawPass.isValid);
    RB::BindVertexBuffer(
        state->graphicCommandBuffers[drawPass.imageIndex], 
        vertexBuffer,
        offset
    );
}

void BindIndexBuffer(
    DrawPass const drawPass,
    RB::BufferGroup const indexBuffer,
    VkDeviceSize const offset,
    VkIndexType const indexType
) {
    MFA_ASSERT(drawPass.isValid);
    RB::BindIndexBuffer(
        state->graphicCommandBuffers[drawPass.imageIndex], 
        indexBuffer,
        offset,
        indexType
    );
}

void DrawIndexed(
    DrawPass const drawPass, 
    uint32_t const indicesCount,
    uint32_t const instanceCount,
    uint32_t const firstIndex,
    uint32_t const vertexOffset,
    uint32_t const firstInstance
) {
    RB::DrawIndexed(
        state->graphicCommandBuffers[drawPass.imageIndex], 
        indicesCount,
        instanceCount,
        firstIndex,
        vertexOffset,
        firstInstance
    );
}

void EndPass(DrawPass & drawPass) {
    if (state->isWindowVisible == false) {
        return;
    }
    // TODO Move these functions to renderBackend, RenderFrontend should not know about backend
    MFA_ASSERT(drawPass.isValid);
    drawPass.isValid = false;
    vkCmdEndRenderPass(state->graphicCommandBuffers[drawPass.imageIndex]);

    // If present and graphics queue families differ, then another barrier is required
    if (state->presentQueueFamily != state->graphicQueueFamily) {
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
        drawToPresentBarrier.srcQueueFamilyIndex = state->graphicQueueFamily;
        drawToPresentBarrier.dstQueueFamilyIndex = state->presentQueueFamily;
        drawToPresentBarrier.image = state->swapChainGroup.swapChainImages[drawPass.imageIndex];
        drawToPresentBarrier.subresourceRange = subResourceRange;
        // TODO Move to RB
        vkCmdPipelineBarrier(
            state->graphicCommandBuffers[drawPass.imageIndex], 
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 
            VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 
            0, 0, nullptr, 
            0, nullptr, 1, 
            &drawToPresentBarrier
        );
    }

    if (vkEndCommandBuffer(state->graphicCommandBuffers[drawPass.imageIndex]) != VK_SUCCESS) {
        MFA_CRASH("Failed to record command buffer");
    }

        // Wait for image to be available and draw
    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore wait_semaphores[] = {state->syncObjects.image_availability_semaphores[drawPass.frame_index]};
    VkPipelineStageFlags wait_stages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = wait_semaphores;
    submitInfo.pWaitDstStageMask = wait_stages;

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &state->graphicCommandBuffers[drawPass.imageIndex];

    VkSemaphore signal_semaphores[] = {state->syncObjects.render_finish_indicator_semaphores[drawPass.frame_index]};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signal_semaphores;

    vkResetFences(state->logicalDevice.device, 1, &state->syncObjects.fences_in_flight[drawPass.frame_index]);

    auto const result = vkQueueSubmit(
        state->graphicQueue, 
        1, &submitInfo, 
        state->syncObjects.fences_in_flight[drawPass.frame_index]
    );
    if (result != VK_SUCCESS) {
        MFA_CRASH("Failed to submit draw command buffer");
    }
    
    // Present drawn image
    // Note: semaphore here is not strictly necessary, because commands are processed in submission order within a single queue
    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signal_semaphores;
    VkSwapchainKHR swapChains[] = {state->swapChainGroup.swapChain};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    uint32_t imageIndices = drawPass.imageIndex;
    presentInfo.pImageIndices = &imageIndices;

    auto const res = vkQueuePresentKHR(state->presentQueue, &presentInfo);
    // TODO Handle resize event
    if (res == VK_SUBOPTIMAL_KHR || res == VK_ERROR_OUT_OF_DATE_KHR || state->windowResized == true) {
        OnWindowResized();
    } else if (res != VK_SUCCESS) {
        MFA_CRASH("Failed to submit present command buffer");
    }
}

void OnNewFrame(float deltaTimeInSec) {
    if (state->windowResized) {
        OnWindowResized();
    }
#ifdef __DESKTOP__
    state->isWindowVisible = (GetWindowFlags() & MSDL::SDL_WINDOW_MINIMIZED) > 0 ? false : true;
#elif defined(__ANDROID__)
#else
    #error Os is not handled
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

void SetScissor(DrawPass const & draw_pass, VkRect2D const & scissor) {
    RB::SetScissor(state->graphicCommandBuffers[draw_pass.imageIndex], scissor);
}

void SetViewport(DrawPass const & draw_pass, VkViewport const & viewport) {
    RB::SetViewport(state->graphicCommandBuffers[draw_pass.imageIndex], viewport);
}

void PushConstants(
    DrawPass const & draw_pass, 
    AssetSystem::ShaderStage const shader_stage, 
    uint32_t const offset, 
    CBlob const data
) {
    RB::PushConstants(
        state->graphicCommandBuffers[draw_pass.imageIndex],
        draw_pass.drawPipeline->pipelineLayout,
        shader_stage,
        offset,
        data
    );
}

uint8_t SwapChainImagesCount() {
    return static_cast<uint8_t>(state->swapChainGroup.swapChainImages.size());
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

void AssignViewportAndScissorToCommandBuffer(VkCommandBuffer commandBuffer) {
    MFA_ASSERT(commandBuffer != nullptr);
    VkExtent2D extent2D {};
    extent2D.width = static_cast<uint32_t>(state->screenWidth);
    extent2D.height = static_cast<uint32_t>(state->screenHeight);
    RB::AssignViewportAndScissorToCommandBuffer(
        extent2D,
        commandBuffer
    );
}

#ifdef __DESKTOP__
// TODO We might need separate SDL class
int AddEventWatch(EventWatch const & eventWatch) {
    MFA_ASSERT(eventWatch != nullptr);

    EventWatchGroup group {};
    group.id = state->nextEventListenerId;
    group.watch = eventWatch;

    ++state->nextEventListenerId;
    state->eventListeners.emplace_back(group);
    return group.id;
}

void RemoveEventWatch(int const watchId) {
    for (int i = static_cast<int>(state->eventListeners.size()) - 1; i >= 0; --i) {
        if (state->eventListeners[i].id == watchId) {
            state->eventListeners.erase(state->eventListeners.begin() + i);
        }
    }
}

#endif

void NotifyDeviceResized() {
    state->windowResized = true;
}

}
