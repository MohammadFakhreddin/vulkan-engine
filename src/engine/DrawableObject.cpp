#include "DrawableObject.hpp"

namespace MFA {
// TODO We need RCMGMT very bad
// We need other overrides for easier use as well
// TODO Each submesh may have it's own textures, We need a data structure called GPUMesh to store all information
DrawableObject::DrawableObject(
    RF::GpuModel & model_,
    RF::SamplerGroup & sampler_group_,
    size_t const uniform_buffer_size,
    VkDescriptorSetLayout_T * descriptor_set_layout
)
    : m_model(&model_)
    , m_sampler_group(&sampler_group_)
    , m_descriptor_sets(RF::CreateDescriptorSets(descriptor_set_layout))
    , m_uniform_buffer_group(RF::CreateUniformBuffer(uniform_buffer_size))
    , m_is_valid(true)
{
    MFA_ASSERT(m_model->valid);
    MFA_ASSERT(m_model->model_asset.mesh.valid());
}

// TODO Constructor that asks for layout,
// TODO constructor that uses paths (Factory)

DrawableObject::~DrawableObject() {
    shutdown();
}

void DrawableObject::draw(RF::DrawPass & draw_pass) {
    RF::BindDescriptorSets(
        draw_pass,
        m_descriptor_sets.data()
    );
    auto const * header_object = m_model->model_asset.mesh.header_object();
    MFA_PTR_ASSERT(header_object);
    // TODo Start from here, We should have only 1 buffer for vertices and indices and use offset for drawing.
    for (U32 i = 0; i < m_model->mesh_buffers.sub_mesh_buffers.size(); i++) {
        auto const & sub_mesh_buffers = m_model->mesh_buffers.sub_mesh_buffers[i];
        auto const & current_sub_mesh = header_object->sub_meshes[i];
        RF::UpdateDescriptorSetsBasic(
            draw_pass,
            m_descriptor_sets.data(),
            m_uniform_buffer_group,
            m_model->textures[current_sub_mesh.base_color_texture_index],
            *m_sampler_group
        );
        BindVertexBuffer(draw_pass, sub_mesh_buffers.vertices_buffers);
        BindIndexBuffer(draw_pass, sub_mesh_buffers.indices_buffers);
        DrawIndexed(draw_pass, sub_mesh_buffers.indices_count);
    }

}

void DrawableObject::update_uniform_buffer(RF::DrawPass const & draw_pass, CBlob const ubo) const {
    RF::UpdateUniformBuffer(
        draw_pass,
        m_uniform_buffer_group, 
        ubo
    );
}

void DrawableObject::shutdown() {
    if(true == m_is_valid) {
        m_is_valid = false;
        RF::DestroyUniformBuffer(m_uniform_buffer_group);
    }
}



};
