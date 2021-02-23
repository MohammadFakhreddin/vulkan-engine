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


U32 DrawableObject::get_required_draw_calls() const {
    return m_required_draw_calls;
}

RF::GpuModel * DrawableObject::get_model() const {
    return m_model;
}

U32 DrawableObject::get_descriptor_set_count() const {
    return static_cast<U32>(m_descriptor_sets.size());
}

VkDescriptorSet_T * DrawableObject::get_descriptor_set(U32 const index) {
    MFA_ASSERT(index < m_descriptor_sets.size());
    return m_descriptor_sets[index];
}

VkDescriptorSet_T ** DrawableObject::get_descriptor_sets() {
    return m_descriptor_sets.data();
}


// Only for model local buffers
RF::UniformBufferGroup * DrawableObject::create_uniform_buffer(char const * name, U32 size) {
    m_uniforma_buffers[name] = RF::CreateUniformBuffer(size);
    return &m_uniforma_buffers[name];
}

// Only for model local buffers
void DrawableObject::delete_uniform_buffers() {
    if (m_uniforma_buffers.empty() == false) {
        for (auto & pair : m_uniforma_buffers) {
            RF::DestroyUniformBuffer(pair.second);
        }
        m_uniforma_buffers.clear();
    }
}

void DrawableObject::update_uniform_buffer(RF::DrawPass const & pass, char const * name, CBlob const ubo) {
    auto const find_result = m_uniforma_buffers.find(name);
    if (find_result != m_uniforma_buffers.end()) {
        RF::UpdateUniformBuffer(pass, find_result->second, CBlobAliasOf(ubo));
    }
}

RF::UniformBufferGroup * DrawableObject::get_uniform_buffer(char const * name) {
    auto const find_result = m_uniforma_buffers.find(name);
    if (find_result != m_uniforma_buffers.end()) {
        return &find_result->second;
    }
    return nullptr;
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
