#include "GLTFMeshViewerScene.hpp"

#include "engine/RenderFrontend.hpp"
#include "engine/BedrockMatrix.hpp"
#include "libs/imgui/imgui.h"
#include "engine/DrawableObject.hpp"
#include "tools/Importer.hpp"
#include "tools/ShapeGenerator.hpp"

namespace RF = MFA::RenderFrontend;
namespace RB = MFA::RenderBackend;
namespace Importer = MFA::Importer;

void GLTFMeshViewerScene::Init() {
    {// Error texture
        auto cpu_texture = Importer::CreateErrorTexture();
        mErrorTexture = RF::CreateTexture(cpu_texture);
    }
    {// Models
        // TODO Start from here, Replace faulty sponza scene with correct one
        mModelsRenderData.emplace_back(ModelRenderRequiredData {
            .gpuModel {},
            .displayName {"SponzaScene"},
            .address {"../assets/models/sponza/sponza.gltf"},
            //.address {"../assets/models/sponza-gltf-pbr/sponza.glb"},
            .drawableObjectId {},
            .initialParams {
                .model {
                    .rotationEulerAngle {180.0f, -90.0f, 0.0f},
                    .translate {0.4f, 2.0f, -6.0f},
                },
                .light {
                    .translateMin {-50.0f, -50.0f, -50.0f},
                    .translateMax {50.0f, 50.0f, 50.0f}
                }
            }
        });
        mModelsRenderData.emplace_back(ModelRenderRequiredData {
            .gpuModel {},
            .displayName {"Car"},
            .address {"../assets/models/free_zuk_3d_model/scene.gltf"},
            .drawableObjectId {},
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
            .drawableObjectId {},
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
            .drawableObjectId {},
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
            .drawableObjectId {},
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
            .drawableObjectId {},
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
            .drawableObjectId {},
        });
        mModelsRenderData.emplace_back(ModelRenderRequiredData {
            .gpuModel {},
            .displayName {"Flight helmet"},
            .address {"../assets/models/FlightHelmet/glTF/FlightHelmet.gltf"},
            .drawableObjectId {},
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
            .drawableObjectId {},
            .initialParams {
            }
        });
    }

    // TODO We should use gltf sampler info here
    mSamplerGroup = RF::CreateSampler();
    
    mPbrPipeline.init(&mSamplerGroup, &mErrorTexture);

    mPointLightPipeline.init();

    // Updating perspective mat once for entire application
    // Perspective
    updateProjectionBuffer();

    {// Point light
        auto cpuModel = MFA::ShapeGenerator::Sphere(0.1f);

        for (uint32_t i = 0; i < cpuModel.mesh.getSubMeshCount(); ++i) {
            for (auto & primitive : cpuModel.mesh.getSubMeshByIndex(i).primitives) {
                primitive.baseColorFactor[0] = 1.0f;
                primitive.baseColorFactor[1] = 1.0f;
                primitive.baseColorFactor[2] = 1.0f;
            }
        }

        mGpuPointLight = RF::CreateGpuModel(cpuModel);
        mPointLightObjectId = mPointLightPipeline.addGpuModel(mGpuPointLight);
    }
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
        ::memcpy(mLightPosition, selectedModel.initialParams.light.position, sizeof(mLightPosition));
        static_assert(sizeof(mLightPosition) == sizeof(selectedModel.initialParams.light.position));

        ::memcpy(mLightColor, selectedModel.initialParams.light.color, sizeof(mLightColor));
        static_assert(sizeof(mLightColor) == sizeof(selectedModel.initialParams.light.color));

        ::memcpy(mLightTranslateMin, selectedModel.initialParams.light.translateMin, sizeof(mLightTranslateMin));
        static_assert(sizeof(mLightTranslateMin) == sizeof(selectedModel.initialParams.light.translateMin));

        ::memcpy(mLightTranslateMax, selectedModel.initialParams.light.translateMax, sizeof(mLightTranslateMax));
        static_assert(sizeof(mLightTranslateMax) == sizeof(selectedModel.initialParams.light.translateMin));

    }

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
        
        static_assert(sizeof(mPbrViewProjectionData.view) == sizeof(transformMat.cells));
        ::memcpy(mPbrViewProjectionData.view, transformMat.cells, sizeof(transformMat.cells));

        mPbrPipeline.updateViewProjectionBuffer(
            selectedModel.drawableObjectId,
            mPbrViewProjectionData
        );
    }
    {// LightViewBuffer
        ::memcpy(mLightViewData.lightPosition, mLightPosition, sizeof(mLightPosition));
        static_assert(sizeof(mLightPosition) == sizeof(mLightViewData.lightPosition));

        ::memcpy(mLightViewData.lightColor, mLightColor, sizeof(mLightColor));
        static_assert(sizeof(mLightColor) == sizeof(mLightViewData.lightColor));

        mPbrPipeline.updateLightViewBuffer(mLightViewData);

        // Position
        MFA::Matrix4X4Float translationMat {};
        MFA::Matrix4X4Float::AssignTranslation(
            translationMat,
            mLightPosition[0],
            mLightPosition[1],
            mLightPosition[2]
        );

        MFA::Matrix4X4Float transformMat {};
        MFA::Matrix4X4Float::identity(transformMat);
        transformMat.multiply(translationMat);
        
        static_assert(sizeof(mPointLightViewProjectionData.view) == sizeof(transformMat.cells));
        ::memcpy(mPointLightViewProjectionData.view, transformMat.cells, sizeof(transformMat.cells));

        mPointLightPipeline.updateViewProjectionBuffer(mPointLightObjectId, mPointLightViewProjectionData);

        MFA::PointLightPipeline::PrimitiveInfo const lightPrimitiveInfo {
            .baseColorFactor {
                mLightViewData.lightColor[0],
                mLightViewData.lightColor[1],
                mLightViewData.lightColor[2],
                1.0f
            }
        };
        mPointLightPipeline.updatePrimitiveInfo(mPointLightObjectId, lightPrimitiveInfo);
    }
    // TODO Pipeline should be able to share buffers such as projection buffer to enable us to update them once
    mPbrPipeline.render(draw_pass, 1, &selectedModel.drawableObjectId);
    if (mIsLightVisible) {
        mPointLightPipeline.render(draw_pass, 1, &mPointLightObjectId);
    }
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
    ImGui::Checkbox("Visible", &mIsLightVisible);
    ImGui::SetNextItemWidth(ItemWidth);
    ImGui::SliderFloat("PositionX", &mLightPosition[0], mLightTranslateMin[0], mLightTranslateMax[0]);
    ImGui::SetNextItemWidth(ItemWidth);
    ImGui::SliderFloat("PositionY", &mLightPosition[1], mLightTranslateMin[1], mLightTranslateMax[1]);
    ImGui::SetNextItemWidth(ItemWidth);
    ImGui::SliderFloat("PositionZ", &mLightPosition[2], mLightTranslateMin[2], mLightTranslateMax[2]);
    ImGui::SetNextItemWidth(ItemWidth);
    ImGui::SliderFloat("ColorR", &mLightColor[0], 0.0f, 400.0f);
    ImGui::SetNextItemWidth(ItemWidth);
    ImGui::SliderFloat("ColorG", &mLightColor[1], 0.0f, 400.0f);
    ImGui::SetNextItemWidth(ItemWidth);
    ImGui::SliderFloat("ColorB", &mLightColor[2], 0.0f, 400.0f);
    ImGui::End();
}

void GLTFMeshViewerScene::Shutdown() {
    mPbrPipeline.shutdown();
    mPointLightPipeline.shutdown();
    RF::DestroySampler(mSamplerGroup);
    destroyModels();
    RF::DestroyTexture(mErrorTexture);
    Importer::FreeTexture(mErrorTexture.cpu_texture());
}

void GLTFMeshViewerScene::OnResize() {
    updateProjectionBuffer();
}

void GLTFMeshViewerScene::createModel(ModelRenderRequiredData & renderRequiredData) {
    auto cpuModel = Importer::ImportGLTF(renderRequiredData.address.c_str());
    renderRequiredData.gpuModel = RF::CreateGpuModel(cpuModel);
    renderRequiredData.drawableObjectId = mPbrPipeline.addGpuModel(renderRequiredData.gpuModel);
    renderRequiredData.isLoaded = true;
}

void GLTFMeshViewerScene::destroyModels() {
    // Gpu model
    if (mModelsRenderData.empty() == false) {
        for (auto & group : mModelsRenderData) {
            if (group.isLoaded) {
                RF::DestroyGpuModel(group.gpuModel);
                Importer::FreeModel(&group.gpuModel.model);
                group.isLoaded = false;
            }
        }
    }

    // Point light
    MFA_ASSERT(mGpuPointLight.valid);
    RF::DestroyGpuModel(mGpuPointLight);
    Importer::FreeModel(&mGpuPointLight.model);
}

void GLTFMeshViewerScene::updateProjectionBuffer() {
    int32_t width; int32_t height;
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

    // PBR
    static_assert(sizeof(mPbrViewProjectionData.projection) == sizeof(perspectiveMat.cells));
    ::memcpy(mPbrViewProjectionData.projection, perspectiveMat.cells, sizeof(perspectiveMat.cells));

    // PointLight
    static_assert(sizeof(mPointLightViewProjectionData.projection) == sizeof(perspectiveMat.cells));
    ::memcpy(mPointLightViewProjectionData.projection, perspectiveMat.cells, sizeof(perspectiveMat.cells));
}
