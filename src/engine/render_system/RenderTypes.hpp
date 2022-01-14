#pragma once

#include "engine/render_system/RenderTypesFWD.hpp"
#include "engine/BedrockPlatforms.hpp"

#ifdef __ANDROID__
#include "vulkan_wrapper.h"
#include <android_native_app_glue.h>
#else
#include <vulkan/vulkan.h>
#endif

namespace MFA
{

    class RenderPass;

    namespace AssetSystem
    {
        class Texture;
    }

    namespace RenderTypes
    {

        struct BufferAndMemory
        {
            const VkBuffer buffer;
            const VkDeviceMemory memory;

            explicit BufferAndMemory(
                VkBuffer buffer_,
                VkDeviceMemory memory_
            );
            ~BufferAndMemory();

            BufferAndMemory(BufferAndMemory const &) noexcept = delete;
            BufferAndMemory(BufferAndMemory &&) noexcept = delete;
            BufferAndMemory & operator= (BufferAndMemory const & rhs) noexcept = delete;
            BufferAndMemory & operator= (BufferAndMemory && rhs) noexcept = delete;

        };

        struct SamplerGroup
        {
            VkSampler const sampler;

            explicit SamplerGroup(VkSampler sampler_);
            ~SamplerGroup();

            SamplerGroup(SamplerGroup const &) noexcept = delete;
            SamplerGroup(SamplerGroup &&) noexcept = delete;
            SamplerGroup & operator= (SamplerGroup const & rhs) noexcept = delete;
            SamplerGroup & operator= (SamplerGroup && rhs) noexcept = delete;
        
        };

        struct MeshBuffers
        {
            std::vector<std::shared_ptr<BufferAndMemory>> const verticesBuffer;
            std::shared_ptr<BufferAndMemory> const indicesBuffer;

            explicit MeshBuffers(
                std::vector<std::shared_ptr<BufferAndMemory>> verticesBuffer_,
                std::shared_ptr<BufferAndMemory> indicesBuffer_
            );
            explicit MeshBuffers(
                std::shared_ptr<BufferAndMemory> verticesBuffer_,
                std::shared_ptr<BufferAndMemory> indicesBuffer_
            );
            ~MeshBuffers();

            MeshBuffers(MeshBuffers const &) noexcept = delete;
            MeshBuffers(MeshBuffers &&) noexcept = delete;
            MeshBuffers & operator= (MeshBuffers const & rhs) noexcept = delete;
            MeshBuffers & operator= (MeshBuffers && rhs) noexcept = delete;
        
        };

        struct ImageGroup
        {
            VkImage const image;
            VkDeviceMemory const memory;

            explicit ImageGroup(
                VkImage image_,
                VkDeviceMemory memory_
            );
            ~ImageGroup();

            ImageGroup(ImageGroup const &) noexcept = delete;
            ImageGroup(ImageGroup &&) noexcept = delete;
            ImageGroup & operator= (ImageGroup const & rhs) noexcept = delete;
            ImageGroup & operator= (ImageGroup && rhs) noexcept = delete;
        };

        struct ImageViewGroup
        {
            VkImageView const imageView;

            explicit ImageViewGroup(VkImageView imageView_);
            ~ImageViewGroup();

            ImageViewGroup(ImageViewGroup const &) noexcept = delete;
            ImageViewGroup(ImageViewGroup &&) noexcept = delete;
            ImageViewGroup & operator= (ImageViewGroup const & rhs) noexcept = delete;
            ImageViewGroup & operator= (ImageViewGroup && rhs) noexcept = delete;

        };

        struct GpuTexture
        {
            explicit GpuTexture() = default;

            explicit GpuTexture(
                std::shared_ptr<ImageGroup> imageGroup,
                std::shared_ptr<ImageViewGroup> imageView
            );
            ~GpuTexture();

            GpuTexture(GpuTexture const &) noexcept = delete;
            GpuTexture(GpuTexture &&) noexcept = delete;
            GpuTexture & operator= (GpuTexture const & rhs) noexcept = delete;
            GpuTexture & operator= (GpuTexture && rhs) noexcept = delete;

            std::shared_ptr<ImageGroup> const imageGroup{};
            std::shared_ptr<ImageViewGroup> const imageView{};
        };

        struct GpuModel
        {
            std::string const address;
            std::shared_ptr<MeshBuffers> const meshBuffers;
            std::vector<std::shared_ptr<GpuTexture>> const textures;
            
            explicit GpuModel(
                std::string address_,
                std::shared_ptr<MeshBuffers> meshBuffers_,
                std::vector<std::shared_ptr<GpuTexture>> textures_
            );
            ~GpuModel();

            GpuModel(GpuModel const &) noexcept = delete;
            GpuModel(GpuModel &&) noexcept = delete;
            GpuModel & operator= (GpuModel const & rhs) noexcept = delete;
            GpuModel & operator= (GpuModel && rhs) noexcept = delete;
        };

        struct GpuShader
        {
        
            explicit GpuShader(
                VkShaderModule const & shaderModule,
                VkShaderStageFlagBits stageFlags,
                std::string entryPointName
            );
            ~GpuShader();

            GpuShader(GpuShader const &) noexcept = delete;
            GpuShader(GpuShader &&) noexcept = delete;
            GpuShader & operator= (GpuShader const & rhs) noexcept = delete;
            GpuShader & operator= (GpuShader && rhs) noexcept = delete;
            
            VkShaderModule const shaderModule;
            VkShaderStageFlagBits const stageFlags;
            std::string const entryPointName;

        };

        struct PipelineGroup
        {
            VkPipelineLayout const pipelineLayout;
            VkPipeline const pipeline;

            explicit PipelineGroup(
                VkPipelineLayout pipelineLayout_,
                VkPipeline pipeline_
            );
            ~PipelineGroup();

            PipelineGroup(PipelineGroup const &) noexcept = delete;
            PipelineGroup(PipelineGroup &&) noexcept = delete;
            PipelineGroup & operator= (PipelineGroup const & rhs) noexcept = delete;
            PipelineGroup & operator= (PipelineGroup && rhs) noexcept = delete;

            [[nodiscard]]
            bool isValid() const noexcept;
        };

        struct CommandRecordState
        {
            uint32_t imageIndex = 0;
            uint32_t frameIndex = 0;
            bool isValid = false;
            PipelineGroup * pipeline = nullptr;
            RenderPass * renderPass = nullptr;
        };

        struct UniformBufferGroup
        {
            explicit UniformBufferGroup(
                std::vector<std::shared_ptr<BufferAndMemory>> buffers_,
                size_t bufferSize_
            );
            ~UniformBufferGroup();

            UniformBufferGroup(UniformBufferGroup const &) noexcept = delete;
            UniformBufferGroup(UniformBufferGroup &&) noexcept = delete;
            UniformBufferGroup & operator= (UniformBufferGroup const & rhs) noexcept = delete;
            UniformBufferGroup & operator= (UniformBufferGroup && rhs) noexcept = delete;

            std::vector<std::shared_ptr<BufferAndMemory>> const buffers;
            size_t const bufferSize;
        };

        struct StorageBufferCollection
        {
            explicit StorageBufferCollection(
                std::vector<std::shared_ptr<BufferAndMemory>> buffers_,
                size_t bufferSize_
            );
            ~StorageBufferCollection();

            StorageBufferCollection(StorageBufferCollection const &) noexcept = delete;
            StorageBufferCollection(StorageBufferCollection &&) noexcept = delete;
            StorageBufferCollection & operator= (StorageBufferCollection const & rhs) noexcept = delete;
            StorageBufferCollection & operator= (StorageBufferCollection && rhs) noexcept = delete;

            std::vector<std::shared_ptr<BufferAndMemory>> const buffers;
            size_t const bufferSize;
        };

        struct SwapChainGroup
        {
            explicit SwapChainGroup(
                VkSwapchainKHR swapChain_,
                VkFormat swapChainFormat_,
                std::vector<VkImage> const & swapChainImages_,
                std::vector<std::shared_ptr<ImageViewGroup>> const & swapChainImageViews_
            );
            ~SwapChainGroup();

            SwapChainGroup(SwapChainGroup const &) noexcept = delete;
            SwapChainGroup(SwapChainGroup &&) noexcept = delete;
            SwapChainGroup & operator= (SwapChainGroup const & rhs) noexcept = delete;
            SwapChainGroup & operator= (SwapChainGroup && rhs) noexcept = delete;

            VkSwapchainKHR const swapChain;
            VkFormat const swapChainFormat;
            std::vector<VkImage> swapChainImages;
            std::vector<std::shared_ptr<ImageViewGroup>> swapChainImageViews;
        };

        struct DepthImageGroup
        {
            explicit DepthImageGroup(
                std::shared_ptr<ImageGroup> imageGroup_,
                std::shared_ptr<ImageViewGroup> imageView_,
                VkFormat imageFormat_
            );
            ~DepthImageGroup();

            DepthImageGroup(DepthImageGroup const &) noexcept = delete;
            DepthImageGroup(DepthImageGroup &&) noexcept = delete;
            DepthImageGroup & operator= (DepthImageGroup const & rhs) noexcept = delete;
            DepthImageGroup & operator= (DepthImageGroup && rhs) noexcept = delete;

            std::shared_ptr<ImageGroup> const imageGroup;
            std::shared_ptr<ImageViewGroup> const imageView;
            VkFormat const imageFormat;
        };

        struct ColorImageGroup
        {
            explicit ColorImageGroup(
                std::shared_ptr<ImageGroup> imageGroup_,
                std::shared_ptr<ImageViewGroup> imageView_,
                VkFormat imageFormat_
            );
            ~ColorImageGroup();

            ColorImageGroup(ColorImageGroup const &) noexcept = delete;
            ColorImageGroup(ColorImageGroup &&) noexcept = delete;
            ColorImageGroup & operator= (ColorImageGroup const & rhs) noexcept = delete;
            ColorImageGroup & operator= (ColorImageGroup && rhs) noexcept = delete;

            std::shared_ptr<ImageGroup> const imageGroup;
            std::shared_ptr<ImageViewGroup> const imageView;
            VkFormat const imageFormat;
        };

        struct DescriptorSetLayoutGroup
        {
            explicit DescriptorSetLayoutGroup(VkDescriptorSetLayout descriptorSetLayout_);
            ~DescriptorSetLayoutGroup();

            DescriptorSetLayoutGroup(DescriptorSetLayoutGroup const &) noexcept = delete;
            DescriptorSetLayoutGroup(DescriptorSetLayoutGroup &&) noexcept = delete;
            DescriptorSetLayoutGroup & operator= (DescriptorSetLayoutGroup const & rhs) noexcept = delete;
            DescriptorSetLayoutGroup & operator= (DescriptorSetLayoutGroup && rhs) noexcept = delete;

            VkDescriptorSetLayout const descriptorSetLayout;

        };

        struct DescriptorSetGroup
        {
            std::vector<VkDescriptorSet> descriptorSets{};

            [[nodiscard]]
            bool IsValid() const noexcept
            {
                return descriptorSets.empty() == false;
            }
        };

        struct LogicalDevice
        {
            VkDevice device{};
            VkPhysicalDeviceMemoryProperties physicalMemoryProperties{};
        };

        // CreateSyncObjects (Fence, Semaphore, ...)
        struct SyncObjects
        {
            std::vector<VkSemaphore> imageAvailabilitySemaphores;
            std::vector<VkSemaphore> renderFinishIndicatorSemaphores;
            std::vector<VkFence> fencesInFlight;
            std::vector<VkFence> imagesInFlight;
        };

        struct CreateGraphicPipelineOptions
        {
            VkFrontFace frontFace = VK_FRONT_FACE_CLOCKWISE;
            VkCullModeFlags cullMode = VK_CULL_MODE_BACK_BIT;                   // TODO We need no cull for certain objects
            VkPipelineDynamicStateCreateInfo * dynamicStateCreateInfo = nullptr;
            VkPipelineDepthStencilStateCreateInfo depthStencil{};
            VkPipelineColorBlendAttachmentState colorBlendAttachments{};
            uint8_t pushConstantsRangeCount = 0;
            VkPushConstantRange * pushConstantRanges = nullptr;
            bool useStaticViewportAndScissor = false;           // Use of dynamic viewport and scissor is recommended
            VkPrimitiveTopology primitiveTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
            VkSampleCountFlagBits rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

            // Default params
            explicit CreateGraphicPipelineOptions()
            {
                depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
                depthStencil.depthTestEnable = VK_TRUE;
                depthStencil.depthWriteEnable = VK_TRUE;                // TODO Instead of using new buffer for occlusion I might be able to use this to disable writting to depth
                depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
                depthStencil.depthBoundsTestEnable = VK_FALSE;
                depthStencil.stencilTestEnable = VK_FALSE;
                // TODO Maybe we can ask whether blend is enable or not
                colorBlendAttachments.blendEnable = VK_TRUE;
                colorBlendAttachments.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
                colorBlendAttachments.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
                colorBlendAttachments.colorBlendOp = VK_BLEND_OP_ADD;
                colorBlendAttachments.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
                colorBlendAttachments.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
                colorBlendAttachments.alphaBlendOp = VK_BLEND_OP_ADD;
                colorBlendAttachments.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
            }
        };

        struct CreateSamplerParams
        {
            float minLod = 0;  // Level of detail
            float maxLod = 1;
            bool anisotropyEnabled = true;
            float maxAnisotropy = 16.0f;
            VkSamplerAddressMode addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            VkSamplerAddressMode addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            VkSamplerAddressMode addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        };

        struct CreateColorImageOptions
        {
            uint16_t layerCount = 1;
            VkImageUsageFlags usageFlags = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
            VkImageViewType viewType = VK_IMAGE_VIEW_TYPE_2D;
            VkImageCreateFlags imageCreateFlags = 0;
            VkSampleCountFlagBits samplesCount = VK_SAMPLE_COUNT_1_BIT;
            VkImageType imageType = VK_IMAGE_TYPE_2D;
        };

        struct CreateDepthImageOptions
        {
            uint16_t layerCount = 1;
            VkImageUsageFlags usageFlags = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
            VkImageViewType viewType = VK_IMAGE_VIEW_TYPE_2D;
            VkImageCreateFlags imageCreateFlags = 0;
            VkSampleCountFlagBits samplesCount = VK_SAMPLE_COUNT_1_BIT;
            VkImageType imageType = VK_IMAGE_TYPE_2D;
        };

    };

    namespace RT = RenderTypes;

};
