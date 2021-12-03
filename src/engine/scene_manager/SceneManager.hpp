#pragma once

#include <functional>
#include <memory>

namespace MFA
{
    class Scene;
    class CameraComponent;
}

namespace MFA::SceneManager
{

    using CreateSceneFunction = std::function<std::shared_ptr<Scene>()>;

    void Init();
    void Shutdown();

    struct RegisterSceneOptions
    {
        bool keepAlive = false;
    };
    void RegisterScene(
        char const * name,
        CreateSceneFunction const & createSceneFunction,
        RegisterSceneOptions const & options = {}
    );

    void SetActiveScene(int nextSceneIndex);
    void SetActiveScene(char const * name);
    void OnNewFrame(float deltaTimeInSec);
    void OnResize();

    void OnUI();            // Can be called optionally for general info about the scenes

    [[nodiscard]]
    Scene * GetActiveScene();
    // TODO Directional light
    // TODO Point lights
    // TODO Sfx lights
    // TODO Camera components
}
