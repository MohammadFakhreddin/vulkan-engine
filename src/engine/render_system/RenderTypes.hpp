#pragma once

#include "engine/render_system/RenderTypesFWD.hpp"
#include "engine/BedrockPlatforms.hpp"
#include "engine/FoundationAsset.hpp"

#ifdef __ANDROID__
#include "vulkan_wrapper.h"
#include <android_native_app_glue.h>
#else
#include <vulkan/vulkan.h>
#endif

namespace MFA
{

    class RenderPass;

    namespace RenderTypes
    {

        struct BufferAndMemory
        {
            VkBuffer buffer{};
            VkDeviceMemory memory{};
            [[nodiscard]]
            bool isValid() const noexcept;
            void revoke();
        };

        struct SamplerGroup
        {
            VkSampler sampler;
            [[nodiscard]]
            bool isValid() const noexcept;
            void revoke();
        };

        struct MeshBuffers
        {
            BufferAndMemory verticesBuffer{};
            BufferAndMemory indicesBuffer{};
        };

        struct ImageGroup
        {
            VkImage image{};
            VkDeviceMemory memory{};
            [[nodiscard]]
            bool isValid() const noexcept;
            void revoke();
        };

        class GpuTexture
        {
        public:

            explicit GpuTexture() = default;

            explicit GpuTexture(
                ImageGroup && imageGroup,
                VkImageView imageView,
                AS::Texture && cpuTexture
            );

            [[nodiscard]]
            AS::Texture const & cpuTexture() const { return mCpuTexture; }

            [[nodiscard]]
            AS::Texture & cpuTexture() { return mCpuTexture; }

            [[nodiscard]]
            bool isValid() const;

            void revoke();

            [[nodiscard]]
            VkImage const & image() const { return mImageGroup.image; }

            [[nodiscard]]
            VkImageView imageView() const { return mImageView; }

            [[nodiscard]]
            ImageGroup const & imageGroup() const { return mImageGroup; };

        private:
            ImageGroup mImageGroup{};
            VkImageView mImageView{};
            AS::Texture mCpuTexture{};
        };


        struct GpuModel
        {
            bool valid = false;
            MeshBuffers meshBuffers{};
            std::vector<GpuTexture> textures{};
            AssetSystem::Model model{};
            // TODO Samplers
        };

        class GpuShader
        {
        public:

            explicit GpuShader() = default;

            explicit GpuShader(VkShaderModule shaderModule, AS::Shader cpuShader);

            [[nodiscard]]
            AS::Shader & cpuShader() { return mCpuShader; }

            [[nodiscard]]
            AS::Shader const & cpuShader() const { return mCpuShader; }

            [[nodiscard]]
            bool valid() const;

            [[nodiscard]]
            VkShaderModule shaderModule() const { return mShaderModule; }

            void revoke();

        private:

            VkShaderModule mShaderModule{};

            AS::Shader mCpuShader{};

        };

        struct PipelineGroup
        {
            VkPipelineLayout pipelineLayout{};
            VkPipeline graphicPipeline{};

            [[nodiscard]]
            bool isValid() const noexcept;

            void revoke();
        };

        struct CommandRecordState
        {
            uint32_t imageIndex;
            uint32_t frameIndex;
            bool isValid = false;
            PipelineGroup * pipeline;
            RenderPass * renderPass;
        };

        struct UniformBufferCollection
        {
            std::vector<BufferAndMemory> buffers;
            size_t bufferSize;
            [[nodiscard]]
            bool isValid() const noexcept;
        };

        struct StorageBufferCollection
        {
            std::vector<BufferAndMemory> buffers;
            size_t bufferSize;
            [[nodiscard]]
            bool isValid() const noexcept;
        };

        struct SwapChainGroup
        {
            VkSwapchainKHR swapChain{};
            VkFormat swapChainFormat{};
            std::vector<VkImage> swapChainImages{};
            std::vector<VkImageView> swapChainImageViews{};
        };

        struct DepthImageGroup
        {
            ImageGroup imageGroup{};
            VkImageView imageView{};
            VkFormat imageFormat{};
        };

        struct ColorImageGroup
        {
            ImageGroup imageGroup{};
            VkImageView imageView{};
            VkFormat imageFormat{};
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
                depthStencil.depthWriteEnable = VK_TRUE;
                depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
                depthStencil.depthBoundsTestEnable = VK_FALSE;
                depthStencil.stencilTestEnable = VK_FALSE;

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

        //static constexpr VkFormat ShadowFormat = VK_FORMAT_R32_SFLOAT;

        inline static const glm::vec4 ForwardVector {0.0f, 0.0f, 1.0f, 0.0f};
        inline static const glm::vec4 RightVector {1.0f, 0.0f, 0.0f, 0.0f};
        inline static const glm::vec4 UpVector {0.0f, 1.0f, 0.0f, 0.0f};

    };

    namespace RT = RenderTypes;

};
