#include "SceneManager.hpp"

#include "engine/BedrockAssert.hpp"
#include "engine/entity_system/EntitySystem.hpp"
#include "engine/ui_system/UISystem.hpp"
#include "engine/render_system/RenderFrontend.hpp"
#include "engine/render_system/render_passes/display_render_pass/DisplayRenderPass.hpp"
#include "engine/scene_manager/Scene.hpp"

namespace MFA::SceneManager {

struct RegisteredScene {
    Scene * scene = nullptr;
    std::string name {};        
};

struct State
{
    std::vector<RegisteredScene> RegisteredScenes {};
    int32_t ActiveScene = -1;
    int32_t LastActiveScene = -1;

    DisplayRenderPass * DisplayRenderPass {};
    
    int UI_RecordListenerId = 0;

    RT::ResizeEventListenerId ResizeListenerId = 0;
    
    float CurrentFps = 0.0f;

};
static State * state = nullptr;

static void OnUI();

//-------------------------------------------------------------------------------------------------

void Init() {
    state = new State();

    UI::Register([]()->void {OnUI();});
    state->ResizeListenerId = RF::AddResizeEventListener([]()->void {OnResize();});
    if(state->ActiveScene < 0 && false == state->RegisteredScenes.empty()) {
        state->ActiveScene = 0;
    }
    if(state->ActiveScene >= 0) {
        state->RegisteredScenes[state->ActiveScene].scene->Init();
        state->LastActiveScene = state->ActiveScene;
    }
    state->DisplayRenderPass = RF::GetDisplayRenderPass();
}

//-------------------------------------------------------------------------------------------------

void Shutdown() {
    RF::RemoveResizeEventListener(state->ResizeListenerId);
    if(state->ActiveScene >= 0) {
        state->RegisteredScenes[state->ActiveScene].scene->Shutdown();
    }
    state->ActiveScene = -1;
    state->LastActiveScene = -1;
    state->RegisteredScenes.resize(0);

    delete state;
}

//-------------------------------------------------------------------------------------------------

void RegisterNew(Scene * scene, char const * name) {
    MFA_ASSERT(scene != nullptr);
    MFA_ASSERT(name != nullptr);
    state->RegisteredScenes.emplace_back(RegisteredScene {scene, name});
}

//-------------------------------------------------------------------------------------------------

void SetActiveScene(char const * name) {
    MFA_ASSERT(name != nullptr);
    for (int32_t i = 0; i < static_cast<int32_t>(state->RegisteredScenes.size()); ++i) {
        if (0 == strcmp(state->RegisteredScenes[i].name.c_str(), name)) {
            state->ActiveScene = i;
        }
    }
}

//-------------------------------------------------------------------------------------------------

void OnNewFrame(float const deltaTimeInSec) {
    if (deltaTimeInSec > 0.0f) {
        state->CurrentFps = state->CurrentFps * 0.9f + (1.0f / deltaTimeInSec) * 0.1f;
    }

    if(state->ActiveScene != state->LastActiveScene) {
        RF::DeviceWaitIdle();
        if(state->LastActiveScene >= 0) {
            state->RegisteredScenes[state->LastActiveScene].scene->Shutdown();
        }
        state->RegisteredScenes[state->ActiveScene].scene->Init();
        state->LastActiveScene = state->ActiveScene;
    }

    EntitySystem::OnNewFrame(deltaTimeInSec);
    
    // Start of graphic record
    auto drawPass = state->DisplayRenderPass->StartGraphicCommandBufferRecording();
    if (drawPass.isValid == false) {
        return;
    }

    // Pre render
    if(state->ActiveScene >= 0) {
        state->RegisteredScenes[state->ActiveScene].scene->OnPreRender(
            deltaTimeInSec, 
            drawPass
        );
    }

    state->DisplayRenderPass->BeginRenderPass(drawPass); // Draw pass being invalid means that RF cannot render anything

    // Render
    if(state->ActiveScene >= 0) {
        state->RegisteredScenes[state->ActiveScene].scene->OnRender(
            deltaTimeInSec,
            drawPass
        );
    }
    UI::OnNewFrame(deltaTimeInSec, drawPass);

    state->DisplayRenderPass->EndRenderPass(drawPass);

    state->DisplayRenderPass->EndGraphicCommandBufferRecording(drawPass);
    // End of graphic record

    // Post render 
    if(state->ActiveScene >= 0) {
        state->RegisteredScenes[state->ActiveScene].scene->OnPostRender(
            deltaTimeInSec, 
            drawPass
        );
    }

}

//-------------------------------------------------------------------------------------------------

void OnResize() {
    if (state->ActiveScene >= 0) {
        state->RegisteredScenes[state->ActiveScene].scene->OnResize();
    }
}

//-------------------------------------------------------------------------------------------------

Scene * GetActiveScene()
{
    if (state->ActiveScene >= 0) {
        return state->RegisteredScenes[state->ActiveScene].scene;
    }
    return nullptr;
}

//-------------------------------------------------------------------------------------------------

static void OnUI() {
    UI::BeginWindow("Scene Subsystem");
    
    UI::SetNextItemWidth(300.0f);
    UI::Text(("Current fps is " + std::to_string(state->CurrentFps)).data());
    
    UI::SetNextItemWidth(300.0f);
    // TODO Bad for performance, Find a better name
    std::vector<char const *> scene_names {};
    if(false == state->RegisteredScenes.empty()) {
        for(auto & registered_scene : state->RegisteredScenes) {
            scene_names.emplace_back(registered_scene.name.c_str());
        }
    }
    UI::Combo(
        "Active scene", 
        &state->ActiveScene,
        scene_names.data(), 
        static_cast<int32_t>(scene_names.size())
    );
    UI::EndWindow();
}

//-------------------------------------------------------------------------------------------------

}
