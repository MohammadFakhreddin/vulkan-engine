#pragma once

#include "engine/render_system/RenderTypesFWD.hpp"

namespace MFA {

class CameraComponent;
class Scene;
class DisplayRenderPass;
class PointLightComponent;
class SpotLightComponent;
class Entity;

class Scene {
public:

    explicit Scene() = default;
    virtual ~Scene() = default;
    Scene & operator= (Scene && rhs) noexcept = delete;
    Scene (Scene const &) noexcept = delete;
    Scene (Scene && rhs) noexcept = delete;
    Scene & operator = (Scene const &) noexcept = delete;

    virtual void OnPreRender(float deltaTimeInSec, RT::CommandRecordState & recordState);
    virtual void OnRender(float deltaTimeInSec, RT::CommandRecordState & drawPass) {}
    virtual void OnPostRender(float deltaTimeInSec, RT::CommandRecordState & drawPass) {}

    virtual void OnResize() = 0;
    virtual void Init();   
    virtual void Shutdown();

    // TODO List of light and camera => Question : Any light or active ones ?

    void SetActiveCamera(CameraComponent * camera);

    [[nodiscard]]
    CameraComponent * GetActiveCamera() const;

    [[nodiscard]]
    Entity * GetRootEntity() const;

    [[nodiscard]]
    RT::UniformBufferCollection * GetCameraBufferCollection() const;

private:

    CameraComponent * mActiveCamera = nullptr;
    std::unique_ptr<RT::UniformBufferCollection> mCameraBufferCollection {};
    
    // Scene root entity
    Entity * mRootEntity = nullptr;


    // TODO We need light container class
    //std::vector<PointLightComponent *> mPointLights {};
    //std::vector<SpotLightComponent *> mSpotLights {};
    
};


}
