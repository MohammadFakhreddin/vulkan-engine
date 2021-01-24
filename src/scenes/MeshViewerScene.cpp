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
        //auto cpu_model = Importer::ImportMeshGLTF("../assets/free_zuk_3d_model/scene.gltf");
        //auto cpu_model = Importer::ImportMeshGLTF("../assets/kirpi_mrap__lowpoly__free_3d_model/scene.gltf");
        auto cpu_model = Importer::ImportMeshGLTF("../assets/gunship/scene.gltf");
        m_gpu_model = RF::CreateGpuModel(cpu_model);
        m_cpu_vertex_shader = Importer::ImportShaderFromSPV(
            "../assets/shaders/vert.spv", 
            MFA::AssetSystem::Shader::Stage::Vertex, 
            "main"
        );
        MFA_ASSERT(m_cpu_vertex_shader.valid());
        m_gpu_vertex_shader = RF::CreateShader(m_cpu_vertex_shader);
        MFA_ASSERT(m_gpu_vertex_shader.valid());
        m_cpu_fragment_shader = Importer::ImportShaderFromSPV(
            "../assets/shaders/frag.spv",
            MFA::AssetSystem::Shader::Stage::Fragment,
            "main"
        );
        m_gpu_fragment_shader = RF::CreateShader(m_cpu_fragment_shader);
        MFA_ASSERT(m_cpu_fragment_shader.valid());
        MFA_ASSERT(m_gpu_fragment_shader.valid());
        std::vector<RB::GpuShader> shaders {m_gpu_vertex_shader, m_gpu_fragment_shader};
        m_sampler_group = RF::CreateSampler();
        m_descriptor_set_layout = RF::CreateBasicDescriptorSetLayout();
        m_draw_pipeline = RF::CreateBasicDrawPipeline(
            static_cast<MFA::U8>(shaders.size()), 
            shaders.data(),
            m_descriptor_set_layout
        );
        m_drawable_object = MFA::DrawableObject(
            m_gpu_model,
            m_sampler_group,
            sizeof(UniformBufferObject),
            m_descriptor_set_layout
        );
    }
    void OnDraw(MFA::U32 const delta_time, RF::DrawPass & draw_pass) override {
        RF::BindDrawPipeline(draw_pass, m_draw_pipeline);
        {// Updating uniform buffer
            UniformBufferObject ubo{};
            {// Model
                MFA::Matrix4X4Float rotation;
                MFA::Matrix4X4Float::assignRotationXYZ(
                    rotation,
                    MFA::Math::Deg2Rad(45.0f + XDegree),
                    0.0f + YDegree,
                    MFA::Math::Deg2Rad(45.0f + zDegree)
                );
                ::memcpy(ubo.rotation,rotation.cells,sizeof(ubo.rotation));
            }
            {// Transformation
                MFA::Matrix4X4Float transformation;
                MFA::Matrix4X4Float::assignTransformation(transformation,xDistance,yDistance,-6 + zDistance);
                ::memcpy(ubo.transformation,transformation.cells,sizeof(ubo.transformation));
            }
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
        m_drawable_object.draw(draw_pass);
    }
    void OnUI(MFA::U32 const delta_time, RF::DrawPass & draw_pass) override {
        ImGui::Begin("Object viewer");
        ImGui::SetNextItemWidth(300.0f);
        ImGui::SliderFloat("XDegree", &XDegree, -360.0f, 360.0f);
        ImGui::SliderFloat("YDegree", &YDegree, -100.0f, 100.0f);
        ImGui::SliderFloat("ZDegree", &zDegree, -360.0f, 360.0f);
        ImGui::SliderFloat("XDistance", &xDistance, -100.0f, 100.0f);
        ImGui::SliderFloat("YDistance", &yDistance, -100.0f, 100.0f);
        ImGui::SliderFloat("ZDistance", &zDistance, -100.0f, 100.0f);
        ImGui::End();
    }
    void Shutdown() override {
        RF::DestroyDrawPipeline(m_draw_pipeline);
        m_drawable_object.shutdown();
        RF::DestroyDescriptorSetLayout(m_descriptor_set_layout);
        RF::DestroySampler(m_sampler_group);
        RF::DestroyShader(m_gpu_fragment_shader);
        Importer::FreeAsset(&m_cpu_fragment_shader);
        RF::DestroyShader(m_gpu_vertex_shader);
        Importer::FreeAsset(&m_cpu_vertex_shader);
        RF::DestroyGpuModel(m_gpu_model);
        Importer::FreeModel(&m_gpu_model.model_asset);
    }
private:
    static constexpr float Z_NEAR = 0.1f;
    static constexpr float Z_FAR = 1000.0f;
    RF::GpuModel m_gpu_model;
    Asset::ShaderAsset m_cpu_vertex_shader;
    RB::GpuShader m_gpu_vertex_shader;
    Asset::ShaderAsset m_cpu_fragment_shader;
    RB::GpuShader m_gpu_fragment_shader;
    RF::SamplerGroup m_sampler_group;
    VkDescriptorSetLayout_T * m_descriptor_set_layout;
    RF::DrawPipeline m_draw_pipeline;
    MFA::DrawableObject m_drawable_object;
    float XDegree = 0;
    float YDegree = 0;
    float zDegree = 0;
    float xDistance = 0;
    float yDistance = 0;
    float zDistance = 0;
};

MeshViewer mesh_viewer {};