#include "RenderTypes.hpp"

#include "engine/BedrockAssert.hpp"

//-------------------------------------------------------------------------------------------------

bool MFA::RenderTypes::BufferGroup::isValid() const noexcept {
    return MFA_VK_VALID(buffer) && MFA_VK_VALID(memory);
}

//-------------------------------------------------------------------------------------------------

void MFA::RenderTypes::BufferGroup::revoke() {
    MFA_VK_MAKE_NULL(buffer);
    MFA_VK_MAKE_NULL(memory);
}

//-------------------------------------------------------------------------------------------------

bool MFA::RenderTypes::SamplerGroup::isValid() const noexcept {
    return MFA_VK_VALID(sampler);
}

//-------------------------------------------------------------------------------------------------

void MFA::RenderTypes::SamplerGroup::revoke() {
    MFA_ASSERT(isValid());
    MFA_VK_MAKE_NULL(sampler);
}

//-------------------------------------------------------------------------------------------------

bool MFA::RenderTypes::ImageGroup::isValid() const noexcept {
    return MFA_VK_VALID(image) && MFA_VK_VALID(memory);
}

//-------------------------------------------------------------------------------------------------

void MFA::RenderTypes::ImageGroup::revoke() {
    MFA_VK_MAKE_NULL(image);
    MFA_VK_MAKE_NULL(memory);
}

//-------------------------------------------------------------------------------------------------

MFA::RenderTypes::GpuTexture::GpuTexture(
    ImageGroup && imageGroup, 
    VkImageView imageView, 
    AS::Texture && cpuTexture
)
    : mImageGroup(imageGroup)
    , mImageView(imageView)
    , mCpuTexture(cpuTexture)
{}

//-------------------------------------------------------------------------------------------------

bool MFA::RenderTypes::GpuTexture::isValid() const {
    if (mCpuTexture.isValid() == false) {
        return false;
    }
    if (mImageGroup.isValid() == false) {
        return false;
    }
    return MFA_VK_VALID(mImageView);
}

//-------------------------------------------------------------------------------------------------

void MFA::RenderTypes::GpuTexture::revoke() {
    mImageGroup.revoke();
    MFA_VK_MAKE_NULL(mImageView);
}

//-------------------------------------------------------------------------------------------------

MFA::RenderTypes::GpuShader::GpuShader(
    VkShaderModule shaderModule, 
    AS::Shader cpuShader
)
    : mShaderModule(shaderModule)
    , mCpuShader(std::move(cpuShader)) {}

//-------------------------------------------------------------------------------------------------

bool MFA::RenderTypes::GpuShader::valid() const {
    return MFA_VK_VALID(mShaderModule);
}

//-------------------------------------------------------------------------------------------------

void MFA::RenderTypes::GpuShader::revoke() {
    MFA_VK_MAKE_NULL(mShaderModule);
}

//-------------------------------------------------------------------------------------------------

bool MFA::RenderTypes::PipelineGroup::isValid() const noexcept {
    return MFA_VK_VALID(pipelineLayout)
    && MFA_VK_VALID(graphicPipeline);
}

//-------------------------------------------------------------------------------------------------

void MFA::RenderTypes::PipelineGroup::revoke() {
    MFA_VK_MAKE_NULL(pipelineLayout);
    MFA_VK_MAKE_NULL(graphicPipeline);
}

//-------------------------------------------------------------------------------------------------

bool MFA::RenderTypes::UniformBufferGroup::isValid() const noexcept {
    if(bufferSize <= 0 || buffers.empty() == true) {
        return false;
    }
    for(auto const & buffer : buffers) {
        if(buffer.isValid() == false) {
            return false;
        }
    }
    return true;
}

//-------------------------------------------------------------------------------------------------