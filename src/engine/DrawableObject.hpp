#pragma once

#include "RenderFrontend.hpp"

namespace MFA {

namespace RF = RenderFrontend;
namespace RB = RenderBackend;
// TODO Handle system could be useful
class DrawableObject {
public:
    DrawableObject(
        RF::MeshBuffers & mesh_buffers_,
        RB::GpuTexture & gpu_texture_,
        RF::SamplerGroup & sampler_group_,
        size_t const uniform_buffer_size,
        VkDescriptorSetLayout_T * descriptor_set_layout
    );
    ~DrawableObject();
    void draw(RF::DrawPass & draw_pass);
    template<typename T>
    void update_uniform_buffer(RF::DrawPass const & draw_pass, T ubo) const {
        update_uniform_buffer(draw_pass, CBlobAliasOf(ubo));
    };
    void update_uniform_buffer(RF::DrawPass const & draw_pass, CBlob ubo) const;
    void shutdown();
    [[nodiscard]]
    bool is_valid() const {return m_is_valid;}
private:
    RF::MeshBuffers & m_mesh_buffers;
    RB::GpuTexture & m_gpu_texture;
    RF::SamplerGroup & m_sampler_group;
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
