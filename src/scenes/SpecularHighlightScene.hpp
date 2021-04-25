#pragma once

#include "engine/Scene.hpp"

class SpecularHighlightScene final : public MFA::Scene {
public:
    void Init() override {}
    void Shutdown() override {}
    void OnDraw(uint32_t delta_time, MFA::RenderFrontend::DrawPass & draw_pass) override {}
    void OnUI(uint32_t delta_time, MFA::RenderFrontend::DrawPass & draw_pass) override {}
};

