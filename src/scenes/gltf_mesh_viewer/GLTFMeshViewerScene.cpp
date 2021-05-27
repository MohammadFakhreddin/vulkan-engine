#include "GLTFMeshViewerScene.hpp"

#include "engine/RenderFrontend.hpp"
#include "engine/BedrockMatrix.hpp"
#include "engine/DrawableObject.hpp"
#include "tools/Importer.hpp"
#include "tools/ShapeGenerator.hpp"

#include <glm/mat4x4.hpp>
#include <vec4.hpp>

namespace RF = MFA::RenderFrontend;
namespace Importer = MFA::Importer;
namespace UI = MFA::UISystem;

void GLTFMeshViewerScene::Init() {
    // TODO Out of pool memory on macos, We need to only keep a few of recent objects data active.
    {// Error texture
        auto cpu_texture = Importer::CreateErrorTexture();
        mErrorTexture = RF::CreateTexture(cpu_texture);
    }
    {// Models
        mModelsRenderData.emplace_back(ModelRenderRequiredData {
            .gpuModel {},
            .displayName {"CesiumMan"},
            .address {"../assets/models/CesiumMan/gltf/CesiumMan.gltf"},
            .drawableObjectId {},
            .initialParams {
                .model {
                    .rotationEulerAngle {0.0f, 0.0f, -180.0f},
                    .translate {0.0f, 0.0f, -7.0f}                    
                },
                .light {
                    .position {0.0f, -2.0f, -2.0f}
                },
                .camera {
                    .position {0.104f, 1.286f, 4.952f},
                    .eulerAngles {-12.0f, 3.0f, 0.0f}
                },
            }
        });
        mModelsRenderData.emplace_back(ModelRenderRequiredData {
            .gpuModel {},
            .displayName {"War-craft soldier"},
            .address {"../assets/models/warcraft_3_alliance_footmanfanmade/scene.gltf"},
            .drawableObjectId {},
            .initialParams {
                .model {
                    .rotationEulerAngle {0.0f, -5.926f, -180.0f},
                    .translate {0.0f, 0.9f, -2.0f}
                },
                .light {
                    .position {0.0f, -2.0f, 0.0f}
                }
            }
        });
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
                    .rotationEulerAngle {-19.0f, -32.0f, 177.0f},
                    .translate {0.0f, 0.0f, -2.0f}
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
                    .rotationEulerAngle {160.0f, 176.0f, 0.0f},
                    .scale = 0.008f,
                    .translate {0.0f, 0.0f, -33.745f}
                },
                .light {
                    .position {-2.0f, -2.0f, -29.0f}
                },
                .camera {
                    .position {2.5f, 5.3f, 21.8f},
                    .eulerAngles {-22.110f, -13.200f, 0.0f}
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
                    .rotationEulerAngle {-3.0f, 340.0f, 180.0f},
                    .translate {0.0f, 0.8f, -3.0f}
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
                    .rotationEulerAngle {180.0f, 180.0f, 0.0f},
                    .scale = 0.010f,
                    .translate {0.0f, -0.5f, -2.0f}
                },
                .light {
                    .position {0.0f, 0.0f, 0.0f}
                }
            }
        });
        mModelsRenderData.emplace_back(ModelRenderRequiredData {
            .gpuModel {},
            .displayName {"Mandalorian2"},
            .address {"../assets/models/mandalorian__the_fortnite_season_6_skin_updated/scene.gltf"},
            .drawableObjectId {},
            .initialParams {
                .model {
                    .rotationEulerAngle {180.0f, 180.0f, 0.0f},
                    .scale = 0.0005f,
                    .translate {0.0f, 4.5f, -10.0f}
                }
            }
        });
        mModelsRenderData.emplace_back(ModelRenderRequiredData {
            .gpuModel {},
            .displayName {"Flight helmet"},
            .address {"../assets/models/FlightHelmet/glTF/FlightHelmet.gltf"},
            .drawableObjectId {},
            .initialParams {
                .model {
                    .rotationEulerAngle {180.0f, 180.0f, 0.0f},
                    .translate {0.0f, 0.0f, -1.0f},
                },
                .light {
                    .position {0.0f, 0.0f, 2.469f}
                }
            }
        });
        
        mModelsRenderData.emplace_back(ModelRenderRequiredData {
            .gpuModel {},
            .displayName {"Warhammer tank"},
            .address {"../assets/models/warhammer_40k_predator_dark_millennium/scene.gltf"},
            .drawableObjectId {},
            .initialParams {
                .model {
                    .rotationEulerAngle {-45.0f, -45.0f, 180.0f},
                    .scale = 0.5f,
                    .translate {0.0f, 0.0f, -16.0f},
                },
                .light {
                    .position {1.5f, 0.0f, -8.0f}
                }
            }
        });
    }

    // TODO We should use gltf sampler info here
    mSamplerGroup = RF::CreateSampler();
    
    mPbrPipeline.init(&mSamplerGroup, &mErrorTexture);

    mPointLightPipeline.init();

    mCamera.init();

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

// TODO Why deltaTime is uint32_t ?
void GLTFMeshViewerScene::OnDraw(uint32_t const delta_time, RF::DrawPass & draw_pass) {
    MFA_ASSERT(mSelectedModelIndex >=0 && mSelectedModelIndex < mModelsRenderData.size());

    auto const fDeltaTime = static_cast<float>(delta_time);

    mCamera.onNewFrame(fDeltaTime);

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

        mCamera.forcePosition(selectedModel.initialParams.camera.position);
        mCamera.forceRotation(selectedModel.initialParams.camera.eulerAngles);
    }

    {// Updating Transform buffer
        // Rotation
        MFA::Matrix4X4Float rotationMat {};
        MFA::Matrix4X4Float::AssignRotation(
            rotationMat,
            m_model_rotation[0],
            m_model_rotation[1],
            m_model_rotation[2]
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
        MFA::Matrix4X4Float::Identity(transformMat);
        transformMat.multiply(translationMat);
        transformMat.multiply(rotationMat);
        transformMat.multiply(scaleMat);
        
        transformMat.copy(mPbrMVPData.model);

        mCamera.getTransform(mPbrMVPData.view);

        mPbrPipeline.updateViewProjectionBuffer(
            selectedModel.drawableObjectId,
            mPbrMVPData
        );
    }
    {// LightViewBuffer
        auto lightPosition = glm::vec4(
            mLightPosition[0], 
            mLightPosition[1], 
            mLightPosition[2], 
            1.0f
        );

        glm::mat4x4 transformMatrix;
        MFA::Matrix4X4Float::ConvertMatrixToGlm(mCamera.getTransform(), transformMatrix);

        lightPosition = transformMatrix * lightPosition;
        mLightViewData.lightPosition[0] = lightPosition[0];
        mLightViewData.lightPosition[1] = lightPosition[1];
        mLightViewData.lightPosition[2] = lightPosition[2];

        MFA::Copy<3>(mLightViewData.lightColor, mLightColor);
        
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
        MFA::Matrix4X4Float::Identity(transformMat);
        transformMat.multiply(translationMat);

        transformMat.copy(mPointLightMVPData.model);
        mCamera.getTransform(mPointLightMVPData.view);
        
        mPointLightPipeline.updateViewProjectionBuffer(mPointLightObjectId, mPointLightMVPData);

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
    mPbrPipeline.render(draw_pass, fDeltaTime, 1, &selectedModel.drawableObjectId);
    if (mIsLightVisible) {
        mPointLightPipeline.render(draw_pass, fDeltaTime, 1, &mPointLightObjectId);
    }
}

void GLTFMeshViewerScene::OnUI(uint32_t const delta_time, MFA::RenderFrontend::DrawPass & draw_pass) {
    static constexpr float ItemWidth = 500;
    UI::BeginWindow("Scene Subsystem");
    UI::Checkbox("Object viewer window", &mIsObjectViewerWindowVisible);
    UI::Checkbox("Light window", &mIsLightWindowVisible);
    UI::Checkbox("Camera window", &mIsCameraWindowVisible);
    UI::EndWindow();

    if (mIsObjectViewerWindowVisible) {
        UI::BeginWindow("Object viewer");
        UI::SetNextItemWidth(ItemWidth);
        // TODO Bad for performance, Find a better name
        std::vector<char const *> modelNames {};
        if(false == mModelsRenderData.empty()) {
            for(auto const & renderData : mModelsRenderData) {
                modelNames.emplace_back(renderData.displayName.c_str());
            }
        }
        UI::Combo(
            "Object selector",
            &mSelectedModelIndex,
            modelNames.data(), 
            static_cast<int32_t>(modelNames.size())
        );
        UI::SetNextItemWidth(ItemWidth);
        UI::SliderFloat("XDegree", &m_model_rotation[0], -360.0f, 360.0f);
        UI::SetNextItemWidth(ItemWidth);
        UI::SliderFloat("YDegree", &m_model_rotation[1], -360.0f, 360.0f);
        UI::SetNextItemWidth(ItemWidth);
        UI::SliderFloat("ZDegree", &m_model_rotation[2], -360.0f, 360.0f);
        UI::SetNextItemWidth(ItemWidth);
        UI::SliderFloat("Scale", &m_model_scale, 0.0f, 1.0f);
        UI::SetNextItemWidth(ItemWidth);
        UI::SliderFloat("XDistance", &m_model_position[0], mModelTranslateMin[0], mModelTranslateMax[0]);
        UI::SetNextItemWidth(ItemWidth);
        UI::SliderFloat("YDistance", &m_model_position[1], mModelTranslateMin[1], mModelTranslateMax[1]);
        UI::SetNextItemWidth(ItemWidth);
        UI::SliderFloat("ZDistance", &m_model_position[2], mModelTranslateMin[2], mModelTranslateMax[2]);
        UI::EndWindow();
    }

    if (mIsLightWindowVisible) {
        UI::BeginWindow("Light");
        UI::SetNextItemWidth(ItemWidth);
        UI::Checkbox("Visible", &mIsLightVisible);
        UI::SetNextItemWidth(ItemWidth);
        UI::SliderFloat("PositionX", &mLightPosition[0], mLightTranslateMin[0], mLightTranslateMax[0]);
        UI::SetNextItemWidth(ItemWidth);
        UI::SliderFloat("PositionY", &mLightPosition[1], mLightTranslateMin[1], mLightTranslateMax[1]);
        UI::SetNextItemWidth(ItemWidth);
        UI::SliderFloat("PositionZ", &mLightPosition[2], mLightTranslateMin[2], mLightTranslateMax[2]);
        UI::SetNextItemWidth(ItemWidth);
        UI::SliderFloat("ColorR", &mLightColor[0], 0.0f, 400.0f);
        UI::SetNextItemWidth(ItemWidth);
        UI::SliderFloat("ColorG", &mLightColor[1], 0.0f, 400.0f);
        UI::SetNextItemWidth(ItemWidth);
        UI::SliderFloat("ColorB", &mLightColor[2], 0.0f, 400.0f);
        UI::EndWindow();
    }

    if (mIsCameraWindowVisible) {
        mCamera.onUI();
    }
    // TODO Node tree
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
    mCamera.onResize();
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
    // PBR
    mCamera.getProjection(mPbrMVPData.projection);
    // PointLight
    mCamera.getProjection(mPointLightMVPData.projection);
}
