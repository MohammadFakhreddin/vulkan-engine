#include "./Scene.hpp"

#include <string>
#include <vector>

#include "libs/imgui/imgui.h"

namespace MFA::SceneSubSystem {

namespace UI = UISystem;
namespace RF = RenderFrontend;

struct RegisteredScene {
    Scene * scene = nullptr;
    std::string name {};        
};

static std::vector<RegisteredScene> registered_scenes {};
static std::vector<char const *> registered_scenes_names {};
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
    registered_scenes_names.emplace_back(registered_scenes.back().name.c_str());
}

void OnNewFrame(U32 const delta_time) {
    if(active_scene != last_active_scene) {
        if(last_active_scene >= 0) {
            registered_scenes[last_active_scene].scene->Shutdown();
        }
        registered_scenes[active_scene].scene->Init();
        last_active_scene = active_scene;
    }
    auto draw_pass = RF::BeginPass();
    MFA_ASSERT(draw_pass.is_valid);
        
    if(active_scene >= 0) {
        registered_scenes[active_scene].scene->OnNewFrame(
            delta_time,
            draw_pass
        );
    }

    //UI::OnNewFrame(delta_time, draw_pass, []()->void{
    //    ImGui::Begin("Scene Subsystem");
    //    ImGui::SetNextItemWidth(200.0f);
    //    ImGui::ListBox(
    //        "Select active scene", 
    //        &active_scene,
    //        registered_scenes_names.data(), 
    //        static_cast<I32>(registered_scenes_names.size())
    //    );
    //    ImGui::End();
    //});

    RF::EndPass(draw_pass);
    
}


}
