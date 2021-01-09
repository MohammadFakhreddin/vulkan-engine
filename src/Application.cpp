#include "Application.hpp"

#include "engine/RenderFrontend.hpp"
#include "tools/Importer.hpp"

void Application::run() {
    namespace RF = MFA::RenderFrontend;
    namespace RB = MFA::RenderBackend;
    namespace Importer = MFA::Importer;
    RF::Init({800, 600, "Cool app"});
    // Importing assets
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
    auto vertex_shader = Importer::ImportShaderFromSPV(
        "../assets/shaders/vert.spv", 
        MFA::Asset::Shader::Stage::Vertex, 
        "main"
    );
    MFA_DEFER {
        Importer::FreeAsset(&vertex_shader);
    };
    MFA_ASSERT(vertex_shader.valid());
    auto fragment_shader = Importer::ImportShaderFromSPV(
        "../assets/shaders/frag.spv",
        MFA::Asset::Shader::Stage::Fragment,
        "main"
    );
    MFA_DEFER {
        Importer::FreeAsset(&fragment_shader);
    };
    MFA_ASSERT(fragment_shader.valid());
    {// Main loop
        bool quit = false;
        //Event handler
        SDL_Event e;
        //While application is running
        MFA::U32 targetFps = 1000 / 60;
        MFA::U32 startTime;
        MFA::U32 deltaTime;
        while (!quit)
        {
            startTime = SDL_GetTicks();
            {// DrawFrame
               /* auto draw_pass = RF::BeginPass();
                MFA_ASSERT(draw_pass.is_valid);

                RF::EndPass(draw_pass);*/
            }
            //Handle events on queue
            if (SDL_PollEvent(&e) != 0)
            {
                //User requests quit
                if (e.type == SDL_QUIT)
                {
                    quit = true;
                }
            }
            deltaTime = SDL_GetTicks() - startTime;
            if(targetFps > deltaTime){
                SDL_Delay( targetFps - deltaTime );
            }
        }
    }
    RF::Shutdown();
}

