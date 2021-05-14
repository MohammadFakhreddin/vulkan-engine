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

void BeginWindow(char const * windowName);

void EndWindow();

void SetNextItemWidth(float nextItemWidth);

void Combo(
    char const * label,
    int32_t * selectedItemIndex,
    char const ** items,
    int32_t itemsCount
);

void SliderInt(
    char const * label,
    int * value,
    int minValue,
    int maxValue
);

void SliderFloat(
    char const * label,
    float * value,
    float minValue,
    float maxValue
);

void Checkbox(
    char const * label,
    bool * value
);

[[nodiscard]]
bool HasFocus();

void Shutdown();

}
