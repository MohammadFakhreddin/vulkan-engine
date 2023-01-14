#include "RenderTypes.hpp"

#include "engine/BedrockAssert.hpp"
#include "engine/render_system/RenderFrontend.hpp"

#include <utility>

//-------------------------------------------------------------------------------------------------

MFA::RT::BufferAndMemory::BufferAndMemory(
    VkBuffer buffer_,
    VkDeviceMemory memory_,
    VkDeviceSize size_
)
    : buffer(buffer_)
    , memory(memory_)
    , size(size_)
{}

//-------------------------------------------------------------------------------------------------

MFA::RT::BufferAndMemory::~BufferAndMemory()
{
    RF::DestroyBuffer(*this);
}

//-------------------------------------------------------------------------------------------------

MFA::RT::SamplerGroup::SamplerGroup(VkSampler sampler_)
    : sampler(sampler_)
{}

//-------------------------------------------------------------------------------------------------

MFA::RT::SamplerGroup::~SamplerGroup()
{
    RF::DestroySampler(*this);
}

//-------------------------------------------------------------------------------------------------

MFA::RT::ImageGroup::ImageGroup(
    VkImage image_,
    VkDeviceMemory memory_
)
    : image(image_)
    , memory(memory_)
{}

//-------------------------------------------------------------------------------------------------

MFA::RT::ImageGroup::~ImageGroup()
{
    RF::DestroyImage(*this);
}

//-------------------------------------------------------------------------------------------------

MFA::RenderTypes::FrameBuffer::FrameBuffer(VkFramebuffer framebuffer_)
    : framebuffer(framebuffer_)
{
}

//-------------------------------------------------------------------------------------------------

MFA::RenderTypes::FrameBuffer::~FrameBuffer()
{
    RF::DestroyFrameBuffer(framebuffer);
}

//-------------------------------------------------------------------------------------------------

MFA::RT::ImageViewGroup::ImageViewGroup(VkImageView imageView_)
    : imageView(imageView_)
{
    MFA_ASSERT(imageView != VK_NULL_HANDLE);
}

//-------------------------------------------------------------------------------------------------

MFA::RT::ImageViewGroup::~ImageViewGroup()
{
    RF::DestroyImageView(*this);
}

//-------------------------------------------------------------------------------------------------

MFA::RT::GpuTexture::GpuTexture(
    std::shared_ptr<ImageGroup> imageGroup,
    std::shared_ptr<ImageViewGroup> imageView
)
    : imageGroup(std::move(imageGroup))
    , imageView(std::move(imageView))
{}

//-------------------------------------------------------------------------------------------------

MFA::RT::GpuTexture::~GpuTexture() = default;

//-------------------------------------------------------------------------------------------------

MFA::RT::GpuShader::GpuShader(
    VkShaderModule const & shaderModule,
    VkShaderStageFlagBits const stageFlags,
    std::string entryPointName
)
    : shaderModule(shaderModule)
    , stageFlags(stageFlags)
    , entryPointName(std::move(entryPointName))
{
    MFA_ASSERT(shaderModule != VK_NULL_HANDLE);
}

//-------------------------------------------------------------------------------------------------

MFA::RT::GpuShader::~GpuShader()
{
    RF::DestroyShader(*this);
}

//-------------------------------------------------------------------------------------------------

MFA::RenderTypes::PipelineGroup::PipelineGroup(
    VkPipelineLayout pipelineLayout_,
    VkPipeline pipeline_
)
    : pipelineLayout(pipelineLayout_)
    , pipeline(pipeline_)
{
    MFA_ASSERT(isValid());
}

//-------------------------------------------------------------------------------------------------

MFA::RenderTypes::PipelineGroup::~PipelineGroup()
{
    RF::DestroyPipeline(*this);
}

//-------------------------------------------------------------------------------------------------

bool MFA::RT::PipelineGroup::isValid() const noexcept
{
    return pipelineLayout != VK_NULL_HANDLE && pipeline != VK_NULL_HANDLE;
}

//-------------------------------------------------------------------------------------------------

MFA::RT::BufferGroup::BufferGroup(
    std::vector<std::shared_ptr<BufferAndMemory>> buffers_,
    size_t const bufferSize_
)
    : buffers(std::move(buffers_))
    , bufferSize(bufferSize_)
{}

//-------------------------------------------------------------------------------------------------

MFA::RT::BufferGroup::~BufferGroup() = default;

//-------------------------------------------------------------------------------------------------

MFA::RenderTypes::RenderPass::RenderPass(VkRenderPass renderPass_)
    : vkRenderPass(renderPass_)
{
}

//-------------------------------------------------------------------------------------------------

MFA::RenderTypes::RenderPass::~RenderPass()
{
    RF::DestroyRenderPass(vkRenderPass);
}

//-------------------------------------------------------------------------------------------------

MFA::RT::SwapChainGroup::SwapChainGroup(
    VkSwapchainKHR swapChain_,
    VkFormat swapChainFormat_,
    std::vector<VkImage> swapChainImages_,
    std::vector<std::shared_ptr<ImageViewGroup>> swapChainImageViews_
)
    : swapChain(swapChain_)
    , swapChainFormat(swapChainFormat_)
    , swapChainImages(std::move(swapChainImages_))
    , swapChainImageViews(std::move(swapChainImageViews_))
{}

//-------------------------------------------------------------------------------------------------

MFA::RT::SwapChainGroup::~SwapChainGroup()
{
    RF::DestroySwapChain(*this);
}

//-------------------------------------------------------------------------------------------------

MFA::RT::DepthImageGroup::DepthImageGroup(
    std::shared_ptr<ImageGroup> imageGroup_,
    std::shared_ptr<ImageViewGroup> imageView_,
    VkFormat imageFormat_
)
    : imageGroup(std::move(imageGroup_))
    , imageView(std::move(imageView_))
    , imageFormat(imageFormat_)
{}

//-------------------------------------------------------------------------------------------------

MFA::RT::DepthImageGroup::~DepthImageGroup() = default;

//-------------------------------------------------------------------------------------------------

MFA::RT::ColorImageGroup::ColorImageGroup(
    std::shared_ptr<ImageGroup> imageGroup_,
    std::shared_ptr<ImageViewGroup> imageView_,
    VkFormat const imageFormat_
)
    : imageGroup(std::move(imageGroup_))
    , imageView(std::move(imageView_))
    , imageFormat(imageFormat_)
{}

//-------------------------------------------------------------------------------------------------

MFA::RT::ColorImageGroup::~ColorImageGroup() = default;

//-------------------------------------------------------------------------------------------------

MFA::RenderTypes::DescriptorSetLayoutGroup::DescriptorSetLayoutGroup(VkDescriptorSetLayout descriptorSetLayout_)
    : descriptorSetLayout(descriptorSetLayout_)
{
    MFA_ASSERT(descriptorSetLayout != VK_NULL_HANDLE);
}

//-------------------------------------------------------------------------------------------------

MFA::RenderTypes::DescriptorSetLayoutGroup::~DescriptorSetLayoutGroup()
{
    RF::DestroyDescriptorSetLayout(descriptorSetLayout);
}

//-------------------------------------------------------------------------------------------------

MFA::RenderTypes::MappedMemory::MappedMemory(VkDeviceMemory memory_)
    : memory(memory_)
{
}

//-------------------------------------------------------------------------------------------------

MFA::RenderTypes::MappedMemory::~MappedMemory()
{
    RF::UnMapHostVisibleMemory(memory);
}

//-------------------------------------------------------------------------------------------------
