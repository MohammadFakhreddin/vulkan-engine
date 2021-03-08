#pragma once

#include "BedrockCommon.hpp"
#include "UISystem.hpp"

namespace MFA {

class Scene;

namespace SceneSubSystem {
void Init();
void Shutdown();
void RegisterNew(Scene * scene, char const * name);
void SetActiveScene(char const * name);
void OnNewFrame(U32 delta_time);
}

class Scene {
public:
    virtual ~Scene() = default;
    virtual void OnDraw(U32 delta_time, RenderFrontend::DrawPass & draw_pass) = 0;
    virtual void OnUI(U32 delta_time, RenderFrontend::DrawPass & draw_pass) = 0;
    virtual void Init() = 0;
    virtual void Shutdown() = 0;
protected:
    explicit Scene(char const * sceneName) {
        SceneSubSystem::RegisterNew(
            this,
            sceneName
        );
    }
};


}