#pragma once

#include "RenderTypes.hpp"
#include "engine/BedrockPlatforms.hpp"
#include "engine/FoundationAsset.hpp"

#ifdef __DESKTOP__
#include "libs/sdl/SDL.hpp"
#endif

#include <functional>

namespace MFA
{
    class RenderPass;
    class DisplayRenderPass;
};

namespace MFA::RenderFrontend
{

    using ScreenWidth = Platforms::ScreenSize;
    using ScreenHeight = Platforms::ScreenSize;

    struct InitParams
    {
#ifdef __DESKTOP__
        ScreenWidth screenWidth = 0;
        ScreenHeight screenHeight = 0;
        bool resizable = true;
#elif defined(__ANDROID__)
        android_app * app = nullptr;
#elif defined(__IOS__)
        void * view = nullptr;
#else
#error Os is not supported
#endif
        char const * applicationName = nullptr;
    };

    bool Init(InitParams const & params);

    bool Shutdown();

    int AddResizeEventListener(RT::ResizeEventListener const & eventListener);
    bool RemoveResizeEventListener(RT::ResizeEventListenerId listenerId);

    [[nodiscard]]
    VkDescriptorSetLayout CreateDescriptorSetLayout(
        uint8_t bindingsCount,
        VkDescriptorSetLayoutBinding * bindings
    );

    void DestroyDescriptorSetLayout(VkDescriptorSetLayout descriptorSetLayout);

    [[nodiscard]]
    RT::PipelineGroup CreatePipeline(
        VkRenderPass vkRenderPass,
        uint8_t gpuShadersCount,
        RT::GpuShader const ** gpuShaders,
        uint32_t descriptorLayoutsCount,
        VkDescriptorSetLayout * descriptorSetLayouts,
        VkVertexInputBindingDescription const & vertexBindingDescription,
        uint32_t inputAttributeDescriptionCount,
        VkVertexInputAttributeDescription * inputAttributeDescriptionData,
        RT::CreateGraphicPipelineOptions const & options
    );

    void DestroyPipelineGroup(RT::PipelineGroup & drawPipeline);
    
    [[nodiscard]]
    RT::DescriptorSetGroup CreateDescriptorSets(
        VkDescriptorPool descriptorPool,
        uint32_t descriptorSetCount,
        VkDescriptorSetLayout descriptorSetLayout
    );

    [[nodiscard]]
    std::shared_ptr<RT::BufferAndMemory> CreateVertexBuffer(CBlob verticesBlob);

    [[nodiscard]]
    std::shared_ptr<RT::BufferAndMemory> CreateIndexBuffer(CBlob indicesBlob);

    [[nodiscard]]
    std::shared_ptr<RT::MeshBuffers> CreateMeshBuffers(AssetSystem::Mesh const & mesh);
    
    [[nodiscard]]
    std::shared_ptr<RT::GpuTexture> CreateTexture(std::shared_ptr<AS::Texture> & texture);
    
    void DestroyImage(RT::ImageGroup const & imageGroup);

    void DestroyImageView(RT::ImageViewGroup const & imageViewGroup);

    // TODO We should ask for options here
    [[nodiscard]]
    std::shared_ptr<RT::SamplerGroup> CreateSampler(RT::CreateSamplerParams const & samplerParams);

    void DestroySampler(RT::SamplerGroup const& samplerGroup);

    [[nodiscard]]
    std::shared_ptr<RT::GpuModel> CreateGpuModel(
        std::shared_ptr<AssetSystem::Model> modelAsset,
        RT::GpuModelId uniqueId
    );

    void DeviceWaitIdle();

    [[nodiscard]]
    VkFormat GetDepthFormat();

    void DestroyBuffer(RT::BufferAndMemory const& bufferGroup);

    //---------------------------------------------Shader------------------------------------------------

    [[nodiscard]]
    std::shared_ptr<RT::GpuShader> CreateShader(std::shared_ptr<AS::Shader> const & shader);

    void DestroyShader(RT::GpuShader const& gpuShader);

    //-------------------------------------------UniformBuffer--------------------------------------------

    std::shared_ptr<RT::UniformBufferGroup> CreateUniformBuffer(size_t bufferSize, uint32_t count);

    void UpdateUniformBuffer(
        RT::CommandRecordState const & recordState,
        RT::UniformBufferGroup const & bufferCollection,
        CBlob data
    );

    void UpdateUniformBuffer(
        RT::BufferAndMemory const & buffer,
        CBlob data
    );

    //void DestroyUniformBuffer(RT::UniformBufferCollection & uniformBuffer);

    //-------------------------------------------StorageBuffer--------------------------------------------

    std::shared_ptr<RT::StorageBufferCollection> CreateStorageBuffer(size_t bufferSize, uint32_t count);

    void UpdateStorageBuffer(
        RT::BufferAndMemory const & buffer,
        CBlob data
    );
    
    //void DestroyStorageBuffer(RT::StorageBufferCollection & storageBuffer);

    //-------------------------------------------------------------------------------------------------

    void BindDrawPipeline(
        RT::CommandRecordState & drawPass,
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


    enum class DescriptorSetType : uint32_t
    {
        PerFrame = 0,
        PerEssence = 1,
        PerVariant = 2
    };

    void BindDescriptorSet(
        RT::CommandRecordState const & drawPass,
        DescriptorSetType frequency,
        RT::DescriptorSetGroup descriptorSetGroup
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
        RT::CommandRecordState const & drawPass,
        RT::BufferAndMemory const & vertexBuffer,
        VkDeviceSize offset = 0
    );

    void BindIndexBuffer(
        RT::CommandRecordState const & drawPass,
        RT::BufferAndMemory const & indexBuffer,
        VkDeviceSize offset = 0,
        VkIndexType indexType = VK_INDEX_TYPE_UINT32
    );

    void DrawIndexed(
        RT::CommandRecordState const & drawPass,
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

    void SetScissor(RT::CommandRecordState const & drawPass, VkRect2D const & scissor);

    void SetViewport(RT::CommandRecordState const & drawPass, VkViewport const & viewport);

    void PushConstants(
        RT::CommandRecordState const & drawPass,
        AS::ShaderStage shaderStage,
        uint32_t offset,
        CBlob data
    );

    void PushConstants(
        RT::CommandRecordState const & drawPass,
        VkShaderStageFlags shaderStage,
        uint32_t offset,
        CBlob data
    );

    [[nodiscard]]
    uint32_t GetSwapChainImagesCount();

    uint32_t GetMaxFramesPerFlight();

    void GetDrawableSize(int32_t & outWidth, int32_t & outHeight);

#ifdef __DESKTOP__
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

#ifdef __DESKTOP__
    bool IsWindowVisible();
#endif

    bool IsWindowResized();

    void WaitForFence(VkFence fence);

    void AcquireNextImage(
        VkSemaphore imageAvailabilitySemaphore,
        RT::SwapChainGroup const & swapChainGroup,
        uint32_t & outImageIndex
    );

    std::vector<VkCommandBuffer> CreateGraphicCommandBuffers(uint32_t maxFramesPerFlight);

    void BeginCommandBuffer(
        VkCommandBuffer commandBuffer,
        VkCommandBufferBeginInfo const & beginInfo = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT
        }
    );

    void EndCommandBuffer(VkCommandBuffer commandBuffer);

    void DestroyGraphicCommandBuffer(
        VkCommandBuffer * commandBuffers,
        uint32_t commandBuffersCount
    );

    uint32_t GetPresentQueueFamily();

    uint32_t GetGraphicQueueFamily();

    void PipelineBarrier(
        VkCommandBuffer commandBuffer,
        VkPipelineStageFlags sourceStageMask,
        VkPipelineStageFlags destinationStateMask,
        uint32_t barrierCount,
        VkImageMemoryBarrier const * memoryBarriers
    );

    void PipelineBarrier(
        VkCommandBuffer commandBuffer,
        VkPipelineStageFlags sourceStageMask,
        VkPipelineStageFlags destinationStateMask,
        VkBufferMemoryBarrier const & memoryBarrier
    );

    void SubmitQueue(
        VkCommandBuffer commandBuffer,
        VkSemaphore imageAvailabilitySemaphore,
        VkSemaphore renderFinishIndicatorSemaphore,
        VkFence inFlightFence
    );

    void PresentQueue(
        uint32_t imageIndex,
        VkSemaphore renderFinishIndicatorSemaphore,
        VkSwapchainKHR swapChain
    );

    DisplayRenderPass * GetDisplayRenderPass();

    RT::SyncObjects createSyncObjects(
        uint32_t maxFramesInFlight,
        uint32_t swapChainImagesCount
    );

    void DestroySyncObjects(RT::SyncObjects const & syncObjects);

    VkSampleCountFlagBits GetMaxSamplesCount();

    // Not recommended
    void WaitForGraphicQueue();

    // Not recommended
    void WaitForPresentQueue();

    //------------------------------------------QueryPool----------------------------------------------

    [[nodiscard]]
    VkQueryPool CreateQueryPool(VkQueryPoolCreateInfo const & createInfo);

    void DestroyQueryPool(VkQueryPool queryPool);

    void BeginQuery(RT::CommandRecordState const & drawPass, VkQueryPool queryPool, uint32_t queryId);

    void EndQuery(RT::CommandRecordState const & drawPass, VkQueryPool queryPool, uint32_t queryId);

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

    VkCommandBuffer GetGraphicCommandBuffer(RT::CommandRecordState const & drawPass);

    [[nodiscard]]
    RT::CommandRecordState StartGraphicCommandBufferRecording();

    void EndGraphicCommandBufferRecording(RT::CommandRecordState & drawPass);

    VkFence GetInFlightFence(RT::CommandRecordState const & drawPass);

    [[nodiscard]]
    VkSemaphore GetRenderFinishIndicatorSemaphore(RT::CommandRecordState const & drawPass);

    [[nodiscard]]
    VkSemaphore getImageAvailabilitySemaphore(RT::CommandRecordState const & drawPass);

    [[nodiscard]]
    VkDescriptorPool CreateDescriptorPool(uint32_t maxSets);

    void DestroyDescriptorPool(VkDescriptorPool descriptorPool);

}

namespace MFA
{
    namespace RF = RenderFrontend;
};

#define RF_CREATE_SHADER(path, stage)                                                           \
auto cpu##stage##Shader = Importer::ImportShaderFromSPV(                                        \
    Path::Asset(path).c_str(),                                                                  \
    AssetSystem::Shader::Stage::stage,                                                          \
    "main"                                                                                      \
);                                                                                              \
auto gpu##stage##Shader = RF::CreateShader(cpu##stage##Shader);                                 \
