#include "BedrockCommon.hpp"

#include "RenderFrontend.hpp"

namespace MFA::UISystem {

namespace RF = MFA::RenderFrontend;

// TODO Support for custom font
void Init();

// TODO It might need to ask for window params
void OnNewFrame(U32 delta_time, RF::DrawPass & draw_pass);

void Shutdown();

}
