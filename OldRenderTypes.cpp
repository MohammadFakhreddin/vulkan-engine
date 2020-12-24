#include "RenderTypes.hpp"

#include <cassert>

#include <Renderer.hpp>
#include <FileSystem.hpp>

namespace MFA {
namespace RenderTypes {

static_assert(sizeof(FileSystem::LoadObjResult::Position) == sizeof(Vertex::pos));
static_assert(sizeof(FileSystem::LoadObjResult::TextureCoordinates) == sizeof(Vertex::tex_coord));

bool Mesh::init () {
    auto const load_obj_result = FileSystem::LoadObj(m_obj_file_address.c_str());
    if(load_obj_result.success){

        m_indices = load_obj_result.indices;
        assert(load_obj_result.positions.size() == load_obj_result.texture_coordinates.size());
        m_vertices.reserve(load_obj_result.positions.size());
        for(uint32_t i = 0; i < m_vertices.size(); i++){
            ::memcpy(m_vertices[i].pos, load_obj_result.positions[i].pos, sizeof(m_vertices[i].pos));
            ::memcpy(m_vertices[i].tex_coord, load_obj_result.texture_coordinates[i].uv, sizeof(m_vertices[i].tex_coord));
        }

        Renderer::CreateVertexBuffer(m_vertex_buffer, m_vertex_memory, m_vertices);
        Renderer::CreateIndexBuffer(m_indices_buffer, m_indices_memory, m_indices);

        auto const texture_asset = FileSystem::LoadTexture(m_texture_file_address.c_str());

        // Texture image
        auto const is_create_texture_successful = Renderer::CreateTextureImage(
            m_texture_image,
            m_texture_memory,
            m_texture_image_view,
            texture_asset
        );
        if(true == is_create_texture_successful) {
            // Texture sampler
            m_sampler = Renderer::CreateTextureSampler();
        }
    }
    return m_valid;
}

void Mesh::draw () {
}

bool Mesh::shutdown() {
    Renderer::DestroyTextureImage(&m_texture_image, &m_texture_memory);
    Renderer::DestoryBuffer(&m_vertex_buffer, &m_vertex_memory);
    Renderer::DestoryBuffer(&m_indices_buffer, &m_indices_memory);
    return true;
}

}
}