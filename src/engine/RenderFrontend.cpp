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

}
