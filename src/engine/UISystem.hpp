#include "BedrockCommon.hpp"

#include "RenderFrontend.hpp"

namespace MFA::UISystem {

namespace RF = MFA::RenderFrontend;

// TODO Support for custom font
void Init();

using RecordUICallback = std::function<void()>;
// TODO It might need to ask for window params
void OnNewFrame(
    uint32_t delta_time, 
    RF::DrawPass & draw_pass,
    RecordUICallback const & record_ui_callback
);

void Shutdown();

}
