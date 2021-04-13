#pragma once

#include "engine/Scene.hpp"

class TextureViewerScene final : public MFA::Scene {
public:
    TextureViewerScene() = default;
    ~TextureViewerScene() = default;
    void Init() override;
    void Shutdown() override;
    void OnDraw(MFA::U32 delta_time, MFA::RenderFrontend::DrawPass & draw_pass) override;
    void OnUI(MFA::U32 delta_time, MFA::RenderFrontend::DrawPass & draw_pass) override;
};