#pragma once

#include "engine/RenderFrontend.hpp"

// TODO inherit from a base pipeline

namespace MFA {

    namespace RB = RenderBackend;
    namespace RF = RenderFrontend;
    
class PointLightPipeline {
public:

    PointLightPipeline();

    ~PointLightPipeline();

    void init();

    void shutdown();

private:

    bool mIsInitialized = false;

    VkDescriptorSetLayout_T * mDescriptorSetLayout = nullptr;
    MFA::RenderFrontend::DrawPipeline mDrawPipeline {};

};

}
