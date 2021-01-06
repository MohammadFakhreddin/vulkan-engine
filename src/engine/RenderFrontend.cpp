#include "RenderFrontend.hpp"

#include <string>

#include "BedrockAssert.hpp"
#include "BedrockLog.hpp"
#include "RenderBackend.hpp"

#include <string>

namespace MFA::RenderFrontend {

using namespace RenderBackend;

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
    LogicalDevice logical_device {};
    VkCommandPool_T * graphic_command_pool = nullptr;
    SwapChainGroup swap_chain_group {};
    VkRenderPass_T * render_pass = nullptr;
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
    MFA_DEBUG_CRASH("Vulkan debug callback");
}

bool Init(InitParams const & params) {
    {
        state.application_name = params.application_name;
        state.screen_width = params.screen_width;
        state.screen_height = params.screen_height;
    }
    state.window = CreateWindow(
        state.screen_width, 
        state.screen_height
    );
    state.vk_instance = CreateInstance(
        state.application_name.c_str(), 
        state.window
    );
    state.vk_debug_report_callback_ext = CreateDebugCallback(
        state.vk_instance,
        DebugCallback
    );
    state.surface = CreateWindowSurface(state.window, state.vk_instance);
    {
        auto const find_physical_device_result = FindPhysicalDevice(state.vk_instance, 10); // TODO Check again for retry count number
        state.physical_device = find_physical_device_result.physical_device;
        state.physical_device_features = find_physical_device_result.physical_device_features;
    }
    if(false == CheckSwapChainSupport(state.physical_device)) {
        MFA_LOG_ERROR("Swapchain is not supported on this device");
        return false;
    }
    {
        auto const find_queue_family_result = FindPresentAndGraphicQueueFamily(state.physical_device, state.surface);
        state.graphic_queue_family = find_queue_family_result.graphic_queue_family;
        state.present_queue_family = find_queue_family_result.present_queue_family;
    }
    state.logical_device = CreateLogicalDevice(
        state.physical_device,
        state.graphic_queue_family,
        state.present_queue_family,
        state.physical_device_features
    );
    state.graphic_command_pool = CreateCommandPool(state.logical_device.device, state.graphic_queue_family);
    // Creating sampler/TextureImage/VertexBuffer and IndexBuffer is on application level
    state.swap_chain_group = CreateSwapChain(
        state.logical_device.device,
        state.physical_device,
        state.surface,
        {state.screen_width, state.screen_height}
    );
    // TODO We might need special render pass
    state.render_pass = CreateRenderPass(
        state.physical_device,
        state.logical_device.device,
        state.swap_chain_group.swap_chain_format
    );
    /*createImageViews();
    createFramebuffers();
    createGraphicsPipeline();
    createUniformBuffer();
    createDescriptorPool();
    createDescriptorSet();
    createCommandBuffers();
    createSyncObjects();*/
}

}
