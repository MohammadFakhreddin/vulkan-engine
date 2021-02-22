#include "engine/BedrockMatrix.hpp"
#include "engine/DrawableObject.hpp"
#include "engine/Scene.hpp"

#include "tools/Importer.hpp"
#include "tools/ShapeGenerator.hpp"
#include "engine/RenderFrontend.hpp"
#include "libs/imgui/imgui.h"

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

        {// Pipeline
            VkVertexInputBindingDescription const vertex_binding_description {
                .binding = 0,
                .stride = sizeof(MFA::AssetSystem::MeshVertex),
                .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
            };
            std::vector<VkVertexInputAttributeDescription> input_attribute_descriptions {2};
            input_attribute_descriptions[0] = VkVertexInputAttributeDescription {
                .location = 0,
                .binding = 0,
                .format = VK_FORMAT_R32G32B32_SFLOAT,
                .offset = offsetof(MFA::AssetSystem::MeshVertex, position),
            };
            input_attribute_descriptions[1] = VkVertexInputAttributeDescription {
                .location = 1,
                .binding = 0,
                .format = VK_FORMAT_R32G32B32_SFLOAT,
                .offset = offsetof(MFA::AssetSystem::MeshVertex, normal_value)
            };

            m_draw_pipeline = RF::CreateDrawPipeline(
                static_cast<MFA::U8>(shaders.size()), 
                shaders.data(),
                m_descriptor_set_layout,
                vertex_binding_description,
                static_cast<MFA::U32>(input_attribute_descriptions.size()),
                input_attribute_descriptions.data(),
                RB::CreateGraphicPipelineOptions {
                    .depth_stencil {
                        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
                        .depthTestEnable = VK_TRUE,
                        .depthWriteEnable = VK_TRUE,
                        .depthCompareOp = VK_COMPARE_OP_LESS,
                        .depthBoundsTestEnable = VK_FALSE,
                        .stencilTestEnable = VK_FALSE
                    },
                    .color_blend_attachments {
                        .blendEnable = VK_TRUE,
                        .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
                        .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
                        .colorBlendOp = VK_BLEND_OP_ADD,
                        .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
                        .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
                        .alphaBlendOp = VK_BLEND_OP_ADD,
                        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
                    },
                    .use_static_viewport_and_scissor = true,
                    .primitive_topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP
                }
            );
        }

        // Descriptor set
        m_sphere_descriptor_sets = RF::CreateDescriptorSets(
            1,//RF::SwapChainImagesCount(), 
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
        RF::DestroyUniformBuffer(m_material_buffer_group);
        RF::DestroyUniformBuffer(m_light_view_buffer_group);
        RF::DestroyUniformBuffer(m_transformation_buffer_group);
        RF::DestroyDrawPipeline(m_draw_pipeline);
        RF::DestroyDescriptorSetLayout(m_descriptor_set_layout);
        RF::DestroyGpuModel(m_sphere);
        Importer::FreeModel(&m_sphere.model_asset);
    }

    void OnDraw(MFA::U32 delta_time, MFA::RenderFrontend::DrawPass & draw_pass) override {
        RF::BindDrawPipeline(draw_pass, m_draw_pipeline);
        {// Updating uniform buffers
            {// Material
                if (m_selected_material_index != CustomMaterialIndex) {
                    auto const & selected_material = MaterialInformation[m_selected_material_index];
                    m_material_data.roughness = selected_material.roughness;
                    m_material_data.metallic = selected_material.metallic;
                    
                    static_assert(sizeof(m_material_data.albedo) == sizeof(selected_material.color));
                    ::memcpy(m_material_data.albedo, selected_material.color, sizeof(m_material_data.albedo));

                } else {
                    m_material_data.roughness = m_sphere_roughness;
                    m_material_data.metallic = m_sphere_metallic;
                    
                    static_assert(sizeof(m_material_data.albedo) == sizeof(m_sphere_color));
                    ::memcpy(m_material_data.albedo, m_sphere_color, sizeof(m_material_data.albedo));
                }
                RF::UpdateUniformBuffer(draw_pass, m_material_buffer_group, MFA::CBlobAliasOf(m_material_data));
            }
            {// LightView
                static_assert(sizeof(m_light_view_data.camPos) == sizeof(m_camera_position));
                ::memcpy(m_light_view_data.camPos, m_camera_position, sizeof(m_camera_position));

                m_light_view_data.lightCount = m_light_count;

               /* static_assert(sizeof(m_light_view_data.lightColors) == sizeof(m_light_colors));
                ::memcpy(m_light_view_data.lightColors, m_light_colors, sizeof(m_light_colors));*/

                static_assert(sizeof(m_light_view_data.lightPositions) == sizeof(m_light_position));
                ::memcpy(m_light_view_data.lightPositions, m_light_position, sizeof(m_light_position));

                RF::UpdateUniformBuffer(draw_pass, m_light_view_buffer_group, MFA::CBlobAliasOf(m_light_view_data));
            }
            {// Transform
                // Rotation
                MFA::Matrix4X4Float rotation;
                MFA::Matrix4X4Float::assignRotationXYZ(
                    rotation,
                    MFA::Math::Deg2Rad(m_sphere_rotation[0]),
                    MFA::Math::Deg2Rad(m_sphere_rotation[1]),
                    MFA::Math::Deg2Rad(m_sphere_rotation[2])
                );
                static_assert(sizeof(m_translate_data.rotation) == sizeof(rotation.cells));
                ::memcpy(m_translate_data.rotation, rotation.cells, sizeof(rotation.cells));

                // Transformation
                MFA::Matrix4X4Float transform;
                MFA::Matrix4X4Float::assignTransformation(
                    transform, 
                    m_sphere_position[0], 
                    m_sphere_position[1], 
                    m_sphere_position[2]
                );
                static_assert(sizeof(transform.cells) == sizeof(m_translate_data.transform));
                ::memcpy(m_translate_data.transform, transform.cells, sizeof(transform.cells));
                
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

                static_assert(sizeof(m_translate_data.perspective) == sizeof(perspective.cells));
                ::memcpy(
                    m_translate_data.perspective, 
                    perspective.cells,
                    sizeof(m_translate_data.perspective)
                );

                RF::UpdateUniformBuffer(
                    draw_pass, 
                    m_transformation_buffer_group, 
                    MFA::CBlobAliasOf(m_translate_data)
                );
            }
        }
        {// Binding and updating descriptor set
            auto * current_descriptor_set = m_sphere_descriptor_sets[0];
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

    void OnUI(MFA::U32 delta_time, MFA::RenderFrontend::DrawPass & draw_pass) override {
        ImGui::Begin("Sphere");
        ImGui::SetNextItemWidth(300.0f);
        ImGui::SliderFloat("XDegree", &m_sphere_rotation[0], -360.0f, 360.0f);
        ImGui::SetNextItemWidth(300.0f);
        ImGui::SliderFloat("YDegree", &m_sphere_rotation[1], -360.0f, 360.0f);
        ImGui::SetNextItemWidth(300.0f);
        ImGui::SliderFloat("ZDegree", &m_sphere_rotation[2], -360.0f, 360.0f);
        ImGui::SetNextItemWidth(300.0f);
        ImGui::SliderFloat("XDistance", &m_sphere_position[0], -40.0f, 40.0f);
        ImGui::SetNextItemWidth(300.0f);
        ImGui::SliderFloat("YDistance", &m_sphere_position[1], -40.0f, 40.0f);
        ImGui::SetNextItemWidth(300.0f);
        ImGui::SliderFloat("ZDistance", &m_sphere_position[2], -40.0f, 40.0f);
        if(m_selected_material_index == CustomMaterialIndex) {
            ImGui::SetNextItemWidth(300.0f);
            ImGui::SliderFloat("RColor", &m_sphere_color[0], 0.0f, 1.0f);
            ImGui::SetNextItemWidth(300.0f);
            ImGui::SliderFloat("GColor", &m_sphere_color[1], 0.0f, 1.0f);
            ImGui::SetNextItemWidth(300.0f);
            ImGui::SliderFloat("BColor", &m_sphere_color[2], 0.0f, 1.0f);
            ImGui::SetNextItemWidth(300.0f);
            ImGui::SliderFloat("Metallic", &m_sphere_metallic, 0.0f, 1.0f);
            ImGui::SetNextItemWidth(300.0f);
            ImGui::SliderFloat("Roughness", &m_sphere_roughness, 0.0f, 1.0f);
        }
        //ImGui::SetNextItemWidth(300.0f);
        //ImGui::SliderFloat("Emission", &m_sphere_emission, 0.0f, 1.0f);
        ImGui::End();
        ImGui::Begin("Camera and light");
        ImGui::SetNextItemWidth(300.0f);
        ImGui::SliderFloat("LightX", &m_light_position[0], -20.0f, 20.0f);
        ImGui::SetNextItemWidth(300.0f);
        ImGui::SliderFloat("LightY", &m_light_position[1], -20.0f, 20.0f);
        ImGui::SetNextItemWidth(300.0f);
        ImGui::SliderFloat("LightZ", &m_light_position[2], -20.0f, 20.0f);

        {// Materials
            std::vector<char const *> items {};
            for (auto const & material: MaterialInformation) {
                items.emplace_back(material.name);
            }
            ImGui::Combo(
                "Material", 
                &m_selected_material_index, 
                items.data(), 
                static_cast<MFA::I32>(items.size())
            );
        }
        //ImGui::SetNextItemWidth(300.0f);
        //ImGui::SliderFloat("LightR", &m_light_colors[0], 0.0f, 1.0f);
        //ImGui::SetNextItemWidth(300.0f);
        //ImGui::SliderFloat("LightG", &m_light_colors[1], 0.0f, 1.0f);
        //ImGui::SetNextItemWidth(300.0f);
        //ImGui::SliderFloat("LightB", &m_light_colors[2], 0.0f, 1.0f);
        //ImGui::SetNextItemWidth(300.0f);
        //ImGui::SliderFloat("CameraX", &m_camera_position[0], -100.0f, 100.0f);
        //ImGui::SetNextItemWidth(300.0f);
        //ImGui::SliderFloat("CameraY", &m_camera_position[1], -100.0f, 100.0f);
        //ImGui::SetNextItemWidth(300.0f);
        //ImGui::SliderFloat("CameraZ", &m_camera_position[2], -1000.0f, 1000.0f);
        ImGui::End();
    }

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

    struct _MaterialInformation {
        char const * name;
        float color[3];
        float roughness;
        float metallic;
    };

    static constexpr int CustomMaterialIndex = 7;
    static constexpr _MaterialInformation MaterialInformation [8] = {
        { .name = "Gold", .color = {1.0f, 0.765557f, 0.336057f}, .roughness = 0.1f, .metallic = 1.0f },
		{ .name = "Copper", .color = {0.955008f, 0.637427f, 0.538163f}, .roughness = 0.1f, .metallic = 1.0f },
		{ .name = "Chromium", .color = {0.549585f, 0.556114f, 0.554256f}, .roughness = 0.1f, .metallic = 1.0f },
		{ .name = "Nickel", .color = {0.659777f, 0.608679f, 0.525649f}, .roughness = 0.1f, .metallic = 1.0f },
		{ .name = "Titanium", .color = {0.541931f, 0.496791f, 0.449419f}, .roughness = 0.1f, .metallic = 1.0f },
		{ .name = "Cobalt", .color = {0.662124f, 0.654864f, 0.633732f}, .roughness = 0.1f, .metallic = 1.0f },
		{ .name = "Platinum", .color = {0.672411f, 0.637331f, 0.585456f}, .roughness = 0.1f, .metallic = 1.0f },
		// Testing materials
		//{ .name = "White", .color = {1.0f, 1.0f, 1.0f}, .roughness = 0.1f, .metallic = 1.0f },
		{ .name = "Custom", .color = {1.0f, 0.0f, 0.0f}, .roughness = 0.5f, .metallic = 0.1f },
		//{ .name = "Blue", .color = {0.0f, 0.0f, 1.0f}, .roughness = 0.1f, .metallic = 1.0f },
		//{ .name = "Black", .color = {0.0f, 0.0f, 0.0f}, .roughness = 0.1f, .metallic = 1.0f },
    };

    RF::UniformBufferGroup m_material_buffer_group;
    struct MaterialBuffer {
        float albedo[3];        // BaseColor
        alignas(4) float metallic;
        alignas(4) float roughness;
        //alignas(4) float ao;               // Emission  
    } m_material_data {};

    RF::UniformBufferGroup m_light_view_buffer_group;
    struct LightViewBuffer {
        alignas(16) float camPos[3];
        alignas(4) MFA::I32 lightCount;
        alignas(16) float lightPositions[3];
        //alignas(16) float lightColors[3];
    } m_light_view_data {};

    RF::UniformBufferGroup m_transformation_buffer_group;
    struct TransformationBuffer {
        alignas(16 * 4) float rotation[16];
        alignas(16 * 4) float transform[16];
        alignas(16 * 4) float perspective[16];
    } m_translate_data {};

    RF::GpuModel m_sphere {};

    VkDescriptorSetLayout_T * m_descriptor_set_layout = nullptr;
    RF::DrawPipeline m_draw_pipeline {};
   // https://seblagarde.wordpress.com/2011/08/17/feeding-a-physical-based-lighting-mode/
    float m_sphere_rotation [3] {0, 0, 0};
    float m_sphere_position[3] {0, 0, -6};
    int m_selected_material_index = 0;
    float m_sphere_color[3] {0.0f, 0.0f, 0.0f};

    float m_sphere_metallic = 1.0f;
    float m_sphere_roughness = 0.1f;
    //float m_sphere_emission = 0.0f; // For now, it's a constant value

    float const m_camera_position[3] {0.0f, 0.0f, 0.0f};   // For now, it's a constant value
 
    MFA::I32 m_light_count = 1; // For now, it's a constant value
    float m_light_position[3] {0.0f, 0.0f, -2.0f};
    //float m_light_colors[3] {1.0f, 1.0f, 1.0f};

    std::vector<VkDescriptorSet_T *> m_sphere_descriptor_sets {};
};

static PBR point_light {};