#pragma once

#include <string>
#include <vector>

#include "ui_system/UISystem.hpp"

namespace MFA {

class Scene;
class DisplayRenderPass;

class SceneSubSystem {
public:
    explicit SceneSubSystem();
    void Init();
    void Shutdown();
    void RegisterNew(Scene * scene, char const * name);
    void SetActiveScene(char const * name);
    void OnNewFrame(float deltaTimeInSec);
    void OnResize();
private:

    void OnUI();

    struct RegisteredScene {
        Scene * scene = nullptr;
        std::string name;        
    };
    
    std::vector<RegisteredScene> mRegisteredScenes {};
    int32_t mActiveScene = -1;
    int32_t mLastActiveScene = -1;

    DisplayRenderPass * mDisplayRenderPass {};
    
    UIRecordObject mUIRecordObject;

    RenderFrontend::ResizeEventListenerId mResizeListenerId = 0;
};

class Scene {
public:

    explicit Scene() = default;
    virtual ~Scene() = default;
    Scene & operator= (Scene && rhs) noexcept = delete;
    Scene (Scene const &) noexcept = delete;
    Scene (Scene && rhs) noexcept = delete;
    Scene & operator = (Scene const &) noexcept = delete;

    virtual void OnUpdate(float deltaTimeInSec) {};
    virtual void OnDraw(float deltaTimeInSec, RenderFrontend::DrawPass & drawPass) = 0;
    virtual void OnResize() = 0;
    virtual void Init() = 0;
    virtual void Shutdown() = 0;

};


}