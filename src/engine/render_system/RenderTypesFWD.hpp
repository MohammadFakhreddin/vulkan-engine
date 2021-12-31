#pragma once
#include <functional>

namespace MFA
{

    namespace RenderTypes
    {

        struct BufferAndMemory;

        struct SamplerGroup;

        struct MeshBuffers;

        struct ImageGroup;

        struct ImageViewGroup;

        struct GpuTexture;

        struct GpuModel;

        struct GpuShader;

        struct PipelineGroup;

        struct CommandRecordState;

        struct UniformBufferGroup;

        struct StorageBufferCollection;

        struct SwapChainGroup;

        struct DepthImageGroup;

        struct ColorImageGroup;

        struct DescriptorSetGroup;

        struct LogicalDevice;

        struct SyncObjects;

        struct CreateGraphicPipelineOptions;

        struct CreateSamplerParams;

        struct CreateColorImageOptions;

        struct CreateDepthImageOptions;

        using ResizeEventListenerId = int;
        using ResizeEventListener = std::function<void()>;

        using VariantId = uint32_t;

        using GpuModelId = uint32_t;

        struct UniformBufferGroup;

    };

    namespace RT = RenderTypes;

};
