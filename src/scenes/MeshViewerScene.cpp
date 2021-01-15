#include "engine/BedrockMatrix.hpp"
#include "engine/Scene.hpp"
#include "engine/RenderFrontend.hpp"
#include "libs/imgui/imgui.h"
#include "engine/DrawableObject.hpp"
#include "tools/Importer.hpp"

namespace RF = MFA::RenderFrontend;
namespace RB = MFA::RenderBackend;
namespace Importer = MFA::Importer;
namespace Asset = MFA::Asset;
namespace UI = MFA::UISystem;

class MeshViewer final : public MFA::Scene {
    struct UniformBufferObject {
        float rotation[16];
        float transformation[16];
        float perspective[16];
    };
public:
    MeshViewer() {
        MFA::SceneSubSystem::RegisterNew(
            this,
            "MeshViewer"
        );  
    }
    void Init() override {
        // Importing assets
        cpu_viking_mesh = Importer::ImportObj("../assets/viking/viking.obj");
        MFA_ASSERT(cpu_viking_mesh.valid());
        gpu_viking_mesh_buffers = RF::CreateMeshBuffers(cpu_viking_mesh);
        cpu_viking_texture = Importer::ImportUncompressedImage(
            "../assets/viking/viking.png", 
            Importer::ImportTextureOptions {
                .generate_mipmaps = false,
                .prefer_srgb = false
            }
        );
        MFA_ASSERT(cpu_viking_texture.valid());
        gpu_viking_texture = RF::CreateTexture(cpu_viking_texture);
        MFA_ASSERT(gpu_viking_texture.valid());
        cpu_vertex_shader = Importer::ImportShaderFromSPV(
            "../assets/shaders/vert.spv", 
            MFA::Asset::Shader::Stage::Vertex, 
            "main"
        );
        MFA_ASSERT(cpu_vertex_shader.valid());
        gpu_vertex_shader = RF::CreateShader(cpu_vertex_shader);
        MFA_ASSERT(gpu_vertex_shader.valid());
        fragment_shader = Importer::ImportShaderFromSPV(
            "../assets/shaders/frag.spv",
            MFA::Asset::Shader::Stage::Fragment,
            "main"
        );
        gpu_fragment_shader = RF::CreateShader(fragment_shader);
        MFA_ASSERT(gpu_fragment_shader.valid());
        MFA_ASSERT(fragment_shader.valid());
        std::vector<RB::GpuShader> shaders {gpu_vertex_shader, gpu_fragment_shader};
        sampler_group = RF::CreateSampler();
        descriptor_set_layout = RF::CreateBasicDescriptorSetLayout();
        draw_pipeline = RF::CreateBasicDrawPipeline(
            static_cast<MFA::U8>(shaders.size()), 
            shaders.data(),
            descriptor_set_layout
        );
        viking_house1 = MFA::DrawableObject(
            gpu_viking_mesh_buffers,
            gpu_viking_texture,
            sampler_group,
            sizeof(UniformBufferObject),
            descriptor_set_layout
        );
    }
    void OnNewFrame(MFA::U32 const delta_time, RF::DrawPass & draw_pass) override {
        RF::BindDrawPipeline(draw_pass, draw_pipeline);
        {// Updating uniform buffer
            UniformBufferObject ubo{};
            {// Model
                MFA::Matrix4X4Float rotation;
                MFA::Matrix4X4Float::assignRotationXYZ(
                    rotation,
                    MFA::Math::Deg2Rad(45.0f),
                    0.0f,
                    MFA::Math::Deg2Rad(45.0f + degree)
                );
                ::memcpy(ubo.rotation,rotation.cells,sizeof(ubo.rotation));
            }
            {// Transformation
                MFA::Matrix4X4Float transformation;
                MFA::Matrix4X4Float::assignTransformation(transformation,0,0,-6);
                ::memcpy(ubo.transformation,transformation.cells,sizeof(ubo.transformation));
            }
            {// Perspective
                MFA::I32 width; MFA::I32 height;
                RF::GetWindowSize(width, height);
                float ratio = static_cast<float>(width) / static_cast<float>(height);
                MFA::Matrix4X4Float perspective;
                MFA::Matrix4X4Float::PreparePerspectiveProjectionMatrix(
                    perspective,
                    ratio,
                    40,
                    Z_NEAR,
                    Z_FAR
                );
                ::memcpy(ubo.perspective,perspective.cells,sizeof(ubo.perspective));
            }
            viking_house1.update_uniform_buffer(draw_pass, ubo);
        }
        viking_house1.draw(draw_pass);
        UI::OnNewFrame(delta_time, draw_pass, [this]()->void{
            ImGui::Begin("Object viewer");
            ImGui::SetNextItemWidth(200.0f);
            ImGui::SliderFloat("float", &degree, -360.0f, 360.0f);
            ImGui::End();
        });
    }
    void Shutdown() override {
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
    }
private:
    static constexpr float Z_NEAR = 0.1f;
    static constexpr float Z_FAR = 1000.0f;
    Asset::MeshAsset cpu_viking_mesh;
    RF::MeshBuffers gpu_viking_mesh_buffers;
    Asset::TextureAsset cpu_viking_texture;
    RB::GpuTexture gpu_viking_texture;
    Asset::ShaderAsset cpu_vertex_shader;
    RB::GpuShader gpu_vertex_shader;
    Asset::ShaderAsset fragment_shader;
    RB::GpuShader gpu_fragment_shader;
    RF::SamplerGroup sampler_group;
    VkDescriptorSetLayout_T * descriptor_set_layout;
    RF::DrawPipeline draw_pipeline;
    MFA::DrawableObject viking_house1;
    float degree = 0;
};

MeshViewer mesh_viewer {};