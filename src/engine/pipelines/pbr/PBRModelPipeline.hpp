#pragma once

#include "engine/DrawableObject.hpp"
#include "engine/RenderFrontend.hpp"
#include "engine/pipelines/BasePipeline.hpp"

namespace MFA {

namespace RB = RenderBackend;
namespace RF = RenderFrontend;
    
class PBRModelPipeline final : public BasePipeline{
public:

    struct PrimitiveInfo {
        alignas(16) float baseColorFactor[4];
        float emissiveFactor[3];
        int placeholder0;
        alignas(4) int hasBaseColorTexture;
        alignas(4) float metallicFactor;
        alignas(4) float roughnessFactor;
        alignas(4) int hasMixedMetallicRoughnessOcclusionTexture;
        alignas(4) int hasNormalTexture;
        alignas(4) int hasEmissiveTexture;
    };

    struct ViewProjectionData {   // For vertices in Vertex shader
        alignas(64) float view[16];
        alignas(64) float perspective[16];
    };

    struct LightViewBuffer {
        alignas(16) float lightPosition[3];
        alignas(16) float cameraPosition[3];
        alignas(16) float lightColor[3];
    };

    explicit PBRModelPipeline() = default;

    ~PBRModelPipeline() override {
        if (mIsInitialized) {
            shutdown();
        }
    }

    PBRModelPipeline (PBRModelPipeline const &) noexcept = delete;
    PBRModelPipeline (PBRModelPipeline &&) noexcept = delete;
    PBRModelPipeline & operator = (PBRModelPipeline const &) noexcept = delete;
    PBRModelPipeline & operator = (PBRModelPipeline &&) noexcept = delete;

    void init(
        RF::SamplerGroup * samplerGroup, 
        RB::GpuTexture * errorTexture
    );

    void shutdown();

    void render(
        RF::DrawPass & drawPass, 
        uint32_t idsCount, 
        DrawableObjectId * ids
    ) override;

    DrawableObjectId addGpuModel(RF::GpuModel & gpuModel);

    bool removeGpuModel(DrawableObjectId drawableObjectId);
    
    bool updateViewProjectionBuffer(
        DrawableObjectId drawableObjectId, 
        ViewProjectionData const & viewProjectionData
    );

    void updateLightViewBuffer(
        LightViewBuffer const & lightViewData
    );

private:

    void createDescriptorSetLayout();

    void destroyDescriptorSetLayout();

    void createPipeline();

    void destroyPipeline();

    void createUniformBuffers();

    void destroyUniformBuffers();

    bool mIsInitialized = false;

    VkDescriptorSetLayout_T * mDescriptorSetLayout = nullptr;

    MFA::RenderFrontend::DrawPipeline mDrawPipeline {};

    std::unordered_map<DrawableObjectId, std::unique_ptr<DrawableObject>> mDrawableObjects {};

    RF::SamplerGroup * mSamplerGroup = nullptr; // TODO Each gltf subMesh has its own settings

    RB::GpuTexture * mErrorTexture = nullptr;

    RF::UniformBufferGroup mLightViewBuffer {};
};

}
