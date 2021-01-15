#include "./Scene.hpp"

#include <string>
#include <vector>

#include "libs/imgui/imgui.h"

namespace MFA::SceneSubSystem {

namespace UI = UISystem;
namespace RF = RenderFrontend;

struct RegisteredScene {
    Scene * scene = nullptr;
    std::string name;        
};

static std::vector<RegisteredScene> registered_scenes {};
static I32 active_scene = -1;
static I32 last_active_scene = -1;

void Init() {
    if(active_scene < 0 && false == registered_scenes.empty()) {
        active_scene = 0;
    }
    if(active_scene >= 0) {
        registered_scenes[active_scene].scene->Init();
        last_active_scene = active_scene;
    }
}

void Shutdown() {
    if(active_scene >= 0) {
        registered_scenes[active_scene].scene->Shutdown();
    }
    active_scene = -1;
    last_active_scene = -1;
    registered_scenes.resize(0);
}

void RegisterNew(Scene * scene, char const * name) {
    MFA_PTR_ASSERT(scene);
    MFA_PTR_ASSERT(name);
    registered_scenes.emplace_back(RegisteredScene {scene, name});
}

void OnNewFrame(U32 const delta_time) {
    if(active_scene != last_active_scene) {
        RF::DeviceWaitIdle();
        if(last_active_scene >= 0) {
            registered_scenes[last_active_scene].scene->Shutdown();
        }
        registered_scenes[active_scene].scene->Init();
        last_active_scene = active_scene;
    }
    auto draw_pass = RF::BeginPass();
    MFA_ASSERT(draw_pass.is_valid);
        
    if(active_scene >= 0) {
        registered_scenes[active_scene].scene->OnDraw(
            delta_time,
            draw_pass
        );
    }

    UI::OnNewFrame(delta_time, draw_pass, [&delta_time, &draw_pass]()->void{
        ImGui::Begin("Scene Subsystem");
        ImGui::SetNextItemWidth(300.0f);
        // TODO Bad for performance, Find a better name
        std::vector<char const *> scene_names {};
        if(false == registered_scenes.empty()) {
            for(auto & registered_scene : registered_scenes) {
                scene_names.emplace_back(registered_scene.name.c_str());
            }
        }
        ImGui::Combo(
            "Active scene", 
            &active_scene,
            scene_names.data(), 
            static_cast<I32>(scene_names.size())
        );
        ImGui::End();
        if(active_scene >= 0) {
            registered_scenes[active_scene].scene->OnUI(
                delta_time,
                draw_pass
            );
        } 
    });

    RF::EndPass(draw_pass);
    
}


}
