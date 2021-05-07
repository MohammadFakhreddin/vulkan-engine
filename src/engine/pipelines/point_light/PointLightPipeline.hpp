#pragma once

#include "engine/DrawableObject.hpp"
#include "engine/RenderFrontend.hpp"
#include "engine/pipelines/BasePipeline.hpp"

namespace MFA {
    
    namespace RB = RenderBackend;
    namespace RF = RenderFrontend;
    
class PointLightPipeline final : public BasePipeline {
public:

    struct ViewProjectionBuffer {
        float view[16];
        float projectionMat[16];
    };

    PointLightPipeline();

    ~PointLightPipeline() override;

    void init();

    void shutdown();

    bool addDrawableObject(DrawableObject * drawableObject);

    void render(
        RF::DrawPass & drawPass, 
        uint32_t idsCount, 
        DrawableObjectId * ids
    ) override;

    bool updateViewProjectionBuffer(
        DrawableObjectId drawableObjectId, 
        ViewProjectionBuffer viewProjectionBuffer
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

    std::unordered_map<DrawableObjectId, DrawableObject *> mDrawableObjects {};

};

}
