#include "engine/BedrockMatrix.hpp"
#include "engine/Scene.hpp"
#include "engine/RenderFrontend.hpp"
#include "libs/imgui/imgui.h"
#include "engine/DrawableObject.hpp"
#include "tools/Importer.hpp"

namespace RF = MFA::RenderFrontend;
namespace RB = MFA::RenderBackend;
namespace Importer = MFA::Importer;
namespace Asset = MFA::AssetSystem;

class TexturedSphereScene final : public MFA::Scene {
public:

    explicit TexturedSphereScene()
        : Scene("TexturedSphere")
    {}

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

TexturedSphereScene instance {};