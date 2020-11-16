#ifndef RENDER_TYPES_CLASS
#define RENDER_TYPES_CLASS

#include <array>
#include <vector>
#include <functional>
#include <vulkan/vulkan.h>

#include "FileSystem.h"
#include "Renderer.hpp"

namespace RenderTypes {
    class Deffer
    {
    public:
        explicit Deffer(std::function<void()> && deferred_process)
            : m_deferred_process(std::move(deferred_process))
        {}
        Deffer(const Deffer & that) = delete;
        Deffer(const Deffer && that) = delete;
        Deffer& operator=(const Deffer & that) = delete;
        Deffer& operator=(const Deffer && that) = delete;
        ~Deffer()
        {
            m_deferred_process();
        }
    private:
        std::function<void()> m_deferred_process;
    };
    using TypeOfIndices = uint32_t;
    using TypeOfPosition = float;
    using TypeOfColor = float;
    using TypeOfTexCoord = float;
    struct Vertex {
        TypeOfPosition pos[3];
        TypeOfColor color[3];
        TypeOfTexCoord tex_coord[2];
        static VkVertexInputBindingDescription getBindingDescription()
        {
            VkVertexInputBindingDescription bindingDescription{};
            bindingDescription.binding = 0;
            bindingDescription.stride = sizeof(Vertex);
            bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

            return bindingDescription;
        }
        static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions() {
            std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};

            attributeDescriptions[0].binding = 0;
            attributeDescriptions[0].location = 0;
            attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
            attributeDescriptions[0].offset = offsetof(Vertex, pos);

            attributeDescriptions[1].binding = 0;
            attributeDescriptions[1].location = 1;
            attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
            attributeDescriptions[1].offset = offsetof(Vertex, color);

            attributeDescriptions[2].binding = 0;
            attributeDescriptions[2].location = 2;
            attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
            attributeDescriptions[2].offset = offsetof(Vertex, tex_coord);

            return attributeDescriptions;
        }
    };
    struct Mesh
    {
        Mesh(char const * obj_file_address, char const * texture_file_address) {
            auto const vertex_and_indices_are_valid = FileSystem::LoadObj(*this, obj_file_address);
            if(vertex_and_indices_are_valid){
                FileSystem::RawTexture cpu_texture;
                FileSystem::LoadTexture(cpu_texture, texture_file_address);
                if(cpu_texture.isValid()) {
                    VkBuffer stagingBuffer;
                    VkDeviceMemory stagingBufferMemory;

                    //auto const format = cpu_texture.format();
                    //auto const mip_level = cpu_texture.mipmap_count() - 1;
                    //auto const mipmap = cpu_texture.pixels(mip_level);
                    auto const format = VK_FORMAT_R8G8B8A8_UNORM;
                    auto const image_size = cpu_texture.image_size();
                    auto const pixels = cpu_texture.pixels;
                    auto const width = cpu_texture.width;
                    auto const height = cpu_texture.height;

                    Renderer::CreateBuffer(
                        image_size, 
                        VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
                        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
                        stagingBuffer, 
                        stagingBufferMemory
                    );

                    void * data = nullptr;
                    vkMapMemory(device, stagingBufferMemory, 0, image_size, 0, &data);
                    assert(nullptr != data);
                    ::memcpy(data, pixels, static_cast<size_t>(image_size));
                    vkUnmapMemory(device, stagingBufferMemory);

                    createImage(
                        width, 
                        height, 
                        format,//VK_FORMAT_R8G8B8A8_UNORM, 
                        VK_IMAGE_TILING_OPTIMAL, 
                        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, 
                        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
                        textureImage, 
                        textureImageMemory
                    );

                    transitionImageLayout(
                        textureImage, 
                        format,//VK_FORMAT_R8G8B8A8_UNORM, 
                        VK_IMAGE_LAYOUT_UNDEFINED, 
                        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
                    );
                    copyBufferToImage(
                        stagingBuffer, 
                        textureImage, 
                        static_cast<uint32_t>(width), 
                        static_cast<uint32_t>(height)
                    );
                    transitionImageLayout(
                        textureImage,
                        format, 
                        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 
                        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
                    );

                    vkDestroyBuffer(device, stagingBuffer, nullptr);
                    vkFreeMemory(device, stagingBufferMemory, nullptr);

                    createImageView(&textureImageView, textureImage, format, VK_IMAGE_ASPECT_COLOR_BIT);
                    valid = true;
                }
            }
        }
        std::vector<Vertex> vertices {};
        std::vector<TypeOfIndices> indices {};
        VkDescriptorSet * texture_descriptor_set = nullptr;
        bool valid = false;
    };
}

#endif
