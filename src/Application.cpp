#include "Application.hpp"


#include "engine/BedrockMatrix.hpp"
#include "engine/DrawableObject.hpp"
#include "engine/RenderFrontend.hpp"
#include "tools/Importer.hpp"
#include "engine/UISystem.hpp"

void Application::run() {
    namespace RF = MFA::RenderFrontend;
    namespace RB = MFA::RenderBackend;
    namespace Importer = MFA::Importer;
    namespace UI = MFA::UISystem;
    static constexpr MFA::U16 SCREEN_WIDTH = 800;
    static constexpr MFA::U16 SCREEN_HEIGHT = 600;
    static constexpr float RATIO = static_cast<float>(SCREEN_WIDTH) / static_cast<float>(SCREEN_HEIGHT);
    static constexpr float Z_NEAR = 0.1f;
    static constexpr float Z_FAR = 1000.0f;
    RF::Init({SCREEN_WIDTH, SCREEN_HEIGHT, "Cool app"});
    UI::Init();
    // Importing assets
    auto cpu_viking_mesh = Importer::ImportObj("../assets/viking/viking.obj");
    MFA_ASSERT(cpu_viking_mesh.valid());
    auto gpu_viking_mesh_buffers = RF::CreateMeshBuffers(cpu_viking_mesh);
    auto cpu_viking_texture = Importer::ImportUncompressedImage(
        "../assets/viking/viking.png", 
        Importer::ImportTextureOptions {
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
    struct UniformBufferObject {
        float rotation[16];
        float transformation[16];
        float perspective[16];
    };
    auto sampler_group = RF::CreateSampler();
    auto * descriptor_set_layout = RF::CreateBasicDescriptorSetLayout();
    auto draw_pipeline = RF::CreateBasicDrawPipeline(
        static_cast<MFA::U8>(shaders.size()), 
        shaders.data(),
        descriptor_set_layout
    );
    auto viking_house1 = MFA::DrawableObject(
        gpu_viking_mesh_buffers,
        gpu_viking_texture,
        sampler_group,
        sizeof(UniformBufferObject),
        descriptor_set_layout
    );
    float degree = 0;
    {// Main loop
        bool quit = false;
        //Event handler
        SDL_Event e;
        //While application is running
        MFA::U32 const target_fps = 1000 / 60;
        MFA::U32 delta_time = 0;
        while (!quit)
        {
            MFA::U32 const start_time = SDL_GetTicks();
            {// DrawFrame
                auto draw_pass = RF::BeginPass();
                MFA_ASSERT(draw_pass.is_valid);
                RF::BindDrawPipeline(draw_pass, draw_pipeline);
                {// Updating uniform buffer
                    degree += delta_time * 0.05f;
                    if(degree >= 360.0f)
                    {
                        degree = 0.0f;
                    }
                    UniformBufferObject ubo{};
                    {// Model
                        MFA::Matrix4X4Float rotation;
                        MFA::Matrix4X4Float::assignRotationXYZ(
                            rotation,
                            MFA::Math::Deg2Rad(45.0f),
                            0.0f,
                            MFA::Math::Deg2Rad(45.0f + degree)
                        );
                        //Matrix4X4Float::assignRotationXYZ(rotation, Math::deg2Rad(degree), Math::deg2Rad(degree),Math::deg2Rad(0.0f));
                        ::memcpy(ubo.rotation,rotation.cells,sizeof(ubo.rotation));
                    }
                    {// Transformation
                        MFA::Matrix4X4Float transformation;
                        MFA::Matrix4X4Float::assignTransformation(transformation,0,0,-6);
                        ::memcpy(ubo.transformation,transformation.cells,sizeof(ubo.transformation));
                    }
                    {// Perspective
                        MFA::Matrix4X4Float perspective;
                        MFA::Matrix4X4Float::PreparePerspectiveProjectionMatrix(
                            perspective,
                            RATIO,
                            40,
                            Z_NEAR,
                            Z_FAR
                        );
                        ::memcpy(ubo.perspective,perspective.cells,sizeof(ubo.perspective));
                    }
                    viking_house1.update_uniform_buffer(draw_pass, ubo);
                }
                viking_house1.draw(draw_pass);
                UI::OnNewFrame(delta_time, draw_pass);
                RF::EndPass(draw_pass);
            }
            //Handle events on queue
            if (SDL_PollEvent(&e) != 0)
            {
                //User requests quit
                if (e.type == SDL_QUIT)
                {
                    quit = true;
                }
                // TODO Capture resize event here
            }
            delta_time = SDL_GetTicks() - start_time;
            if(target_fps > delta_time){
                SDL_Delay( target_fps - delta_time );
            }
            delta_time = SDL_GetTicks() - start_time;
        }
    }
    RF::DeviceWaitIdle();
    RF::DestroyDrawPipeline(draw_pipeline);
    viking_house1.shutdown();
    RF::DestroyDescriptorSetLayout(descriptor_set_layout);
    RF::DestroySampler(sampler_group);
    RF::DestroyShader(gpu_fragment_shader);
    Importer::FreeAsset(&fragment_shader);
    RF::DestroyShader(gpu_vertex_shader);
    Importer::FreeAsset(&cpu_vertex_shader);
    RF::DestroyTexture(gpu_viking_texture);
    Importer::FreeAsset(&cpu_viking_texture);
    RF::DestroyMeshBuffers(gpu_viking_mesh_buffers);
    Importer::FreeAsset(&cpu_viking_mesh);
    UI::Shutdown();
    RF::Shutdown();
}

