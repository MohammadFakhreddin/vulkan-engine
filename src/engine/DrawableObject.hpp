#pragma once

#include "RenderFrontend.hpp"

namespace MFA {

namespace RF = RenderFrontend;
namespace RB = RenderBackend;
// TODO Start from here, Drawable object currently can be a struct that holds data, Also we can update all descriptor sets once at start
// TODO Handle system could be useful
class DrawableObject {
public:

    DrawableObject() = default;

    ~DrawableObject() = default;

    DrawableObject(
        RF::GpuModel & model_,
        VkDescriptorSetLayout_T * descriptor_set_layout
    );

    DrawableObject & operator= (DrawableObject && rhs) noexcept {
        this->m_required_draw_calls = rhs.m_required_draw_calls;
        this->m_model = rhs.m_model;
        this->m_descriptor_sets = std::move(rhs.m_descriptor_sets);
        return *this;
    }

    DrawableObject (DrawableObject const &) noexcept = delete;

    DrawableObject (DrawableObject &&) noexcept = delete;

    DrawableObject & operator = (DrawableObject const &) noexcept = delete;

    [[nodiscard]]
    U32 get_required_draw_calls() const;

    [[nodiscard]]
    RF::GpuModel * get_model() const;

    [[nodiscard]]
    U32 get_descriptor_set_count() const;

    [[nodiscard]]
    VkDescriptorSet_T * get_descriptor_set(U32 index);

    // Only for model local buffers
    void create_uniform_buffer(char const * name, U32 size);

    // Only for model local buffers
    void delete_uniform_buffers();

    void update_uniform_buffer(RF::DrawPass const & pass, char const * name, CBlob ubo);
    //void draw(RF::DrawPass & draw_pass);
    //template<typename T>
    //void update_uniform_buffer(RF::DrawPass const & draw_pass, T ubo) const {
        //update_uniform_buffer(draw_pass, CBlobAliasOf(ubo));
    //}
    //void update_uniform_buffer(RF::DrawPass const & draw_pass, CBlob ubo) const;
    //void shutdown();
    //[[nodiscard]]
    //bool is_valid() const {return m_is_valid;}

private:
    // Note: Order is important
    U32 m_required_draw_calls = 0;
    RF::GpuModel * m_model = nullptr;
    std::vector<VkDescriptorSet_T *> m_descriptor_sets {};
    std::unordered_map<std::string, RF::UniformBufferGroup> m_uniforma_buffers;
};

}
