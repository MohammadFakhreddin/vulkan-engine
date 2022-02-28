#pragma once

#include "engine/render_system/pipelines/BasePipeline.hpp"

#include <functional>
#include <memory>

namespace MFA
{
    class DirectionalLightComponent;
    class PointLightComponent;
    class BasePipeline;
    class Scene;
    class CameraComponent;
}

namespace MFA::SceneManager
{

    using CreateSceneFunction = std::function<std::shared_ptr<Scene>()>;

    
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

    void RegisterPipeline(std::unique_ptr<BasePipeline> && pipeline);

    template<typename Pipeline>
    void RegisterPipeline()
    {
        RegisterPipeline(std::make_unique<Pipeline>());
    }

    [[nodiscard]]
    BasePipeline * GetPipeline(std::string const & name);

    template<typename Pipeline>
    Pipeline * GetPipeline()
    {
        return dynamic_cast<Pipeline *>(GetPipeline(Pipeline::Name));
    }

    // This method must only be called by pipeline and sceneManager
    void UpdatePipeline(BasePipeline * pipeline);

    // Only for editor, Not optimized performance wise.
    std::vector<BasePipeline *> GetAllPipelines();

    // It will be called automatically at end of every scene
    // But you can call it to avoid facing out of memory on GPU
    void TriggerCleanup();

    void SetActiveScene(int nextSceneIndex);
    void SetActiveScene(char const * name);
    void Update(float deltaTimeInSec);
    void Render(float deltaTimeInSec);
    void OnResize();

    void OnUI();            // Can be called optionally for general info about the scenes

    [[nodiscard]]
    Scene * GetActiveScene();

    RT::UniformBufferGroup const & GetCameraBuffers();
    void RegisterPointLight(std::weak_ptr<PointLightComponent> const & pointLight);
    void RegisterDirectionalLight(std::weak_ptr<DirectionalLightComponent> const & directionalLight);
    RT::UniformBufferGroup const & GetPointLightsBuffers();
    RT::UniformBufferGroup const & GetDirectionalLightBuffers();
    uint32_t GetPointLightCount();
    uint32_t GetDirectionalLightCount();
    std::vector<PointLightComponent *> const & GetActivePointLights();

}
