#pragma once

#include "engine/render_system/RenderTypes.hpp"

namespace MFA {

class CameraComponent;
class Scene;
class DisplayRenderPass;
class PointLightComponent;
class DirectionalLightComponent;
class SpotLightComponent;
class Entity;

class Scene {
public:

    static constexpr int MAX_VISIBLE_POINT_LIGHT_COUNT = 10;        // It can be more but currently 10 is more than enough for me
    static constexpr int MAX_DIRECTIONAL_LIGHT_COUNT = 3;
    static constexpr uint32_t SHADOW_WIDTH = 2048;
    static constexpr uint32_t SHADOW_HEIGHT = 2048;

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

    void SetActiveCamera(std::weak_ptr<CameraComponent> const & camera);

    [[nodiscard]]
    std::weak_ptr<CameraComponent> const & GetActiveCamera() const;

    [[nodiscard]]
    Entity * GetRootEntity() const;

    [[nodiscard]]
    RT::UniformBufferCollection const & GetCameraBufferCollection() const;

    void RegisterPointLight(std::weak_ptr<PointLightComponent> const & pointLight);

    [[nodiscard]]
    RT::UniformBufferCollection const & GetPointLightsBufferCollection() const;

    [[nodiscard]]
    uint32_t GetPointLightCount() const;

private:

    void prepareCameraBuffer();

    void preparePointLightsBuffer();
    
    std::weak_ptr<CameraComponent> mActiveCamera {};

    // Scene root entity
    Entity * mRootEntity = nullptr;

    // TODO We need light container class
    std::vector<std::weak_ptr<PointLightComponent>> mPointLights {};
    std::vector<std::weak_ptr<DirectionalLightComponent>> mDirectionalLights {};

    // Buffers 
    RT::UniformBufferCollection mCameraBufferCollection {};

    // Point light
    struct PointLight
    {
        float position [3] {};
        float placeholder0 = 0.0f;
        float color [3] {};
        float maxSquareDistance = 0.0f;
        float linearAttenuation = 0.0f;
        float quadraticAttenuation = 0.0f;
        float placeholder1[2] {};
        float viewProjectionMatrices[6][16] {};
    };
    struct PointLightsBufferData
    {
        uint32_t count = 0;
        float constantAttenuation = 1.0f;
        float placeholder[2] {};
        PointLight items [MAX_VISIBLE_POINT_LIGHT_COUNT] {};
    };
    PointLightsBufferData mPointLightData {};
    RT::UniformBufferCollection mPointLightsBufferCollection {};

    // Directional light (TODO)
    struct DirectionalLightData
    {
        uint32_t count = 0;
        uint32_t placeholder[3] {};
        struct DirectionalLight
        {
            float direction[3] {};
            float placeholder0 = 0.0f;
            float color[3] {};
            float placeholder1 = 0.0f;
        };
        DirectionalLight items [MAX_DIRECTIONAL_LIGHT_COUNT] {};
    };
    DirectionalLightData mDirectionalLightData {};
    RT::UniformBufferCollection mDirectionalLightBufferCollection {};

    // TODO Spot light
};


}
