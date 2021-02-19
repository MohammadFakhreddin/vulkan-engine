#include "DrawableObject.hpp"

namespace MFA {
// TODO We need RCMGMT very bad
// We need other overrides for easier use as well
DrawableObject::DrawableObject(
    RF::GpuModel & model_,
    VkDescriptorSetLayout_T * descriptor_set_layout
)
    : m_required_draw_calls(static_cast<U32>(model_.mesh_buffers.sub_mesh_buffers.size()))
    , m_model(&model_)
    , m_descriptor_sets(RF::CreateDescriptorSets(
        static_cast<U32>(model_.mesh_buffers.sub_mesh_buffers.size()), 
        descriptor_set_layout
    ))
{
    MFA_ASSERT(m_model->valid);
    MFA_ASSERT(m_model->model_asset.mesh.valid());
    MFA_ASSERT(m_required_draw_calls > 0);
}

// TODo Remove this comments
//DrawableObject::~DrawableObject() {
//    shutdown();
//}
//
//void DrawableObject::draw(RF::DrawPass & draw_pass) {
//    auto const * header_object = m_model->model_asset.mesh.header_object();
//    MFA_PTR_ASSERT(header_object);
//    BindVertexBuffer(draw_pass, m_model->mesh_buffers.vertices_buffer);
//    BindIndexBuffer(draw_pass, m_model->mesh_buffers.indices_buffer);
//    for (U32 i = 0; i < m_required_draw_calls; ++i) {
//        auto * current_descriptor_set = m_descriptor_sets[draw_pass.image_index * m_required_draw_calls + i];
//        RF::BindDescriptorSet(draw_pass, current_descriptor_set);
//        auto const & current_sub_mesh = header_object->sub_meshes[i];
//        RF::UpdateDescriptorSetBasic(
//            draw_pass,
//            current_descriptor_set,
//            m_uniform_buffer_group,
//            m_model->textures[current_sub_mesh.base_color_texture_index],
//            *m_sampler_group
//        ); 
//        auto const & sub_mesh_buffers = m_model->mesh_buffers.sub_mesh_buffers[i];
//        DrawIndexed(
//            draw_pass,
//            sub_mesh_buffers.index_count,
//            1,
//            current_sub_mesh.indices_starting_index
//        );
//    }
//}
//
//void DrawableObject::update_uniform_buffer(RF::DrawPass const & draw_pass, CBlob const ubo) const {
//    RF::UpdateUniformBuffer(
//        draw_pass,
//        m_uniform_buffer_group, 
//        ubo
//    );
//}


};
