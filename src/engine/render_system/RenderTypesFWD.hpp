#pragma once
#include <functional>

namespace MFA {

namespace RenderTypes {

struct BufferGroup;

struct SamplerGroup;

struct MeshBuffers;

struct ImageGroup;

class GpuTexture;

struct GpuModel;

class GpuShader;

struct PipelineGroup;

struct CommandRecordState;

struct UniformBufferGroup;

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

using DrawableVariantId = uint32_t;
    

};

namespace RT = RenderTypes;

};