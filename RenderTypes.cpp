#include "RenderTypes.hpp"

 bool RenderTypes::Mesh::init (Renderer * renderer) {
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

void RenderTypes::Mesh::draw (Renderer * renderer) {
    assert(nullptr != renderer);
}

bool RenderTypes::Mesh::shutdown(Renderer * renderer) {
    assert(nullptr != renderer);
    renderer->destroy_texture_image(&m_texture_image, &m_texture_memory);
    renderer->destroy_buffer(&m_vertex_buffer, &m_vertex_memory);
    renderer->destroy_buffer(&m_indices_buffer, &m_indices_memory);
    return true;
}