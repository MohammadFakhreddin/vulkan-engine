#pragma once

#include <vec3.hpp>

#include "engine/render_system/RenderTypesFWD.hpp"

class UIRecordObject;

namespace MFA::UISystem
{

    // TODO Support for custom font
    void Init();

    void OnNewFrame(
        float deltaTimeInSec,
        RT::CommandRecordState & drawPass
    );

    void BeginWindow(char const * windowName);

    void EndWindow();

    int Register(std::function<void()> const & listener);

    bool UnRegister(int listenerId);

    void SetNextItemWidth(float nextItemWidth);

    void Text(char const * label);

    void InputFloat(char const * label, float * value);

    void InputFloat2(char const * label, float * value);

    void InputFloat3(char const * label, float * value);

    void InputFloat4(char const * label, float * value);

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

    [[nodiscard]]
    bool IsItemActive();

    [[nodiscard]]
    bool TreeNode(char const * name);

    void TreePop();

#ifdef __ANDROID__
    void SetAndroidApp(android_app * pApp);
#endif

}

namespace MFA
{
    namespace UI = UISystem;
}
