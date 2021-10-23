#pragma once

namespace MFA
{
    class Scene;
    class CameraComponent;
}

namespace MFA::SceneManager
{

    void Init();
    void Shutdown();
    void RegisterNew(Scene * scene, char const * name);
    void SetActiveScene(char const * name);
    void OnNewFrame(float deltaTimeInSec);
    void OnResize();

    [[nodiscard]]
    Scene * GetActiveScene();
    // TODO Directional light
    // TODO Point lights
    // TODO Sfx lights
    // TODO Camera components
}
