#include "engine/Scene.hpp"

class SpecularHighlightScene final : public MFA::Scene {
public:
    SpecularHighlightScene() : Scene("SpecularHighlight") {}
    void Init() override {}
    void Shutdown() override {}
    void OnDraw(MFA::U32 delta_time, MFA::RenderFrontend::DrawPass & draw_pass) override {}
    void OnUI(MFA::U32 delta_time, MFA::RenderFrontend::DrawPass & draw_pass) override {}
};

static SpecularHighlightScene point_light {};