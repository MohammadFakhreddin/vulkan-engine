#include "engine/Scene.hpp"

class PointLight final : public MFA::Scene {
public:
    PointLight() {
        MFA::SceneSubSystem::RegisterNew(
            this,
            "PointLight"
        );
    }
    void Init() override {}
    void Shutdown() override {}
    void OnDraw(MFA::U32 delta_time, MFA::RenderFrontend::DrawPass & draw_pass) override {}
    void OnUI(MFA::U32 delta_time, MFA::RenderFrontend::DrawPass & draw_pass) override {}
};

static PointLight point_light {};