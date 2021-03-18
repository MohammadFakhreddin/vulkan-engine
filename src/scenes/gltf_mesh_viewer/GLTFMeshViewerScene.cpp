#include "GLTFMeshViewerScene.hpp"

#include "engine/RenderFrontend.hpp"
#include "engine/BedrockMatrix.hpp"
#include "libs/imgui/imgui.h"
#include "engine/DrawableObject.hpp"
#include "tools/Importer.hpp"

namespace RF = MFA::RenderFrontend;
namespace RB = MFA::RenderBackend;
namespace Asset = MFA::AssetSystem;
namespace Importer = MFA::Importer;

void GLTFMeshViewerScene::Init() {
    //auto cpu_model = Importer::ImportMeshGLTF("../assets/models/free_zuk_3d_model/scene.gltf");
    //auto cpu_model = Importer::ImportMeshGLTF("../assets/models/gunship/scene.gltf");
    auto cpu_model = Importer::ImportMeshGLTF("../assets/models/warcraft_3_alliance_footmanfanmade/scene.gltf");
    //auto cpu_model = Importer::ImportMeshGLTF("../assets/models/female_full-body_cyberpunk_themed_avatar/scene.gltf");
    //auto cpu_model = Importer::ImportMeshGLTF("../assets/models/fortnite_the_mandalorianbaby_yoda/scene.gltf");
    //auto cpu_model = Importer::ImportMeshGLTF("../assets/models/mandalorian__the_fortnite_season_6_skin_updated/scene.gltf");
    MFA_ASSERT(cpu_model.mesh.valid());

    //auto * headerObject = cpu_model.mesh.header_object();
    //for (MFA::U32 i = 0; i < headerObject->sub_mesh_count; ++i) {
    //    if (
    //        headerObject->sub_meshes[i].has_normal_texture == false ||
    //        headerObject->sub_meshes[i].has_tangent_buffer == false ||
    //        headerObject->sub_meshes[i].has_base_color_texture == false ||
    //        headerObject->sub_meshes[i].has_combined_metallic_roughness_texture == false
    //    ) {
    //        for (MFA::U32 j = i; j < headerObject->sub_mesh_count - 1; ++j) {
    //            headerObject->sub_meshes[j] = headerObject->sub_meshes[j + 1];
    //        }
    //        --headerObject->sub_mesh_count;
    //    }
    //}
    
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
        "../assets/shaders/gltf_mesh_viewer/GLTFMeshViewer2.frag.spv",
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

    m_lv_buffer = RF::CreateUniformBuffer(sizeof(LightViewBuffer), 1);

    createDescriptorSetLayout();

    createDrawPipeline(static_cast<MFA::U8>(shaders.size()), shaders.data());
    
    createDrawableObject();
}

void GLTFMeshViewerScene::OnDraw(MFA::U32 const delta_time, RF::DrawPass & draw_pass) {
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
        RF::UpdateUniformBuffer(m_lv_buffer.buffers[0], MFA::CBlobAliasOf(m_lv_data));
    }
    m_drawable_object.draw(draw_pass);
}

void GLTFMeshViewerScene::OnUI(MFA::U32 const delta_time, MFA::RenderFrontend::DrawPass & draw_pass) {
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
    ImGui::SliderFloat("LightX", &m_light_position[0], -200.0f, 200.0f);
    ImGui::SetNextItemWidth(300.0f);
    ImGui::SliderFloat("LightY", &m_light_position[1], -200.0f, 200.0f);
    ImGui::SetNextItemWidth(300.0f);
    ImGui::SliderFloat("LightZ", &m_light_position[2], -200.0f, 200.0f);
    ImGui::End();
}

void GLTFMeshViewerScene::Shutdown() {
    destroyDrawableObject();
    RF::DestroyUniformBuffer(m_lv_buffer);
    RF::DestroyDrawPipeline(m_draw_pipeline);
    RF::DestroyDescriptorSetLayout(m_descriptor_set_layout);
    RF::DestroySampler(m_sampler_group);
    RF::DestroyGpuModel(m_gpu_model);
    Importer::FreeModel(&m_gpu_model.model_asset);
}

void GLTFMeshViewerScene::createDrawableObject() {
    m_drawable_object = MFA::DrawableObject(
        m_gpu_model,
        m_descriptor_set_layout
    );

    const auto * subMeshInfoBuffer = m_drawable_object.create_multiple_uniform_buffer(
        "subMeshInfo", 
        sizeof(SubMeshInfo), 
        m_drawable_object.get_descriptor_set_count()
    );
    MFA_PTR_ASSERT(subMeshInfoBuffer);

    const auto * transform_buffer = m_drawable_object.create_uniform_buffer("transform", sizeof(ModelTransformBuffer));
    MFA_PTR_ASSERT(transform_buffer);

    auto * model_header = m_drawable_object.get_model()->model_asset.mesh.header_object();
    MFA_ASSERT(model_header->sub_mesh_count == m_drawable_object.get_descriptor_set_count());

    auto const & textures = m_drawable_object.get_model()->textures;

    for(MFA::U8 i = 0; i < m_drawable_object.get_descriptor_set_count(); ++i) {// Updating descriptor sets
        auto * descriptor_set = m_drawable_object.get_descriptor_set(i);
        auto const & sub_mesh = model_header->sub_meshes[i];

        std::vector<VkWriteDescriptorSet> writeInfo {};

        // SubMeshInfo
        VkDescriptorBufferInfo subMeshBufferInfo {
            .buffer = subMeshInfoBuffer->buffers[i].buffer,
            .offset = 0,
            .range = subMeshInfoBuffer->buffer_size
        };
        writeInfo.emplace_back(VkWriteDescriptorSet {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = descriptor_set,
            .dstBinding = static_cast<uint32_t>(writeInfo.size()),
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .pBufferInfo = &subMeshBufferInfo,
        });
        SubMeshInfo info {
            .baseColorFactor {},
            .hasBaseColorTexture = sub_mesh.has_base_color_texture ? 1 : 0,
            .metallicFactor = sub_mesh.metallic_factor,
            .roughnessFactor = sub_mesh.roughness_factor,
            .hasMetallicRoughnessTexture = sub_mesh.has_combined_metallic_roughness_texture ? 1 : 0,
            .hasNormalTexture = sub_mesh.has_normal_texture ? 1 : 0,
        };
        ::memcpy(info.baseColorFactor, sub_mesh.base_color_factor, sizeof(info.baseColorFactor));
        static_assert(sizeof(info.baseColorFactor) == sizeof(sub_mesh.base_color_factor));

        RF::UpdateUniformBuffer(
            subMeshInfoBuffer->buffers[i], 
            MFA::CBlobAliasOf(info)
        );

        // Transform
        VkDescriptorBufferInfo transformBufferInfo {
            .buffer = transform_buffer->buffers[0].buffer,
            .offset = 0,
            .range = transform_buffer->buffer_size
        };
        writeInfo.emplace_back(VkWriteDescriptorSet {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = descriptor_set,
            .dstBinding = static_cast<uint32_t>(writeInfo.size()),
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
            .dstBinding = static_cast<uint32_t>(writeInfo.size()),
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .pImageInfo = &baseColorImageInfo,
        });

        // Metallic/RoughnessTexture
        VkDescriptorImageInfo metallicImageInfo {
            .sampler = m_sampler_group.sampler,          // TODO Each texture has it's own properties that may need it's own sampler (Not sure yet)
            .imageView = textures[sub_mesh.metallic_roughness_texture_index].image_view(),
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        };
        writeInfo.emplace_back(VkWriteDescriptorSet {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = descriptor_set,
            .dstBinding = static_cast<uint32_t>(writeInfo.size()),
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .pImageInfo = &metallicImageInfo,
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
            .dstBinding = static_cast<uint32_t>(writeInfo.size()),
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
            .dstBinding = static_cast<uint32_t>(writeInfo.size()),
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .pBufferInfo = &light_view_buffer_info,
        });

        RF::UpdateDescriptorSets(
            static_cast<MFA::U8>(writeInfo.size()),
            writeInfo.data()
        );

        // SubMeshInfoBuffer
        // TODO Start from here
    }
}

void GLTFMeshViewerScene::destroyDrawableObject() {
    m_drawable_object.delete_uniform_buffers();
}

void GLTFMeshViewerScene::createDrawPipeline(MFA::U8 const gpu_shader_count, MFA::RenderBackend::GpuShader * gpu_shaders) {

    VkVertexInputBindingDescription const vertex_binding_description {
        .binding = 0,
        .stride = sizeof(Asset::MeshVertex),
        .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
    };

    std::vector<VkVertexInputAttributeDescription> input_attribute_descriptions {};
    // Position
    input_attribute_descriptions.emplace_back(VkVertexInputAttributeDescription {
        .location = static_cast<MFA::U32>(input_attribute_descriptions.size()),
        .binding = 0,
        .format = VK_FORMAT_R32G32B32_SFLOAT,
        .offset = offsetof(Asset::MeshVertex, position),   
    });
    // BaseColor
    input_attribute_descriptions.emplace_back(VkVertexInputAttributeDescription {
        .location = static_cast<MFA::U32>(input_attribute_descriptions.size()),
        .binding = 0,
        .format = VK_FORMAT_R32G32_SFLOAT,
        .offset = offsetof(Asset::MeshVertex, base_color_uv),   
    });
    // Metallic/Roughness
    input_attribute_descriptions.emplace_back(VkVertexInputAttributeDescription {
        .location = static_cast<MFA::U32>(input_attribute_descriptions.size()),
        .binding = 0,
        .format = VK_FORMAT_R32G32_SFLOAT,
        .offset = offsetof(Asset::MeshVertex, metallic_uv), // Metallic and roughness have same uv for gltf files  
    });
    // Normal
    input_attribute_descriptions.emplace_back(VkVertexInputAttributeDescription {
        .location = static_cast<MFA::U32>(input_attribute_descriptions.size()),
        .binding = 0,
        .format = VK_FORMAT_R32G32_SFLOAT,
        .offset = offsetof(Asset::MeshVertex, normal_map_uv),   
    });
    input_attribute_descriptions.emplace_back(VkVertexInputAttributeDescription {
        .location = static_cast<MFA::U32>(input_attribute_descriptions.size()),
        .binding = 0,
        .format = VK_FORMAT_R32G32B32A32_SFLOAT,
        .offset = offsetof(Asset::MeshVertex, tangent_value),   
    });
    input_attribute_descriptions.emplace_back(VkVertexInputAttributeDescription {
        .location = static_cast<MFA::U32>(input_attribute_descriptions.size()),
        .binding = 0,
        .format = VK_FORMAT_R32G32B32_SFLOAT,
        .offset = offsetof(Asset::MeshVertex, normal_value),   
    });
    
    m_draw_pipeline = RF::CreateBasicDrawPipeline(
        gpu_shader_count, 
        gpu_shaders,
        m_descriptor_set_layout,
        vertex_binding_description,
        static_cast<MFA::U8>(input_attribute_descriptions.size()),
        input_attribute_descriptions.data()
    );

}

void GLTFMeshViewerScene::createDescriptorSetLayout() {
    std::vector<VkDescriptorSetLayoutBinding> bindings {};
    // Transformation 
    bindings.emplace_back(VkDescriptorSetLayoutBinding {
        .binding = static_cast<MFA::U32>(bindings.size()),
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
        .pImmutableSamplers = nullptr, // Optional
    });
    // SubMeshInfo
    bindings.emplace_back(VkDescriptorSetLayoutBinding {
        .binding = static_cast<MFA::U32>(bindings.size()),
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
        .pImmutableSamplers = nullptr, // Optional
    });
    // BaseColor
    bindings.emplace_back(VkDescriptorSetLayoutBinding {
        .binding = static_cast<MFA::U32>(bindings.size()),
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
        .pImmutableSamplers = nullptr,
    });
    // Metallic/Roughness
    bindings.emplace_back(VkDescriptorSetLayoutBinding {
        .binding = static_cast<MFA::U32>(bindings.size()),
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
        .pImmutableSamplers = nullptr,
    });
    // Normal
    bindings.emplace_back(VkDescriptorSetLayoutBinding {
        .binding = static_cast<MFA::U32>(bindings.size()),
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
        .pImmutableSamplers = nullptr,
    });
    // Light/View
    bindings.emplace_back(VkDescriptorSetLayoutBinding {
        .binding = static_cast<MFA::U32>(bindings.size()),
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