#include "Application.hpp"


#include "engine/BedrockMatrix.hpp"
#include "engine/RenderFrontend.hpp"
#include "tools/Importer.hpp"

void Application::run() {
    namespace RF = MFA::RenderFrontend;
    namespace RB = MFA::RenderBackend;
    namespace Importer = MFA::Importer;
    static constexpr MFA::U16 SCREEN_WIDTH = 800;
    static constexpr MFA::U16 SCREEN_HEIGHT = 600;
    static constexpr float RATIO = static_cast<float>(SCREEN_WIDTH) / static_cast<float>(SCREEN_HEIGHT);
    static constexpr float Z_NEAR = 0.1f;
    static constexpr float Z_FAR = 1000.0f;
    RF::Init({SCREEN_WIDTH, SCREEN_HEIGHT, "Cool app"});
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
    struct UniformBufferObject {
        float rotation[16];
        float transformation[16];
        float perspective[16];
    };
    auto viking1_uniform_buffer_group = RF::CreateUniformBuffer(sizeof(UniformBufferObject));
    auto viking2_uniform_buffer_group = RF::CreateUniformBuffer(sizeof(UniformBufferObject));
    auto sampler_group = RF::CreateSampler();
    auto draw_pipeline = RF::CreateBasicDrawPipeline(
        static_cast<MFA::U8>(shaders.size()), 
        shaders.data()
    );
    auto viking1_descriptor_sets = RF::CreateDescriptorSetsForDrawPipeline(draw_pipeline);
    auto viking2_descriptor_sets = RF::CreateDescriptorSetsForDrawPipeline(draw_pipeline);
    float degree = 0;
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
                auto draw_pass = RF::BeginPass();
                MFA_ASSERT(draw_pass.is_valid);
                {// Updating uniform buffer
                    degree++;
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
                    RF::BindDataToUniformBuffer(
                        draw_pass,
                        viking1_uniform_buffer_group, 
                        MFA::CBlobAliasOf(ubo)
                    );
                }
                RF::BindDrawPipeline(draw_pass, draw_pipeline);
                RF::BindDescriptorSetsBasic(
                    draw_pass,
                    viking1_descriptor_sets.data()
                );
                RF::UpdateDescriptorSetsBasic(
                    draw_pass,
                    viking1_descriptor_sets.data(),
                    viking1_uniform_buffer_group,
                    gpu_viking_texture,
                    sampler_group
                );
                /*RF::DrawBasicTexturedMesh(
                    draw_pass,
                    draw_pipeline,
                    descriptor_sets.data(),
                    uniform_buffer_group,
                    gpu_viking_mesh_buffers,
                    gpu_viking_texture,
                    sampler_group
                );*/
                RF::DrawBasicTexturedMesh(
                    draw_pass,
                    draw_pipeline,
                    gpu_viking_mesh_buffers
                );
                {// Updating uniform buffer
                    UniformBufferObject ubo{};
                    {// Model
                        MFA::Matrix4X4Float rotation;
                        MFA::Matrix4X4Float::assignRotationXYZ(
                            rotation,
                            0.0f,
                            0.0f,
                            0.0f
                        );
                        //Matrix4X4Float::assignRotationXYZ(rotation, Math::deg2Rad(degree), Math::deg2Rad(degree),Math::deg2Rad(0.0f));
                        ::memcpy(ubo.rotation,rotation.cells,sizeof(ubo.rotation));
                    }
                    {// Transformation
                        MFA::Matrix4X4Float transformation;
                        MFA::Matrix4X4Float::assignTransformation(transformation,1,0,-7);
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
                    RF::BindDataToUniformBuffer(
                        draw_pass,
                        viking2_uniform_buffer_group, 
                        MFA::CBlobAliasOf(ubo)
                    );
                }
                RF::BindDescriptorSetsBasic(
                    draw_pass,
                    viking2_descriptor_sets.data()
                );
                RF::UpdateDescriptorSetsBasic(
                    draw_pass,
                    viking2_descriptor_sets.data(),
                    viking2_uniform_buffer_group,
                    gpu_viking_texture,
                    sampler_group
                );
                //{// Transformation
                //    MFA::Matrix4X4Float transformation;
                //    MFA::Matrix4X4Float::assignTransformation(transformation,100,0,-5);
                //    ::memcpy(ubo.transformation,transformation.cells,sizeof(ubo.transformation));
                //    RF::BindDataToUniformBuffer(
                //        draw_pass,
                //        uniform_buffer_group, 
                //        MFA::CBlobAliasOf(ubo)
                //    );
                //}
                RF::DrawBasicTexturedMesh(
                    draw_pass,
                    draw_pipeline,
                    gpu_viking_mesh_buffers
                );
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
            }
            MFA::U32 const delta_time = SDL_GetTicks() - start_time;
            if(target_fps > delta_time){
                SDL_Delay( target_fps - delta_time );
            }
        }
    }
    RF::DeviceWaitIdle();
    RF::DestroyDrawPipeline(draw_pipeline);
    RF::DestroySampler(sampler_group);
    RF::DestroyUniformBuffer(viking1_uniform_buffer_group);
    RF::DestroyUniformBuffer(viking2_uniform_buffer_group);
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

