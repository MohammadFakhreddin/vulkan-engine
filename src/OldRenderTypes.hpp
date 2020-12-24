#if 0
#ifndef RENDER_TYPES_CLASS
#define RENDER_TYPES_CLASS

#include <array>
#include <vector>
#include <functional>
#include <vulkan/vulkan.h>

#include "AssetTypes.hpp"

namespace MFA {
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
struct Vertex {
    float pos[3];
    float color[3];
    float tex_coord[2];
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
    bool init ();
    void draw ();
    bool shutdown();
private:
    bool m_valid = false;
    std::string m_obj_file_address;
    std::string m_texture_file_address;

    std::vector<Vertex> m_vertices {};
    VkBuffer m_vertex_buffer;
    VkDeviceMemory m_vertex_memory;

    std::vector<uint32_t> m_indices {};
    VkBuffer m_indices_buffer;
    VkDeviceMemory m_indices_memory;

    VkImage m_texture_image = nullptr;
    VkImageView m_texture_image_view = nullptr;
    VkDeviceMemory m_texture_memory = nullptr;

    VkSampler m_sampler = nullptr;
    VkDescriptorSet * m_texture_descriptor_set = nullptr;
};
}
}

#endif
#endif