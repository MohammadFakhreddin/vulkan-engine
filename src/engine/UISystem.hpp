#pragma once

#include "RenderFrontend.hpp"

namespace MFA::UISystem {

namespace RF = MFA::RenderFrontend;

// TODO Support for custom font
void Init();

using RecordUICallback = std::function<void()>;
// TODO It might need to ask for window params
void OnNewFrame(
    float deltaTimeInSec, 
    RF::DrawPass & drawPass,
    RecordUICallback const & recordUiCallback
);

void BeginWindow(char const * windowName);

void EndWindow();

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

[[nodiscard]]
bool HasFocus();

void Shutdown();

}
