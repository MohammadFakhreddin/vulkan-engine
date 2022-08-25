#pragma once

#include "engine/render_system/RenderTypesFWD.hpp"

#include <glm/vec3.hpp>

#ifdef __ANDROID__
#include <android_native_app_glue.h>
#endif

namespace MFA::UI_System
{

    // TODO Support for custom font
    void Init();

    bool Render(
        float deltaTimeInSec,
        RT::CommandRecordState & recordState
    );

    void PostRender(float deltaTimeInSec);

    void BeginWindow(char const * windowName);

    void EndWindow();

    int Register(std::function<void()> const & listener);

    bool UnRegister(int listenerId);

    void SetNextItemWidth(float nextItemWidth);

    void Text(char const * label, ...);

    void InputFloat1(char const * label, float * value);

    void InputFloat2(char const * label, float * value);

    void InputFloat3(char const * label, float * value);

    void InputFloat4(char const * label, float * value);

    // Returns true if changed
    template<typename T>
    bool InputFloat(char const * label, T & value)
    {
        auto constexpr Count = sizeof(T) / sizeof(float);

        //static_assert(sizeof(float) * Count <= sizeof(T));
        bool changed = false;

        float tempValue[Count];
        Copy<Count>(tempValue, value);

        //float tempValue[3] {value.x, value.y, value.z};
        if constexpr (Count == 1)
        {
            InputFloat1(label, tempValue);
        }
        else if constexpr (Count == 2)
        {
            InputFloat2(label, tempValue);
        }
        else if constexpr (Count == 3)
        {
            InputFloat3(label, tempValue);
        }
        else if constexpr (Count == 4)
        {
            InputFloat4(label, tempValue);
        }

        if (IsEqual<Count>(value, tempValue) == false)
        {
            changed = true;
            Copy<Count>(value, tempValue);
        }

        return changed;
    }

    bool Combo(
        char const * label,
        int32_t * selectedItemIndex,
        char const ** items,
        int32_t itemsCount
    );

    bool Combo(
        const char * label,
        int * selectedItemIndex,
        std::vector<std::string> & values
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

    bool Checkbox(
        char const * label,
        bool & value
    );

    void Spacing();

    void Button(
        char const * label,
        std::function<void()> const & onPress
    );

    void InputText(
        char const * label,
        std::string & outValue
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
    namespace UI = UI_System;
}
