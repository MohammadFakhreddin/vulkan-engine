#include "engine/DrawableObject.hpp"
#include "engine/Scene.hpp"

#include "tools/Importer.hpp"
#include "tools/ShapeGenerator.hpp"
#include "engine/RenderFrontend.hpp"

namespace RF = MFA::RenderFrontend;
namespace RB = MFA::RenderBackend;
namespace Importer = MFA::Importer;
namespace Asset = MFA::AssetSystem;
namespace SG = MFA::ShapeGenerator;

class PBR final : public MFA::Scene {
public:
    PBR() {
        MFA::SceneSubSystem::RegisterNew(
            this,
            "PBR"
        );
    }
    void Init() override {
        // Gpu model
        auto cpu_model = SG::Sphere();
        MFA_DEFER {
            Importer::FreeModel(&cpu_model);
        };
        m_gpu_model = RF::CreateGpuModel(cpu_model);
        
        // Vertex shader
        auto cpu_vertex_shader = Importer::ImportShaderFromSPV(
            "../assets/shaders/pbr/pbr.vert.spv", 
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
        auto cpu_frag_shader = Importer::ImportShaderFromSPV(
            "../assets/shaders/pbr/pbr.frag.spv", 
            MFA::AssetSystem::Shader::Stage::Fragment, 
            "main"
        );
        MFA_ASSERT(cpu_frag_shader.valid());
        auto gpu_fragment_shader = RF::CreateShader(cpu_frag_shader);
        MFA_ASSERT(gpu_fragment_shader.valid());
        MFA_DEFER {
            RF::DestroyShader(gpu_fragment_shader);
            Importer::FreeAsset(&cpu_frag_shader);
        };

        // Shader
        std::vector<RB::GpuShader> shaders {gpu_vertex_shader, gpu_fragment_shader};

        {// Descriptor set layout
            // Transformation 
            VkDescriptorSetLayoutBinding const transform_buff_binding {
                .binding = 0,
                .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .descriptorCount = 1,
                .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
                .pImmutableSamplers = nullptr, // Optional
            };
            // Material
            VkDescriptorSetLayoutBinding const material_buff_binding {
                .binding = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .descriptorCount = 1,
                .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
                .pImmutableSamplers = nullptr, // Optional     
            };
            // Light and view
            VkDescriptorSetLayoutBinding const light_buff_binding {
                .binding = 2,
                .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .descriptorCount = 1,
                .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
                .pImmutableSamplers = nullptr, // Optional    
            };

            std::vector<VkDescriptorSetLayoutBinding> bindings {
                transform_buff_binding,
                material_buff_binding,
                light_buff_binding
            };
            m_descriptor_set_layout = RF::CreateDescriptorSetLayout(
                bindings.size(), 
                bindings.data()
            );
        }

        m_draw_pipeline = RF::CreateBasicDrawPipeline(
            static_cast<MFA::U8>(shaders.size()), 
            shaders.data(),
            m_descriptor_set_layout
        );
    }
    void Shutdown() override {
        RF::DestroyDrawPipeline(m_draw_pipeline);
        RF::DestroyDescriptorSetLayout(m_descriptor_set_layout);
        RF::DestroyGpuModel(m_gpu_model);
    }
    void OnDraw(MFA::U32 delta_time, MFA::RenderFrontend::DrawPass & draw_pass) override {}
    void OnUI(MFA::U32 delta_time, MFA::RenderFrontend::DrawPass & draw_pass) override {}

private:

    struct MaterialBuffer {
        float albedo[3];        // BaseColor
        float metallic;
        float roughness;
        float ao;               // Emission  
    };

    struct LightViewBuffer {
        float camPos[3];
        MFA::I32 lightCount;
        float lightPositions[8][3];
        float lightColors[8][3];
    };

    struct TransformationBuffer {
        float rotation[16];
        float transformation[16];
        float perspective[16];
    };

    RF::GpuModel m_gpu_model {};

    VkDescriptorSetLayout_T * m_descriptor_set_layout = nullptr;
    RF::DrawPipeline m_draw_pipeline {};
    MFA::DrawableObject m_drawable_object {};
};

static PBR point_light {};