#include "TexturedSphereScene.hpp"


#include "libs/imgui/imgui.h"
#include "tools/Importer.hpp"
#include "tools/ShapeGenerator.hpp"

void TexturedSphereScene::Init() {
    // Gpu model
    createGpuModel();

    // Cpu shader
    auto cpu_vertex_shader = MFA::Importer::ImportShaderFromSPV(
        "../assets/shaders/textured_sphere/TexturedSphere.vert.spv", 
        MFA::AssetSystem::Shader::Stage::Vertex, 
        "main"
    );
    MFA_ASSERT(cpu_vertex_shader.valid());
    auto gpu_vertex_shader = RF::CreateShader(cpu_vertex_shader);
    MFA_ASSERT(gpu_vertex_shader.valid());
    MFA_DEFER {
        RF::DestroyShader(gpu_vertex_shader);
        MFA::Importer::FreeAsset(&cpu_vertex_shader);
    };
    
    // Fragment shader
    auto cpu_fragment_shader = MFA::Importer::ImportShaderFromSPV(
        "../assets/shaders/textured_sphere/TexturedSphere.frag.spv",
        MFA::AssetSystem::Shader::Stage::Fragment,
        "main"
    );
    auto gpu_fragment_shader = RF::CreateShader(cpu_fragment_shader);
    MFA_ASSERT(cpu_fragment_shader.valid());
    MFA_ASSERT(gpu_fragment_shader.valid());
    MFA_DEFER {
        RF::DestroyShader(gpu_fragment_shader);
        MFA::Importer::FreeAsset(&cpu_fragment_shader);
    };

    std::vector<RB::GpuShader> shaders {gpu_vertex_shader, gpu_fragment_shader};

    // TODO We should use gltf sampler info here
    m_sampler_group = RF::CreateSampler();

    m_lv_buffer = RF::CreateUniformBuffer(sizeof(LightViewBuffer), 1);

    createDescriptorSetLayout();

    createDrawPipeline(static_cast<MFA::U8>(shaders.size()), shaders.data());
    
    createDrawableObject();
}

void TexturedSphereScene::OnDraw(MFA::U32 delta_time, MFA::RenderFrontend::DrawPass & draw_pass) {
    RF::BindDrawPipeline(draw_pass, m_draw_pipeline);

    {// Updating Transform buffer
        // Rotation
        // TODO Try sending Matrices directly
        MFA::Matrix4X4Float rotationMat {};
        MFA::Matrix4X4Float::assignRotationXYZ(
            rotationMat,
            MFA::Math::Deg2Rad(m_model_rotation[0]),
            MFA::Math::Deg2Rad(m_model_rotation[1]),
            MFA::Math::Deg2Rad(m_model_rotation[2])
        );
        static_assert(sizeof(m_translate_data.rotation) == sizeof(rotationMat.cells));
        ::memcpy(m_translate_data.rotation, rotationMat.cells, sizeof(rotationMat.cells));
        // Position
        MFA::Matrix4X4Float transformationMat {};
        MFA::Matrix4X4Float::assignTransformation(
            transformationMat,
            m_model_position[0],
            m_model_position[1],
            m_model_position[2]
        );
        static_assert(sizeof(m_translate_data.transformation) == sizeof(transformationMat.cells));
        ::memcpy(m_translate_data.transformation, transformationMat.cells, sizeof(transformationMat.cells));
        // Perspective
        MFA::I32 width; MFA::I32 height;
        RF::GetWindowSize(width, height);
        float const ratio = static_cast<float>(width) / static_cast<float>(height);
        MFA::Matrix4X4Float perspectiveMat {};
        MFA::Matrix4X4Float::PreparePerspectiveProjectionMatrix(
            perspectiveMat,
            ratio,
            40,
            Z_NEAR,
            Z_FAR
        );
        static_assert(sizeof(m_translate_data.perspective) == sizeof(perspectiveMat.cells));
        ::memcpy(m_translate_data.perspective, perspectiveMat.cells, sizeof(perspectiveMat.cells));

        m_drawable_object.update_uniform_buffer(
            "transform", 
            MFA::CBlobAliasOf(m_translate_data)
        );
    }
    {// LightViewBuffer
        ::memcpy(m_lv_data.light_position, m_light_position, sizeof(m_light_position));
        static_assert(sizeof(m_light_position) == sizeof(m_lv_data.light_position));

        ::memcpy(m_lv_data.light_color, m_light_color, sizeof(m_light_color));
        static_assert(sizeof(m_light_color) == sizeof(m_lv_data.light_color));

        RF::UpdateUniformBuffer(m_lv_buffer.buffers[0], MFA::CBlobAliasOf(m_lv_data));
    }
    m_drawable_object.draw(draw_pass);
}

void TexturedSphereScene::OnUI(MFA::U32 delta_time, MFA::RenderFrontend::DrawPass & draw_pass) {
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

    ImGui::Begin("Light");
    ImGui::SetNextItemWidth(300.0f);
    ImGui::SliderFloat("PositionX", &m_light_position[0], -200.0f, 200.0f);
    ImGui::SetNextItemWidth(300.0f);
    ImGui::SliderFloat("PositionY", &m_light_position[1], -200.0f, 200.0f);
    ImGui::SetNextItemWidth(300.0f);
    ImGui::SliderFloat("PositionZ", &m_light_position[2], -200.0f, 200.0f);
    ImGui::SetNextItemWidth(300.0f);
    ImGui::SliderFloat("ColorR", &m_light_color[0], 0.0f, 1.0f);
    ImGui::SetNextItemWidth(300.0f);
    ImGui::SliderFloat("ColorG", &m_light_color[1], 0.0f, 1.0f);
    ImGui::SetNextItemWidth(300.0f);
    ImGui::SliderFloat("ColorB", &m_light_color[2], 0.0f, 1.0f);
    ImGui::End();
}

void TexturedSphereScene::Shutdown() {
    destroyDrawableObject();
    RF::DestroyUniformBuffer(m_lv_buffer);
    RF::DestroyDrawPipeline(m_draw_pipeline);
    RF::DestroyDescriptorSetLayout(m_descriptor_set_layout);
    RF::DestroySampler(m_sampler_group);
    RF::DestroyGpuModel(m_gpu_model);
    MFA::Importer::FreeModel(&m_gpu_model.model_asset);
}

void TexturedSphereScene::createDrawableObject(){
     m_drawable_object = MFA::DrawableObject(
        m_gpu_model,
        m_descriptor_set_layout
    );
    // TODO AO map for emission
    const auto * transform_buffer = m_drawable_object.create_uniform_buffer("transform", sizeof(ModelTransformBuffer));
    MFA_PTR_ASSERT(transform_buffer);
    
    auto * model_header = m_drawable_object.get_model()->model_asset.mesh.header_object();
    MFA_ASSERT(model_header->sub_mesh_count == m_drawable_object.get_descriptor_set_count());

    auto const & textures = m_drawable_object.get_model()->textures;

    for(MFA::U8 i = 0; i < m_drawable_object.get_descriptor_set_count(); ++i) {// Updating descriptor sets
        auto * descriptor_set = m_drawable_object.get_descriptor_set(i);
        auto const & sub_mesh = model_header->sub_meshes[i];

        std::vector<VkWriteDescriptorSet> writeInfo {};

        // Transform
        VkDescriptorBufferInfo transformBufferInfo {
            .buffer = transform_buffer->buffers[0].buffer,
            .offset = 0,
            .range = transform_buffer->buffer_size
        };
        writeInfo.emplace_back(VkWriteDescriptorSet {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = descriptor_set,
            .dstBinding = 0,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .pBufferInfo = &transformBufferInfo,
        });

        // BaseColorTexture
        VkDescriptorImageInfo baseColorImageInfo {
            .sampler = m_sampler_group.sampler,          // TODO Each texture has it's own properties that may need it's own sampler (Not sure yet)
            .imageView = textures[sub_mesh.base_color_texture_index].image_view(),
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        };
        writeInfo.emplace_back(VkWriteDescriptorSet {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = descriptor_set,
            .dstBinding = 1,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .pImageInfo = &baseColorImageInfo,
        });

        // MetallicTexture
        VkDescriptorImageInfo metallicImageInfo {
            .sampler = m_sampler_group.sampler,          // TODO Each texture has it's own properties that may need it's own sampler (Not sure yet)
            .imageView = textures[sub_mesh.metallic_texture_index].image_view(),
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        };
        writeInfo.emplace_back(VkWriteDescriptorSet {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = descriptor_set,
            .dstBinding = 2,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .pImageInfo = &metallicImageInfo,
        });

        // RoughnessTexture
        VkDescriptorImageInfo roughnessImageInfo {
            .sampler = m_sampler_group.sampler,          // TODO Each texture has it's own properties that may need it's own sampler (Not sure yet)
            .imageView = textures[sub_mesh.roughness_texture_index].image_view(),
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        };
        writeInfo.emplace_back(VkWriteDescriptorSet {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = descriptor_set,
            .dstBinding = 3,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .pImageInfo = &roughnessImageInfo,
        });

        // NormalTexture  
        VkDescriptorImageInfo normalImageInfo {
            .sampler = m_sampler_group.sampler,
            .imageView = textures[sub_mesh.normal_texture_index].image_view(),
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        };
        writeInfo.emplace_back(VkWriteDescriptorSet {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = descriptor_set,
            .dstBinding = 4,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .pImageInfo = &normalImageInfo,
        });

        // LightViewBuffer
        VkDescriptorBufferInfo light_view_buffer_info {
            .buffer = m_lv_buffer.buffers[0].buffer,
            .offset = 0,
            .range = m_lv_buffer.buffer_size
        };
        writeInfo.emplace_back(VkWriteDescriptorSet {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = descriptor_set,
            .dstBinding = 5,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .pBufferInfo = &light_view_buffer_info,
        });

        RF::UpdateDescriptorSets(
            static_cast<MFA::U8>(writeInfo.size()),
            writeInfo.data()
        );
    }
}

void TexturedSphereScene::destroyDrawableObject(){
    m_drawable_object.delete_uniform_buffers();
}

void TexturedSphereScene::createDrawPipeline(MFA::U8 gpu_shader_count, MFA::RenderBackend::GpuShader * gpu_shaders){
    VkVertexInputBindingDescription const vertex_binding_description {
        .binding = 0,
        .stride = sizeof(Asset::MeshVertex),
        .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
    };

    std::vector<VkVertexInputAttributeDescription> input_attribute_descriptions {};

    input_attribute_descriptions.emplace_back(VkVertexInputAttributeDescription {
        .location = 0,
        .binding = 0,
        .format = VK_FORMAT_R32G32B32_SFLOAT,
        .offset = offsetof(Asset::MeshVertex, position),   
    });
    input_attribute_descriptions.emplace_back(VkVertexInputAttributeDescription {
        .location = 1,
        .binding = 0,
        .format = VK_FORMAT_R32G32_SFLOAT,
        .offset = offsetof(Asset::MeshVertex, baseColorUV),   
    });
    input_attribute_descriptions.emplace_back(VkVertexInputAttributeDescription {
        .location = 2,
        .binding = 0,
        .format = VK_FORMAT_R32G32_SFLOAT,
        .offset = offsetof(Asset::MeshVertex, metallicUV),   
    });
    input_attribute_descriptions.emplace_back(VkVertexInputAttributeDescription {
        .location = 3,
        .binding = 0,
        .format = VK_FORMAT_R32G32_SFLOAT,
        .offset = offsetof(Asset::MeshVertex, roughnessUV),   
    });
    input_attribute_descriptions.emplace_back(VkVertexInputAttributeDescription {
        .location = 4,
        .binding = 0,
        .format = VK_FORMAT_R32G32_SFLOAT,
        .offset = offsetof(Asset::MeshVertex, normalMapUV),   
    });
    input_attribute_descriptions.emplace_back(VkVertexInputAttributeDescription {
        .location = 5,
        .binding = 0,
        .format = VK_FORMAT_R32G32B32_SFLOAT,
        .offset = offsetof(Asset::MeshVertex, normalValue),   
    });
    input_attribute_descriptions.emplace_back(VkVertexInputAttributeDescription {
        .location = 6,
        .binding = 0,
        .format = VK_FORMAT_R32G32B32A32_SFLOAT,
        .offset = offsetof(Asset::MeshVertex, tangentValue),   
    });

    m_draw_pipeline = RF::CreateDrawPipeline(
        gpu_shader_count, 
        gpu_shaders,
        m_descriptor_set_layout,
        vertex_binding_description,
        static_cast<MFA::U8>(input_attribute_descriptions.size()),
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

void TexturedSphereScene::createDescriptorSetLayout(){
    std::vector<VkDescriptorSetLayoutBinding> bindings {};
    // Transformation 
    bindings.emplace_back(VkDescriptorSetLayoutBinding {
        .binding = 0,
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
        .pImmutableSamplers = nullptr, // Optional
    });
    // BaseColor
    bindings.emplace_back(VkDescriptorSetLayoutBinding {
        .binding = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
        .pImmutableSamplers = nullptr,
    });
    // Metallic
    bindings.emplace_back(VkDescriptorSetLayoutBinding {
        .binding = 2,
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
        .pImmutableSamplers = nullptr,
    });
    // Roughness
    bindings.emplace_back(VkDescriptorSetLayoutBinding {
        .binding = 3,
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
        .pImmutableSamplers = nullptr,
    });
    // Normal
    bindings.emplace_back(VkDescriptorSetLayoutBinding {
        .binding = 4,
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
        .pImmutableSamplers = nullptr,
    });
    // Light/View
    bindings.emplace_back(VkDescriptorSetLayoutBinding {
        .binding = 5,
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
        .pImmutableSamplers = nullptr, // Optional
    });
    m_descriptor_set_layout = RF::CreateDescriptorSetLayout(
        static_cast<MFA::U8>(bindings.size()),
        bindings.data()
    );
}

void TexturedSphereScene::createGpuModel() {
    auto cpuModel = MFA::ShapeGenerator::Sphere();

    auto const importTextureForModel = [&cpuModel](char const * address) -> MFA::I16 {
        auto const texture = MFA::Importer::ImportUncompressedImage(
            address, 
            MFA::Importer::ImportTextureOptions {.generate_mipmaps = false}
        );
        MFA_ASSERT(texture.valid());
        cpuModel.textures.emplace_back(texture);
        return static_cast<MFA::I16>(cpuModel.textures.size() - 1);
    };

    // BaseColor
    auto const baseColorIndex = importTextureForModel("../assets/models/rusted_sphere_texture/rustediron2_basecolor.png");
    // Metallic
    auto const metallicIndex = importTextureForModel("../assets/models/rusted_sphere_texture/rustediron2_metallic.png");
    // Normal
    auto const normalIndex = importTextureForModel("../assets/models/rusted_sphere_texture/rustediron2_normal.png");
    // Roughness
    auto const roughnessIndex = importTextureForModel("../assets/models/rusted_sphere_texture/rustediron2_roughness.png");
    
    auto * meshHeader = cpuModel.mesh.header_object();
    MFA_PTR_ASSERT(meshHeader);
    MFA_ASSERT(meshHeader->is_valid());
    for (MFA::U32 i = 0; i < meshHeader->sub_mesh_count; ++i) {
        auto & currentSubMesh = meshHeader->sub_meshes[i];
        // Texture index
        currentSubMesh.base_color_texture_index = baseColorIndex;
        currentSubMesh.normal_texture_index = normalIndex;
        currentSubMesh.metallic_texture_index = metallicIndex;
        currentSubMesh.roughness_texture_index = roughnessIndex;
        // SubMesh features
        currentSubMesh.has_base_color_texture = true;
        currentSubMesh.has_normal_texture = true;
        currentSubMesh.has_metallic_texture = true;
        currentSubMesh.has_roughness_texture = true;
    }

    m_gpu_model = RF::CreateGpuModel(cpuModel);
}
