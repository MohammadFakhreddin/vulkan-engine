#include "DrawableObject.hpp"

namespace MFA {
// TODO We need RCMGMT very bad
// We need other overrides for easier use as well
DrawableObject::DrawableObject(
    RF::MeshBuffers & mesh_buffers_,
    RB::GpuTexture & gpu_texture_,
    RF::SamplerGroup & sampler_group_,
    size_t const uniform_buffer_size,
    VkDescriptorSetLayout_T * descriptor_set_layout
)
    : m_mesh_buffers(&mesh_buffers_)
    , m_gpu_texture(&gpu_texture_)
    , m_sampler_group(&sampler_group_)
    , m_descriptor_sets(RF::CreateDescriptorSets(descriptor_set_layout))
    , m_uniform_buffer_group(RF::CreateUniformBuffer(uniform_buffer_size))
    , m_is_valid(true)
{}

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
    // TODO Each mesh may have it's own texture Fuck! (Basically we need material support for that)
    RF::UpdateDescriptorSetsBasic(
        draw_pass,
        m_descriptor_sets.data(),
        m_uniform_buffer_group,
        *m_gpu_texture,
        *m_sampler_group
    );

    for (U32 i = 0; i < m_mesh_buffers->indices_count.size(); i++) {
        BindVertexBuffer(draw_pass, m_mesh_buffers->vertices_buffers[i]);
        BindIndexBuffer(draw_pass, m_mesh_buffers->indices_buffers[i]);
        DrawIndexed(draw_pass, m_mesh_buffers->indices_count[i]);
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
