#include "SceneManager.hpp"

#include "engine/BedrockAssert.hpp"
#include "engine/entity_system/EntitySystem.hpp"
#include "engine/ui_system/UISystem.hpp"
#include "engine/render_system/RenderFrontend.hpp"
#include "engine/render_system/render_passes/display_render_pass/DisplayRenderPass.hpp"
#include "engine/scene_manager/Scene.hpp"

namespace MFA::SceneManager {

struct RegisteredScene {
    std::weak_ptr<Scene> scene {};
    std::string name {};        
};

struct State
{
    std::vector<RegisteredScene> RegisteredScenes {};
    int32_t ActiveSceneIndex = -1;
    int32_t LastActiveSceneIndex = -1;
    std::weak_ptr<Scene> ActiveScene {};

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
    if(state->ActiveSceneIndex < 0 && false == state->RegisteredScenes.empty()) {
        state->ActiveSceneIndex = 0;
    }
    if(state->ActiveSceneIndex >= 0) {
        state->ActiveScene = state->RegisteredScenes[state->ActiveSceneIndex].scene;
        if (auto const ptr = state->ActiveScene.lock())
        {
            ptr->Init();
        }
        state->LastActiveSceneIndex = state->ActiveSceneIndex;
    }
    state->DisplayRenderPass = RF::GetDisplayRenderPass();
}

//-------------------------------------------------------------------------------------------------

void Shutdown() {
    RF::RemoveResizeEventListener(state->ResizeListenerId);
    if (auto const ptr = state->ActiveScene.lock())
    {
        ptr->Shutdown();
    }
    state->ActiveSceneIndex = -1;
    state->LastActiveSceneIndex = -1;
    state->RegisteredScenes.resize(0);

    delete state;
}

//-------------------------------------------------------------------------------------------------

void RegisterScene(std::weak_ptr<Scene> const & scene, char const * name) {
    MFA_ASSERT(scene.expired() == false);
    MFA_ASSERT(name != nullptr);
    state->RegisteredScenes.emplace_back(RegisteredScene {scene, name});
}

//-------------------------------------------------------------------------------------------------

void SetActiveScene(char const * name) {
    MFA_ASSERT(name != nullptr);
    for (int32_t i = 0; i < static_cast<int32_t>(state->RegisteredScenes.size()); ++i) {
        if (0 == strcmp(state->RegisteredScenes[i].name.c_str(), name)) {
            state->ActiveSceneIndex = i;
            state->ActiveScene = state->RegisteredScenes[i].scene;
            MFA_ASSERT(state->ActiveScene.expired() == false);
        }
    }
}

//-------------------------------------------------------------------------------------------------

void OnNewFrame(float const deltaTimeInSec) {
    if (deltaTimeInSec > 0.0f) {
        state->CurrentFps = state->CurrentFps * 0.9f + (1.0f / deltaTimeInSec) * 0.1f;
    }

    // TODO We might need a logic step here as well

    if(state->ActiveSceneIndex != state->LastActiveSceneIndex) {
        RF::DeviceWaitIdle();
        if (state->LastActiveSceneIndex >= 0)
        {
            if(auto const ptr = state->RegisteredScenes[state->LastActiveSceneIndex].scene.lock()) {
                ptr->Shutdown();
            }
        }
        if (auto const ptr = state->ActiveScene.lock())
        {
            ptr->Init();
        }
        state->LastActiveSceneIndex = state->ActiveSceneIndex;
    }

    // Start of graphic record
    auto drawPass = RF::StartGraphicCommandBufferRecording();
    if (drawPass.isValid == false) {
        return;
    }

    EntitySystem::OnNewFrame(deltaTimeInSec, drawPass);

    auto const activeScenePtr = state->ActiveScene.lock();
    // Pre render
    if(activeScenePtr) {
        activeScenePtr->OnPreRender(
            deltaTimeInSec, 
            drawPass
        );
    }

    state->DisplayRenderPass->BeginRenderPass(drawPass); // Draw pass being invalid means that RF cannot render anything

    // Render
    if(activeScenePtr) {
        activeScenePtr->OnRender(
            deltaTimeInSec,
            drawPass
        );
    }
    UI::OnNewFrame(deltaTimeInSec, drawPass);

    state->DisplayRenderPass->EndRenderPass(drawPass);

    // Post render 
    if(activeScenePtr) {
        activeScenePtr->OnPostRender(
            deltaTimeInSec, 
            drawPass
        );
    }

    RF::EndGraphicCommandBufferRecording(drawPass);
    // End of graphic record

}

//-------------------------------------------------------------------------------------------------

void OnResize() {
    if (auto const ptr = state->ActiveScene.lock()) {
        ptr->OnResize();
    }
}

//-------------------------------------------------------------------------------------------------

std::weak_ptr<Scene> const & GetActiveScene()
{
    return state->ActiveScene;
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
        &state->ActiveSceneIndex,
        scene_names.data(), 
        static_cast<int32_t>(scene_names.size())
    );
    UI::EndWindow();
}

//-------------------------------------------------------------------------------------------------

}
