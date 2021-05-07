#include "PointLightPipeline.hpp"

MFA::PointLightPipeline::PointLightPipeline() {}

MFA::PointLightPipeline::~PointLightPipeline() {}

void MFA::PointLightPipeline::init() {}

void MFA::PointLightPipeline::shutdown() {}

bool MFA::PointLightPipeline::addDrawableObject(DrawableObject * drawableObject) {
    MFA_NOT_IMPLEMENTED_YET("M.Fakhreddin");
}

void MFA::PointLightPipeline::render(
    RF::DrawPass & drawPass, 
    uint32_t const idsCount, 
    DrawableObjectId * ids
) {}

bool MFA::PointLightPipeline::updateViewProjectionBuffer(
    DrawableObjectId drawableObjectId, 
    ViewProjectionBuffer viewProjectionBuffer
) {
    MFA_NOT_IMPLEMENTED_YET("M.Fakhreddin");
}

void MFA::PointLightPipeline::createDescriptorSetLayout() {}

void MFA::PointLightPipeline::destroyDescriptorSetLayout() {}

void MFA::PointLightPipeline::createPipeline() {}

void MFA::PointLightPipeline::destroyPipeline() {}

void MFA::PointLightPipeline::createUniformBuffers() {}

void MFA::PointLightPipeline::destroyUniformBuffers() {}
