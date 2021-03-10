#pragma once

#include "BedrockCommon.hpp"
#include "UISystem.hpp"

namespace MFA {

class Scene;

class SceneSubSystem {
public:
    void Init();
    void Shutdown();
    void RegisterNew(Scene * scene, char const * name);
    void SetActiveScene(char const * name);
    void OnNewFrame(U32 deltaTime);
private:

    struct RegisteredScene {
        Scene * scene = nullptr;
        std::string name;        
    };
    
    std::vector<RegisteredScene> mRegisteredScenes;
    I32 mActiveScene = -1;
    I32 mLastActiveScene = -1;
};

class Scene {
public:
    virtual ~Scene() = default;
    virtual void OnDraw(U32 delta_time, RenderFrontend::DrawPass & draw_pass) = 0;
    virtual void OnUI(U32 delta_time, RenderFrontend::DrawPass & draw_pass) = 0;
    virtual void Init() = 0;
    virtual void Shutdown() = 0;
};


}