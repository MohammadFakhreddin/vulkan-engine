#include "SceneManager.hpp"

#include "engine/BedrockAssert.hpp"
#include "engine/entity_system/EntitySystem.hpp"
#include "engine/ui_system/UISystem.hpp"
#include "engine/render_system/RenderFrontend.hpp"
#include "engine/render_system/render_passes/display_render_pass/DisplayRenderPass.hpp"
#include "engine/scene_manager/Scene.hpp"
#include "engine/job_system/JobSystem.hpp"

namespace MFA::SceneManager
{

    struct RegisteredScene
    {
        CreateSceneFunction createFunction;
        std::string name{};
        bool keepAlive = false;
        std::shared_ptr<Scene> keepAlivePtr = nullptr;
    };

    struct State
    {
        std::vector<RegisteredScene> RegisteredScenes{};
        int32_t ActiveSceneIndex = -1;
        int32_t NextActiveSceneIndex = -1;
        std::shared_ptr<Scene> ActiveScene{};

        DisplayRenderPass * DisplayRenderPass{};

        int UI_RecordListenerId = 0;

        RT::ResizeEventListenerId ResizeListenerId = 0;
        float CurrentFps = 0.0f;


    };
    static State * state = nullptr;

    //-------------------------------------------------------------------------------------------------

    static void startNextActiveScene()
    {
        RF::DeviceWaitIdle();
        MFA_ASSERT(state->NextActiveSceneIndex >= 0);
        MFA_ASSERT(state->NextActiveSceneIndex < static_cast<int>(state->RegisteredScenes.size()));

        // TODO: We should help resource manager to keep files that are needed but also there is a risk to face out of memory
        if (state->ActiveScene != nullptr)
        {
            state->ActiveScene->Shutdown();
        }

        state->ActiveSceneIndex = state->NextActiveSceneIndex;
        auto & registeredScene = state->RegisteredScenes[state->ActiveSceneIndex];
        if (registeredScene.keepAlivePtr != nullptr)
        {
            state->ActiveScene = registeredScene.keepAlivePtr;
        }
        else
        {
            state->ActiveScene = registeredScene.createFunction();
            if (registeredScene.keepAlive)
            {
                registeredScene.keepAlivePtr = state->ActiveScene;
            }
        }
        MFA_ASSERT(state->ActiveScene != nullptr);
        state->ActiveScene->Init();

        state->NextActiveSceneIndex = -1;

        state->DisplayRenderPass->UseDepthImageLayoutAsUndefined(state->ActiveScene->isDisplayPassDepthImageInitialLayoutUndefined());


        // TODO We should reuse particles instead of re-creating them!
    }

    //-------------------------------------------------------------------------------------------------

    void Init()
    {
        state = new State();

        state->DisplayRenderPass = RF::GetDisplayRenderPass();

        state->ResizeListenerId = RF::AddResizeEventListener([]()->void { OnResize(); });
        if (state->ActiveSceneIndex < 0 && false == state->RegisteredScenes.empty())
        {
            state->ActiveSceneIndex = 0;
        }
        if (state->ActiveSceneIndex >= 0)
        {
            state->NextActiveSceneIndex = state->ActiveSceneIndex;
            startNextActiveScene();
        }
    }

    //-------------------------------------------------------------------------------------------------

    void Shutdown()
    {
        RF::RemoveResizeEventListener(state->ResizeListenerId);

        if (state->ActiveScene != nullptr)
        {
            state->ActiveScene->Shutdown();
        }
        state->ActiveSceneIndex = -1;
        state->NextActiveSceneIndex = -1;
        state->RegisteredScenes.clear();
        
        delete state;
    }

    //-------------------------------------------------------------------------------------------------

    void RegisterScene(
        char const * name,
        CreateSceneFunction const & createSceneFunction,
        RegisterSceneOptions const & options
    )
    {
        MFA_ASSERT(name != nullptr);
        MFA_ASSERT(strlen(name) > 0);
        MFA_ASSERT(createSceneFunction != nullptr);
        state->RegisteredScenes.emplace_back(RegisteredScene{
            .createFunction = createSceneFunction,
            .name = name,
            .keepAlive = options.keepAlive,
            .keepAlivePtr = nullptr
        });
    }

    //-------------------------------------------------------------------------------------------------

    void SetActiveScene(int const nextSceneIndex)
    {
        state->NextActiveSceneIndex = nextSceneIndex;
    }

    //-------------------------------------------------------------------------------------------------

    void SetActiveScene(char const * name)
    {
        MFA_ASSERT(name != nullptr);
        for (int32_t i = 0; i < static_cast<int32_t>(state->RegisteredScenes.size()); ++i)
        {
            if (0 == strcmp(state->RegisteredScenes[i].name.c_str(), name))
            {
                SetActiveScene(i);
                return;
            }
        }
        MFA_LOG_ERROR("Scene with name %s not found", name);
    }

    //-------------------------------------------------------------------------------------------------

    void OnNewFrame(float const deltaTimeInSec)
    {
        if (deltaTimeInSec > 0.0f)
        {
            state->CurrentFps = state->CurrentFps * 0.9f + (1.0f / deltaTimeInSec) * 0.1f;
        }
        
        if (state->NextActiveSceneIndex != -1)
        {
            startNextActiveScene();
        }

        // Start of graphic record
        auto recordState = RF::StartGraphicCommandBufferRecording();
        if (recordState.isValid == false)
        {
            return;
        }

        // Pre render
        if (state->ActiveScene)
        {
            state->ActiveScene->OnPreRender(
                deltaTimeInSec,
                recordState
            );
        }

        state->DisplayRenderPass->BeginRenderPass(recordState); // Draw pass being invalid means that RF cannot render anything

        // Render
        if (state->ActiveScene)
        {
            state->ActiveScene->OnRender(
                deltaTimeInSec,
                recordState
            );
        }
        UI::OnNewFrame(deltaTimeInSec, recordState);

        state->DisplayRenderPass->EndRenderPass(recordState);

        RF::EndGraphicCommandBufferRecording(recordState);
        // End of graphic record

        // Note: Order is important
        EntitySystem::OnNewFrame(deltaTimeInSec);

        // Post render 
        if (state->ActiveScene)
        {
            state->ActiveScene->OnPostRender(deltaTimeInSec);
        }

    }

    //-------------------------------------------------------------------------------------------------

    void OnResize()
    {
        if (state->ActiveScene != nullptr)
        {
            state->ActiveScene->OnResize();
        }
    }

    //-------------------------------------------------------------------------------------------------

    Scene * GetActiveScene()
    {
        return state->ActiveScene.get();
    }

    //-------------------------------------------------------------------------------------------------

    void OnUI()
    {
        UI::BeginWindow("Scene Subsystem");

        UI::SetNextItemWidth(300.0f);
        UI::Text(("Current fps is " + std::to_string(state->CurrentFps)).data());

        UI::SetNextItemWidth(300.0f);
        // TODO Bad for performance, Find a better name
        std::vector<char const *> scene_names{};
        if (false == state->RegisteredScenes.empty())
        {
            for (auto & registered_scene : state->RegisteredScenes)
            {
                scene_names.emplace_back(registered_scene.name.c_str());
            }
        }

        int activeSceneIndex = state->ActiveSceneIndex;
        UI::Combo(
            "Active scene",
            &activeSceneIndex,
            scene_names.data(),
            static_cast<int32_t>(scene_names.size())
        );
        if (activeSceneIndex != state->ActiveSceneIndex)
        {
            SetActiveScene(activeSceneIndex);
        }

        UI::EndWindow();
    }

    //-------------------------------------------------------------------------------------------------

}
