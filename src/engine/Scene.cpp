#include "./Scene.hpp"

#include <vector>

#include "libs/imgui/imgui.h"

namespace MFA {

namespace UI = UISystem;
namespace RF = RenderFrontend;

void SceneSubSystem::Init() {
    RF::SetResizeEventListener([this]()->void {OnResize();});
    if(mActiveScene < 0 && false == mRegisteredScenes.empty()) {
        mActiveScene = 0;
    }
    if(mActiveScene >= 0) {
        mRegisteredScenes[mActiveScene].scene->Init();
        mLastActiveScene = mActiveScene;
    }
}

void SceneSubSystem::Shutdown() {
    RF::SetResizeEventListener(nullptr);
    if(mActiveScene >= 0) {
        mRegisteredScenes[mActiveScene].scene->Shutdown();
    }
    mActiveScene = -1;
    mLastActiveScene = -1;
    mRegisteredScenes.resize(0);
}

void SceneSubSystem::RegisterNew(Scene * scene, char const * name) {
    MFA_ASSERT(scene != nullptr);
    MFA_ASSERT(name != nullptr);
    mRegisteredScenes.emplace_back(RegisteredScene {scene, name});
}

void SceneSubSystem::SetActiveScene(char const * name) {
    MFA_ASSERT(name != nullptr);
    for (int32_t i = 0; i < static_cast<int32_t>(mRegisteredScenes.size()); ++i) {
        if (0 == strcmp(mRegisteredScenes[i].name.c_str(), name)) {
            mActiveScene = i;
        }
    }
}

void SceneSubSystem::OnNewFrame(float const deltaTimeInSec) {
    if(mActiveScene != mLastActiveScene) {
        RF::DeviceWaitIdle();
        if(mLastActiveScene >= 0) {
            mRegisteredScenes[mLastActiveScene].scene->Shutdown();
        }
        mRegisteredScenes[mActiveScene].scene->Init();
        mLastActiveScene = mActiveScene;
    }
    auto draw_pass = RF::BeginPass();
    MFA_ASSERT(draw_pass.isValid);
        
    if(mActiveScene >= 0) {
        mRegisteredScenes[mActiveScene].scene->OnDraw(
            deltaTimeInSec,
            draw_pass
        );
    }
    // TODO Refactor and use interface and register instead
    UI::OnNewFrame(deltaTimeInSec, draw_pass, [&deltaTimeInSec, &draw_pass, this]()->void{
        UI::BeginWindow("Scene Subsystem");
        UI::SetNextItemWidth(300.0f);
        // TODO Bad for performance, Find a better name
        std::vector<char const *> scene_names {};
        if(false == mRegisteredScenes.empty()) {
            for(auto & registered_scene : mRegisteredScenes) {
                scene_names.emplace_back(registered_scene.name.c_str());
            }
        }
        UI::Combo(
            "Active scene", 
            &mActiveScene,
            scene_names.data(), 
            static_cast<int32_t>(scene_names.size())
        );
        UI::EndWindow();
        if(mActiveScene >= 0) {
            mRegisteredScenes[mActiveScene].scene->OnUI(
                deltaTimeInSec,
                draw_pass
            );
        } 
    });

    RF::EndPass(draw_pass);
    
}

void SceneSubSystem::OnResize() {
    if (mActiveScene >= 0) {
        mRegisteredScenes[mActiveScene].scene->OnResize();
    }
}


}
