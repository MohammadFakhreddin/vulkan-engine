#pragma once

#include "UIRecordObject.hpp"
#include "engine/render_system/RenderFrontend.hpp"

class UIRecordObject;

namespace MFA::UISystem {

namespace RF = MFA::RenderFrontend;

// TODO Support for custom font
void Init();

void OnNewFrame(
    float deltaTimeInSec, 
    RF::DrawPass & drawPass
);

void BeginWindow(char const * windowName);

void EndWindow();

void Register(UIRecordObject * recordObject);

void UnRegister(UIRecordObject * recordObject);

void SetNextItemWidth(float nextItemWidth);

void InputFloat(char const * label, float * value);

void InputFloat2(char const * label, float value[2]);

void InputFloat3(char const * label, float value[3]);

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

void Spacing();

void Button(
    char const * label, 
    std::function<void()> const & onPress
);

[[nodiscard]]
bool HasFocus();

void Shutdown();

#ifdef __ANDROID__
void SetAndroidApp(android_app *pApp);
#endif

}
