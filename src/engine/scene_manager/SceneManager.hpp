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
    /*void SetActiveCamera(CameraComponent * camera);
    void RemoveActiveCamera(CameraComponent * camera);

    [[nodiscard]]
    CameraComponent const * GetActiveCamera();*/
    // TODO Directional light
    // TODO Active lights

}
