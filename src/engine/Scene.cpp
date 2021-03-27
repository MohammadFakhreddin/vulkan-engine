#include "./Scene.hpp"

#include <vector>

#include "libs/imgui/imgui.h"

namespace MFA {

namespace UI = UISystem;
namespace RF = RenderFrontend;

void SceneSubSystem::Init() {
    if(mActiveScene < 0 && false == mRegisteredScenes.empty()) {
        mActiveScene = 0;
    }
    if(mActiveScene >= 0) {
        mRegisteredScenes[mActiveScene].scene->Init();
        mLastActiveScene = mActiveScene;
    }
}

void SceneSubSystem::Shutdown() {
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
    for (I32 i = 0; i < static_cast<I32>(mRegisteredScenes.size()); ++i) {
        if (0 == strcmp(mRegisteredScenes[i].name.c_str(), name)) {
            mActiveScene = i;
        }
    }
}

void SceneSubSystem::OnNewFrame(U32 const deltaTime) {
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
            deltaTime,
            draw_pass
        );
    }
    // TODO Refactor and use interface and register instead
    UI::OnNewFrame(deltaTime, draw_pass, [&deltaTime, &draw_pass, this]()->void{
        ImGui::Begin("Scene Subsystem");
        ImGui::SetNextItemWidth(300.0f);
        // TODO Bad for performance, Find a better name
        std::vector<char const *> scene_names {};
        if(false == mRegisteredScenes.empty()) {
            for(auto & registered_scene : mRegisteredScenes) {
                scene_names.emplace_back(registered_scene.name.c_str());
            }
        }
        ImGui::Combo(
            "Active scene", 
            &mActiveScene,
            scene_names.data(), 
            static_cast<I32>(scene_names.size())
        );
        ImGui::End();
        if(mActiveScene >= 0) {
            mRegisteredScenes[mActiveScene].scene->OnUI(
                deltaTime,
                draw_pass
            );
        } 
    });

    RF::EndPass(draw_pass);
    
}


}
