#pragma once

#include "RenderFrontend.hpp"

namespace MFA {

namespace RF = RenderFrontend;
namespace RB = RenderBackend;
// TODO Handle system could be useful
class DrawableObject {
public:
    explicit DrawableObject() = default;
    DrawableObject(
        RF::GpuModel & model_,
        RF::SamplerGroup & sampler_group_,
        size_t uniform_buffer_size,
        VkDescriptorSetLayout_T * descriptor_set_layout
    );
    ~DrawableObject();
    DrawableObject & operator= (DrawableObject && rhs) noexcept {
        this->m_required_draw_calls = rhs.m_required_draw_calls;
        this->m_is_valid = rhs.m_is_valid;
        this->m_model = rhs.m_model;
        this->m_sampler_group = rhs.m_sampler_group;
        this->m_descriptor_sets = std::move(rhs.m_descriptor_sets);
        this->m_uniform_buffer_group = std::move(rhs.m_uniform_buffer_group);
        return *this;
    }
    void draw(RF::DrawPass & draw_pass);
    template<typename T>
    void update_uniform_buffer(RF::DrawPass const & draw_pass, T ubo) const {
        update_uniform_buffer(draw_pass, CBlobAliasOf(ubo));
    }
    void update_uniform_buffer(RF::DrawPass const & draw_pass, CBlob ubo) const;
    void shutdown();
    [[nodiscard]]
    bool is_valid() const {return m_is_valid;}
private:
    // Note: Order is important
    U32 m_required_draw_calls;
    RF::GpuModel * m_model = nullptr;
    RF::SamplerGroup * m_sampler_group = nullptr;
    std::vector<VkDescriptorSet_T *> m_descriptor_sets {};
    RF::UniformBufferGroup m_uniform_buffer_group {};
    bool m_is_valid = false;
};

// TODO Future plan
//class DrawableObjectFactory {
//public:
//    // DrawableObjectFactory will take care of destruction and construction(If path provided)
//    void init(
//       RF::SamplerGroup && sampler_group,
//       RF::MeshBuffers && mesh_buffers,
//       RB::GpuTexture m_gpu_texture
//    );
//    // TODO void init(char const * mesh_path, char const * texture_path, SamplerGroup && sampler)
//private:
//    RF::SamplerGroup m_sampler_group {};
//    RF::MeshBuffers m_mesh_buffers {};
//    RB::GpuTexture m_gpu_texture {};
//    std::vector<DrawableObject> objects;
//};

//DrawableObjectFactory CreateDrawableObjectFactory

}
