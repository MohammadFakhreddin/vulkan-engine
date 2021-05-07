#include "GLTFMeshViewerScene.hpp"

#include "engine/RenderFrontend.hpp"
#include "engine/BedrockMatrix.hpp"
#include "libs/imgui/imgui.h"
#include "engine/DrawableObject.hpp"
#include "tools/Importer.hpp"
#include "engine/BedrockFileSystem.hpp"

namespace RF = MFA::RenderFrontend;
namespace RB = MFA::RenderBackend;
namespace AS = MFA::AssetSystem;
namespace Importer = MFA::Importer;

void GLTFMeshViewerScene::Init() {
    {// Error texture
        auto cpu_texture = Importer::CreateErrorTexture();
        m_error_texture = RF::CreateTexture(cpu_texture);
    }
    {// Models
        // TODO Start from here, Replace faulty sponza scene with correct one
        mModelsRenderData.emplace_back(ModelRenderRequiredData {
            .gpuModel {},
            .displayName {"SponzaScene"},
            .address {"../assets/models/sponza/sponza.gltf"},
            //.address {"../assets/models/sponza-gltf-pbr/sponza.glb"},
            .drawableObject {},
            .initialParams {
                .model {
                    .rotationEulerAngle {180.0f, -90.0f, 0.0f},
                    .translate {0.4f, 2.0f, -6.0f},
                }
            }
        });
        mModelsRenderData.emplace_back(ModelRenderRequiredData {
            .gpuModel {},
            .displayName {"Car"},
            .address {"../assets/models/free_zuk_3d_model/scene.gltf"},
            .drawableObject {},
            .initialParams {
                .model {
                    .rotationEulerAngle {8.0f, 158.0f, 0.0f},
                    .translate {0.0f, 0.0f, -4.0f}
                }
            }
        });
        mModelsRenderData.emplace_back(ModelRenderRequiredData {
            .gpuModel {},
            .displayName {"Gunship"},
            .address {"../assets/models/gunship/scene.gltf"},
            .drawableObject {},
            .initialParams {
                .model {
                    .rotationEulerAngle {37.0f, 155.0f, 0.0f},
                    .scale = 0.012f,
                    .translate {0.0f, 0.0f, -44.0f}
                },
                .light {
                    .position {-2.0f, -2.0f, -29.0f}
                }
            }
        });
        mModelsRenderData.emplace_back(ModelRenderRequiredData {
            .gpuModel {},
            .displayName {"War-craft soldier"},
            .address {"../assets/models/warcraft_3_alliance_footmanfanmade/scene.gltf"},
            .drawableObject {},
            .initialParams {
                .model {
                    .rotationEulerAngle {-1.0f, 127.0f, -5.0f},
                    .translate {0.0f, 0.0f, -7.0f}
                },
                .light {
                    .position {0.0f, -2.0f, -2.0f}
                }
            }
        });
        mModelsRenderData.emplace_back(ModelRenderRequiredData {
            .gpuModel {},
            .displayName {"Cyberpunk lady"},
            .address {"../assets/models/female_full-body_cyberpunk_themed_avatar/scene.gltf"},
            .drawableObject {},
            .initialParams {
                .model {
                    .rotationEulerAngle {4.0f, 152.0f, 0.0f},
                    .translate {0.0f, 0.5f, -7.0f}
                }
            }
        });
        mModelsRenderData.emplace_back(ModelRenderRequiredData {
            .gpuModel {},
            .displayName {"Mandalorian"},
            .address {"../assets/models/fortnite_the_mandalorianbaby_yoda/scene.gltf"},
            .drawableObject {},
            .initialParams {
                .model {
                    .rotationEulerAngle {302.0f, -2.0f, 0.0f},
                    .scale = 0.187f,
                    .translate {-2.0f, 0.0f, -190.0f}
                },
                .light {
                    .position {0.0f, 0.0f, -135.0f}
                }
            }
        });
        mModelsRenderData.emplace_back(ModelRenderRequiredData {
            .gpuModel {},
            .displayName {"Mandalorian2"},
            .address {"../assets/models/mandalorian__the_fortnite_season_6_skin_updated/scene.gltf"},
            .drawableObject {},
        });
        mModelsRenderData.emplace_back(ModelRenderRequiredData {
            .gpuModel {},
            .displayName {"Flight helmet"},
            .address {"../assets/models/FlightHelmet/glTF/FlightHelmet.gltf"},
            .drawableObject {},
            .initialParams {
                .model {
                    .rotationEulerAngle {197.0f, 186.0f, -1.5f},
                    .translate {0.0f, 0.0f, -4.0f},
                }
            }
        });
        
        mModelsRenderData.emplace_back(ModelRenderRequiredData {
            .gpuModel {},
            .displayName {"Warhammer tank"},
            .address {"../assets/models/warhammer_40k_predator_dark_millennium/scene.gltf"},
            .drawableObject {},
            .initialParams {
            }
        });
    }
    
    // Vertex shader
    auto cpu_vertex_shader = Importer::ImportShaderFromSPV(
        "../assets/shaders/gltf_mesh_viewer/GLTFMeshViewer.vert.spv", 
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
    auto cpu_fragment_shader = Importer::ImportShaderFromSPV(
        "../assets/shaders/gltf_mesh_viewer/GLTFMeshViewer.frag.spv",
        MFA::AssetSystem::Shader::Stage::Fragment,
        "main"
    );
    auto gpu_fragment_shader = RF::CreateShader(cpu_fragment_shader);
    MFA_ASSERT(cpu_fragment_shader.isValid());
    MFA_ASSERT(gpu_fragment_shader.valid());
    MFA_DEFER {
        RF::DestroyShader(gpu_fragment_shader);
        Importer::FreeShader(&cpu_fragment_shader);
    };

    std::vector<RB::GpuShader> shaders {gpu_vertex_shader, gpu_fragment_shader};

    // TODO We should use gltf sampler info here
    m_sampler_group = RF::CreateSampler();

    m_lv_buffer = RF::CreateUniformBuffer(sizeof(LightViewBuffer), 1);

    createDescriptorSetLayout();

    createDrawPipeline(static_cast<uint8_t>(shaders.size()), shaders.data());

    // Updating perspective mat once for entire application
    // Perspective
    updateProjectionBuffer();

}

void GLTFMeshViewerScene::OnDraw(uint32_t const delta_time, RF::DrawPass & draw_pass) {
    MFA_ASSERT(mSelectedModelIndex >=0 && mSelectedModelIndex < mModelsRenderData.size());
    auto & selectedModel = mModelsRenderData[mSelectedModelIndex];
    if (selectedModel.isLoaded == false) {
        createModel(selectedModel);
    }
    if (mPreviousModelSelectedIndex != mSelectedModelIndex) {
        mPreviousModelSelectedIndex = mSelectedModelIndex;
        // Model
        ::memcpy(m_model_rotation, selectedModel.initialParams.model.rotationEulerAngle, sizeof(m_model_rotation));
        static_assert(sizeof(m_model_rotation) == sizeof(selectedModel.initialParams.model.rotationEulerAngle));

        ::memcpy(m_model_position, selectedModel.initialParams.model.translate, sizeof(m_model_position));
        static_assert(sizeof(m_model_position) == sizeof(selectedModel.initialParams.model.translate));

        m_model_scale = selectedModel.initialParams.model.scale;

        ::memcpy(mModelTranslateMin, selectedModel.initialParams.model.translateMin, sizeof(mModelTranslateMin));
        static_assert(sizeof(mModelTranslateMin) == sizeof(selectedModel.initialParams.model.translateMin));

        ::memcpy(mModelTranslateMax, selectedModel.initialParams.model.translateMax, sizeof(mModelTranslateMax));
        static_assert(sizeof(mModelTranslateMax) == sizeof(selectedModel.initialParams.model.translateMin));

        // Light
        ::memcpy(m_light_position, selectedModel.initialParams.light.position, sizeof(m_light_position));
        static_assert(sizeof(m_light_position) == sizeof(selectedModel.initialParams.light.position));

        ::memcpy(m_light_color, selectedModel.initialParams.light.color, sizeof(m_light_color));
        static_assert(sizeof(m_light_color) == sizeof(selectedModel.initialParams.light.color));

        ::memcpy(mLightTranslateMin, selectedModel.initialParams.light.translateMin, sizeof(mLightTranslateMin));
        static_assert(sizeof(mLightTranslateMin) == sizeof(selectedModel.initialParams.light.translateMin));

        ::memcpy(mLightTranslateMax, selectedModel.initialParams.light.translateMax, sizeof(mLightTranslateMax));
        static_assert(sizeof(mLightTranslateMax) == sizeof(selectedModel.initialParams.light.translateMin));

    }

    RF::BindDrawPipeline(draw_pass, m_draw_pipeline);

    {// Updating Transform buffer
        // Rotation
        MFA::Matrix4X4Float rotationMat {};
        MFA::Matrix4X4Float::AssignRotation(
            rotationMat,
            MFA::Math::Deg2Rad(m_model_rotation[0]),
            MFA::Math::Deg2Rad(m_model_rotation[1]),
            MFA::Math::Deg2Rad(m_model_rotation[2])
        );

        // Scale
        MFA::Matrix4X4Float scaleMat {};
        MFA::Matrix4X4Float::AssignScale(scaleMat, m_model_scale);

        // Position
        MFA::Matrix4X4Float translationMat {};
        MFA::Matrix4X4Float::AssignTranslation(
            translationMat,
            m_model_position[0],
            m_model_position[1],
            m_model_position[2]
        );

        MFA::Matrix4X4Float transformMat {};
        MFA::Matrix4X4Float::identity(transformMat);
        transformMat.multiply(translationMat);
        transformMat.multiply(rotationMat);
        transformMat.multiply(scaleMat);
        
        static_assert(sizeof(mTransformData.view) == sizeof(transformMat.cells));
        ::memcpy(mTransformData.view, transformMat.cells, sizeof(transformMat.cells));

        selectedModel.drawableObject.updateUniformBuffer(
            "transform", 
            MFA::CBlobAliasOf(mTransformData)
        );
    }
    {// LightViewBuffer
        ::memcpy(m_lv_data.light_position, m_light_position, sizeof(m_light_position));
        static_assert(sizeof(m_light_position) == sizeof(m_lv_data.light_position));

        ::memcpy(m_lv_data.light_color, m_light_color, sizeof(m_light_color));
        static_assert(sizeof(m_light_color) == sizeof(m_lv_data.light_color));

        RF::UpdateUniformBuffer(m_lv_buffer.buffers[0], MFA::CBlobAliasOf(m_lv_data));
    }
    selectedModel.drawableObject.draw(draw_pass);
}

void GLTFMeshViewerScene::OnUI(uint32_t const delta_time, MFA::RenderFrontend::DrawPass & draw_pass) {
    static constexpr float ItemWidth = 500;
    ImGui::Begin("Object viewer");
    ImGui::SetNextItemWidth(ItemWidth);
    // TODO Bad for performance, Find a better name
    std::vector<char const *> modelNames {};
    if(false == mModelsRenderData.empty()) {
        for(auto const & renderData : mModelsRenderData) {
            modelNames.emplace_back(renderData.displayName.c_str());
        }
    }
    ImGui::Combo(
        "Object selector",
        &mSelectedModelIndex,
        modelNames.data(), 
        static_cast<int32_t>(modelNames.size())
    );
    ImGui::SetNextItemWidth(ItemWidth);
    ImGui::SliderFloat("XDegree", &m_model_rotation[0], -360.0f, 360.0f);
    ImGui::SetNextItemWidth(ItemWidth);
    ImGui::SliderFloat("YDegree", &m_model_rotation[1], -360.0f, 360.0f);
    ImGui::SetNextItemWidth(ItemWidth);
    ImGui::SliderFloat("ZDegree", &m_model_rotation[2], -360.0f, 360.0f);
    ImGui::SetNextItemWidth(ItemWidth);
    ImGui::SliderFloat("Scale", &m_model_scale, 0.0f, 1.0f);
    ImGui::SetNextItemWidth(ItemWidth);
    ImGui::SliderFloat("XDistance", &m_model_position[0], mModelTranslateMin[0], mModelTranslateMax[0]);
    ImGui::SetNextItemWidth(ItemWidth);
    ImGui::SliderFloat("YDistance", &m_model_position[1], mModelTranslateMin[1], mModelTranslateMax[1]);
    ImGui::SetNextItemWidth(ItemWidth);
    ImGui::SliderFloat("ZDistance", &m_model_position[2], mModelTranslateMin[2], mModelTranslateMax[2]);
    ImGui::End();

    ImGui::Begin("Light");
    ImGui::SetNextItemWidth(ItemWidth);
    ImGui::SliderFloat("PositionX", &m_light_position[0], mLightTranslateMin[0], mLightTranslateMax[0]);
    ImGui::SetNextItemWidth(ItemWidth);
    ImGui::SliderFloat("PositionY", &m_light_position[1], mLightTranslateMin[1], mLightTranslateMax[1]);
    ImGui::SetNextItemWidth(ItemWidth);
    ImGui::SliderFloat("PositionZ", &m_light_position[2], mLightTranslateMin[2], mLightTranslateMax[2]);
    ImGui::SetNextItemWidth(ItemWidth);
    ImGui::SliderFloat("ColorR", &m_light_color[0], 0.0f, 400.0f);
    ImGui::SetNextItemWidth(ItemWidth);
    ImGui::SliderFloat("ColorG", &m_light_color[1], 0.0f, 400.0f);
    ImGui::SetNextItemWidth(ItemWidth);
    ImGui::SliderFloat("ColorB", &m_light_color[2], 0.0f, 400.0f);
    ImGui::End();
}

void GLTFMeshViewerScene::Shutdown() {
    RF::DestroyUniformBuffer(m_lv_buffer);
    RF::DestroyDrawPipeline(m_draw_pipeline);
    RF::DestroyDescriptorSetLayout(m_descriptor_set_layout);
    RF::DestroySampler(m_sampler_group);
    destroyModels();
    RF::DestroyTexture(m_error_texture);
    Importer::FreeTexture(m_error_texture.cpu_texture());
}

void GLTFMeshViewerScene::OnResize() {
    updateProjectionBuffer();
}

void GLTFMeshViewerScene::createModel(ModelRenderRequiredData & renderRequiredData) {
    auto cpuModel = Importer::ImportGLTF(renderRequiredData.address.c_str());

    renderRequiredData.gpuModel = RF::CreateGpuModel(cpuModel);

    renderRequiredData.drawableObject = MFA::DrawableObject(
        renderRequiredData.gpuModel,
        m_descriptor_set_layout
    );

    auto & drawableObject = renderRequiredData.drawableObject;

    const auto * primitiveInfoBuffer = drawableObject.createMultipleUniformBuffer(
        "primitiveInfo", 
        sizeof(PrimitiveInfo), 
        drawableObject.getDescriptorSetCount()
    );
    MFA_ASSERT(primitiveInfoBuffer != nullptr);

    const auto * modelTransformBuffer = drawableObject.createUniformBuffer("transform", sizeof(ModelTransformBuffer));
    MFA_ASSERT(modelTransformBuffer != nullptr);

    const auto & nodeTransformBuffer = drawableObject.getNodeTransformBuffer();

    auto const & textures = drawableObject.getModel()->textures;

    for (uint32_t nodeIndex = 0; nodeIndex < cpuModel.mesh.getNodesCount(); ++nodeIndex) {// Updating descriptor sets
        auto const & node = cpuModel.mesh.getNodeByIndex(nodeIndex);
        if (node.hasSubMesh()) {
            auto const & subMesh = cpuModel.mesh.getSubMeshByIndex(node.subMeshIndex);
            if (subMesh.primitives.empty() == false) {
                for (auto const & primitive : subMesh.primitives) {
                    MFA_ASSERT(primitive.uniqueId >= 0);
                    auto * descriptorSet = drawableObject.getDescriptorSetByPrimitiveUniqueId(primitive.uniqueId);
                    MFA_ASSERT(descriptorSet != nullptr);

                    std::vector<VkWriteDescriptorSet> writeInfo {};

                    // ModelTransform
                    VkDescriptorBufferInfo modelTransformBufferInfo {
                        .buffer = modelTransformBuffer->buffers[0].buffer,
                        .offset = 0,
                        .range = modelTransformBuffer->bufferSize
                    };
                    writeInfo.emplace_back(VkWriteDescriptorSet {
                        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                        .dstSet = descriptorSet,
                        .dstBinding = static_cast<uint32_t>(writeInfo.size()),
                        .dstArrayElement = 0,
                        .descriptorCount = 1,
                        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                        .pBufferInfo = &modelTransformBufferInfo,
                    });

                    //NodeTransform
                    VkDescriptorBufferInfo nodeTransformBufferInfo {
                        .buffer = nodeTransformBuffer.buffers[node.subMeshIndex].buffer,
                        .offset = 0,
                        .range = nodeTransformBuffer.bufferSize
                    };
                    writeInfo.emplace_back(VkWriteDescriptorSet {
                        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                        .dstSet = descriptorSet,
                        .dstBinding = static_cast<uint32_t>(writeInfo.size()),
                        .dstArrayElement = 0,
                        .descriptorCount = 1,
                        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                        .pBufferInfo = &nodeTransformBufferInfo,
                    });

                    // SubMeshInfo
                    VkDescriptorBufferInfo primitiveBufferInfo {
                        .buffer = primitiveInfoBuffer->buffers[primitive.uniqueId].buffer,
                        .offset = 0,
                        .range = primitiveInfoBuffer->bufferSize
                    };
                    writeInfo.emplace_back(VkWriteDescriptorSet {
                        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                        .dstSet = descriptorSet,
                        .dstBinding = static_cast<uint32_t>(writeInfo.size()),
                        .dstArrayElement = 0,
                        .descriptorCount = 1,
                        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                        .pBufferInfo = &primitiveBufferInfo,
                    });
                    // Update primitive buffer information
                    PrimitiveInfo info {
                        .baseColorFactor {},
                        .emissiveFactor {},
                        .placeholder0 {},
                        .hasBaseColorTexture = primitive.hasBaseColorTexture ? 1 : 0,
                        .metallicFactor = primitive.metallicFactor,
                        .roughnessFactor = primitive.roughnessFactor,
                        .hasMixedMetallicRoughnessOcclusionTexture = primitive.hasMixedMetallicRoughnessOcclusionTexture ? 1 : 0,
                        .hasNormalTexture = primitive.hasNormalTexture ? 1 : 0,
                        .hasEmissiveTexture = primitive.hasEmissiveTexture ? 1 : 0,
                    };
                    ::memcpy(info.baseColorFactor, primitive.baseColorFactor, sizeof(info.baseColorFactor));
                    static_assert(sizeof(info.baseColorFactor) == sizeof(primitive.baseColorFactor));
                    ::memcpy(info.emissiveFactor, primitive.emissiveFactor, sizeof(info.emissiveFactor));
                    static_assert(sizeof(info.emissiveFactor) == sizeof(primitive.emissiveFactor));
                    RF::UpdateUniformBuffer(
                        primitiveInfoBuffer->buffers[primitive.uniqueId], 
                        MFA::CBlobAliasOf(info)
                    );

                    // BaseColorTexture
                    VkDescriptorImageInfo baseColorImageInfo {
                        .sampler = m_sampler_group.sampler,          // TODO Each texture has it's own properties that may need it's own sampler (Not sure yet)
                        .imageView = primitive.hasBaseColorTexture
                            ? textures[primitive.baseColorTextureIndex].image_view()
                            : m_error_texture.image_view(),
                        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                    };
                    writeInfo.emplace_back(VkWriteDescriptorSet {
                        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                        .dstSet = descriptorSet,
                        .dstBinding = static_cast<uint32_t>(writeInfo.size()),
                        .dstArrayElement = 0,
                        .descriptorCount = 1,
                        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                        .pImageInfo = &baseColorImageInfo,
                    });

                    // Metallic/RoughnessTexture
                    VkDescriptorImageInfo metallicImageInfo {
                        .sampler = m_sampler_group.sampler,          // TODO Each texture has it's own properties that may need it's own sampler (Not sure yet)
                        .imageView = primitive.hasMixedMetallicRoughnessOcclusionTexture
                            ? textures[primitive.mixedMetallicRoughnessOcclusionTextureIndex].image_view()
                            : m_error_texture.image_view(),
                        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
                    };
                    writeInfo.emplace_back(VkWriteDescriptorSet {
                        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                        .dstSet = descriptorSet,
                        .dstBinding = static_cast<uint32_t>(writeInfo.size()),
                        .dstArrayElement = 0,
                        .descriptorCount = 1,
                        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                        .pImageInfo = &metallicImageInfo,
                    });

                    // NormalTexture  
                    VkDescriptorImageInfo normalImageInfo {
                        .sampler = m_sampler_group.sampler,
                        .imageView = primitive.hasNormalTexture
                            ? textures[primitive.normalTextureIndex].image_view()
                            : m_error_texture.image_view(),
                        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
                    };
                    writeInfo.emplace_back(VkWriteDescriptorSet {
                        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                        .dstSet = descriptorSet,
                        .dstBinding = static_cast<uint32_t>(writeInfo.size()),
                        .dstArrayElement = 0,
                        .descriptorCount = 1,
                        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                        .pImageInfo = &normalImageInfo,
                    });

                    // EmissiveTexture
                    VkDescriptorImageInfo emissiveImageInfo {
                        .sampler = m_sampler_group.sampler,
                        .imageView = primitive.hasEmissiveTexture
                            ? textures[primitive.emissiveTextureIndex].image_view()
                            : m_error_texture.image_view(),
                        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
                    };
                    writeInfo.emplace_back(VkWriteDescriptorSet {
                        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                        .dstSet = descriptorSet,
                        .dstBinding = static_cast<uint32_t>(writeInfo.size()),
                        .dstArrayElement = 0,
                        .descriptorCount = 1,
                        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                        .pImageInfo = &emissiveImageInfo,
                    });

                    // LightViewBuffer
                    VkDescriptorBufferInfo light_view_buffer_info {
                        .buffer = m_lv_buffer.buffers[0].buffer,
                        .offset = 0,
                        .range = m_lv_buffer.bufferSize
                    };
                    writeInfo.emplace_back(VkWriteDescriptorSet {
                        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                        .dstSet = descriptorSet,
                        .dstBinding = static_cast<uint32_t>(writeInfo.size()),
                        .dstArrayElement = 0,
                        .descriptorCount = 1,
                        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                        .pBufferInfo = &light_view_buffer_info,
                    });
                    
                    RF::UpdateDescriptorSets(
                        static_cast<uint8_t>(writeInfo.size()),
                        writeInfo.data()
                    );
                }
            }
        }
    }

    renderRequiredData.isLoaded = true;
}

void GLTFMeshViewerScene::destroyModels() {
    if (mModelsRenderData.empty() == false) {
        for (auto & group : mModelsRenderData) {
            if (group.isLoaded) {
                RF::DestroyGpuModel(group.gpuModel);
                Importer::FreeModel(&group.gpuModel.model);
                group.drawableObject.deleteUniformBuffers();
                group.isLoaded = false;
            }
        }
    }
 
}

void GLTFMeshViewerScene::createDrawPipeline(uint8_t const gpu_shader_count, MFA::RenderBackend::GpuShader * gpu_shaders) {

    VkVertexInputBindingDescription const vertex_binding_description {
        .binding = 0,
        .stride = sizeof(AS::MeshVertex),
        .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
    };

    std::vector<VkVertexInputAttributeDescription> input_attribute_descriptions {};
    // Position
    input_attribute_descriptions.emplace_back(VkVertexInputAttributeDescription {
        .location = static_cast<uint32_t>(input_attribute_descriptions.size()),
        .binding = 0,
        .format = VK_FORMAT_R32G32B32_SFLOAT,
        .offset = offsetof(AS::MeshVertex, position),   
    });
    // BaseColor
    input_attribute_descriptions.emplace_back(VkVertexInputAttributeDescription {
        .location = static_cast<uint32_t>(input_attribute_descriptions.size()),
        .binding = 0,
        .format = VK_FORMAT_R32G32_SFLOAT,
        .offset = offsetof(AS::MeshVertex, baseColorUV),   
    });
    // Metallic/Roughness
    input_attribute_descriptions.emplace_back(VkVertexInputAttributeDescription {
        .location = static_cast<uint32_t>(input_attribute_descriptions.size()),
        .binding = 0,
        .format = VK_FORMAT_R32G32_SFLOAT,
        .offset = offsetof(AS::MeshVertex, metallicUV), // Metallic and roughness have same uv for gltf files  
    });
    // Normal
    input_attribute_descriptions.emplace_back(VkVertexInputAttributeDescription {
        .location = static_cast<uint32_t>(input_attribute_descriptions.size()),
        .binding = 0,
        .format = VK_FORMAT_R32G32_SFLOAT,
        .offset = offsetof(AS::MeshVertex, normalMapUV),   
    });
    input_attribute_descriptions.emplace_back(VkVertexInputAttributeDescription {
        .location = static_cast<uint32_t>(input_attribute_descriptions.size()),
        .binding = 0,
        .format = VK_FORMAT_R32G32B32A32_SFLOAT,
        .offset = offsetof(AS::MeshVertex, tangentValue),   
    });
    input_attribute_descriptions.emplace_back(VkVertexInputAttributeDescription {
        .location = static_cast<uint32_t>(input_attribute_descriptions.size()),
        .binding = 0,
        .format = VK_FORMAT_R32G32B32_SFLOAT,
        .offset = offsetof(AS::MeshVertex, normalValue),   
    });
    // Emissive
    input_attribute_descriptions.emplace_back(VkVertexInputAttributeDescription {
        .location = static_cast<uint32_t>(input_attribute_descriptions.size()),
        .binding = 0,
        .format = VK_FORMAT_R32G32_SFLOAT,
        .offset = offsetof(AS::MeshVertex, emissionUV)
    });
    m_draw_pipeline = RF::CreateBasicDrawPipeline(
        gpu_shader_count, 
        gpu_shaders,
        m_descriptor_set_layout,
        vertex_binding_description,
        static_cast<uint8_t>(input_attribute_descriptions.size()),
        input_attribute_descriptions.data()
    );

}

void GLTFMeshViewerScene::createDescriptorSetLayout() {
    std::vector<VkDescriptorSetLayoutBinding> bindings {};
    // ModelTransformation 
    bindings.emplace_back(VkDescriptorSetLayoutBinding {
        .binding = static_cast<uint32_t>(bindings.size()),
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
        .pImmutableSamplers = nullptr, // Optional
    });
    // NodeTransformation
    bindings.emplace_back(VkDescriptorSetLayoutBinding {
        .binding = static_cast<uint32_t>(bindings.size()),
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
        .pImmutableSamplers = nullptr, // Optional
    });
    // SubMeshInfo
    bindings.emplace_back(VkDescriptorSetLayoutBinding {
        .binding = static_cast<uint32_t>(bindings.size()),
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
        .pImmutableSamplers = nullptr, // Optional
    });
    // BaseColor
    bindings.emplace_back(VkDescriptorSetLayoutBinding {
        .binding = static_cast<uint32_t>(bindings.size()),
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
        .pImmutableSamplers = nullptr,
    });
    // Metallic/Roughness
    bindings.emplace_back(VkDescriptorSetLayoutBinding {
        .binding = static_cast<uint32_t>(bindings.size()),
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
        .pImmutableSamplers = nullptr,
    });
    // Normal
    bindings.emplace_back(VkDescriptorSetLayoutBinding {
        .binding = static_cast<uint32_t>(bindings.size()),
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
        .pImmutableSamplers = nullptr,
    });
    // Emissive
    bindings.emplace_back(VkDescriptorSetLayoutBinding {
        .binding = static_cast<uint32_t>(bindings.size()),
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
        .pImmutableSamplers = nullptr
    });
    // Light/View
    bindings.emplace_back(VkDescriptorSetLayoutBinding {
        .binding = static_cast<uint32_t>(bindings.size()),
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
        .pImmutableSamplers = nullptr, // Optional
    });
    m_descriptor_set_layout = RF::CreateDescriptorSetLayout(
        static_cast<uint8_t>(bindings.size()),
        bindings.data()
    );
}

void GLTFMeshViewerScene::updateProjectionBuffer() {
    int32_t width; int32_t height;
    RF::GetWindowSize(width, height);
    float const ratio = static_cast<float>(width) / static_cast<float>(height);
    MFA::Matrix4X4Float perspectiveMat {};
    MFA::Matrix4X4Float::PreparePerspectiveProjectionMatrix(
        perspectiveMat,
        ratio,
        80,
        Z_NEAR,
        Z_FAR
    );
    static_assert(sizeof(mTransformData.perspective) == sizeof(perspectiveMat.cells));
    ::memcpy(mTransformData.perspective, perspectiveMat.cells, sizeof(perspectiveMat.cells));
}
