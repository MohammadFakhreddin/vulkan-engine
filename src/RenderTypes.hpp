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
            VkVertexInputBindingDescription binding_description{};
            binding_description.binding = 0;
            binding_description.stride = sizeof(Vertex);
            binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

            return binding_description;
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
    class Mesh
    {
    public:
        Mesh(char const * obj_file_address_, char const * texture_file_address_)
            : m_obj_file_address(obj_file_address_)
            , m_texture_file_address(texture_file_address_)
        {}
        bool init (Renderer * renderer) {
            assert(nullptr != renderer);
            auto const is_load_obj_successful = FileSystem::LoadObj(*this, m_obj_file_address.c_str());
            if(is_load_obj_successful){
                renderer->createVertexBuffer(m_vertex_buffer, m_vertex_memory, m_vertices);
                renderer->createIndexBuffer(m_indices_buffer, m_indices_memory, m_indices);

                FileSystem::RawTexture cpu_texture;
                FileSystem::LoadTexture(cpu_texture, m_texture_file_address.c_str());

                // Texture image
                auto const is_create_texture_successful = renderer->create_texture_image(
                    m_texture_image,
                    m_texture_memory,
                    m_texture_image_view,
                    cpu_texture
                );
                if(true == is_create_texture_successful) {
                    // Texture sampler
                    m_sampler = renderer->create_texture_sampler();
                }
            }
            return m_valid;
        }
        void draw (Renderer * renderer) {
            assert(nullptr != renderer);
        }
        bool shutdown(Renderer * renderer) {
            assert(nullptr != renderer);
            renderer->destroy_texture_image(m_texture_image, m_texture_memory);
        }
    private:
        bool m_valid = false;
        std::string m_obj_file_address;
        std::string m_texture_file_address;

        std::vector<Vertex> m_vertices {};
        VkBuffer m_vertex_buffer;
        VkDeviceMemory m_vertex_memory;

        std::vector<TypeOfIndices> m_indices {};
        VkBuffer m_indices_buffer;
        VkDeviceMemory m_indices_memory;

        VkImage m_texture_image = nullptr;
        VkImageView m_texture_image_view = nullptr;
        VkDeviceMemory m_texture_memory = nullptr;

        VkSampler m_sampler = nullptr;
        VkDescriptorSet * m_texture_descriptor_set = nullptr;
    };
}

#endif
