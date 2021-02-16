#include "engine/BedrockMatrix.hpp"
#include "engine/DrawableObject.hpp"
#include "engine/Scene.hpp"

#include "tools/Importer.hpp"
#include "tools/ShapeGenerator.hpp"
#include "engine/RenderFrontend.hpp"

namespace RF = MFA::RenderFrontend;
namespace RB = MFA::RenderBackend;
namespace Importer = MFA::Importer;
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
        m_sphere = RF::CreateGpuModel(cpu_model);

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
                static_cast<MFA::U8>(bindings.size()), 
                bindings.data()
            );
        }

        // Pipeline
        m_draw_pipeline = RF::CreateBasicDrawPipeline(
            static_cast<MFA::U8>(shaders.size()), 
            shaders.data(),
            m_descriptor_set_layout
        );

        // Descriptor set
        m_sphere_descriptor_sets = RF::CreateDescriptorSets(
            RF::SwapChainImagesCount(), 
            m_descriptor_set_layout
        );

        {// Uniform buffer
            m_material_buffer_group = RF::CreateUniformBuffer(sizeof(MaterialBuffer));
            m_light_view_buffer_group = RF::CreateUniformBuffer(sizeof(LightViewBuffer));
            m_transformation_buffer_group = RF::CreateUniformBuffer(sizeof(TransformationBuffer));
        }

        updateAllDescriptorSets();

    }

    void Shutdown() override {
        RF::DestroyDrawPipeline(m_draw_pipeline);
        RF::DestroyDescriptorSetLayout(m_descriptor_set_layout);
        RF::DestroyGpuModel(m_sphere);
        Importer::FreeModel(&m_sphere.model_asset);
    }

    void OnDraw(MFA::U32 delta_time, MFA::RenderFrontend::DrawPass & draw_pass) override {
        RF::BindDrawPipeline(draw_pass, m_draw_pipeline);
        {// Updating uniform buffers
            {// Material
                m_material_data.roughness = sphereRoughness;
                m_material_data.metallic = sphereMetallic;
                m_material_data.ao = sphereEmission;

                static_assert(sizeof(m_material_data.albedo) == sizeof(sphereColor));
                ::memcpy(m_material_data.albedo, sphereColor, sizeof(sphereColor));

                RF::UpdateUniformBuffer(draw_pass, m_material_buffer_group, MFA::CBlobAliasOf(m_material_data));
            }
            {// LightView
                static_assert(sizeof(m_light_view_data.camPos) == sizeof(cameraPosition));
                ::memcpy(m_light_view_data.camPos, cameraPosition, sizeof(cameraPosition));

                m_light_view_data.lightCount = lightCount;

                static_assert(sizeof(m_light_view_data.lightColors) == sizeof(lightColors));
                ::memcpy(m_light_view_data.lightColors, lightColors, sizeof(lightColors));

                static_assert(sizeof(m_light_view_data.lightPositions) == sizeof(lightPositions));
                ::memcpy(m_light_view_data.lightPositions, lightPositions, sizeof(lightPositions));

                RF::UpdateUniformBuffer(draw_pass, m_light_view_buffer_group, MFA::CBlobAliasOf(m_light_view_data));
            }
            {// Transform
                // Rotation
                static_assert(sizeof(m_transformation_data.rotation) == sizeof(sphereRotation.cells));
                ::memcpy(m_transformation_data.rotation, sphereRotation.cells, sizeof(sphereRotation.cells));

                // Position
                static_assert(sizeof(spherePosition->cells) == sizeof(m_transformation_data.position));
                ::memcpy(m_transformation_data.position, spherePosition->cells, sizeof(spherePosition->cells));
                
                // Perspective
                MFA::I32 width; MFA::I32 height;
                RF::GetWindowSize(width, height);
                float const ratio = static_cast<float>(width) / static_cast<float>(height);
                // TODO I think we should use camera position here
                // TODO Try storing it as class variable
                MFA::Matrix4X4Float perspective;
                MFA::Matrix4X4Float::PreparePerspectiveProjectionMatrix(
                    perspective,
                    ratio,
                    40,
                    Z_NEAR,
                    Z_FAR
                );
                static_assert(sizeof(m_transformation_data.perspective) == sizeof(perspective.cells));
                ::memcpy(
                    m_transformation_data.perspective, 
                    perspective.cells,
                    sizeof(m_transformation_data.perspective)
                );

                RF::UpdateUniformBuffer(
                    draw_pass, 
                    m_transformation_buffer_group, 
                    MFA::CBlobAliasOf(m_transformation_data)
                );
            }
        }
        {// Binding and updating descriptor set
            auto * current_descriptor_set = m_sphere_descriptor_sets[draw_pass.image_index];
            // Bind
            RF::BindDescriptorSet(draw_pass, current_descriptor_set);
        }       
        {// Drawing spheres
            auto const * header_object = m_sphere.model_asset.mesh.header_object();
            MFA_PTR_ASSERT(header_object);
            BindVertexBuffer(draw_pass, m_sphere.mesh_buffers.vertices_buffer);
            BindIndexBuffer(draw_pass, m_sphere.mesh_buffers.indices_buffer);
            auto const required_draw_calls = m_sphere.mesh_buffers.sub_mesh_buffers.size();
            for (MFA::U32 i = 0; i < required_draw_calls; ++i) {
                auto const & current_sub_mesh = header_object->sub_meshes[i];
                auto const & sub_mesh_buffers = m_sphere.mesh_buffers.sub_mesh_buffers[i];
                DrawIndexed(
                    draw_pass,
                    sub_mesh_buffers.index_count,
                    1,
                    current_sub_mesh.indices_starting_index
                );
            }
        }
    }

    void OnUI(MFA::U32 delta_time, MFA::RenderFrontend::DrawPass & draw_pass) override {}

private:

    void updateAllDescriptorSets() {
        for (MFA::U8 i = 0; i < m_sphere_descriptor_sets.size(); ++i) {
            updateDescriptorSet(i);
        }
    }

    void updateDescriptorSet(MFA::U8 const index) {
        auto * current_descriptor_set = m_sphere_descriptor_sets[index];
        std::vector<VkWriteDescriptorSet> write_info {};
        // Transform
        VkDescriptorBufferInfo transform_buffer_info {
            .buffer = m_transformation_buffer_group.buffers[index].buffer,
            .offset = 0,
            .range = m_transformation_buffer_group.buffer_size
        };
        
        write_info.emplace_back(VkWriteDescriptorSet {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = current_descriptor_set,
            .dstBinding = 0,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .pBufferInfo = &transform_buffer_info,
        });
        // Material
        VkDescriptorBufferInfo material_buffer_info {
            .buffer = m_material_buffer_group.buffers[index].buffer,
            .offset = 0,
            .range = m_material_buffer_group.buffer_size
        };
        
        write_info.emplace_back(VkWriteDescriptorSet {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = current_descriptor_set,
            .dstBinding = 1,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .pBufferInfo = &material_buffer_info,
        });
        // LightView
        VkDescriptorBufferInfo light_view_buffer_info {
            .buffer = m_light_view_buffer_group.buffers[index].buffer,
            .offset = 0,
            .range = m_light_view_buffer_group.buffer_size
        };
        
        write_info.emplace_back(VkWriteDescriptorSet {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = current_descriptor_set,
            .dstBinding = 2,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .pBufferInfo = &light_view_buffer_info,
        });

        RF::UpdateDescriptorSets(
            1, 
            &current_descriptor_set, 
            static_cast<MFA::U8>(write_info.size()), 
            write_info.data()
        );
    }

    static constexpr float Z_NEAR = 0.1f;
    static constexpr float Z_FAR = 1000.0f;

    RF::UniformBufferGroup m_material_buffer_group;
    struct MaterialBuffer {
        float albedo[3];        // BaseColor
        float metallic;
        float roughness;
        float ao;               // Emission  
    } m_material_data {};

    RF::UniformBufferGroup m_light_view_buffer_group;
    struct LightViewBuffer {
        float camPos[3];
        MFA::I32 lightCount;
        float lightPositions[8][3];
        float lightColors[8][3];
    } m_light_view_data {};

    RF::UniformBufferGroup m_transformation_buffer_group;
    struct TransformationBuffer {
        float rotation[16];
        float position[16];
        float perspective[16];
    } m_transformation_data {};

    RF::GpuModel m_sphere {};

    VkDescriptorSetLayout_T * m_descriptor_set_layout = nullptr;
    RF::DrawPipeline m_draw_pipeline {};
    MFA::DrawableObject m_drawable_object {};

    MFA::Matrix4X4Float sphereRotation {};
    MFA::Matrix4X4Float spherePosition[3] {};
    float sphereColor[3] {};

    float sphereMetallic = 0.5f;
    float sphereRoughness = 0.5f;
    float sphereEmission = 0.0f; // For now, it's a constant value

    float cameraPosition[3];   // For now, it's a constant value
 
    MFA::I32 lightCount = 1; // For now, it's a constant value
    float lightPositions[8][3] {};
    float lightColors[8][3] {};

    std::vector<VkDescriptorSet_T *> m_sphere_descriptor_sets {};
};

static PBR point_light {};