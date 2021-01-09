#include "Application.hpp"

#include "engine/RenderFrontend.hpp"
#include "tools/Importer.hpp"

void Application::run() {
    namespace RF = MFA::RenderFrontend;
    namespace RB = MFA::RenderBackend;
    namespace Importer = MFA::Importer;
    RF::Init({800, 600, "Cool app"});
    auto viking_mesh = Importer::ImportObj("../assets/viking/viking.obj");
    MFA_DEFER {
        Importer::FreeAsset(&viking_mesh);
    };
    MFA_ASSERT(viking_mesh.valid());
    auto viking_texture = Importer::ImportUncompressedImage(
        "../assets/viking/viking.png", 
        Importer::ImportUncompressedImageOptions {
            .generate_mipmaps = false,
            .prefer_srgb = false
        }
    );
    MFA_DEFER {
        Importer::FreeAsset(&viking_texture);
    };
    MFA_ASSERT(viking_texture.valid());
    RF::Shutdown();
}

