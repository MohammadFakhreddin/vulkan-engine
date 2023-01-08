#pragma once

#include "RenderTypes.hpp"
#include "engine/BedrockPlatforms.hpp"
#include "engine/asset_system/AssetTypes.hpp"
#include "engine/BedrockCommon.hpp"

#ifdef __DESKTOP__
#include "libs/sdl/SDL.hpp"
#endif

#include <functional>

namespace MFA
{
    class RenderPass;
    class DisplayRenderPass;
}

namespace MFA::AssetSystem
{
    class Texture;
    class MeshBase;
    struct Model;
    class Shader;
}


// Idea: Maybe we can return unique ptr so it can converted to shared_ptr when necessary

namespace MFA::RenderFrontend
{

    using InitParams = RT::FrontendInitParams;

    bool Init(InitParams const & params);

    bool Shutdown();

    int AddResizeEventListener(RT::ResizeEventListener const & eventListener);
    bool RemoveResizeEventListener(RT::ResizeEventListenerId listenerId);

    [[nodiscard]]
    std::shared_ptr<RT::DescriptorSetLayoutGroup> CreateDescriptorSetLayout(
        uint8_t bindingsCount,
        VkDescriptorSetLayoutBinding * bindings
    );

    void DestroyDescriptorSetLayout(VkDescriptorSetLayout descriptorSetLayout);

    //---------------------------------------------Pipelines--------------------------------------------
    
    [[nodiscard]]
    VkPipelineLayout CreatePipelineLayout(
        uint32_t setLayoutCount,
        const VkDescriptorSetLayout * pSetLayouts,
        uint32_t pushConstantRangeCount = 0,
        const VkPushConstantRange * pPushConstantRanges = nullptr
    );

    [[nodiscard]]
    std::shared_ptr<RT::PipelineGroup> CreateGraphicPipeline(
        VkRenderPass vkRenderPass,
        uint8_t gpuShadersCount,
        RT::GpuShader const ** gpuShaders,
        VkPipelineLayout pipelineLayout,
        uint32_t vertexBindingDescriptionCount,
        VkVertexInputBindingDescription const * vertexBindingDescriptionData,
        uint32_t inputAttributeDescriptionCount,
        VkVertexInputAttributeDescription * inputAttributeDescriptionData,
        RT::CreateGraphicPipelineOptions const & options
    );

    [[nodiscard]]
    std::shared_ptr<RT::PipelineGroup> CreateComputePipeline(
        RT::GpuShader const & shaderStage,
        VkPipelineLayout pipelineLayout
    );

    void DestroyPipeline(RT::PipelineGroup & recordState);

    //-------------------------------------------DescriptorSetGroup--------------------------------------------

    [[nodiscard]]
    RT::DescriptorSetGroup CreateDescriptorSets(
        VkDescriptorPool descriptorPool,
        uint32_t descriptorSetCount,
        RT::DescriptorSetLayoutGroup const & descriptorSetLayoutGroup
    );

    [[nodiscard]]
    RT::DescriptorSetGroup CreateDescriptorSets(
        VkDescriptorPool descriptorPool,
        uint32_t descriptorSetCount,
        VkDescriptorSetLayout descriptorSetLayout
    );

    //-----------------------------------------------------------------------------------------------------------

    /*[[nodiscard]]
    std::shared_ptr<RT::MeshBuffer> CreateMeshBuffers(
        VkCommandBuffer commandBuffer,
        AS::MeshBase const & mesh,
        RT::BufferAndMemory const & vertexStageBuffer,
        RT::BufferAndMemory const & indexStageBuffer
    );*/

    [[nodiscard]]
    std::shared_ptr<RT::GpuTexture> CreateTexture(AS::Texture const & texture);

    void DestroyImage(RT::ImageGroup const & imageGroup);

    void DestroyImageView(RT::ImageViewGroup const & imageViewGroup);

    [[nodiscard]]
    std::shared_ptr<RT::SamplerGroup> CreateSampler(RT::CreateSamplerParams const & samplerParams);

    void DestroySampler(RT::SamplerGroup const & samplerGroup);

    //[[nodiscard]]
    //std::shared_ptr<RT::GpuModel> CreateGpuModel(
    //    VkCommandBuffer commandBuffer,
    //    AS::Model const & modelAsset,
    //    std::string const & nameId,
    //    RT::BufferAndMemory const & vertexStageBuffer,
    //    RT::BufferAndMemory const & indexStageBuffer
    //);

    void DeviceWaitIdle();

    VkSurfaceFormatKHR GetSurfaceFormat();

    [[nodiscard]]
    VkFormat GetDepthFormat();

    //---------------------------------------------Shader------------------------------------------------

    [[nodiscard]]
    std::shared_ptr<RT::GpuShader> CreateShader(std::shared_ptr<AS::Shader> const & shader);

    void DestroyShader(RT::GpuShader const & gpuShader);

    //-------------------------------------------Buffers--------------------------------------------------

    std::shared_ptr<RT::BufferAndMemory> CreateBuffer(
        VkDeviceSize size,
        VkBufferUsageFlags usage,
        VkMemoryPropertyFlags properties
    );

    std::shared_ptr<RT::MappedMemory> MapHostVisibleMemory(
        VkDeviceMemory bufferMemory,
        size_t offset,
        size_t size
    );

    void UnMapHostVisibleMemory(VkDeviceMemory bufferMemory);

    [[nodiscard]]
    std::shared_ptr<RT::BufferGroup> CreateBufferGroup(
        VkDeviceSize bufferSize,
        uint32_t count,
        VkBufferUsageFlags bufferUsageFlagBits,
        VkMemoryPropertyFlags memoryPropertyFlags
    );

    std::shared_ptr<RT::BufferAndMemory> CreateVertexBuffer(
        VkCommandBuffer commandBuffer,
        RT::BufferAndMemory const & stageBuffer,
        CBlob const & verticesBlob
    );

    std::shared_ptr<RT::BufferAndMemory> CreateVertexBuffer(VkDeviceSize bufferSize);

    std::shared_ptr<RT::BufferAndMemory> CreateIndexBuffer(
        VkCommandBuffer commandBuffer,
        RT::BufferAndMemory const & stageBuffer,
        CBlob const & indicesBlob
    );

    std::shared_ptr<RT::BufferAndMemory> CreateIndexBuffer(VkDeviceSize bufferSize);

    std::shared_ptr<RT::BufferGroup> CreateStageBuffer(
        VkDeviceSize bufferSize,
        uint32_t count
    );

    [[nodiscard]]
    std::shared_ptr<RT::BufferGroup> CreateLocalUniformBuffer(size_t bufferSize, uint32_t count);

    [[nodiscard]]
    std::shared_ptr<RT::BufferGroup> CreateHostVisibleUniformBuffer(size_t bufferSize, uint32_t count);

    [[nodiscard]]
    std::shared_ptr<RT::BufferGroup> CreateLocalStorageBuffer(size_t bufferSize, uint32_t count);

    [[nodiscard]]
    std::shared_ptr<RT::BufferGroup> CreateHostVisibleStorageBuffer(size_t bufferSize, uint32_t count);

    void UpdateHostVisibleBuffer(
        RT::BufferAndMemory const & buffer,
        CBlob const & data
    );

    void UpdateLocalBuffer(
        VkCommandBuffer commandBuffer,
        RT::BufferAndMemory const & buffer,
        RT::BufferAndMemory const & stageBuffer
    );

    void DestroyBuffer(RT::BufferAndMemory const & bufferGroup);
    
    //-------------------------------------------------------------------------------------------------

    void BindPipeline(
        RT::CommandRecordState & recordState,
        RT::PipelineGroup & pipeline
    );

    void UpdateDescriptorSets(
        uint32_t writeInfoCount,
        VkWriteDescriptorSet * writeInfo
    );

    void UpdateDescriptorSets(
        uint8_t descriptorSetsCount,
        VkDescriptorSet * descriptorSets,
        uint8_t writeInfoCount,
        VkWriteDescriptorSet * writeInfo
    );


    enum class UpdateFrequency : uint32_t
    {
        PerFrame = 0,
        PerEssence = 1,
        PerVariant = 2
    };

    void AutoBindDescriptorSet(
        RT::CommandRecordState const & recordState,
        UpdateFrequency frequency,
        RT::DescriptorSetGroup const & descriptorGroup
    );

    void BindDescriptorSet(
        RT::CommandRecordState const & recordState,
        UpdateFrequency frequency,
        VkDescriptorSet descriptorSet
    );

    void BindDescriptorSet(
        VkCommandBuffer commandBuffer,
        VkPipelineBindPoint bindPoint,
        VkPipelineLayout pipelineLayout,
        UpdateFrequency frequency,
        VkDescriptorSet descriptorSet
    );
    
    //: loop through each mesh instance in the mesh instances list to render
    //
    //  - get the mesh from the asset manager
    //  - populate the push constants with the transformation matrix of the mesh
    //  - bind the pipeline to the command buffer
    //  - bind the vertex buffer for the mesh to the command buffer
    //  - bind the index buffer for the mesh to the command buffer
    //  - get the texture instance to apply to the mesh from the asset manager
    //  - get the descriptor set for the texture that defines how it maps to the pipeline's descriptor set layout
    //  - bind the descriptor set for the texture into the pipeline's descriptor set layout
    //  - tell the command buffer to draw the mesh
    //
    //: end loop

    void BindVertexBuffer(
        RT::CommandRecordState const & recordState,
        RT::BufferAndMemory const & vertexBuffer,
        uint32_t firstBinding = 0,
        VkDeviceSize offset = 0
    );

    void BindIndexBuffer(
        RT::CommandRecordState const & recordState,
        RT::BufferAndMemory const & indexBuffer,
        VkDeviceSize offset = 0,
        VkIndexType indexType = VK_INDEX_TYPE_UINT32
    );

    void DrawIndexed(
        RT::CommandRecordState const & recordState,
        uint32_t indicesCount,
        uint32_t instanceCount = 1,
        uint32_t firstIndex = 0,
        uint32_t vertexOffset = 0,
        uint32_t firstInstance = 0
    );

    void BeginRenderPass(
        VkCommandBuffer commandBuffer,
        VkRenderPass renderPass,
        VkFramebuffer frameBuffer,
        VkExtent2D const & extent2D,
        uint32_t clearValuesCount,
        VkClearValue const * clearValues
    );

    void EndRenderPass(VkCommandBuffer commandBuffer);

    void OnNewFrame(float deltaTimeInSec);

    void SetScissor(RT::CommandRecordState const & recordState, VkRect2D const & scissor);

    void SetViewport(RT::CommandRecordState const & recordState, VkViewport const & viewport);

    void PushConstants(
        RT::CommandRecordState const & recordState,
        AS::ShaderStage shaderStage,
        uint32_t offset,
        CBlob data
    );

    void PushConstants(
        RT::CommandRecordState const & recordState,
        VkShaderStageFlags shaderStage,
        uint32_t offset,
        CBlob data
    );

    [[nodiscard]]
    uint32_t GetSwapChainImagesCount();

    uint32_t GetMaxFramesPerFlight();

    void GetDrawableSize(int32_t & outWidth, int32_t & outHeight);

#ifdef __DESKTOP__// TODO: Move these functions to somewhere else
    // SDL Functions

    void WarpMouseInWindow(int32_t x, int32_t y);

    uint32_t GetMouseState(int32_t * x, int32_t * y);

    uint8_t const * GetKeyboardState(int * numKeys = nullptr);

    [[nodiscard]]
    uint32_t GetWindowFlags();

#endif

    void AssignViewportAndScissorToCommandBuffer(VkCommandBuffer commandBuffer, VkExtent2D const & extent2D);

#ifdef __DESKTOP__
    using SDLEventWatchId = int;
    using SDLEventWatch = std::function<int(void * data, MSDL::SDL_Event * event)>;
    SDLEventWatchId AddEventWatch(SDLEventWatch const & eventWatch);

    void RemoveEventWatch(SDLEventWatchId watchId);
#endif

    void NotifyDeviceResized();

    [[nodiscard]]
    VkSurfaceCapabilitiesKHR GetSurfaceCapabilities();


    [[nodiscard]]
    std::shared_ptr<RT::SwapChainGroup> CreateSwapChain(VkSwapchainKHR oldSwapChain = VkSwapchainKHR{});

    void DestroySwapChain(RT::SwapChainGroup const & swapChainGroup);


    [[nodiscard]]
    std::shared_ptr<RT::ColorImageGroup> CreateColorImage(
        VkExtent2D const & imageExtent,
        VkFormat imageFormat,
        RT::CreateColorImageOptions const & options
    );

    [[nodiscard]]
    VkRenderPass CreateRenderPass(
        VkAttachmentDescription * attachments,
        uint32_t attachmentsCount,
        VkSubpassDescription * subPasses,
        uint32_t subPassesCount,
        VkSubpassDependency * dependencies,
        uint32_t dependenciesCount
    );

    void DestroyRenderPass(VkRenderPass renderPass);

    std::shared_ptr<RT::DepthImageGroup> CreateDepthImage(
        VkExtent2D const & imageSize,
        RT::CreateDepthImageOptions const & options
    );

    [[nodiscard]]
    VkFramebuffer CreateFrameBuffer(
        VkRenderPass renderPass,
        VkImageView const * attachments,
        uint32_t attachmentsCount,
        VkExtent2D const & frameBufferExtent,
        uint32_t layersCount
    );

    void DestroyFrameBuffers(
        uint32_t frameBuffersCount,
        VkFramebuffer * frameBuffers
    );

    void DestroyFrameBuffer(VkFramebuffer frameBuffer);

    bool IsWindowVisible();

    bool IsWindowResized();

    void WaitForFence(VkFence fence);

    void AcquireNextImage(
        VkSemaphore imageAvailabilitySemaphore,
        RT::SwapChainGroup const & swapChainGroup,
        uint32_t & outImageIndex
    );

    //-----------------------------------------CommandBuffer-------------------------------------------------

    void BeginGraphicCommandBuffer(
        RT::CommandRecordState & recordState,
        VkCommandBufferBeginInfo const & beginInfo = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT
        }
    );

    void BeginComputeCommandBuffer(
        RT::CommandRecordState & recordState,
        VkCommandBufferBeginInfo const & beginInfo = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT
        }
    );

    void BeginCommandBuffer(
        RT::CommandRecordState & recordState,
        VkCommandBuffer commandBuffer,
        RT::CommandBufferType commandBufferType,
        VkCommandBufferBeginInfo const & beginInfo = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT
        }
    );

    void EndCommandBuffer(RT::CommandRecordState & recordState);

    [[nodiscard]]
    VkCommandBuffer BeginSingleTimeGraphicCommand();

    void EndAndSubmitGraphicSingleTimeCommand(VkCommandBuffer const & commandBuffer);

    [[nodiscard]]
    VkCommandBuffer BeginSingleTimeComputeCommand();

    void EndAndSubmitComputeSingleTimeCommand(VkCommandBuffer const & commandBuffer);

    void DestroyComputeCommand(VkCommandBuffer commandBuffer);

    //--------------------------------------------------------------------------------------------------------

    uint32_t GetPresentQueueFamily();

    uint32_t GetGraphicQueueFamily();

    uint32_t GetComputeQueueFamily();

    VkCommandBuffer GetComputeCommandBuffer(RT::CommandRecordState const & recordState);

    VkCommandBuffer GetGraphicCommandBuffer(RT::CommandRecordState const & recordState);
    
    void PipelineBarrier(
        RT::CommandRecordState const & recordState,
        VkPipelineStageFlags sourceStageMask,
        VkPipelineStageFlags destinationStateMask,
        uint32_t barrierCount,
        VkImageMemoryBarrier const * memoryBarriers
    );

    void PipelineBarrier(
        RT::CommandRecordState const & recordState,
        VkPipelineStageFlags sourceStageMask,
        VkPipelineStageFlags destinationStateMask,
        uint32_t barrierCount,
        VkBufferMemoryBarrier const * bufferMemoryBarrier
    );

    void ResetFence(VkFence fence);

    void SubmitGraphicQueue(
        uint32_t const submitCount,
        const VkSubmitInfo * submitInfos,
        VkFence fence
    );

    void SubmitComputeQueue(
        uint32_t const submitCount,
        const VkSubmitInfo * submitInfos,
        VkFence fence
    );

    void PresentQueue(
        RT::CommandRecordState const & recordState,
        uint32_t waitSemaphoresCount,
        const VkSemaphore * waitSemaphores
    );

    DisplayRenderPass * GetDisplayRenderPass();
    
    VkSampleCountFlagBits GetMaxSamplesCount();
    
    //------------------------------------------QueryPool----------------------------------------------

    [[nodiscard]]
    VkQueryPool CreateQueryPool(VkQueryPoolCreateInfo const & createInfo);

    void DestroyQueryPool(VkQueryPool queryPool);

    void BeginQuery(RT::CommandRecordState const & recordState, VkQueryPool queryPool, uint32_t queryId);

    void EndQuery(RT::CommandRecordState const & recordState, VkQueryPool queryPool, uint32_t queryId);

    void GetQueryPoolResult(
        VkQueryPool queryPool,
        uint32_t samplesCount,
        uint64_t * outSamplesData,
        uint32_t samplesOffset = 0
    );

    void ResetQueryPool(
        RT::CommandRecordState const & recordState,
        VkQueryPool queryPool,
        uint32_t queryCount,
        uint32_t firstQueryIndex = 0
    );

    //-------------------------------------------------------------------------------------------------

    [[nodiscard]]
    RT::CommandRecordState AcquireRecordState();

    [[nodiscard]]
    VkFence GetGraphicFence(RT::CommandRecordState const & recordState);

    [[nodiscard]]
    VkSemaphore GetGraphicSemaphore(RT::CommandRecordState const & recordState);
    
    [[nodiscard]]
    VkFence GetComputeFence(RT::CommandRecordState const & recordState);

    [[nodiscard]]
    VkSemaphore GetComputeSemaphore(RT::CommandRecordState const & recordState);

    [[nodiscard]]
    VkSemaphore GetPresentSemaphore(RT::CommandRecordState const & recordState);

    [[nodiscard]]
    VkDescriptorPool CreateDescriptorPool(uint32_t maxSets);

    void DestroyDescriptorPool(VkDescriptorPool descriptorPool);

    void Dispatch(
        RT::CommandRecordState const & recordState,
        uint32_t groupCountX,
        uint32_t groupCountY,
        uint32_t groupCountZ
    );

}

namespace MFA
{
    namespace RF = RenderFrontend;
};

#define RF_CREATE_SHADER(path, stage)                                                           \
auto cpu##stage##Shader = Importer::ImportShaderFromSPV(                                        \
    Path::ForReadWrite(path).c_str(),                                                           \
    AssetSystem::Shader::Stage::stage,                                                          \
    "main"                                                                                      \
);                                                                                              \
auto gpu##stage##Shader = RF::CreateShader(cpu##stage##Shader);                                 \
