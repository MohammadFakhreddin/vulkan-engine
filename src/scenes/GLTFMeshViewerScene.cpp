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

class GLTFMeshViewer final : public MFA::Scene {
public:
    explicit GLTFMeshViewer() {
        MFA::SceneSubSystem::RegisterNew(
            this,
            "GLTFMeshViewer"
        );  
    }
    void Init() override {
        //auto cpu_model = Importer::ImportMeshGLTF("../assets/free_zuk_3d_model/scene.gltf");
        //auto cpu_model = Importer::ImportMeshGLTF("../assets/kirpi_mrap__lowpoly__free_3d_model/scene.gltf");
        auto cpu_model = Importer::ImportMeshGLTF("../assets/models/gunship/scene.gltf");
        m_gpu_model = RF::CreateGpuModel(cpu_model);
        
        // Cpu shader
        auto cpu_vertex_shader = Importer::ImportShaderFromSPV(
            "../assets/shaders/gltf_mesh_viewer/GLTFMeshViewer.vert.spv", 
            MFA::AssetSystem::Shader::Stage::Vertex, 
            "main"
        );
        MFA_ASSERT(cpu_vertex_shader.valid());
        auto gpu_vertex_shader = RF::CreateShader(cpu_vertex_shader);
        MFA_ASSERT(gpu_vertex_shader.valid());
        MFA_DEFER {
            RF::DestroyShader(gpu_vertex_shader);
            Importer::FreeAsset(&cpu_vertex_shader);
        };
        
        // Fragment shader
        auto cpu_fragment_shader = Importer::ImportShaderFromSPV(
            "../assets/shaders/gltf_mesh_viewer/GLTFMeshViewer.frag.spv",
            MFA::AssetSystem::Shader::Stage::Fragment,
            "main"
        );
        auto gpu_fragment_shader = RF::CreateShader(cpu_fragment_shader);
        MFA_ASSERT(cpu_fragment_shader.valid());
        MFA_ASSERT(gpu_fragment_shader.valid());
        MFA_DEFER {
            RF::DestroyShader(gpu_fragment_shader);
            Importer::FreeAsset(&cpu_fragment_shader);
        };

        std::vector<RB::GpuShader> shaders {gpu_vertex_shader, gpu_fragment_shader};

        // TODO We should use gltf sampler info here
        m_sampler_group = RF::CreateSampler();

        m_descriptor_set_layout = RF::CreateBasicDescriptorSetLayout();

        m_draw_pipeline = RF::CreateBasicDrawPipeline(
            static_cast<MFA::U8>(shaders.size()), 
            shaders.data(),
            m_descriptor_set_layout
        );

        m_drawable_object = MFA::DrawableObject(
            m_gpu_model,
            m_descriptor_set_layout
        );
        // TODO We can remove cpu shaders here
    }
    void OnDraw(MFA::U32 const delta_time, RF::DrawPass & draw_pass) override {
        RF::BindDrawPipeline(draw_pass, m_draw_pipeline);
        {// Updating Transform buffer
            // Rotation
            MFA::Matrix4X4Float::assignRotationXYZ(
                m_translate_data.rotation,
                MFA::Math::Deg2Rad(m_model_rotation[0]),
                MFA::Math::Deg2Rad(m_model_rotation[1]),
                MFA::Math::Deg2Rad(m_model_rotation[2])
            );
            // Position
            MFA::Matrix4X4Float::assignTransformation(
                m_translate_data.transformation,
                m_model_position[0],
                m_model_position[1],
                m_model_position[2]
            );
            {// Perspective
                MFA::I32 width; MFA::I32 height;
                RF::GetWindowSize(width, height);
                float const ratio = static_cast<float>(width) / static_cast<float>(height);
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
            m_drawable_object.update_uniform_buffer(draw_pass, ubo);
        }
        {// TODO LightViewBuffer

        }
        {// TODO MaterialBuffer

        }
        {// TODO RotationBuffer
        }
        
        m_drawable_object.draw(draw_pass);
    }
    void OnUI(MFA::U32 const delta_time, RF::DrawPass & draw_pass) override {
        ImGui::Begin("Object viewer");
        ImGui::SetNextItemWidth(300.0f);
        ImGui::SliderFloat("XDegree", &m_model_rotation[0], -360.0f, 360.0f);
        ImGui::SetNextItemWidth(300.0f);
        ImGui::SliderFloat("YDegree", &m_model_rotation[1], -360.0f, 360.0f);
        ImGui::SetNextItemWidth(300.0f);
        ImGui::SliderFloat("ZDegree", &m_model_rotation[2], -360.0f, 360.0f);
        ImGui::SetNextItemWidth(300.0f);
        ImGui::SliderFloat("XDistance", &m_model_position[0], -100.0f, 100.0f);
        ImGui::SetNextItemWidth(300.0f);
        ImGui::SliderFloat("YDistance", &m_model_position[1], -100.0f, 100.0f);
        ImGui::SetNextItemWidth(300.0f);
        ImGui::SliderFloat("ZDistance", &m_model_position[2], -100.0f, 100.0f);
        ImGui::End();
    }
    void Shutdown() override {
        RF::DestroyDrawPipeline(m_draw_pipeline);
        RF::DestroyDescriptorSetLayout(m_descriptor_set_layout);
        RF::DestroySampler(m_sampler_group);
        RF::DestroyGpuModel(m_gpu_model);
        Importer::FreeModel(&m_gpu_model.model_asset);
    }
private:
    static constexpr float Z_NEAR = 0.1f;
    static constexpr float Z_FAR = 1000.0f;

    struct ModelTransformBuffer {
        MFA::Matrix4X4Float rotation;
        MFA::Matrix4X4Float transformation;
        MFA::Matrix4X4Float perspective;
    } m_translate_data;

    float m_model_rotation[3] {45.0f, 45.0f, 45.0f};
    float m_model_position[3] {0.0f, 0.0f, -6.0f};

    RF::GpuModel m_gpu_model {};
    RF::SamplerGroup m_sampler_group {};
    VkDescriptorSetLayout_T * m_descriptor_set_layout = nullptr;
    RF::DrawPipeline m_draw_pipeline {};
    MFA::DrawableObject m_drawable_object {};

};

GLTFMeshViewer mesh_viewer {};