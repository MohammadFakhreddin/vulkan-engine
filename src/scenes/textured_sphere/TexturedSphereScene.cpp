#include "TexturedSphereScene.hpp"

#include "libs/imgui/imgui.h"
#include "tools/ShapeGenerator.hpp"

void TexturedSphereScene::Init() {
    // Gpu model
    createGpuModel();

    // Cpu shader
    auto cpuVertexShader = Importer::ImportShaderFromSPV(
        "../assets/shaders/textured_sphere/TexturedSphere.vert.spv", 
        AS::Shader::Stage::Vertex, 
        "main"
    );
    MFA_ASSERT(cpuVertexShader.isValid());
    auto gpuVertexShader = RF::CreateShader(cpuVertexShader);
    MFA_ASSERT(gpuVertexShader.valid());
    MFA_DEFER {
        RF::DestroyShader(gpuVertexShader);
        Importer::FreeShader(&cpuVertexShader);
    };
    
    // Fragment shader
    auto cpuFragmentShader = Importer::ImportShaderFromSPV(
        "../assets/shaders/textured_sphere/TexturedSphere.frag.spv",
        MFA::AssetSystem::Shader::Stage::Fragment,
        "main"
    );
    auto gpuFragmentShader = RF::CreateShader(cpuFragmentShader);
    MFA_ASSERT(cpuFragmentShader.isValid());
    MFA_ASSERT(gpuFragmentShader.valid());
    MFA_DEFER {
        RF::DestroyShader(gpuFragmentShader);
        Importer::FreeShader(&cpuFragmentShader);
    };

    std::vector<RB::GpuShader> shaders {gpuVertexShader, gpuFragmentShader};

    // TODO We should use gltf sampler info here
    mSamplerGroup = RF::CreateSampler();

    mLVBuffer = RF::CreateUniformBuffer(sizeof(LightViewBuffer), 1);

    createDescriptorSetLayout();

    createDrawPipeline(static_cast<MFA::U8>(shaders.size()), shaders.data());
    
    createDrawableObject();
}

void TexturedSphereScene::OnDraw(MFA::U32 delta_time, MFA::RenderFrontend::DrawPass & draw_pass) {
    RF::BindDrawPipeline(draw_pass, mDrawPipeline);

    {// Updating Transform buffer
        // Rotation
        // TODO Try sending Matrices directly
        MFA::Matrix4X4Float rotationMat {};
        MFA::Matrix4X4Float::assignRotationXYZ(
            rotationMat,
            MFA::Math::Deg2Rad(mModelRotation[0]),
            MFA::Math::Deg2Rad(mModelRotation[1]),
            MFA::Math::Deg2Rad(mModelRotation[2])
        );
        static_assert(sizeof(m_translate_data.rotation) == sizeof(rotationMat.cells));
        ::memcpy(m_translate_data.rotation, rotationMat.cells, sizeof(rotationMat.cells));
        // Position
        MFA::Matrix4X4Float transformationMat {};
        MFA::Matrix4X4Float::assignTransformation(
            transformationMat,
            mModelPosition[0],
            mModelPosition[1],
            mModelPosition[2]
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

        mDrawableObject.updateUniformBuffer(
            "transform", 
            MFA::CBlobAliasOf(m_translate_data)
        );
    }
    {// LightViewBuffer
        ::memcpy(m_lv_data.light_position, mLightPosition, sizeof(mLightPosition));
        static_assert(sizeof(mLightPosition) == sizeof(m_lv_data.light_position));

        ::memcpy(m_lv_data.light_color, mLightColor, sizeof(mLightColor));
        static_assert(sizeof(mLightColor) == sizeof(m_lv_data.light_color));

        RF::UpdateUniformBuffer(mLVBuffer.buffers[0], MFA::CBlobAliasOf(m_lv_data));
    }
    mDrawableObject.draw(draw_pass);
}

void TexturedSphereScene::OnUI(MFA::U32 delta_time, MFA::RenderFrontend::DrawPass & draw_pass) {
    ImGui::Begin("Object viewer");
    ImGui::SetNextItemWidth(300.0f);
    ImGui::SliderFloat("XDegree", &mModelRotation[0], -360.0f, 360.0f);
    ImGui::SetNextItemWidth(300.0f);
    ImGui::SliderFloat("YDegree", &mModelRotation[1], -360.0f, 360.0f);
    ImGui::SetNextItemWidth(300.0f);
    ImGui::SliderFloat("ZDegree", &mModelRotation[2], -360.0f, 360.0f);
    ImGui::SetNextItemWidth(300.0f);
    ImGui::SliderFloat("XDistance", &mModelPosition[0], -100.0f, 100.0f);
    ImGui::SetNextItemWidth(300.0f);
    ImGui::SliderFloat("YDistance", &mModelPosition[1], -100.0f, 100.0f);
    ImGui::SetNextItemWidth(300.0f);
    ImGui::SliderFloat("ZDistance", &mModelPosition[2], -100.0f, 100.0f);
    ImGui::End();

    ImGui::Begin("Light");
    ImGui::SetNextItemWidth(300.0f);
    ImGui::SliderFloat("PositionX", &mLightPosition[0], -200.0f, 200.0f);
    ImGui::SetNextItemWidth(300.0f);
    ImGui::SliderFloat("PositionY", &mLightPosition[1], -200.0f, 200.0f);
    ImGui::SetNextItemWidth(300.0f);
    ImGui::SliderFloat("PositionZ", &mLightPosition[2], -200.0f, 200.0f);
    ImGui::SetNextItemWidth(300.0f);
    ImGui::SliderFloat("ColorR", &mLightColor[0], 0.0f, 1.0f);
    ImGui::SetNextItemWidth(300.0f);
    ImGui::SliderFloat("ColorG", &mLightColor[1], 0.0f, 1.0f);
    ImGui::SetNextItemWidth(300.0f);
    ImGui::SliderFloat("ColorB", &mLightColor[2], 0.0f, 1.0f);
    ImGui::End();
}

void TexturedSphereScene::Shutdown() {
    destroyDrawableObject();
    RF::DestroyUniformBuffer(mLVBuffer);
    RF::DestroyDrawPipeline(mDrawPipeline);
    RF::DestroyDescriptorSetLayout(mDescriptorSetLayout);
    RF::DestroySampler(mSamplerGroup);
    RF::DestroyGpuModel(mGpuModel);
    Importer::FreeModel(&mGpuModel.model);
}

void TexturedSphereScene::createDrawableObject(){
     mDrawableObject = MFA::DrawableObject(
        mGpuModel,
        mDescriptorSetLayout
    );
    // TODO AO map for emission
    const auto * transformBuffer = mDrawableObject.createUniformBuffer("transform", sizeof(ModelTransformBuffer));
    MFA_ASSERT(transformBuffer != nullptr);
    
    auto const & textures = mDrawableObject.getModel()->textures;

    for(MFA::U8 i = 0; i < mDrawableObject.getDescriptorSetCount(); ++i) {// Updating descriptor sets
        auto * descriptor_set = mDrawableObject.getDescriptorSet(i);
        auto const & sub_mesh = model_header->sub_meshes[i];

        std::vector<VkWriteDescriptorSet> writeInfo {};

        // Transform
        VkDescriptorBufferInfo transformBufferInfo {
            .buffer = transformBuffer->buffers[0].buffer,
            .offset = 0,
            .range = transformBuffer->buffer_size
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
            .sampler = mSamplerGroup.sampler,          // TODO Each texture has it's own properties that may need it's own sampler (Not sure yet)
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
            .sampler = mSamplerGroup.sampler,          // TODO Each texture has it's own properties that may need it's own sampler (Not sure yet)
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
            .sampler = mSamplerGroup.sampler,          // TODO Each texture has it's own properties that may need it's own sampler (Not sure yet)
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
            .sampler = mSamplerGroup.sampler,
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
            .buffer = mLVBuffer.buffers[0].buffer,
            .offset = 0,
            .range = mLVBuffer.buffer_size
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
    mDrawableObject.deleteUniformBuffers();
}

void TexturedSphereScene::createDrawPipeline(MFA::U8 gpu_shader_count, MFA::RenderBackend::GpuShader * gpu_shaders){
    VkVertexInputBindingDescription const vertex_binding_description {
        .binding = 0,
        .stride = sizeof(AS::MeshVertex),
        .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
    };

    std::vector<VkVertexInputAttributeDescription> input_attribute_descriptions {};

    input_attribute_descriptions.emplace_back(VkVertexInputAttributeDescription {
        .location = 0,
        .binding = 0,
        .format = VK_FORMAT_R32G32B32_SFLOAT,
        .offset = offsetof(AS::MeshVertex, position),   
    });
    input_attribute_descriptions.emplace_back(VkVertexInputAttributeDescription {
        .location = 1,
        .binding = 0,
        .format = VK_FORMAT_R32G32_SFLOAT,
        .offset = offsetof(AS::MeshVertex, baseColorUV),   
    });
    input_attribute_descriptions.emplace_back(VkVertexInputAttributeDescription {
        .location = 2,
        .binding = 0,
        .format = VK_FORMAT_R32G32_SFLOAT,
        .offset = offsetof(AS::MeshVertex, metallicUV),   
    });
    input_attribute_descriptions.emplace_back(VkVertexInputAttributeDescription {
        .location = 3,
        .binding = 0,
        .format = VK_FORMAT_R32G32_SFLOAT,
        .offset = offsetof(AS::MeshVertex, roughnessUV),   
    });
    input_attribute_descriptions.emplace_back(VkVertexInputAttributeDescription {
        .location = 4,
        .binding = 0,
        .format = VK_FORMAT_R32G32_SFLOAT,
        .offset = offsetof(AS::MeshVertex, normalMapUV),   
    });
    input_attribute_descriptions.emplace_back(VkVertexInputAttributeDescription {
        .location = 5,
        .binding = 0,
        .format = VK_FORMAT_R32G32B32_SFLOAT,
        .offset = offsetof(AS::MeshVertex, normalValue),   
    });
    input_attribute_descriptions.emplace_back(VkVertexInputAttributeDescription {
        .location = 6,
        .binding = 0,
        .format = VK_FORMAT_R32G32B32A32_SFLOAT,
        .offset = offsetof(AS::MeshVertex, tangentValue),   
    });

    mDrawPipeline = RF::CreateDrawPipeline(
        gpu_shader_count, 
        gpu_shaders,
        mDescriptorSetLayout,
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
    mDescriptorSetLayout = RF::CreateDescriptorSetLayout(
        static_cast<MFA::U8>(bindings.size()),
        bindings.data()
    );
}

void TexturedSphereScene::createGpuModel() {
    auto cpuModel = MFA::ShapeGenerator::Sphere();

    auto const importTextureForModel = [&cpuModel](char const * address) -> MFA::I16 {
        auto const texture = Importer::ImportUncompressedImage(
            address, 
            Importer::ImportTextureOptions {.generate_mipmaps = false}
        );
        MFA_ASSERT(texture.isValid());
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
    
    auto const & mesh = cpuModel.mesh;
    MFA_ASSERT(mesh.isValid());
    for (MFA::U32 i = 0; i < mesh.getSubMeshCount(); ++i) {
        auto subMesh = mesh.getSubMeshByIndex(i);
        for (auto & primitive : subMesh.primitives) {
            // Texture index
            primitive.baseColorTextureIndex = baseColorIndex;
            primitive.normalTextureIndex = normalIndex;
            primitive.metallicTextureIndex = metallicIndex;
            primitive.roughnessTextureIndex = roughnessIndex;
            // SubMesh features
            primitive.hasBaseColorTexture = true;
            primitive.hasNormalTexture = true;
            primitive.hasMetallicTexture = true;
            primitive.hasRoughnessTexture = true;
        }
    }

    mGpuModel = RF::CreateGpuModel(cpuModel);
}
