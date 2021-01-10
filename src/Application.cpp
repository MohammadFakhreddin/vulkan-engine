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
    auto cpu_viking_texture = Importer::ImportUncompressedImage(
        "../assets/viking/viking.png", 
        Importer::ImportUncompressedImageOptions {
            .generate_mipmaps = false,
            .prefer_srgb = false
        }
    );
    MFA_ASSERT(cpu_viking_texture.valid());
    auto gpu_viking_texture = RF::CreateTexture(cpu_viking_texture);
    MFA_ASSERT(gpu_viking_texture.valid());
    MFA_DEFER {
        RF::DestroyTexture(gpu_viking_texture);
        Importer::FreeAsset(&cpu_viking_texture);
    };
    auto cpu_vertex_shader = Importer::ImportShaderFromSPV(
        "../assets/shaders/vert.spv", 
        MFA::Asset::Shader::Stage::Vertex, 
        "main"
    );
    MFA_ASSERT(cpu_vertex_shader.valid());
    auto gpu_vertex_shader = RF::CreateShader(cpu_vertex_shader);
    MFA_ASSERT(gpu_vertex_shader.valid());
    MFA_DEFER {
        RF::DestroyShader(gpu_vertex_shader);
        Importer::FreeAsset(&cpu_vertex_shader);
    };
    auto fragment_shader = Importer::ImportShaderFromSPV(
        "../assets/shaders/frag.spv",
        MFA::Asset::Shader::Stage::Fragment,
        "main"
    );
    auto gpu_fragment_shader = RF::CreateShader(fragment_shader);
    MFA_ASSERT(gpu_fragment_shader.valid());
    MFA_DEFER {
        RF::DestroyShader(gpu_fragment_shader);
        Importer::FreeAsset(&fragment_shader);
    };
    MFA_ASSERT(fragment_shader.valid());
    std::vector<RB::GpuShader> shaders {};
    auto draw_pipeline = RF::CreateBasicDrawPipeline(2, {});
    {// Main loop
        bool quit = false;
        //Event handler
        SDL_Event e;
        //While application is running
        MFA::U32 const target_fps = 1000 / 60;
        while (!quit)
        {
            MFA::U32 const start_time = SDL_GetTicks();
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
            MFA::U32 const delta_time = SDL_GetTicks() - start_time;
            if(target_fps > delta_time){
                SDL_Delay( target_fps - delta_time );
            }
        }
    }
    RF::Shutdown();
}

