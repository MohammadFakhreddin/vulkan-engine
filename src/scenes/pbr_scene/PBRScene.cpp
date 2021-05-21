#include "PBRScene.hpp"

#include "engine/BedrockMatrix.hpp"
#include "tools/ShapeGenerator.hpp"
#include "tools/Importer.hpp"

namespace RF = MFA::RenderFrontend;
namespace RB = MFA::RenderBackend;
namespace SG = MFA::ShapeGenerator;
namespace Importer = MFA::Importer;
namespace UI = MFA::UISystem;

void PBRScene::Init() {

    // Gpu model
    auto cpu_model = SG::Sphere();
    m_sphere = RF::CreateGpuModel(cpu_model);

    // Vertex shader
    auto cpu_vertex_shader = Importer::ImportShaderFromSPV(
        "../assets/shaders/pbr/pbr.vert.spv", 
        MFA::AssetSystem::Shader::Stage::Vertex, 
        "main"
    );
    MFA_ASSERT(cpu_vertex_shader.isValid());
    auto gpu_vertex_shader = RF::CreateShader(cpu_vertex_shader);
    MFA_ASSERT(gpu_vertex_shader.valid());
    MFA_DEFER {
        RF::DestroyShader(gpu_vertex_shader);
        Importer::FreeShader(&cpu_vertex_shader);
    };

    // Fragment shader
    auto cpu_frag_shader = Importer::ImportShaderFromSPV(
        "../assets/shaders/pbr/pbr.frag.spv", 
        MFA::AssetSystem::Shader::Stage::Fragment, 
        "main"
    );
    MFA_ASSERT(cpu_frag_shader.isValid());
    auto gpu_fragment_shader = RF::CreateShader(cpu_frag_shader);
    MFA_ASSERT(gpu_fragment_shader.valid());
    MFA_DEFER {
        RF::DestroyShader(gpu_fragment_shader);
        Importer::FreeShader(&cpu_frag_shader);
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
            static_cast<uint8_t>(bindings.size()), 
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
            .offset = offsetof(MFA::AssetSystem::MeshVertex, normalValue)
        };

        m_draw_pipeline = RF::CreateDrawPipeline(
            static_cast<uint8_t>(shaders.size()), 
            shaders.data(),
            1,
            &m_descriptor_set_layout,
            vertex_binding_description,
            static_cast<uint32_t>(input_attribute_descriptions.size()),
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
                .use_static_viewport_and_scissor = false,
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
        m_material_buffer_group = RF::CreateUniformBuffer(sizeof(MaterialBuffer), 1);
        m_light_view_buffer_group = RF::CreateUniformBuffer(sizeof(LightViewBuffer), 1);
        m_transformation_buffer_group = RF::CreateUniformBuffer(sizeof(TransformationBuffer), 1);
    }

    updateAllDescriptorSets();

    updateProjection();
}

void PBRScene::Shutdown() {
    RF::DestroyUniformBuffer(m_material_buffer_group);
    RF::DestroyUniformBuffer(m_light_view_buffer_group);
    RF::DestroyUniformBuffer(m_transformation_buffer_group);
    RF::DestroyDrawPipeline(m_draw_pipeline);
    RF::DestroyDescriptorSetLayout(m_descriptor_set_layout);
    RF::DestroyGpuModel(m_sphere);
    Importer::FreeModel(&m_sphere.model);
}

void PBRScene::OnDraw(uint32_t delta_time, MFA::RenderFrontend::DrawPass & draw_pass) {
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
            RF::UpdateUniformBuffer(m_material_buffer_group.buffers[0], MFA::CBlobAliasOf(m_material_data));
        }
        {// LightView
            static_assert(sizeof(m_light_view_data.camPos) == sizeof(m_camera_position));
            ::memcpy(m_light_view_data.camPos, m_camera_position, sizeof(m_camera_position));

            m_light_view_data.lightCount = m_light_count;

           /* static_assert(sizeof(m_light_view_data.lightColors) == sizeof(m_light_colors));
            ::memcpy(m_light_view_data.lightColors, m_light_colors, sizeof(m_light_colors));*/

            static_assert(sizeof(m_light_view_data.lightPositions) == sizeof(m_light_position));
            ::memcpy(m_light_view_data.lightPositions, m_light_position, sizeof(m_light_position));

            RF::UpdateUniformBuffer(m_light_view_buffer_group.buffers[0], MFA::CBlobAliasOf(m_light_view_data));
        }
        {// Transform
            // Rotation
            MFA::Matrix4X4Float rotation;
            MFA::Matrix4X4Float::AssignRotation(
                rotation,
                m_sphere_rotation[0],
                m_sphere_rotation[1],
                m_sphere_rotation[2]
            );
            static_assert(sizeof(m_translate_data.rotation) == sizeof(rotation.cells));
            ::memcpy(m_translate_data.rotation, rotation.cells, sizeof(rotation.cells));

            // Transformation
            MFA::Matrix4X4Float transform;
            MFA::Matrix4X4Float::AssignTranslation(
                transform, 
                m_sphere_position[0], 
                m_sphere_position[1], 
                m_sphere_position[2]
            );
            static_assert(sizeof(transform.cells) == sizeof(m_translate_data.transform));
            ::memcpy(m_translate_data.transform, transform.cells, sizeof(transform.cells));
            
            RF::UpdateUniformBuffer(
                m_transformation_buffer_group.buffers[0], 
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
        BindVertexBuffer(draw_pass, m_sphere.meshBuffers.verticesBuffer);
        BindIndexBuffer(draw_pass, m_sphere.meshBuffers.indicesBuffer);
        for (uint32_t i = 0; i < m_sphere.model.mesh.getSubMeshCount(); ++i) {
            auto const & subMesh = m_sphere.model.mesh.getSubMeshByIndex(i);
            if (subMesh.primitives.empty() == false) {
                for (auto const & primitive : subMesh.primitives) {
                    DrawIndexed(
                        draw_pass,
                        primitive.indicesCount,
                        1,
                        primitive.indicesStartingIndex
                    );
                }
            }
        }
    }
}

void PBRScene::OnUI(uint32_t delta_time, MFA::RenderFrontend::DrawPass & draw_pass) {
    UI::BeginWindow("Sphere");
    UI::SetNextItemWidth(300.0f);
    UI::SliderFloat("XDegree", &m_sphere_rotation[0], -360.0f, 360.0f);
    UI::SetNextItemWidth(300.0f);
    UI::SliderFloat("YDegree", &m_sphere_rotation[1], -360.0f, 360.0f);
    UI::SetNextItemWidth(300.0f);
    UI::SliderFloat("ZDegree", &m_sphere_rotation[2], -360.0f, 360.0f);
    UI::SetNextItemWidth(300.0f);
    UI::SliderFloat("XDistance", &m_sphere_position[0], -40.0f, 40.0f);
    UI::SetNextItemWidth(300.0f);
    UI::SliderFloat("YDistance", &m_sphere_position[1], -40.0f, 40.0f);
    UI::SetNextItemWidth(300.0f);
    UI::SliderFloat("ZDistance", &m_sphere_position[2], -40.0f, 40.0f);
    if(m_selected_material_index == CustomMaterialIndex) {
        UI::SetNextItemWidth(300.0f);
        UI::SliderFloat("RColor", &m_sphere_color[0], 0.0f, 1.0f);
        UI::SetNextItemWidth(300.0f);
        UI::SliderFloat("GColor", &m_sphere_color[1], 0.0f, 1.0f);
        UI::SetNextItemWidth(300.0f);
        UI::SliderFloat("BColor", &m_sphere_color[2], 0.0f, 1.0f);
        UI::SetNextItemWidth(300.0f);
        UI::SliderFloat("Metallic", &m_sphere_metallic, 0.0f, 1.0f);
        UI::SetNextItemWidth(300.0f);
        UI::SliderFloat("Roughness", &m_sphere_roughness, 0.0f, 1.0f);
    }
    //UI::SetNextItemWidth(300.0f);
    //UI::SliderFloat("Emission", &m_sphere_emission, 0.0f, 1.0f);
    UI::EndWindow();
    UI::BeginWindow("Camera and light");
    UI::SetNextItemWidth(300.0f);
    UI::SliderFloat("LightX", &m_light_position[0], -20.0f, 20.0f);
    UI::SetNextItemWidth(300.0f);
    UI::SliderFloat("LightY", &m_light_position[1], -20.0f, 20.0f);
    UI::SetNextItemWidth(300.0f);
    UI::SliderFloat("LightZ", &m_light_position[2], -20.0f, 20.0f);

    {// Materials
        std::vector<char const *> items {};
        for (auto const & material: MaterialInformation) {
            items.emplace_back(material.name);
        }
        UI::Combo(
            "Material", 
            &m_selected_material_index, 
            items.data(), 
            static_cast<int32_t>(items.size())
        );
    }
    //UI::SetNextItemWidth(300.0f);
    //UI::SliderFloat("LightR", &m_light_colors[0], 0.0f, 1.0f);
    //UI::SetNextItemWidth(300.0f);
    //UI::SliderFloat("LightG", &m_light_colors[1], 0.0f, 1.0f);
    //UI::SetNextItemWidth(300.0f);
    //UI::SliderFloat("LightB", &m_light_colors[2], 0.0f, 1.0f);
    //UI::SetNextItemWidth(300.0f);
    //UI::SliderFloat("CameraX", &m_camera_position[0], -100.0f, 100.0f);
    //UI::SetNextItemWidth(300.0f);
    //UI::SliderFloat("CameraY", &m_camera_position[1], -100.0f, 100.0f);
    //UI::SetNextItemWidth(300.0f);
    //UI::SliderFloat("CameraZ", &m_camera_position[2], -1000.0f, 1000.0f);
    UI::EndWindow();
}

void PBRScene::OnResize() {
    updateProjection();
}

void PBRScene::updateAllDescriptorSets() {
    for (uint8_t i = 0; i < m_sphere_descriptor_sets.size(); ++i) {
        updateDescriptorSet(i);
    }
}

void PBRScene::updateDescriptorSet(uint8_t const index) {
    auto * current_descriptor_set = m_sphere_descriptor_sets[index];
    std::vector<VkWriteDescriptorSet> write_info {};
    // Transform
    VkDescriptorBufferInfo transform_buffer_info {
        .buffer = m_transformation_buffer_group.buffers[index].buffer,
        .offset = 0,
        .range = m_transformation_buffer_group.bufferSize
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
        .range = m_material_buffer_group.bufferSize
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
        .range = m_light_view_buffer_group.bufferSize
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
        static_cast<uint8_t>(write_info.size()), 
        write_info.data()
    );
}

void PBRScene::updateProjection() {
    // Perspective
    int32_t width; int32_t height;
    RF::GetWindowSize(width, height);
    float const ratio = static_cast<float>(width) / static_cast<float>(height);
    MFA::Matrix4X4Float perspective {};
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
}
