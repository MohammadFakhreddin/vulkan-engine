#include "engine/Scene.hpp"

class TextureViewer final : public MFA::Scene {
public:
    TextureViewer() : Scene("TextureViewer") {}
    void Init() override {}
    void Shutdown() override {}
    void OnDraw(MFA::U32 delta_time, MFA::RenderFrontend::DrawPass & draw_pass) override {}
    void OnUI(MFA::U32 delta_time, MFA::RenderFrontend::DrawPass & draw_pass) override {}
};

static TextureViewer point_light {};