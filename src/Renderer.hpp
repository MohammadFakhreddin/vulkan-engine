#ifndef RENDERER_NAMESPACE
#define RENDERER_NAMESPACE

#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>
#include <vulkan/vulkan.h>
#include <vector>

#include "MathHelper.hpp"
#include "AssetTypes.hpp"

// TODO Make it functional
namespace MFA {
namespace Renderer {
    
VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& surface_capabilities);
    
struct InitParams {
    uint32_t screen_width;
    uint32_t screen_height;
};
bool Init(InitParams const & params);

bool Shutdown();

void CreateBuffer(
    VkDeviceSize const size, 
    VkBufferUsageFlags const usage, 
    VkMemoryPropertyFlags const properties, 
    VkBuffer & out_buffer, 
    VkDeviceMemory & out_buffer_memory
);

bool CreateTextureImage(
    VkImage & out_vk_texture,
    VkDeviceMemory & out_vk_memory,
    VkImageView & out_vk_image_view,
    AssetTypes::RawTexture const & cpu_texture
);

void DestroyTextureImage(
    VkImage * texture_image, 
    VkDeviceMemory * texture_memory
);

VkSampler CreateTextureSampler();

template <typename T>
void CreateVertexBuffer(
    VkBuffer & out_vertex_buffer,
    VkDeviceMemory & out_vertex_buffer_memory, 
    std::vector<T> vertices
) {
    VkDeviceSize const buffer_size = sizeof(vertices[0]) * vertices.size();

    VkBuffer staging_buffer;
    VkDeviceMemory staging_buffer_memory;
    CreateBuffer(
        buffer_size, 
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
        staging_buffer, staging_buffer_memory
    );

    void * data;
    vkMapMemory(*m_device, staging_buffer_memory, 0, buffer_size, 0, &data);
    ::memcpy(data, vertices.data(), static_cast<size_t>(buffer_size));
    vkUnmapMemory(*m_device, staging_buffer_memory);

    CreateBuffer(
        buffer_size, 
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
        out_vertex_buffer, 
        out_vertex_buffer_memory
    );

    copy_buffer(staging_buffer, out_vertex_buffer, buffer_size);

    vkDestroyBuffer(*m_device, staging_buffer, nullptr);
    vkFreeMemory(*m_device, staging_buffer_memory, nullptr);
}

template <typename T>
void CreateIndexBuffer(
    VkBuffer & out_index_buffer,
    VkDeviceMemory & out_index_buffer_memory, 
    std::vector<T> indices
) {
    VkDeviceSize const buffer_size = sizeof(indices[0]) * indices.size();

    VkBuffer staging_buffer;
    VkDeviceMemory staging_buffer_memory;
    create_buffer(
        buffer_size, 
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
        staging_buffer, 
        staging_buffer_memory
    );

    void* data;
    vkMapMemory(*m_device, staging_buffer_memory, 0, buffer_size, 0, &data);
    ::memcpy(data, indices.data(), static_cast<size_t>(buffer_size));
    vkUnmapMemory(*m_device, staging_buffer_memory);

    create_buffer(
        buffer_size,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        out_index_buffer, 
        out_index_buffer_memory
    );

    copy_buffer(staging_buffer, out_index_buffer, buffer_size);

    vkDestroyBuffer(*m_device, staging_buffer, nullptr);
    vkFreeMemory(*m_device, staging_buffer_memory, nullptr);
}

void DestoryBuffer(VkBuffer * buffer, VkDeviceMemory * memory);

};

#endif

}