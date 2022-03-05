#include "RenderFrontend.hpp"

#include "engine/render_system/RenderTypes.hpp"
#include "engine/BedrockAssert.hpp"
#include "RenderBackend.hpp"
#include "engine/BedrockLog.hpp"
#include "render_passes/display_render_pass/DisplayRenderPass.hpp"
#include "engine/BedrockSignal.hpp"
#include "engine/asset_system/AssetBaseMesh.hpp"
#include "engine/asset_system/AssetModel.hpp"

#ifdef __DESKTOP__
#include "libs/sdl/SDL.hpp"
#elif defined(__ANDROID__)
#include <android_native_app_glue.h>
#elif defined(__IOS__)
#else
#error Os is not supported
#endif

#include "libs/imgui/imgui.h"

#include <string>

namespace MFA::RenderFrontend
{

#ifdef __DESKTOP__
    struct SDLEventWatchGroup
    {
        SDLEventWatchId id = 0;
        SDLEventWatch watch = nullptr;
    };
#endif

    struct State
    {
        uint32_t maxFramesPerFlight = 0;
        // CreateWindow
        ScreenWidth screenWidth = 0;
        ScreenHeight screenHeight = 0;
        // CreateInstance
        std::string application_name{};
        VkInstance vk_instance = nullptr;
        // CreateDebugCallback
        VkDebugReportCallbackEXT vkDebugReportCallbackExt{};
        VkSurfaceKHR surface{};
        VkPhysicalDevice physicalDevice{};
        VkPhysicalDeviceFeatures physicalDeviceFeatures{};
        VkPhysicalDeviceProperties physicalDeviceProperties{};
        VkSampleCountFlagBits maxSampleCount{};

        std::vector<VkFence> fences {};

        // Graphic
        uint32_t graphicQueueFamily = 0;
        VkQueue graphicQueue{};
        std::vector<VkCommandBuffer> graphicCommandBuffer{};
        std::vector<VkSemaphore> graphicSemaphores;
        VkCommandPool graphicCommandPool{};
        // Compute
        uint32_t computeQueueFamily = 0;
        VkQueue computeQueue{};
        std::vector<VkCommandBuffer> computeCommandBuffer{};
        std::vector<VkSemaphore> computeSemaphores;
        VkCommandPool computeCommandPool{};
        // Presentation
        uint32_t presentQueueFamily = 0;
        VkQueue presentQueue{};
        std::vector<VkSemaphore> presentSemaphores;

        RT::LogicalDevice logicalDevice{};
        // Resize
        bool isWindowResizable = false;
        bool windowResized = false;
        Signal<> resizeEventSignal{};
        VkSurfaceCapabilitiesKHR surfaceCapabilities{};
        uint32_t swapChainImageCount = 0;
        DisplayRenderPass displayRenderPass{};
        int nextEventListenerId = 0;
        uint8_t currentFrame = 0;
        VkFormat depthFormat{};
#ifdef __DESKTOP__
        // CreateWindow
        MSDL::SDL_Window * window = nullptr;
        // Event watches
        std::vector<SDLEventWatchGroup> sdlEventListeners{};
        bool isWindowVisible = true;                        // Currently only minimize can cause this to be false
#elif defined(__ANDROID__)
        ANativeWindow * window = nullptr;
#elif defined(__IOS__)
        void * window = nullptr;
#else
#error Os is not handled
#endif
    } static * state = nullptr;

    //-------------------------------------------------------------------------------------------------

    static VkBool32 VKAPI_PTR DebugCallback(
        VkDebugReportFlagsEXT const flags,
        VkDebugReportObjectTypeEXT object_type,
        uint64_t src_object,
        size_t location,
        int32_t const message_code,
        char const * player_prefix,
        char const * message,
        void * user_data
    )
    {
        if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT)
        {
            MFA_LOG_ERROR("Message code: %d\nMessage: %s\nLocation: %llu\n", message_code, message, static_cast<uint64_t>(location));
        }
        else if (flags & VK_DEBUG_REPORT_WARNING_BIT_EXT)
        {
            MFA_LOG_WARN("Message code: %d\nMessage: %s\nLocation: %llu\n", message_code, message, static_cast<uint64_t>(location));
        }
        else
        {
            MFA_LOG_INFO("Message code: %d\nMessage: %s\nLocation: %llu\n", message_code, message, static_cast<uint64_t>(location));
        }
        MFA_ASSERT(false);
        return true;
    }

    //-------------------------------------------------------------------------------------------------

#ifdef __DESKTOP__
    static bool IsResizeEvent(uint8_t const sdlEvent)
    {
#if defined(__PLATFORM_WIN__) || defined(__PLATFORM_LINUX__)
        return sdlEvent == MSDL::SDL_WINDOWEVENT_RESIZED;
#elif defined(__PLATFORM_MAC__)
        return sdlEvent == MSDL::SDL_WINDOWEVENT_RESIZED ||
            sdlEvent == MSDL::SDL_WINDOWEVENT_SIZE_CHANGED ||
            sdlEvent == MSDL::SDL_WINDOWEVENT_MAXIMIZED ||
            sdlEvent == MSDL::SDL_WINDOWEVENT_MINIMIZED ||
            sdlEvent == MSDL::SDL_WINDOWEVENT_EXPOSED;;
#else
#error Unhandled platform
#endif
    }

    //-------------------------------------------------------------------------------------------------

    static int SDLEventWatcher(void * data, MSDL::SDL_Event * event)
    {
        if (
            state->isWindowResizable == true &&
            event->type == MSDL::SDL_WINDOWEVENT &&
            IsResizeEvent(event->window.event)
        )
        {
            MSDL::SDL_Window * sdlWindow = MSDL::SDL_GetWindowFromID(event->window.windowID);
            if (sdlWindow == static_cast<MSDL::SDL_Window *>(data))
            {
                state->windowResized = true;
            }
        }
        for (auto & eventListener : state->sdlEventListeners)
        {
            MFA_ASSERT(eventListener.watch != nullptr);
            eventListener.watch(data, event);
        }
        return 0;
    }
#endif

    //-------------------------------------------------------------------------------------------------

    [[nodiscard]]
    static VkSurfaceCapabilitiesKHR computeSurfaceCapabilities()
    {
        return RB::GetSurfaceCapabilities(state->physicalDevice, state->surface);
    }

    //-------------------------------------------------------------------------------------------------

    bool Init(InitParams const & params)
    {
        state = new State();
        state->application_name = params.applicationName;
#ifdef __DESKTOP__
        state->window = RB::CreateWindow(
            params.screenWidth,
            params.screenHeight
        );
        MFA_ASSERT(state->window != nullptr);
        state->isWindowResizable = params.resizable;

        if (params.resizable)
        {
            // Make window resizable
            MSDL::SDL_SetWindowResizable(state->window, MSDL::SDL_TRUE);
        }

        MSDL::SDL_SetWindowMinimumSize(state->window, 100, 100);

        MSDL::SDL_AddEventWatch(SDLEventWatcher, state->window);

#elif defined(__ANDROID__)
        state->window = params.app->window;
#elif defined(__IOS__)
        state->window = params.view;
#else
#error "Os is not supported"
#endif

#ifdef __DESKTOP__
        state->vk_instance = RB::CreateInstance(
            state->application_name.c_str(),
            state->window
        );
#elif defined(__ANDROID__) || defined(__IOS__)
        state->vk_instance = RB::CreateInstance(state->application_name.c_str());
#else
#error "Os is not supported"
#endif
        MFA_VK_VALID_ASSERT(state->vk_instance);

#if defined(MFA_DEBUG)  // TODO Fix support for android
        state->vkDebugReportCallbackExt = RB::CreateDebugCallback(
            state->vk_instance,
            DebugCallback
        );
        MFA_LOG_INFO("Validation layers are enabled");
#endif

        state->surface = RB::CreateWindowSurface(state->window, state->vk_instance);

        {// FindPhysicalDevice
            auto const findPhysicalDeviceResult = RB::FindPhysicalDevice(state->vk_instance);   // TODO Check again for retry count number
            state->physicalDevice = findPhysicalDeviceResult.physicalDevice;
            // I'm not sure if this is a correct thing to do but currently I'm enabling all gpu features.
            state->physicalDeviceFeatures = findPhysicalDeviceResult.physicalDeviceFeatures;
            state->maxSampleCount = VK_SAMPLE_COUNT_2_BIT;//findPhysicalDeviceResult.maxSampleCount;                    // TODO It should be a setting
            state->physicalDeviceProperties = findPhysicalDeviceResult.physicalDeviceProperties;

            std::string message = "Supported physical device features are:";
            message += "\nSample rate shading support: ";
            message += state->physicalDeviceFeatures.sampleRateShading ? "True" : "False";
            message += "\nSampler anisotropy support: ";
            message += state->physicalDeviceFeatures.samplerAnisotropy ? "True" : "False";
            MFA_LOG_INFO("%s", message.c_str());
        }

        // Find surface capabilities
        state->surfaceCapabilities = computeSurfaceCapabilities();

        state->swapChainImageCount = RB::ComputeSwapChainImagesCount(state->surfaceCapabilities);
        state->maxFramesPerFlight = state->swapChainImageCount;

        state->screenWidth = static_cast<ScreenWidth>(state->surfaceCapabilities.currentExtent.width);
        state->screenHeight = static_cast<ScreenHeight>(state->surfaceCapabilities.currentExtent.height);
        MFA_LOG_INFO("ScreenWidth: %f \nScreenHeight: %f", static_cast<float>(state->screenWidth), static_cast<float>(state->screenHeight));

        if (RB::CheckSwapChainSupport(state->physicalDevice) == false)
        {
            MFA_LOG_ERROR("Swapchain is not supported on this device");
            return false;
        }

        {// Trying to find queue family
            auto const result = RB::FindQueueFamilies(state->physicalDevice, state->surface);
            state->graphicQueueFamily = result.graphicQueueFamily;
            state->computeQueueFamily = result.computeQueueFamily;
            state->presentQueueFamily = result.presentQueueFamily;
        }

        state->logicalDevice = RB::CreateLogicalDevice(
            state->physicalDevice,
            state->graphicQueueFamily,
            state->presentQueueFamily,
            state->physicalDeviceFeatures
        );

        // Get graphics and presentation queues (which may be the same)
        state->graphicQueue = RB::GetQueueByFamilyIndex(
            state->logicalDevice.device,
            state->graphicQueueFamily
        );
        MFA_VK_VALID_ASSERT(state->graphicQueue);

        state->computeQueue = RB::GetQueueByFamilyIndex(
            state->logicalDevice.device,
            state->computeQueueFamily
        );
        MFA_VK_VALID_ASSERT(state->computeQueue);

        state->presentQueue = RB::GetQueueByFamilyIndex(
            state->logicalDevice.device,
            state->presentQueueFamily
        );
        MFA_VK_VALID_ASSERT(state->presentQueue);

        MFA_LOG_INFO("Acquired graphics, compute and presentation queues");

        auto const maxFramePerFlight = GetMaxFramesPerFlight();
        // Graphic 
        state->graphicCommandPool = RB::CreateCommandPool(state->logicalDevice.device, state->graphicQueueFamily);
        state->graphicCommandBuffer = RB::CreateCommandBuffers(
            state->logicalDevice.device,
            maxFramePerFlight,
            state->graphicCommandPool
        );
        state->graphicSemaphores = RB::CreateSemaphores(
            state->logicalDevice.device,
            maxFramePerFlight
        );
        // Compute
        state->computeCommandPool = RB::CreateCommandPool(state->logicalDevice.device, state->computeQueueFamily);
        state->computeCommandBuffer = RB::CreateCommandBuffers(
            state->logicalDevice.device,
            maxFramePerFlight,
            state->computeCommandPool
        );
        state->computeSemaphores = RB::CreateSemaphores(
            state->logicalDevice.device,
            maxFramePerFlight
        );
        // Presentation
        state->fences = RB::CreateFence(
            state->logicalDevice.device,
            maxFramePerFlight
        );
        state->presentSemaphores = RB::CreateSemaphores(
            state->logicalDevice.device,
            maxFramePerFlight
        );

        state->depthFormat = RB::FindDepthFormat(state->physicalDevice);

        state->displayRenderPass.Init();

        return true;
    }

    //-------------------------------------------------------------------------------------------------

    static void OnResize()
    {
        DeviceWaitIdle();

        state->surfaceCapabilities = computeSurfaceCapabilities();

        state->screenWidth = static_cast<ScreenWidth>(state->surfaceCapabilities.currentExtent.width);
        state->screenHeight = static_cast<ScreenHeight>(state->surfaceCapabilities.currentExtent.height);

        // Screen width and height can be equal to zero as well
        state->windowResized = false;

        if (state->screenWidth > 0 && state->screenHeight > 0)
        {
            // We need display render pass to resize before everything else
            state->displayRenderPass.OnResize();
            state->resizeEventSignal.Emit();
        }
    }

    //-------------------------------------------------------------------------------------------------

    bool Shutdown()
    {
        // Common part with resize
        DeviceWaitIdle();

        MFA_ASSERT(state->resizeEventSignal.IsEmpty());

#ifdef __DESKTOP__
        MFA_ASSERT(state->sdlEventListeners.empty());
        MSDL::SDL_DelEventWatch(SDLEventWatcher, state->window);
#endif

        state->displayRenderPass.Shutdown();

        // Graphic
        RB::DestroySemaphored(
            state->logicalDevice.device,
            state->graphicSemaphores
        );
        RB::DestroyCommandBuffers(
            state->logicalDevice.device,
            state->graphicCommandPool,
            static_cast<uint32_t>(state->graphicCommandBuffer.size()),
            state->graphicCommandBuffer.data()
        );
        RB::DestroyCommandPool(
            state->logicalDevice.device,
            state->graphicCommandPool
        );
        // Compute
        RB::DestroySemaphored(
            state->logicalDevice.device,
            state->computeSemaphores
        );
        RB::DestroyCommandBuffers(
            state->logicalDevice.device,
            state->computeCommandPool,
            static_cast<uint32_t>(state->computeCommandBuffer.size()),
            state->computeCommandBuffer.data()
        );
        RB::DestroyCommandPool(
            state->logicalDevice.device,
            state->computeCommandPool
        );
        // Presentation
        RB::DestroySemaphored(
            state->logicalDevice.device,
            state->presentSemaphores
        );
        RB::DestroyFence(
            state->logicalDevice.device,
            state->fences
        );

        RB::DestroyLogicalDevice(state->logicalDevice);

        RB::DestroyWindowSurface(state->vk_instance, state->surface);

#ifdef MFA_DEBUG
        RB::DestroyDebugReportCallback(state->vk_instance, state->vkDebugReportCallbackExt);
#endif

        RB::DestroyInstance(state->vk_instance);

        delete state;

        return true;
    }

    //-------------------------------------------------------------------------------------------------

    int AddResizeEventListener(RT::ResizeEventListener const & eventListener)
    {
        MFA_ASSERT(eventListener != nullptr);
        return state->resizeEventSignal.Register(eventListener);
    }

    //-------------------------------------------------------------------------------------------------

    bool RemoveResizeEventListener(RT::ResizeEventListenerId const listenerId)
    {
        return state->resizeEventSignal.UnRegister(listenerId);
    }

    //-------------------------------------------------------------------------------------------------

    std::shared_ptr<RT::DescriptorSetLayoutGroup> CreateDescriptorSetLayout(
        uint8_t const bindingsCount,
        VkDescriptorSetLayoutBinding * bindings
    )
    {
        auto descriptorSetLayout = RB::CreateDescriptorSetLayout(
            state->logicalDevice.device,
            bindingsCount,
            bindings
        );
        MFA_VK_VALID_ASSERT(descriptorSetLayout);
        return descriptorSetLayout;
    }

    //-------------------------------------------------------------------------------------------------

    void DestroyDescriptorSetLayout(VkDescriptorSetLayout descriptorSetLayout)
    {
        MFA_ASSERT(descriptorSetLayout);
        RB::DestroyDescriptorSetLayout(state->logicalDevice.device, descriptorSetLayout);
    }

    //-------------------------------------------------------------------------------------------------

    [[nodiscard]]
    std::shared_ptr<RT::PipelineGroup> CreatePipeline(
        VkRenderPass vkRenderPass,
        uint8_t gpuShadersCount,
        RT::GpuShader const ** gpuShaders,
        uint32_t const descriptorLayoutsCount,
        VkDescriptorSetLayout const * descriptorSetLayouts,
        uint32_t const vertexBindingDescriptionCount,
        VkVertexInputBindingDescription const * vertexBindingDescriptionData,
        uint32_t const inputAttributeDescriptionCount,
        VkVertexInputAttributeDescription * inputAttributeDescriptionData,
        RT::CreateGraphicPipelineOptions const & options
    )
    {

        VkExtent2D const extent2D{
            .width = static_cast<uint32_t>(state->screenWidth),
            .height = static_cast<uint32_t>(state->screenHeight),
        };

        return RB::CreatePipelineGroup(
            state->logicalDevice.device,
            gpuShadersCount,
            gpuShaders,
            vertexBindingDescriptionCount,
            vertexBindingDescriptionData,
            inputAttributeDescriptionCount,
            inputAttributeDescriptionData,
            extent2D,
            vkRenderPass,
            descriptorLayoutsCount,
            descriptorSetLayouts,
            options
        );
    }

    //-------------------------------------------------------------------------------------------------

    void DestroyPipelineGroup(RT::PipelineGroup & recordState)
    {
        RB::DestroyPipelineGroup(
            state->logicalDevice.device,
            recordState
        );
    }

    //-------------------------------------------------------------------------------------------------

    RT::DescriptorSetGroup CreateDescriptorSets(
        VkDescriptorPool descriptorPool,
        uint32_t const descriptorSetCount,
        RT::DescriptorSetLayoutGroup const & descriptorSetLayout
    )
    {
        return RB::CreateDescriptorSet(
            state->logicalDevice.device,
            descriptorPool,
            descriptorSetLayout.descriptorSetLayout,
            descriptorSetCount
        );
    }

    //-------------------------------------------------------------------------------------------------

    RT::DescriptorSetGroup CreateDescriptorSets(
        VkDescriptorPool descriptorPool,
        uint32_t const descriptorSetCount,
        VkDescriptorSetLayout descriptorSetLayout
    )
    {
        MFA_VK_VALID_ASSERT(descriptorSetLayout);
        return RB::CreateDescriptorSet(
            state->logicalDevice.device,
            descriptorPool,
            descriptorSetLayout,
            descriptorSetCount
        );
    }

    //-------------------------------------------------------------------------------------------------

    std::shared_ptr<RT::UniformBufferGroup> CreateUniformBuffer(
        size_t const bufferSize,
        uint32_t const count
    )
    {
        std::vector<std::shared_ptr<RT::BufferAndMemory>> buffers(count);
        RB::CreateUniformBuffer(
            state->logicalDevice.device,
            state->physicalDevice,
            count,
            bufferSize,
            buffers.data()
        );
        auto uniformBufferGroup = std::make_shared<RT::UniformBufferGroup>(
           std::move(buffers),
            bufferSize
        );
        return uniformBufferGroup;
    }

    //-------------------------------------------------------------------------------------------------

    void UpdateUniformBuffer(
        RT::CommandRecordState const & recordState,
        RT::UniformBufferGroup const & bufferCollection,
        CBlob const data
    )
    {
        UpdateBuffer(*bufferCollection.buffers[recordState.frameIndex], data);
    }

    //-------------------------------------------------------------------------------------------------

    std::shared_ptr<RT::StorageBufferCollection> CreateStorageBuffer(
        size_t const bufferSize,
        uint32_t const count
    )
    {
        std::vector<std::shared_ptr<RT::BufferAndMemory>> buffers(count);

        RB::CreateStorageBuffer(
            state->logicalDevice.device,
            state->physicalDevice,
            count,
            bufferSize,
            buffers.data()
        );

        auto uniformBufferGroup = std::make_shared<RT::StorageBufferCollection>(
            std::move(buffers),
            bufferSize
        );

        return uniformBufferGroup;
    }

    //-------------------------------------------------------------------------------------------------

    void UpdateBuffer(RT::BufferAndMemory const & buffer, CBlob data)
    {
        RB::UpdateBuffer(
            state->logicalDevice.device,
            buffer,
            data
        );
    }

    //-------------------------------------------------------------------------------------------------

    void CreateVertexBuffer(
        CBlob const & verticesBlob,
        std::shared_ptr<RT::BufferAndMemory> & outVertexBuffer,
        std::shared_ptr<RT::BufferAndMemory> & inOutStagingVertexBuffer
    )
    {
        RB::CreateVertexBuffer(
            state->logicalDevice.device,
            state->physicalDevice,
            state->graphicCommandPool,
            state->graphicQueue,
            verticesBlob,
            outVertexBuffer,
            inOutStagingVertexBuffer
        );
    }

    //-------------------------------------------------------------------------------------------------

    void UpdateVertexBuffer(
        CBlob const & verticesBlob,
        RT::BufferAndMemory const & vertexBuffer,
        RT::BufferAndMemory const & stagingBuffer
    )
    {
        RB::UpdateVertexBuffer(
            state->logicalDevice.device,
            state->graphicCommandPool,
            state->graphicQueue,
            verticesBlob,
            vertexBuffer,
            stagingBuffer
        );
    }

    //-------------------------------------------------------------------------------------------------

    void CreateIndexBuffer(
        CBlob const & indicesBlob,
        std::shared_ptr<RT::BufferAndMemory> & outIndexBuffer,
        std::shared_ptr<RT::BufferAndMemory> & inOutStagingIndexBuffer
    )
    {
        RB::CreateIndexBuffer(
            state->logicalDevice.device,
            state->physicalDevice,
            state->graphicCommandPool,
            state->graphicQueue,
            indicesBlob,
            outIndexBuffer,
            inOutStagingIndexBuffer
        );
    }

    //-------------------------------------------------------------------------------------------------

    std::shared_ptr<RT::MeshBuffers> CreateMeshBuffers(AS::MeshBase const & mesh)
    {
        MFA_ASSERT(mesh.isValid());

        std::vector<std::shared_ptr<RT::BufferAndMemory>> vertexBuffers{};
        MFA_ASSERT(mesh.requiredVertexBufferCount > 0);

        std::shared_ptr<RT::BufferAndMemory> vertexStagingBuffer = nullptr;
        for (uint32_t i = 0; i < mesh.requiredVertexBufferCount; ++i)
        {
            vertexBuffers.emplace_back(std::shared_ptr<RT::BufferAndMemory> {});
            CreateVertexBuffer(
                mesh.getVertexBuffer()->memory,
                vertexBuffers.back(),
                vertexStagingBuffer
            );
        }

        std::shared_ptr<RT::BufferAndMemory> indexBuffer = nullptr;
        std::shared_ptr<RT::BufferAndMemory> indexStagingBuffer = nullptr;

        CreateIndexBuffer(
            mesh.getIndexBuffer()->memory,
            indexBuffer,
            indexStagingBuffer
        );

        return std::make_shared<RT::MeshBuffers>(
            vertexBuffers,
            indexBuffer,
            mesh.keepVertexStagingBuffer ? vertexStagingBuffer : nullptr,
            mesh.keepIndexStagingBuffer ? indexStagingBuffer : nullptr
        );
    }

    //-------------------------------------------------------------------------------------------------

    std::shared_ptr<RT::GpuTexture> CreateTexture(AS::Texture const & texture)
    {
        auto gpuTexture = RB::CreateTexture(
            texture,
            state->logicalDevice.device,
            state->physicalDevice,
            state->graphicQueue,
            state->graphicCommandPool
        );
        MFA_ASSERT(gpuTexture != nullptr);
        return gpuTexture;
    }

    //-------------------------------------------------------------------------------------------------

    void DestroyImage(RT::ImageGroup const & imageGroup)
    {
        DeviceWaitIdle();
        RB::DestroyImage(
            state->logicalDevice.device,
            imageGroup
        );
    }

    //-------------------------------------------------------------------------------------------------

    void DestroyImageView(RT::ImageViewGroup const & imageViewGroup)
    {
        DeviceWaitIdle();
        RB::DestroyImageView(
            state->logicalDevice.device,
            imageViewGroup
        );
    }

    //-------------------------------------------------------------------------------------------------

    // TODO Ask for options
    std::shared_ptr<RT::SamplerGroup> CreateSampler(RT::CreateSamplerParams const & samplerParams)
    {
        return RB::CreateSampler(state->logicalDevice.device, samplerParams);
    }

    //-------------------------------------------------------------------------------------------------

    void DestroySampler(RT::SamplerGroup const & samplerGroup)
    {
        DeviceWaitIdle();
        RB::DestroySampler(state->logicalDevice.device, samplerGroup);
    }

    //-------------------------------------------------------------------------------------------------

    std::shared_ptr<RT::GpuModel> CreateGpuModel(
        AS::Model const * modelAsset,
        std::string const & address
    )
    {
        MFA_ASSERT(modelAsset->mesh->isValid());
        auto meshBuffers = CreateMeshBuffers(*modelAsset->mesh);
        std::vector<std::shared_ptr<RT::GpuTexture>> textures{};
        for (auto & textureAsset : modelAsset->textures)
        {
            MFA_ASSERT(textureAsset->isValid());
            textures.emplace_back(CreateTexture(*textureAsset));
        }
        return std::make_shared<RT::GpuModel>(
            address,
            std::move(meshBuffers),
            std::move(textures)
        );
    }

    //-------------------------------------------------------------------------------------------------

    void DeviceWaitIdle()
    {
        RB::DeviceWaitIdle(state->logicalDevice.device);
    }

    //-------------------------------------------------------------------------------------------------

    VkFormat GetDepthFormat()
    {
        return state->depthFormat;
    }

    //-------------------------------------------------------------------------------------------------

    void DestroyBuffer(RT::BufferAndMemory const & bufferGroup)
    {
        DeviceWaitIdle();
        RB::DestroyBuffer(state->logicalDevice.device, bufferGroup);
    }

    //-------------------------------------------------------------------------------------------------

    void BindPipeline(
        RT::CommandRecordState & drawPass,
        RT::PipelineGroup & pipeline
    )
    {
        MFA_ASSERT(drawPass.isValid);
        drawPass.pipeline = &pipeline;
        // We can bind command buffer to multiple pipeline
        vkCmdBindPipeline(
            GetGraphicCommandBuffer(drawPass),
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            pipeline.pipeline
        );
    }

    //-------------------------------------------------------------------------------------------------

    void UpdateDescriptorSets(
        uint32_t const writeInfoCount,
        VkWriteDescriptorSet * writeInfo
    )
    {
        RB::UpdateDescriptorSets(
            state->logicalDevice.device,
            writeInfoCount,
            writeInfo
        );
    }

    //-------------------------------------------------------------------------------------------------

    void UpdateDescriptorSets(
        uint8_t const descriptorSetsCount,
        VkDescriptorSet * descriptorSets,
        uint8_t const writeInfoCount,
        VkWriteDescriptorSet * writeInfo
    )
    {
        RB::UpdateDescriptorSets(
            state->logicalDevice.device,
            descriptorSetsCount,
            descriptorSets,
            writeInfoCount,
            writeInfo
        );
    }

    //-------------------------------------------------------------------------------------------------

    void BindDescriptorSet(
        RT::CommandRecordState const & drawPass,
        DescriptorSetType frequency,
        RT::DescriptorSetGroup descriptorSetGroup
    )
    {
        MFA_ASSERT(drawPass.isValid);
        MFA_ASSERT(drawPass.pipeline);
        // We should bind specific descriptor set with different texture for each mesh
        vkCmdBindDescriptorSets(
            GetGraphicCommandBuffer(drawPass),
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            drawPass.pipeline->pipelineLayout,
            static_cast<uint32_t>(frequency),
            1,
            &descriptorSetGroup.descriptorSets[drawPass.frameIndex],
            0,
            nullptr
        );
    }

    //-------------------------------------------------------------------------------------------------

    void BindVertexBuffer(
        RT::CommandRecordState const & drawPass,
        RT::BufferAndMemory const & vertexBuffer,
        uint32_t const firstBinding,
        VkDeviceSize const offset
    )
    {
        MFA_ASSERT(drawPass.isValid);
        RB::BindVertexBuffer(
            GetGraphicCommandBuffer(drawPass),
            vertexBuffer,
            firstBinding,
            offset
        );
    }

    //-------------------------------------------------------------------------------------------------

    void BindIndexBuffer(
        RT::CommandRecordState const & drawPass,
        RT::BufferAndMemory const & indexBuffer,
        VkDeviceSize const offset,
        VkIndexType const indexType
    )
    {
        MFA_ASSERT(drawPass.isValid);
        RB::BindIndexBuffer(
            GetGraphicCommandBuffer(drawPass),
            indexBuffer,
            offset,
            indexType
        );
    }

    //-------------------------------------------------------------------------------------------------

    void DrawIndexed(
        RT::CommandRecordState const & drawPass,
        uint32_t const indicesCount,
        uint32_t const instanceCount,
        uint32_t const firstIndex,
        uint32_t const vertexOffset,
        uint32_t const firstInstance
    )
    {
        MFA_ASSERT(drawPass.isValid);
        RB::DrawIndexed(
            GetGraphicCommandBuffer(drawPass),
            indicesCount,
            instanceCount,
            firstIndex,
            vertexOffset,
            firstInstance
        );
    }

    //-------------------------------------------------------------------------------------------------

    void BeginRenderPass(
        VkCommandBuffer commandBuffer,
        VkRenderPass renderPass,
        VkFramebuffer frameBuffer,
        VkExtent2D const & extent2D,
        uint32_t clearValuesCount,
        VkClearValue const * clearValues
    )
    {
        VkRenderPassBeginInfo renderPassBeginInfo = {};
        renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassBeginInfo.renderPass = renderPass;
        renderPassBeginInfo.framebuffer = frameBuffer;
        renderPassBeginInfo.renderArea.offset.x = 0;
        renderPassBeginInfo.renderArea.offset.y = 0;
        renderPassBeginInfo.renderArea.extent.width = extent2D.width;
        renderPassBeginInfo.renderArea.extent.height = extent2D.height;
        renderPassBeginInfo.clearValueCount = clearValuesCount;
        renderPassBeginInfo.pClearValues = clearValues;

        RB::BeginRenderPass(commandBuffer, renderPassBeginInfo);
    }

    //-------------------------------------------------------------------------------------------------

    void EndRenderPass(VkCommandBuffer commandBuffer)
    {
        RB::EndRenderPass(commandBuffer);
    }

    //-------------------------------------------------------------------------------------------------

    void OnNewFrame(float deltaTimeInSec)
    {
#ifdef __DESKTOP__
        auto const isWindowVisible = (GetWindowFlags() & MSDL::SDL_WINDOW_MINIMIZED) > 0 ? false : true;
        if (state->isWindowVisible != isWindowVisible)
        {
            if (isWindowVisible)
            {
                OnResize();
            }
            state->isWindowVisible = isWindowVisible;
        }
#endif
        if (state->windowResized)
        {
            OnResize();
        }

    }

    //-------------------------------------------------------------------------------------------------

    [[nodiscard]]
    std::shared_ptr<RT::GpuShader> CreateShader(std::shared_ptr<AS::Shader> const & shader)
    {
        return RB::CreateShader(
            state->logicalDevice.device,
            shader
        );
    }

    //-------------------------------------------------------------------------------------------------

    void DestroyShader(RT::GpuShader const & gpuShader)
    {
        RB::DestroyShader(state->logicalDevice.device, gpuShader);
    }

    //-------------------------------------------------------------------------------------------------

    void SetScissor(RT::CommandRecordState const & drawPass, VkRect2D const & scissor)
    {
        MFA_ASSERT(drawPass.isValid);
        MFA_ASSERT(drawPass.renderPass != nullptr);
        RB::SetScissor(GetGraphicCommandBuffer(drawPass), scissor);
    }

    //-------------------------------------------------------------------------------------------------

    void SetViewport(RT::CommandRecordState const & drawPass, VkViewport const & viewport)
    {
        MFA_ASSERT(drawPass.isValid);
        MFA_ASSERT(drawPass.renderPass != nullptr);
        RB::SetViewport(GetGraphicCommandBuffer(drawPass), viewport);
    }

    //-------------------------------------------------------------------------------------------------

    void PushConstants(
        RT::CommandRecordState const & drawPass,
        AssetSystem::ShaderStage const shaderStage,
        uint32_t const offset,
        CBlob const data
    )
    {
        MFA_ASSERT(drawPass.isValid);
        MFA_ASSERT(drawPass.renderPass != nullptr);
        RB::PushConstants(
            GetGraphicCommandBuffer(drawPass),
            drawPass.pipeline->pipelineLayout,
            shaderStage,
            offset,
            data
        );
    }

    //-------------------------------------------------------------------------------------------------

    void PushConstants(
        RT::CommandRecordState const & drawPass,
        VkShaderStageFlags const shaderStage,
        uint32_t const offset,
        CBlob const data
    )
    {
        MFA_ASSERT(drawPass.isValid);
        MFA_ASSERT(drawPass.renderPass != nullptr);
        RB::PushConstants(
            GetGraphicCommandBuffer(drawPass),
            drawPass.pipeline->pipelineLayout,
            shaderStage,
            offset,
            data
        );
    }

    //-------------------------------------------------------------------------------------------------

    uint32_t GetSwapChainImagesCount()
    {
        return state->swapChainImageCount;
    }

    //-------------------------------------------------------------------------------------------------

    uint32_t GetMaxFramesPerFlight()
    {
        return state->maxFramesPerFlight;
    }

    //-------------------------------------------------------------------------------------------------

    void GetDrawableSize(int32_t & outWidth, int32_t & outHeight)
    {
        outWidth = state->screenWidth;
        outHeight = state->screenHeight;
    }

#ifdef __DESKTOP__
    // SDL functions

    //-------------------------------------------------------------------------------------------------

    void WarpMouseInWindow(int32_t const x, int32_t const y)
    {
        MSDL::SDL_WarpMouseInWindow(state->window, x, y);
    }

    //-------------------------------------------------------------------------------------------------

    uint32_t GetMouseState(int32_t * x, int32_t * y)
    {
        return MSDL::SDL_GetMouseState(x, y);
    }

    //-------------------------------------------------------------------------------------------------

    uint8_t const * GetKeyboardState(int * numKeys)
    {
        return MSDL::SDL_GetKeyboardState(numKeys);
    }

    //-------------------------------------------------------------------------------------------------

    uint32_t GetWindowFlags()
    {
        return MSDL::SDL_GetWindowFlags(state->window);
    }

#endif

    //-------------------------------------------------------------------------------------------------

    void AssignViewportAndScissorToCommandBuffer(VkCommandBuffer commandBuffer, VkExtent2D const & extent2D)
    {
        RB::AssignViewportAndScissorToCommandBuffer(
            extent2D,
            commandBuffer
        );
    }

    //-------------------------------------------------------------------------------------------------

#ifdef __DESKTOP__
// TODO We might need separate SDL class
    int AddEventWatch(SDLEventWatch const & eventWatch)
    {
        SDLEventWatchGroup const group{
            .id = state->nextEventListenerId,
            .watch = eventWatch,
        };

        ++state->nextEventListenerId;
        state->sdlEventListeners.emplace_back(group);
        return group.id;
    }

    //-------------------------------------------------------------------------------------------------

    void RemoveEventWatch(SDLEventWatchId const watchId)
    {
        for (int i = static_cast<int>(state->sdlEventListeners.size()) - 1; i >= 0; --i)
        {
            if (state->sdlEventListeners[i].id == watchId)
            {
                state->sdlEventListeners.erase(state->sdlEventListeners.begin() + i);
            }
        }
    }

    //-------------------------------------------------------------------------------------------------

#endif

    void NotifyDeviceResized()
    {
        state->windowResized = true;
    }

    //-------------------------------------------------------------------------------------------------

    VkSurfaceCapabilitiesKHR GetSurfaceCapabilities()
    {
        return state->surfaceCapabilities;
    }

    //-------------------------------------------------------------------------------------------------

    std::shared_ptr<RT::SwapChainGroup> CreateSwapChain(VkSwapchainKHR oldSwapChain)
    {
        return RB::CreateSwapChain(
            state->logicalDevice.device,
            state->physicalDevice,
            state->surface,
            state->surfaceCapabilities,
            oldSwapChain
        );
    }

    //-------------------------------------------------------------------------------------------------

    void DestroySwapChain(RT::SwapChainGroup const & swapChainGroup)
    {
        DeviceWaitIdle();
        RB::DestroySwapChain(state->logicalDevice.device, swapChainGroup);
    }

    //-------------------------------------------------------------------------------------------------

    std::shared_ptr<RT::ColorImageGroup> CreateColorImage(
        VkExtent2D const & imageExtent,
        VkFormat const imageFormat,
        RT::CreateColorImageOptions const & options
    )
    {
        return RB::CreateColorImage(
            state->physicalDevice,
            state->logicalDevice.device,
            imageExtent,
            imageFormat,
            options
        );
    }

    //-------------------------------------------------------------------------------------------------

    VkRenderPass CreateRenderPass(
        VkAttachmentDescription * attachments,
        uint32_t const attachmentsCount,
        VkSubpassDescription * subPasses,
        uint32_t const subPassesCount,
        VkSubpassDependency * dependencies,
        uint32_t const dependenciesCount
    )
    {
        return RB::CreateRenderPass(
            state->logicalDevice.device,
            attachments,
            attachmentsCount,
            subPasses,
            subPassesCount,
            dependencies,
            dependenciesCount
        );
    }

    //-------------------------------------------------------------------------------------------------

    void DestroyRenderPass(VkRenderPass renderPass)
    {
        DeviceWaitIdle();
        RB::DestroyRenderPass(state->logicalDevice.device, renderPass);
    }

    //-------------------------------------------------------------------------------------------------

    [[nodiscard]]
    std::shared_ptr<RT::DepthImageGroup> CreateDepthImage(
        VkExtent2D const & imageSize,
        RT::CreateDepthImageOptions const & options
    )
    {
        return RB::CreateDepthImage(
            state->physicalDevice,
            state->logicalDevice.device,
            imageSize,
            state->depthFormat,
            options
        );
    }

    //-------------------------------------------------------------------------------------------------

    VkFramebuffer CreateFrameBuffer(
        VkRenderPass renderPass,
        VkImageView const * attachments,
        uint32_t const attachmentsCount,
        VkExtent2D const & frameBufferExtent,
        uint32_t const layersCount
    )
    {
        return RB::CreateFrameBuffers(
            state->logicalDevice.device,
            renderPass,
            attachments,
            attachmentsCount,
            frameBufferExtent,
            layersCount
        );
    }

    //-------------------------------------------------------------------------------------------------

    void DestroyFrameBuffers(
        uint32_t const frameBuffersCount,
        VkFramebuffer * frameBuffers
    )
    {
        DeviceWaitIdle();
        RB::DestroyFrameBuffers(
            state->logicalDevice.device,
            frameBuffersCount,
            frameBuffers
        );
    }

    //-------------------------------------------------------------------------------------------------

#ifdef __DESKTOP__
    bool IsWindowVisible()
    {
        return state->isWindowVisible;
    }
#endif

    //-------------------------------------------------------------------------------------------------

    bool IsWindowResized()
    {
        return state->windowResized;
    }

    //-------------------------------------------------------------------------------------------------

    void WaitForFence(VkFence fence)
    {
        RB::WaitForFence(
            state->logicalDevice.device,
            fence
        );
    }

    //-------------------------------------------------------------------------------------------------

    void AcquireNextImage(
        VkSemaphore imageAvailabilitySemaphore,
        RT::SwapChainGroup const & swapChainGroup,
        uint32_t & outImageIndex
    )
    {
        RB::AcquireNextImage(
            state->logicalDevice.device,
            imageAvailabilitySemaphore,
            swapChainGroup,
            outImageIndex
        );
    }

    //-------------------------------------------------------------------------------------------------

    void BeginCommandBuffer(
        VkCommandBuffer commandBuffer,
        VkCommandBufferBeginInfo const & beginInfo
    )
    {
        RB::BeginCommandBuffer(
            commandBuffer,
            beginInfo
        );
    }

    //-------------------------------------------------------------------------------------------------

    void EndCommandBuffer(VkCommandBuffer commandBuffer)
    {
        RB::EndCommandBuffer(commandBuffer);
    }

    //-------------------------------------------------------------------------------------------------

    uint32_t GetPresentQueueFamily()
    {
        return state->presentQueueFamily;
    }

    //-------------------------------------------------------------------------------------------------

    uint32_t GetGraphicQueueFamily()
    {
        return state->graphicQueueFamily;
    }

    //-------------------------------------------------------------------------------------------------

    void PipelineBarrier(
        VkCommandBuffer commandBuffer,
        VkPipelineStageFlags sourceStageMask,
        VkPipelineStageFlags destinationStateMask,
        uint32_t barrierCount,
        VkImageMemoryBarrier const * memoryBarriers
    )
    {
        RB::PipelineBarrier(
            commandBuffer,
            sourceStageMask,
            destinationStateMask,
            barrierCount,
            memoryBarriers
        );
    }

    //-------------------------------------------------------------------------------------------------

    void PipelineBarrier(
        VkCommandBuffer commandBuffer,
        VkPipelineStageFlags sourceStageMask,
        VkPipelineStageFlags destinationStateMask,
        VkBufferMemoryBarrier const & bufferMemoryBarrier
    )
    {
        RB::PipelineBarrier(
            commandBuffer,
            sourceStageMask,
            destinationStateMask,
            bufferMemoryBarrier
        );
    }

    //-------------------------------------------------------------------------------------------------

    void ResetFence(VkFence fence)
    {
        RB::ResetFences(
            state->logicalDevice.device,
            1,
            &fence
        );
    }

    //-------------------------------------------------------------------------------------------------

    void SubmitQueue(
        VkCommandBuffer commandBuffer,
        uint32_t waitSemaphoresCount,
        const VkSemaphore * waitSemaphores,
        const VkPipelineStageFlags * waitStageFlags,
        uint32_t signalSemaphoresCount,
        const VkSemaphore * signalSemaphores,
        VkFence fence
    )
    {
        VkSubmitInfo submitInfo = {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        submitInfo.waitSemaphoreCount = waitSemaphoresCount;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStageFlags;

        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;

        submitInfo.signalSemaphoreCount = signalSemaphoresCount;
        submitInfo.pSignalSemaphores = signalSemaphores;

        RB::SubmitQueues(
            state->graphicQueue,
            1,
            &submitInfo,
            fence
        );
    }

    //-------------------------------------------------------------------------------------------------

    void PresentQueue(
        uint32_t const imageIndex,
        VkSemaphore waitSemaphore,
        VkSwapchainKHR swapChain
    )
    {
        // Present drawn image
        // Note: semaphore here is not strictly necessary, because commands are processed in submission order within a single queue
        VkPresentInfoKHR presentInfo = {};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = &waitSemaphore;
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = &swapChain;
        presentInfo.pImageIndices = &imageIndex;

        // TODO Move to renderBackend
        auto const res = vkQueuePresentKHR(state->presentQueue, &presentInfo);
        if (res == VK_SUBOPTIMAL_KHR || res == VK_ERROR_OUT_OF_DATE_KHR || state->windowResized == true)
        {
            OnResize();
        }
        else if (res != VK_SUCCESS)
        {
            MFA_CRASH("Failed to submit present command buffer");
        }
    }

    //-------------------------------------------------------------------------------------------------

    DisplayRenderPass * GetDisplayRenderPass()
    {
        return &state->displayRenderPass;
    }

    //-------------------------------------------------------------------------------------------------

    /*void DestroySyncObjects(RT::SyncObjects const & syncObjects)
    {
        DeviceWaitIdle();
        RB::DestroySyncObjects(state->logicalDevice.device, syncObjects);
    }*/

    //-------------------------------------------------------------------------------------------------

    VkSampleCountFlagBits GetMaxSamplesCount()
    {
        return state->maxSampleCount;
    }

    //-------------------------------------------------------------------------------------------------

    void WaitForGraphicQueue()
    {
        RB::WaitForQueue(state->graphicQueue);
    }

    //-------------------------------------------------------------------------------------------------

    void WaitForPresentQueue()
    {
        RB::WaitForQueue(state->presentQueue);
    }

    //-------------------------------------------------------------------------------------------------

    VkQueryPool CreateQueryPool(VkQueryPoolCreateInfo const & createInfo)
    {
        return RB::CreateQueryPool(state->logicalDevice.device, createInfo);
    }

    //-------------------------------------------------------------------------------------------------

    void DestroyQueryPool(VkQueryPool queryPool)
    {
        RB::DestroyQueryPool(state->logicalDevice.device, queryPool);
    }

    //-------------------------------------------------------------------------------------------------

    void BeginQuery(
        RT::CommandRecordState const & drawPass,
        VkQueryPool queryPool,
        uint32_t const queryId
    )
    {
        RB::BeginQuery(GetGraphicCommandBuffer(drawPass), queryPool, queryId);
    }

    //-------------------------------------------------------------------------------------------------

    void EndQuery(
        RT::CommandRecordState const & drawPass,
        VkQueryPool queryPool,
        uint32_t queryId
    )
    {
        RB::EndQuery(GetGraphicCommandBuffer(drawPass), queryPool, queryId);
    }

    //-------------------------------------------------------------------------------------------------

    void GetQueryPoolResult(
        VkQueryPool queryPool,
        uint32_t const samplesCount,
        uint64_t * outSamplesData,
        uint32_t const samplesOffset
    )
    {
        RB::GetQueryPoolResult(
            state->logicalDevice.device,
            queryPool,
            samplesCount,
            outSamplesData,
            samplesOffset
        );
    }

    //-------------------------------------------------------------------------------------------------

    void ResetQueryPool(
        RT::CommandRecordState const & recordState,
        VkQueryPool queryPool,
        uint32_t queryCount,
        uint32_t firstQueryIndex
    )
    {
        RB::ResetQueryPool(
            GetGraphicCommandBuffer(recordState),
            queryPool,
            queryCount,
            firstQueryIndex
        );
    }

    //-------------------------------------------------------------------------------------------------

    VkCommandBuffer GetGraphicCommandBuffer(RT::CommandRecordState const & drawPass)
    {
        return state->graphicCommandBuffer[drawPass.frameIndex];
    }

    //-------------------------------------------------------------------------------------------------

    RT::CommandRecordState StartGraphicCommandBufferRecording()
    {
        // TODO: Separate this function into multiple ones
        MFA_ASSERT(GetMaxFramesPerFlight() > state->currentFrame);
        RT::CommandRecordState recordState{ .renderPass = nullptr };
        if (IsWindowVisible() == false || IsWindowResized() == true)
        {
            recordState.isValid = false;
            return recordState;
        }

        recordState.frameIndex = state->currentFrame;
        recordState.isValid = true;
        ++state->currentFrame;
        if (state->currentFrame >= GetMaxFramesPerFlight())
        {
            state->currentFrame = 0;
        }

        auto * fence = GetFence(recordState);
        WaitForFence(fence);
        ResetFence(fence);

        // We ignore failed acquire of image because a resize will be triggered at end of pass
        AcquireNextImage(
            GetPresentationSemaphore(recordState),
            state->displayRenderPass.GetSwapChainImages(),
            recordState.imageIndex
        );

        // Recording command buffer data at each render frame
        // We need 1 renderPass and multiple command buffer recording
        // Each pipeline has its own set of shader, But we can reuse a pipeline for multiple shaders.
        // For each model we need to record command buffer with our desired pipeline (For example light and objects have different fragment shader)
        // Prepare data for recording command buffers
        BeginCommandBuffer(GetGraphicCommandBuffer(recordState));

        return recordState;
    }

    //-------------------------------------------------------------------------------------------------

    void EndGraphicCommandBufferRecording(RT::CommandRecordState & recordState)
    {
        // TODO: Separate this function into multiple ones
        MFA_ASSERT(recordState.isValid);
        recordState.isValid = false;

        EndCommandBuffer(GetGraphicCommandBuffer(recordState));

        std::vector<VkSemaphore> const waitSemaphores {GetPresentationSemaphore(recordState)};
        std::vector<VkPipelineStageFlags> const waitStages { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
        std::vector<VkSemaphore> const signalSemaphores {GetGraphicSemaphore(recordState)};

        // Wait for image to be available and draw
        SubmitQueue(
            GetGraphicCommandBuffer(recordState),
            static_cast<uint32_t>(waitSemaphores.size()),
            waitSemaphores.data(),
            waitStages.data(),
            static_cast<uint32_t>(signalSemaphores.size()),
            signalSemaphores.data(),
            GetFence(recordState)
        );

        // Present drawn image
        PresentQueue(
            recordState.imageIndex,
            GetGraphicSemaphore(recordState),
            state->displayRenderPass.GetSwapChainImages().swapChain
        );
    }

    //-------------------------------------------------------------------------------------------------
    
    VkFence GetFence(RT::CommandRecordState const & recordState)
    {
        return state->fences[recordState.frameIndex];
    }

    //-------------------------------------------------------------------------------------------------
    // TODO: RenderFinishIndicator must belong to presentation queue
    VkSemaphore GetGraphicSemaphore(RT::CommandRecordState const & drawPass)
    {
        return state->graphicSemaphores[drawPass.frameIndex];
    }

    //-------------------------------------------------------------------------------------------------

    VkSemaphore GetPresentationSemaphore(RT::CommandRecordState const & drawPass)
    {
        return state->presentSemaphores[drawPass.frameIndex];
    }

    //-------------------------------------------------------------------------------------------------

    VkDescriptorPool CreateDescriptorPool(uint32_t const maxSets)
    {
        return RB::CreateDescriptorPool(
            state->logicalDevice.device,
            maxSets
        );
    }

    //-------------------------------------------------------------------------------------------------

    void DestroyDescriptorPool(VkDescriptorPool descriptorPool)
    {
        RB::DestroyDescriptorPool(
            state->logicalDevice.device,
            descriptorPool
        );
    }

    //-------------------------------------------------------------------------------------------------

}
