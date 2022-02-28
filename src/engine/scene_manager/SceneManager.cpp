#include "SceneManager.hpp"

#include "engine/BedrockAssert.hpp"
#include "engine/BedrockMatrix.hpp"
#include "engine/BedrockSignal.hpp"
#include "engine/camera/CameraComponent.hpp"
#include "engine/entity_system/EntitySystem.hpp"
#include "engine/entity_system/components/DirectionalLightComponent.hpp"
#include "engine/entity_system/components/PointLightComponent.hpp"
#include "engine/ui_system/UI_System.hpp"
#include "engine/render_system/RenderFrontend.hpp"
#include "engine/render_system/pipelines/BasePipeline.hpp"
#include "engine/render_system/render_passes/display_render_pass/DisplayRenderPass.hpp"
#include "engine/scene_manager/Scene.hpp"

namespace MFA::SceneManager
{

    static void prepareCameraBuffer();
    static void prepareDirectionalLightsBuffer();
    static void preparePointLightsBuffer();
    static void updateCameraBuffer(RT::CommandRecordState const & recordState);
    static void updateDirectionalLightsBuffer(RT::CommandRecordState const & recordState);
    static void updatePointLightsBuffer(RT::CommandRecordState const & recordState);

    struct PointLight
    {
        float position[3]{};
        float placeholder0 = 0.0f;
        float color[3]{};
        float maxSquareDistance = 0.0f;
        float linearAttenuation = 0.0f;
        float quadraticAttenuation = 0.0f;
        float placeholder1[2]{};
        float viewProjectionMatrices[6][16]{};
    };
    struct PointLightsBufferData
    {
        uint32_t count = 0;
        float constantAttenuation = 1.0f;
        float placeholder[2]{};
        PointLight items[RT::MAX_POINT_LIGHT_COUNT]{};
    };

    // https://stackoverflow.com/questions/9486364/why-cant-c-compilers-rearrange-struct-members-to-eliminate-alignment-padding
    // Directional light
    struct DirectionalLight
    {
        float direction[3]{};
        float placeholder0 = 0.0f;
        float color[3]{};
        float placeholder1 = 0.0f;
        float viewProjectionMatrix[16]{};
    };
    struct DirectionalLightData
    {
        uint32_t count = 0;
        float placeholder0 = 0.0f;
        float placeholder1 = 0.0f;
        float placeholder2 = 0.0f;
        DirectionalLight items[RT::MAX_DIRECTIONAL_LIGHT_COUNT]{};
    };

    struct RegisteredScene
    {
        CreateSceneFunction createFunction;
        std::string name{};
        bool keepAlive = false;
        std::shared_ptr<Scene> keepAlivePtr = nullptr;
    };


    using PreRenderSignal = Signal<RT::CommandRecordState &, float>;
    using RenderSignal = Signal<RT::CommandRecordState &, float>;
    using PostRenderSignal = Signal<float>;

    struct State
    {
        std::vector<RegisteredScene> registeredScenes{};
        int32_t activeSceneIndex = -1;
        int32_t nextActiveSceneIndex = -1;
        std::shared_ptr<Scene> activeScene{};
        SignalId activeSceneListenerId = InvalidSignalId;

        DisplayRenderPass * displayRenderPass{};

        int UI_RecordListenerId = 0;

        RT::ResizeEventListenerId resizeListenerId = 0;
        float currentFps = 0.0f;

        std::unordered_map<std::string, std::unique_ptr<BasePipeline>> pipelines{};

        PreRenderSignal preRenderSignal{};
        RenderSignal renderSignal1 {};
        RenderSignal renderSignal2 {};
        RenderSignal renderSignal3 {};
        PostRenderSignal postRenderSignal{};

        std::vector<std::weak_ptr<PointLightComponent>> pointLightComponents{};
        std::vector<std::weak_ptr<DirectionalLightComponent>> directionalLightComponents{};

        // Buffers 
        std::shared_ptr<RT::UniformBufferGroup> cameraBuffer{};

        // Point light
        PointLightsBufferData pointLightData{};
        std::vector<PointLightComponent *> activePointLights{};
        std::shared_ptr<RT::UniformBufferGroup> pointLightsBuffers{};

        // Directional light
        DirectionalLightData directionalLightData{};
        std::shared_ptr<RT::UniformBufferGroup> directionalLightBuffers{};

        // TODO Spot light

    };
    static State * state = nullptr;

    //-------------------------------------------------------------------------------------------------

    static void startNextActiveScene()
    {
        RF::DeviceWaitIdle();
        MFA_ASSERT(state->nextActiveSceneIndex >= 0);
        MFA_ASSERT(state->nextActiveSceneIndex < static_cast<int>(state->registeredScenes.size()));

        // We need to keep old scene pointer for a chance for reusing assets
        std::shared_ptr<Scene> oldScene = state->activeScene;
        if (state->activeScene != nullptr)
        {
            state->activeScene->Shutdown();
            if (state->activeScene->RequiresUpdate())
            {
                state->postRenderSignal.UnRegister(state->activeSceneListenerId);
            }
        }

        state->activeSceneIndex = state->nextActiveSceneIndex;
        auto & registeredScene = state->registeredScenes[state->activeSceneIndex];
        if (registeredScene.keepAlivePtr != nullptr)
        {
            state->activeScene = registeredScene.keepAlivePtr;
        }
        else
        {
            state->activeScene = registeredScene.createFunction();
            if (registeredScene.keepAlive)
            {
                registeredScene.keepAlivePtr = state->activeScene;
            }
        }
        MFA_ASSERT(state->activeScene != nullptr);
        state->activeScene->Init();
        if (state->activeScene->RequiresUpdate())
        {
            state->activeSceneListenerId = state->postRenderSignal.Register(
            [](float const deltaTime)->void
            {
                state->activeScene->Update(deltaTime);
            });
        }

        state->nextActiveSceneIndex = -1;
        
        oldScene = nullptr;
        TriggerCleanup();
    }

    //-------------------------------------------------------------------------------------------------

    void Init()
    {
        state = new State();

        state->displayRenderPass = RF::GetDisplayRenderPass();

        state->resizeListenerId = RF::AddResizeEventListener([]()->void { OnResize(); });
        if (state->activeSceneIndex < 0 && false == state->registeredScenes.empty())
        {
            state->activeSceneIndex = 0;
        }

        prepareCameraBuffer();
        prepareDirectionalLightsBuffer();
        preparePointLightsBuffer();

        if (state->activeSceneIndex >= 0)
        {
            state->nextActiveSceneIndex = state->activeSceneIndex;
            startNextActiveScene();
        }
    }

    //-------------------------------------------------------------------------------------------------

    void Shutdown()
    {
        RF::RemoveResizeEventListener(state->resizeListenerId);

        if (state->activeScene != nullptr)
        {
            state->activeScene->Shutdown();
        }

        for (auto const & entry : state->pipelines)
        {
            entry.second->shutdown();
        }

        state->activeSceneIndex = -1;
        state->nextActiveSceneIndex = -1;
        state->registeredScenes.clear();

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
        state->registeredScenes.emplace_back(RegisteredScene{
            .createFunction = createSceneFunction,
            .name = name,
            .keepAlive = options.keepAlive,
            .keepAlivePtr = nullptr
        });
    }

    //-------------------------------------------------------------------------------------------------

    void RegisterPipeline(std::unique_ptr<BasePipeline> && pipeline)
    {
        std::string const name = pipeline->GetName();
        MFA_ASSERT(name.empty() == false);
        if (MFA_VERIFY(state->pipelines.contains(name) == false))
        {
            UpdatePipeline(pipeline.get());
            pipeline->init();
            state->pipelines[name] = std::move(pipeline);
        }
        else
        {
            MFA_LOG_WARN("Pipeline with name %s already exists.", name.c_str());
        }
    }

    //-------------------------------------------------------------------------------------------------

    BasePipeline * GetPipeline(std::string const & name)
    {
        MFA_ASSERT(name.empty() == false);
        BasePipeline * pipeline = nullptr;

        auto const findResult = state->pipelines.find(name);
        if (findResult != state->pipelines.end())
        {
            pipeline = findResult->second.get();
        }
        return pipeline;
    }

    //-------------------------------------------------------------------------------------------------

    static RenderSignal * getRenderSignal(BasePipeline * pipeline)
    {
        MFA_ASSERT(pipeline != nullptr);

        RenderSignal * signal = nullptr;
        switch (pipeline->renderOrder())
        {
            case BasePipeline::RenderOrder::BeforeEverything:
                signal = &state->renderSignal1;
            break;
            case BasePipeline::RenderOrder::DontCare:
                signal = &state->renderSignal2;
            break;
            case BasePipeline::RenderOrder::AfterEverything:
                signal = &state->renderSignal3;
            break;
            default:
                MFA_NOT_IMPLEMENTED_YET("Mohammad Fakhreddin");
        }
        MFA_ASSERT(signal != nullptr);

        return signal;
    }

    //-------------------------------------------------------------------------------------------------

    void UpdatePipeline(BasePipeline * pipeline)
    {
        MFA_ASSERT(pipeline != nullptr);
        auto const requiredEvents = pipeline->requiredEvents();

        // Pre-render listener
        if (
            pipeline->mIsActive &&
            (requiredEvents & BasePipeline::EventTypes::PreRenderEvent) > 0 &&
            pipeline->mAllVariantsList.empty() == false
        )
        {
            if (pipeline->mPreRenderListenerId == InvalidSignalId)
            {
                pipeline->mPreRenderListenerId = state->preRenderSignal.Register(
                [pipeline](RT::CommandRecordState & commandRecord, float const deltaTime)->void{
                    pipeline->preRender(commandRecord, deltaTime);           
                });
            }
        } else if (pipeline->mPreRenderListenerId != InvalidSignalId)
        {
            state->preRenderSignal.UnRegister(pipeline->mPreRenderListenerId);
            pipeline->mPreRenderListenerId = InvalidSignalId;
        }

        // Render listener
        if (
            pipeline->mIsActive &&
            (requiredEvents & BasePipeline::EventTypes::RenderEvent) > 0 &&
            pipeline->mAllVariantsList.empty() == false
        )
        {
            if (pipeline->mRenderListenerId == InvalidSignalId)
            {
                RenderSignal * signal = getRenderSignal(pipeline);

                pipeline->mRenderListenerId = signal->Register(
                [pipeline](RT::CommandRecordState & commandRecord, float const deltaTime)->void{
                    pipeline->render(commandRecord, deltaTime);           
                });
            }
        } else if (pipeline->mRenderListenerId != InvalidSignalId)
        {
            RenderSignal * signal = getRenderSignal(pipeline);

            signal->UnRegister(pipeline->mRenderListenerId);
            pipeline->mRenderListenerId = InvalidSignalId;
        }

        // Post render
        if (
            pipeline->mIsActive &&
            (requiredEvents & BasePipeline::EventTypes::PostRenderEvent) > 0 &&
            pipeline->mAllVariantsList.empty() == false
        )
        {
            if (pipeline->mPostRenderListenerId == InvalidSignalId)
            {
                pipeline->mPostRenderListenerId = state->postRenderSignal.Register(
                [pipeline](float const deltaTime)->void{
                    pipeline->postRender(deltaTime);           
                });
            }
        } else if (pipeline->mPostRenderListenerId != InvalidSignalId)
        {
            state->postRenderSignal.UnRegister(pipeline->mPostRenderListenerId);
            pipeline->mPostRenderListenerId = InvalidSignalId;
        }
    }

    //-------------------------------------------------------------------------------------------------

    std::vector<BasePipeline *> GetAllPipelines()
    {
        std::vector<BasePipeline *> pipelines {};
        for (auto const & pair : state->pipelines)
        {
            pipelines.emplace_back(pair.second.get());
        }
        return pipelines;
    }

    //-------------------------------------------------------------------------------------------------

    void TriggerCleanup()
    {
        for (auto const & entry : state->pipelines)
        {
            entry.second->freeUnusedEssences();
        }
    }

    //-------------------------------------------------------------------------------------------------

    void SetActiveScene(int const nextSceneIndex)
    {
        state->nextActiveSceneIndex = nextSceneIndex;
    }

    //-------------------------------------------------------------------------------------------------

    void SetActiveScene(char const * name)
    {
        MFA_ASSERT(name != nullptr);
        for (int32_t i = 0; i < static_cast<int32_t>(state->registeredScenes.size()); ++i)
        {
            if (0 == strcmp(state->registeredScenes[i].name.c_str(), name))
            {
                SetActiveScene(i);
                return;
            }
        }
        MFA_LOG_ERROR("Scene with name %s not found", name);
    }

    //-------------------------------------------------------------------------------------------------

    void Update(float const deltaTimeInSec)
    {
        EntitySystem::Update(deltaTimeInSec);

        state->postRenderSignal.Emit(deltaTimeInSec);
        UI::PostRender(deltaTimeInSec);
    }

    //-------------------------------------------------------------------------------------------------

    void Render(float const deltaTimeInSec)
    {
        if (deltaTimeInSec > 0.0f)
        {
            state->currentFps = state->currentFps * 0.9f + (1.0f / deltaTimeInSec) * 0.1f;
        }

        if (state->nextActiveSceneIndex != -1)
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
        updateCameraBuffer(recordState);
        updateDirectionalLightsBuffer(recordState);
        updatePointLightsBuffer(recordState);

        state->preRenderSignal.Emit(recordState, deltaTimeInSec);

        state->displayRenderPass->BeginRenderPass(recordState); // Draw pass being invalid means that RF cannot render anything

        state->renderSignal1.Emit(recordState, deltaTimeInSec);
        state->renderSignal2.Emit(recordState, deltaTimeInSec);
        state->renderSignal3.Emit(recordState, deltaTimeInSec);

        // TODO: UI System should contain a pipeline and register it instead
        UI::Render(deltaTimeInSec, recordState);

        state->displayRenderPass->EndRenderPass(recordState);

        RF::EndGraphicCommandBufferRecording(recordState);

        // End of graphic record

    }

    //-------------------------------------------------------------------------------------------------

    void OnResize()
    {
        for (auto const & entry : state->pipelines)
        {
            entry.second->onResize();
        }
    }

    //-------------------------------------------------------------------------------------------------

    Scene * GetActiveScene()
    {
        return state->activeScene.get();
    }

    //-------------------------------------------------------------------------------------------------

    void OnUI()
    {
        UI::BeginWindow("Scene Subsystem");

        UI::SetNextItemWidth(300.0f);
        UI::Text(("Current fps is " + std::to_string(state->currentFps)).data());

        UI::SetNextItemWidth(300.0f);
        // TODO Bad for performance, Find a better name
        std::vector<char const *> scene_names{};
        if (false == state->registeredScenes.empty())
        {
            for (auto & registered_scene : state->registeredScenes)
            {
                scene_names.emplace_back(registered_scene.name.c_str());
            }
        }

        int activeSceneIndex = state->activeSceneIndex;
        UI::Combo(
            "Active scene",
            &activeSceneIndex,
            scene_names.data(),
            static_cast<int32_t>(scene_names.size())
        );
        if (activeSceneIndex != state->activeSceneIndex)
        {
            SetActiveScene(activeSceneIndex);
        }

        UI::EndWindow();
    }

    //-------------------------------------------------------------------------------------------------

    RT::UniformBufferGroup const & GetCameraBuffers()
    {
        return *state->cameraBuffer;
    }

    //-------------------------------------------------------------------------------------------------

    void RegisterPointLight(std::weak_ptr<PointLightComponent> const & pointLight)
    {
        state->pointLightComponents.emplace_back(pointLight);
    }

    //-------------------------------------------------------------------------------------------------

    void RegisterDirectionalLight(std::weak_ptr<DirectionalLightComponent> const & directionalLight)
    {
        state->directionalLightComponents.emplace_back(directionalLight);
    }

    //-------------------------------------------------------------------------------------------------

    RT::UniformBufferGroup const & GetPointLightsBuffers()
    {
        return *state->pointLightsBuffers;
    }

    //-------------------------------------------------------------------------------------------------

    RT::UniformBufferGroup const & GetDirectionalLightBuffers()
    {
        return *state->directionalLightBuffers;
    }

    //-------------------------------------------------------------------------------------------------

    uint32_t GetPointLightCount()
    {
        return state->pointLightData.count;
    }

    //-------------------------------------------------------------------------------------------------

    uint32_t GetDirectionalLightCount()
    {
        return state->directionalLightData.count;
    }

    //-------------------------------------------------------------------------------------------------

    std::vector<PointLightComponent *> const & GetActivePointLights()
    {
        return state->activePointLights;
    }

    //-------------------------------------------------------------------------------------------------

    void prepareCameraBuffer()
    {
        state->cameraBuffer = RF::CreateUniformBuffer(
            sizeof(CameraComponent::CameraBufferData),
            RF::GetMaxFramesPerFlight()
        );

        CameraComponent::CameraBufferData const cameraBufferData{};

        for (uint32_t frameIndex = 0; frameIndex < RF::GetMaxFramesPerFlight(); ++frameIndex)
        {
            RF::UpdateBuffer(
                *state->cameraBuffer->buffers[frameIndex],
                CBlobAliasOf(cameraBufferData)
            );
        }
    }

    //-------------------------------------------------------------------------------------------------

    void preparePointLightsBuffer()
    {
        state->pointLightsBuffers = RF::CreateUniformBuffer(
            sizeof(PointLightsBufferData),
            RF::GetMaxFramesPerFlight()
        );

        for (uint32_t frameIndex = 0; frameIndex < RF::GetMaxFramesPerFlight(); ++frameIndex)
        {
            RF::UpdateBuffer(
                *state->pointLightsBuffers->buffers[frameIndex],
                CBlobAliasOf(state->pointLightData)
            );
        }
    }

    //-------------------------------------------------------------------------------------------------

    void prepareDirectionalLightsBuffer()
    {
        state->directionalLightBuffers = RF::CreateUniformBuffer(
            sizeof(DirectionalLightData),
            RF::GetMaxFramesPerFlight()
        );

        for (uint32_t frameIndex = 0; frameIndex < RF::GetMaxFramesPerFlight(); ++frameIndex)
        {
            RF::UpdateBuffer(
                *state->directionalLightBuffers->buffers[frameIndex],
                CBlobAliasOf(state->directionalLightData)
            );
        }
    }

    //-------------------------------------------------------------------------------------------------

    static void updateCameraBuffer(RT::CommandRecordState const & recordState)
    {
        if (state->activeScene != nullptr)
        {
            auto const activeCamera = state->activeScene->GetActiveCamera();
            if (auto const cameraPtr = activeCamera.lock())
            {
                if (cameraPtr->IsCameraDataDirty())
                {
                    RF::UpdateBuffer(
                        *state->cameraBuffer->buffers[recordState.frameIndex],
                        CBlobAliasOf(cameraPtr->GetCameraData())
                    );
                }
            }
        }
    }

    //-------------------------------------------------------------------------------------------------

    static void updatePointLightsBuffer(RT::CommandRecordState const & recordState)
    {
        // Maybe we can search for another active camera 
        state->pointLightData.count = 0;
        for (int i = static_cast<int>(state->pointLightComponents.size()) - 1; i >= 0; --i)
        {
            if (state->pointLightComponents[i].expired())
            {
                state->pointLightComponents[i] = state->pointLightComponents.back();
                state->pointLightComponents.pop_back();
            }
        }

        state->activePointLights.clear();

        for (auto & pointLightComponent : state->pointLightComponents)
        {
            auto const ptr = pointLightComponent.lock();
            MFA_ASSERT(ptr != nullptr);
            if (ptr->IsVisible())
            {
                MFA_ASSERT(state->pointLightData.count < RT::MAX_POINT_LIGHT_COUNT);
                auto & item = state->pointLightData.items[state->pointLightData.count];

                //Future optimization: if (item.id != ptr->GetUniqueId() || ptr->IsDataDirty() == true){
                Matrix::CopyGlmToCells(ptr->GetLightColor(), item.color);
                Matrix::CopyGlmToCells(ptr->GetPosition(), item.position);
                ptr->GetShadowViewProjectionMatrices(item.viewProjectionMatrices);
                item.maxSquareDistance = ptr->GetMaxSquareDistance();
                item.linearAttenuation = ptr->GetLinearAttenuation();
                item.quadraticAttenuation = ptr->GetQuadraticAttenuation();

                state->activePointLights.emplace_back(ptr.get());
                ++state->pointLightData.count;
            }
        }

        RF::UpdateUniformBuffer(
            recordState,
            *state->pointLightsBuffers,
            CBlobAliasOf(state->pointLightData)
        );
    }

    //-------------------------------------------------------------------------------------------------

    static void updateDirectionalLightsBuffer(RT::CommandRecordState const & recordState)
    {
        state->directionalLightData.count = 0;
        for (int i = static_cast<int>(state->directionalLightComponents.size()) - 1; i >= 0; --i)
        {
            if (state->directionalLightComponents[i].expired())
            {
                state->directionalLightComponents[i] = state->directionalLightComponents.back();
                state->directionalLightComponents.pop_back();
            }
        }

        for (auto & directionalLightComponent : state->directionalLightComponents)
        {
            auto const ptr = directionalLightComponent.lock();
            MFA_ASSERT(ptr != nullptr);
            if (ptr->IsActive())
            {
                MFA_ASSERT(state->directionalLightData.count < RT::MAX_DIRECTIONAL_LIGHT_COUNT);
                auto & item = state->directionalLightData.items[state->directionalLightData.count];

                ptr->GetDirection(item.direction);
                ptr->GetColor(item.color);
                ptr->GetShadowViewProjectionMatrix(item.viewProjectionMatrix);
                
                ++state->directionalLightData.count;
            }
        }

        RF::UpdateUniformBuffer(
            recordState,
            *state->directionalLightBuffers,
            CBlobAliasOf(state->directionalLightData)
        );
    }

    //-------------------------------------------------------------------------------------------------

}
