#include "./Scene.hpp"

#include <vector>

#include "libs/imgui/imgui.h"
#include "render_system/render_passes/DisplayRenderPass.hpp"

namespace MFA {

namespace UI = UISystem;
namespace RF = RenderFrontend;

SceneSubSystem::SceneSubSystem()
    : mUIRecordObject([this]()->void {OnUI();})
{}

void SceneSubSystem::Init() {
    mUIRecordObject.Enable();
    mResizeListenerId = RF::AddResizeEventListener([this]()->void {OnResize();});
    if(mActiveScene < 0 && false == mRegisteredScenes.empty()) {
        mActiveScene = 0;
    }
    if(mActiveScene >= 0) {
        mRegisteredScenes[mActiveScene].scene->Init();
        mLastActiveScene = mActiveScene;
    }
    mDisplayRenderPass = RF::GetDisplayRenderPass();
}

void SceneSubSystem::Shutdown() {
    RF::RemoveResizeEventListener(mResizeListenerId);
    if(mActiveScene >= 0) {
        mRegisteredScenes[mActiveScene].scene->Shutdown();
    }
    mActiveScene = -1;
    mLastActiveScene = -1;
    mRegisteredScenes.resize(0);
    mUIRecordObject.Disable();
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

    RF::OnNewFrame(deltaTimeInSec);

    if(mActiveScene >= 0) {
        mRegisteredScenes[mActiveScene].scene->OnUpdate(deltaTimeInSec);
    }

    auto drawPass = mDisplayRenderPass->Begin();                // Draw pass being invalid means that RF cannot render anything
    if (drawPass.isValid == false) {
        return;
    }
    
    if(mActiveScene >= 0) {
        mRegisteredScenes[mActiveScene].scene->OnDraw(
            deltaTimeInSec,
            drawPass
        );
    }
    UI::OnNewFrame(deltaTimeInSec, drawPass);

    mDisplayRenderPass->End(drawPass); 
}

void SceneSubSystem::OnResize() {
    if (mActiveScene >= 0) {
        mRegisteredScenes[mActiveScene].scene->OnResize();
    }
}

void SceneSubSystem::OnUI() {
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
}

}
