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
    RB::LogicalDevice logical_device {};
    VkCommandPool_T * graphic_command_pool = nullptr;
    RB::SwapChainGroup swap_chain_group {};
    VkRenderPass_T * render_pass = nullptr;
    RB::DepthImageGroup depth_image_group {};
    std::vector<VkFramebuffer_T *> frame_buffers {};
    VkDescriptorPool_T * descriptor_pool {};
    std::vector<VkCommandBuffer_T *> graphic_command_buffers {};
    RB::SyncObjects sync_objects {};
};

State state {};

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


}
