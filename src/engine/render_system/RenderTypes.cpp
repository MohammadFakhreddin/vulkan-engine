#include "RenderTypes.hpp"

#include "engine/BedrockAssert.hpp"
#include "engine/render_system/RenderFrontend.hpp"

#include <utility>

//-------------------------------------------------------------------------------------------------

MFA::RT::BufferAndMemory::BufferAndMemory(
    VkBuffer buffer_,
    VkDeviceMemory memory_
)
    : buffer(buffer_)
    , memory(memory_)
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

MFA::RT::MeshBuffers::MeshBuffers(
    std::shared_ptr<BufferAndMemory> verticesBuffer_,
    std::shared_ptr<BufferAndMemory> indicesBuffer_
)
    : verticesBuffer(std::move(verticesBuffer_))
    , indicesBuffer(std::move(indicesBuffer_))
{}

//-------------------------------------------------------------------------------------------------

MFA::RT::MeshBuffers::~MeshBuffers() = default;

//-------------------------------------------------------------------------------------------------

MFA::RT::ImageGroup::ImageGroup(
    const VkImage image_,
    const VkDeviceMemory memory_
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

MFA::RT::ImageViewGroup::ImageViewGroup(VkImageView imageView_)
    : imageView(imageView_)
{
    MFA_VK_VALID_ASSERT(imageView);
}

//-------------------------------------------------------------------------------------------------

MFA::RT::ImageViewGroup::~ImageViewGroup()
{
    RF::DestroyImageView(*this);
}

//-------------------------------------------------------------------------------------------------

MFA::RT::GpuTexture::GpuTexture(
    std::shared_ptr<ImageGroup> imageGroup,
    std::shared_ptr<ImageViewGroup> imageView,
    std::shared_ptr<AS::Texture> cpuTexture
)
    : imageGroup(std::move(imageGroup))
    , imageView(std::move(imageView))
    , cpuTexture(std::move(cpuTexture))
{}

//-------------------------------------------------------------------------------------------------

MFA::RT::GpuTexture::~GpuTexture() = default;

//-------------------------------------------------------------------------------------------------

MFA::RT::GpuModel::GpuModel(
    GpuModelId const id_,
    std::string address_,
    std::shared_ptr<MeshBuffers> meshBuffers_,
    std::vector<std::shared_ptr<GpuTexture>> textures_
)
    : id(id_)
    , address(std::move(address_))
    , meshBuffers(std::move(meshBuffers_))
    , textures(std::move(textures_))
{}

//-------------------------------------------------------------------------------------------------

MFA::RT::GpuModel::~GpuModel() = default;

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
    MFA_ASSERT(shaderModule != nullptr);
}

//-------------------------------------------------------------------------------------------------

MFA::RT::GpuShader::~GpuShader()
{
    RF::DestroyShader(*this);
}

//-------------------------------------------------------------------------------------------------

bool MFA::RT::PipelineGroup::isValid() const noexcept
{
    return MFA_VK_VALID(pipelineLayout)
        && MFA_VK_VALID(graphicPipeline);
}

//-------------------------------------------------------------------------------------------------

void MFA::RT::PipelineGroup::revoke()
{
    MFA_VK_MAKE_NULL(pipelineLayout);
    MFA_VK_MAKE_NULL(graphicPipeline);
}

//-------------------------------------------------------------------------------------------------

MFA::RT::UniformBufferGroup::UniformBufferGroup(
    std::vector<std::shared_ptr<BufferAndMemory>> buffers_,
    size_t const bufferSize_
)
    : buffers(std::move(buffers_))
    , bufferSize(bufferSize_)
{}

//-------------------------------------------------------------------------------------------------

MFA::RT::UniformBufferGroup::~UniformBufferGroup() = default;

//-------------------------------------------------------------------------------------------------

MFA::RT::StorageBufferCollection::StorageBufferCollection(
    std::vector<std::shared_ptr<BufferAndMemory>> buffers_,
    size_t const bufferSize_
)
    : buffers(std::move(buffers_))
    , bufferSize(bufferSize_)
{}

//-------------------------------------------------------------------------------------------------

MFA::RT::StorageBufferCollection::~StorageBufferCollection() = default;

//-------------------------------------------------------------------------------------------------

MFA::RT::SwapChainGroup::SwapChainGroup(
    VkSwapchainKHR swapChain_,
    VkFormat swapChainFormat_,
    std::vector<VkImage> const & swapChainImages_,
    std::vector<std::shared_ptr<ImageViewGroup>> const & swapChainImageViews_
)
    : swapChain(swapChain_)
    , swapChainFormat(swapChainFormat_)
    , swapChainImages(swapChainImages_)
    , swapChainImageViews(swapChainImageViews_)
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
    VkFormat imageFormat_
)
    : imageGroup(std::move(imageGroup_))
    , imageView(std::move(imageView_))
    , imageFormat(imageFormat_)
{}

//-------------------------------------------------------------------------------------------------

MFA::RT::ColorImageGroup::~ColorImageGroup() = default;

//-------------------------------------------------------------------------------------------------
