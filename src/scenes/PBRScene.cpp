#include "engine/Scene.hpp"

class PBR final : public MFA::Scene {
public:
    PBR() {
        MFA::SceneSubSystem::RegisterNew(
            this,
            "PBR"
        );
    }
    void Init() override {}
    void Shutdown() override {}
    void OnDraw(MFA::U32 delta_time, MFA::RenderFrontend::DrawPass & draw_pass) override {}
    void OnUI(MFA::U32 delta_time, MFA::RenderFrontend::DrawPass & draw_pass) override {}
};

static PBR point_light {};