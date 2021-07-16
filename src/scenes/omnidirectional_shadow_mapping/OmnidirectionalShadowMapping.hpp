#pragma once

// TODO Delete this file

//
//#include "engine/Scene.hpp"
//#include "engine/camera/FirstPersonCamera.hpp"
//
//class OmnidirectionalShadowMapping final : public MFA::Scene {
//public:
//
//    explicit OmnidirectionalShadowMapping();
//
//    ~OmnidirectionalShadowMapping() override = default;
//
//    void Init() override;
//
//    void Shutdown() override;
//
//    void OnDraw(float delta_time, MFA::RenderFrontend::DrawPass & draw_pass) override;
//
//    void OnResize() override;
//
//private:
//
//    static constexpr float Z_NEAR = 0.1f;
//    static constexpr float Z_FAR = 3000.0f;
//
//    #ifdef __DESKTOP__
//    static constexpr float FOV = 80;
//#elif defined(__ANDROID__) || defined(__IOS__)
//    static constexpr float FOV = 40;    // TODO It should be only for standing orientation
//#else
//#error Os is not handled
//#endif
//    MFA::FirstPersonCamera mCamera {
//        FOV,
//        Z_FAR,
//        Z_NEAR
//    };
//
//    MFA::PBRWithShadowPipeline mPointLightPipeline {};
//
//};
