#include "Application.hpp"

#include "engine/RenderFrontend.hpp"
#include "tools/Importer.hpp"

void Application::run() {
    namespace RF = MFA::RenderFrontend;
    namespace RB = MFA::RenderBackend;
    namespace Importer = MFA::Importer;
    RF::Init({800, 600, "Cool app"});
    // Importing assets
    auto cpu_viking_mesh = Importer::ImportObj("../assets/viking/viking.obj");
    MFA_ASSERT(cpu_viking_mesh.valid());
    auto gpu_viking_mesh_buffers = RF::CreateMeshBuffers(cpu_viking_mesh);
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
    auto cpu_vertex_shader = Importer::ImportShaderFromSPV(
        "../assets/shaders/vert.spv", 
        MFA::Asset::Shader::Stage::Vertex, 
        "main"
    );
    MFA_ASSERT(cpu_vertex_shader.valid());
    auto gpu_vertex_shader = RF::CreateShader(cpu_vertex_shader);
    MFA_ASSERT(gpu_vertex_shader.valid());
    auto fragment_shader = Importer::ImportShaderFromSPV(
        "../assets/shaders/frag.spv",
        MFA::Asset::Shader::Stage::Fragment,
        "main"
    );
    auto gpu_fragment_shader = RF::CreateShader(fragment_shader);
    MFA_ASSERT(gpu_fragment_shader.valid());
    MFA_ASSERT(fragment_shader.valid());
    std::vector<RB::GpuShader> shaders {gpu_vertex_shader, gpu_fragment_shader};
    auto draw_pipeline = RF::CreateBasicDrawPipeline(
        static_cast<MFA::U8>(shaders.size()), 
        shaders.data()
    );
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
    RF::DestroyDrawPipeline(draw_pipeline);
    RF::DestroyShader(gpu_fragment_shader);
    Importer::FreeAsset(&fragment_shader);
    RF::DestroyShader(gpu_vertex_shader);
    Importer::FreeAsset(&cpu_vertex_shader);
    RF::DestroyTexture(gpu_viking_texture);
    Importer::FreeAsset(&cpu_viking_texture);
    RF::DestroyMeshBuffers(gpu_viking_mesh_buffers);
    Importer::FreeAsset(&cpu_viking_mesh);
    RF::Shutdown();
}

