#pragma once

#include "engine/Scene.hpp"
#include "engine/RenderFrontend.hpp"

namespace RF = MFA::RenderFrontend;
namespace RB = MFA::RenderBackend;
namespace Asset = MFA::AssetSystem;

class TexturedSphereScene final : public MFA::Scene {
public:

    void Init() override {
        
    }

    void OnDraw(MFA::U32 delta_time, MFA::RenderFrontend::DrawPass & draw_pass) override {
        
    }

    void OnUI(MFA::U32 delta_time, MFA::RenderFrontend::DrawPass & draw_pass) override {
        
    }

    void Shutdown() override {
        
    }

private:
};
