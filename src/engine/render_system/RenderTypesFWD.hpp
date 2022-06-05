#pragma once

#include "engine/BedrockCommon.hpp"

#include <cstdint>
#include <functional>

namespace MFA
{

    namespace RenderTypes
    {

        static constexpr int MAX_POINT_LIGHT_COUNT = 10;        // It can be more but currently 10 is more than enough for me
        static constexpr int MAX_DIRECTIONAL_LIGHT_COUNT = 3;

        static constexpr uint32_t POINT_LIGHT_SHADOW_WIDTH = 1024;
        static constexpr uint32_t POINT_LIGHT_SHADOW_HEIGHT = 1024;

        // TODO: Use cascade shadow technique to decrease this value
        static constexpr uint32_t DIRECTIONAL_LIGHT_SHADOW_TEXTURE_WIDTH = 1024 * 10;
        static constexpr uint32_t DIRECTIONAL_LIGHT_SHADOW_TEXTURE_HEIGHT = 1024 * 10;
        static constexpr uint32_t DIRECTIONAL_LIGHT_PROJECTION_WIDTH = DIRECTIONAL_LIGHT_SHADOW_TEXTURE_WIDTH / 200;
        static constexpr uint32_t DIRECTIONAL_LIGHT_PROJECTION_HEIGHT = DIRECTIONAL_LIGHT_SHADOW_TEXTURE_HEIGHT / 200;

        struct BufferAndMemory;

        struct SamplerGroup;

        //struct MeshBuffer;

        struct ImageGroup;

        struct ImageViewGroup;

        struct GpuTexture;

        //struct GpuModel;

        struct GpuShader;

        struct PipelineGroup;

        struct CommandRecordState;

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

        struct DescriptorSetLayoutGroup;

        struct BufferGroup;
        //struct StorageBufferGroup;
        //struct UniformBufferGroup;

    };

    namespace RT = RenderTypes;

};
