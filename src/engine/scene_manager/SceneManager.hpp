#pragma once

#include <memory>

namespace MFA
{
    class Scene;
    class CameraComponent;
}

namespace MFA::SceneManager
{

    void Init();
    void Shutdown();
    void RegisterScene(std::weak_ptr<Scene> const & scene, char const * name);
    void SetActiveScene(char const * name);
    void OnNewFrame(float deltaTimeInSec);
    void OnResize();

    void OnUI();            // Can be called optionally for general info about the scenes

    [[nodiscard]]
    std::weak_ptr<Scene> const & GetActiveScene();
    // TODO Directional light
    // TODO Point lights
    // TODO Sfx lights
    // TODO Camera components
}
