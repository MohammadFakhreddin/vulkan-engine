#pragma once

#include "engine/render_system/RenderTypesFWD.hpp"
#include "engine/BedrockPlatforms.hpp"

#ifdef __ANDROID__
#include "vulkan_wrapper.h"
#include <android_native_app_glue.h>
#else
#include <vulkan/vulkan.h>
#endif

#include <string>
#include <vector>
#include <memory>
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
            VkDeviceSize const size;

            explicit BufferAndMemory(
                VkBuffer buffer_,
                VkDeviceMemory memory_,
                VkDeviceSize size_
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

        struct ImageGroup
        {
            const VkImage image;
            const VkDeviceMemory memory;

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

        enum class CommandBufferType
        {
            Invalid,
            Graphic,
            Compute
        };

        struct CommandRecordState
        {
            uint32_t imageIndex = 0;
            uint32_t frameIndex = 0;
            bool isValid = false;

            CommandBufferType commandBufferType = CommandBufferType::Invalid;   // We could have used queue family index as well
            VkCommandBuffer commandBuffer = nullptr;
            PipelineGroup * pipeline = nullptr;
            RenderPass * renderPass = nullptr;
        };

        struct BufferGroup
        {
            explicit BufferGroup(
                std::vector<std::shared_ptr<BufferAndMemory>> buffers_,
                size_t bufferSize_
            );
            ~BufferGroup();

            BufferGroup(BufferGroup const &) noexcept = delete;
            BufferGroup(BufferGroup &&) noexcept = delete;
            BufferGroup & operator= (BufferGroup const & rhs) noexcept = delete;
            BufferGroup & operator= (BufferGroup && rhs) noexcept = delete;

            std::vector<std::shared_ptr<BufferAndMemory>> const buffers;
            size_t const bufferSize;
        };

        /*struct UniformBufferGroup : public BufferGroup
        {
            using BufferGroup::BufferGroup;
        };

        struct StorageBufferGroup : public BufferGroup
        {
            using BufferGroup::BufferGroup;
        };*/

        struct SwapChainGroup
        {
            explicit SwapChainGroup(
                VkSwapchainKHR swapChain_,
                VkFormat swapChainFormat_,
                std::vector<VkImage> swapChainImages_,
                std::vector<std::shared_ptr<ImageViewGroup>> swapChainImageViews_
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

        struct CreateGraphicPipelineOptions
        {
            VkFrontFace frontFace = VK_FRONT_FACE_CLOCKWISE;
            VkCullModeFlags cullMode = VK_CULL_MODE_BACK_BIT;                   // TODO We need no cull for certain objects
            VkPipelineDynamicStateCreateInfo * dynamicStateCreateInfo = nullptr;
            VkPipelineDepthStencilStateCreateInfo depthStencil{};
            VkPipelineColorBlendAttachmentState colorBlendAttachments{};
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

        struct MappedMemory
        {
            explicit MappedMemory(VkDeviceMemory memory_);
            ~MappedMemory();

            MappedMemory(MappedMemory const &) noexcept = delete;
            MappedMemory(MappedMemory &&) noexcept = delete;
            MappedMemory & operator= (MappedMemory const & rhs) noexcept = delete;
            MappedMemory & operator= (MappedMemory && rhs) noexcept = delete;

            [[nodiscard]]
            void ** getMappingPtr()
            {
                return &ptr;
            }

            template<typename T>
            T * getPtr()
            {
                return static_cast<T *>(ptr);
            }

            [[nodiscard]]
            bool isValid() const noexcept
            {
                return ptr != nullptr;
            }

        private:

            void * ptr = nullptr;
            VkDeviceMemory memory {};

        };

        using ScreenWidth = Platforms::ScreenSize;
        using ScreenHeight = Platforms::ScreenSize;

        struct FrontendInitParams
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

    };

    namespace RT = RenderTypes;

};
