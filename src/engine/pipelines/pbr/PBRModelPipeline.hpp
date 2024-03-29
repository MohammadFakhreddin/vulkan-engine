#pragma once

#include "engine/render_system/DrawableObject.hpp"
#include "engine/render_system/RenderFrontend.hpp"
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
        alignas(4) int hasSkin;
    };

    struct ViewProjectionData {   // For vertices in Vertex shader
        alignas(64) float model[16];
        alignas(64) float view[16];
        alignas(64) float projection[16];
    };

    struct LightViewBuffer {
        alignas(16) float lightPosition[3] {0, 0, 0};
        alignas(16) float cameraPosition[3] {0, 0, 0};
        alignas(16) float lightColor[3] {0, 0, 0};
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
        float deltaTime,
        uint32_t idsCount, 
        DrawableObjectId * ids
    ) override;

    DrawableObjectId addGpuModel(RF::GpuModel & gpuModel) override;

    bool removeGpuModel(DrawableObjectId drawableObjectId) override;
    
    bool updateViewProjectionBuffer(
        DrawableObjectId drawableObjectId, 
        ViewProjectionData const & viewProjectionData
    );

    void updateLightViewBuffer(
        LightViewBuffer const & lightViewData
    );

    DrawableObject * GetDrawableById(DrawableObjectId objectId);

private:

    void createDescriptorSetLayout();

    void destroyDescriptorSetLayout();

    void createPipeline();

    void destroyPipeline();

    void createUniformBuffers();

    void destroyUniformBuffers();

    bool mIsInitialized = false;

    // TODO Support multiple descriptorSetLayouts
    VkDescriptorSetLayout mDescriptorSetLayout {};

    RenderFrontend::DrawPipeline mDrawPipeline {};

    std::unordered_map<DrawableObjectId, std::unique_ptr<DrawableObject>> mDrawableObjects {};

    RF::SamplerGroup * mSamplerGroup = nullptr; // TODO Each gltf subMesh has its own settings

    RB::GpuTexture * mErrorTexture = nullptr;

    RF::UniformBufferGroup mErrorBuffer {};

    RF::UniformBufferGroup mLightViewBuffer {};
};

}
