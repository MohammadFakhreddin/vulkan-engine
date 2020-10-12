#ifndef M_TYPES_CLASS
#define M_TYPES_CLASS

#include <array>
#include <vector>
#include <functional>

#include <vulkan/vulkan.h>

namespace MTypes {
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
        std::vector<Vertex> vertices {};
        std::vector<TypeOfIndices> indices {};
        bool valid = false;
    };
}

#endif
