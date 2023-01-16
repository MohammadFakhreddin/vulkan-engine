#pragma once

#include "engine/render_system/pipelines/BaseMaterial.hpp"

#include <functional>
#include <memory>

namespace MFA
{
    class DirectionalLightComponent;
    class PointLightComponent;
    class BaseMaterial;
    class Scene;
    class CameraComponent;
}

namespace MFA::SceneManager
{

    using CreateSceneFunction = std::function<std::shared_ptr<Scene>()>;
    using MainThreadTask = std::function<void()>;

    void Init();
    void Shutdown();

    // TODO: Scenes do not need update signal. They should have components for updating
    struct RegisterSceneOptions
    {
        bool keepAlive = false;
    };
    void RegisterScene(
        char const * name,
        CreateSceneFunction const & createSceneFunction,
        RegisterSceneOptions const & options = {}
    );

    void RegisterPipeline(std::unique_ptr<BaseMaterial> && pipeline);

    template<typename Pipeline>
    void RegisterPipeline()
    {
        RegisterPipeline(std::make_unique<Pipeline>());
    }

    [[nodiscard]]
    BaseMaterial * GetPipeline(std::string const & name);

    template<typename Pipeline>
    Pipeline * GetPipeline()
    {
        return dynamic_cast<Pipeline *>(GetPipeline(Pipeline::Name));
    }

    void AssignMainThreadTask(MainThreadTask const & task, bool executeIfMainThread = true);

    // This method must only be called by pipeline and sceneManager
    void UpdatePipeline(BaseMaterial * pipeline);

    // Only for editor, Not optimized performance wise.
    std::vector<BaseMaterial *> GetAllPipelines();

    // It will be called automatically at end of every scene
    // But you can call it to avoid facing out of memory on GPU
    void TriggerCleanup();

    void SetActiveScene(int nextSceneIndex);
    void SetActiveScene(char const * name);
    void Update(float deltaTime);
    void Render(float deltaTime);
    void OnResize();

    void OnUI();            // Can be called optionally for general info about the scenes

    [[nodiscard]]
    Scene * GetActiveScene();

    [[nodiscard]]
    RT::BufferGroup const * GetCameraBuffers();

    [[nodiscard]]
    RT::BufferGroup const * GetTimeBuffers();

    void RegisterPointLight(std::weak_ptr<PointLightComponent> const & pointLight);

    void RegisterDirectionalLight(std::weak_ptr<DirectionalLightComponent> const & directionalLight);

    [[nodiscard]]
    RT::BufferGroup const * GetPointLightsBuffers();

    [[nodiscard]]
    RT::BufferGroup const * GetDirectionalLightBuffers();

    [[nodiscard]]
    uint32_t GetPointLightCount();

    [[nodiscard]]
    uint32_t GetDirectionalLightCount();

    [[nodiscard]]
    std::vector<PointLightComponent *> const & GetActivePointLights();

    [[nodiscard]]
    std::weak_ptr<CameraComponent> GetActiveCamera();

}
