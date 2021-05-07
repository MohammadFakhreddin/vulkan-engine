#pragma once

#include "UISystem.hpp"

namespace MFA {

class Scene;

class SceneSubSystem {
public:
    void Init();
    void Shutdown();
    void RegisterNew(Scene * scene, char const * name);
    void SetActiveScene(char const * name);
    void OnNewFrame(uint32_t deltaTime);
    void OnResize();
private:

    struct RegisteredScene {
        Scene * scene = nullptr;
        std::string name;        
    };
    
    std::vector<RegisteredScene> mRegisteredScenes;
    int32_t mActiveScene = -1;
    int32_t mLastActiveScene = -1;
};

class Scene {
public:
    virtual ~Scene() = default;
    virtual void OnDraw(uint32_t delta_time, RenderFrontend::DrawPass & draw_pass) = 0;
    virtual void OnUI(uint32_t delta_time, RenderFrontend::DrawPass & draw_pass) = 0;
    virtual void OnResize() = 0;
    virtual void Init() = 0;
    virtual void Shutdown() = 0;
};


}