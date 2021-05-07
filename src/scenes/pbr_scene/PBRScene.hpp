#pragma once

#include "engine/Scene.hpp"
#include "engine/RenderFrontend.hpp"

class PBRScene final : public MFA::Scene {
public:

    explicit PBRScene() = default;

    void Init() override;

    void Shutdown() override;

    void OnDraw(uint32_t delta_time, MFA::RenderFrontend::DrawPass & draw_pass) override;

    void OnUI(uint32_t delta_time, MFA::RenderFrontend::DrawPass & draw_pass) override;

    void OnResize() override;

private:

    void updateAllDescriptorSets();

    void updateDescriptorSet(uint8_t index);

    void updateProjection();

    static constexpr float Z_NEAR = 0.1f;
    static constexpr float Z_FAR = 1000.0f;

    struct _MaterialInformation {
        char const * name;
        float color[3];
        float roughness;
        float metallic;
    };

    static constexpr int CustomMaterialIndex = 7;
    static constexpr _MaterialInformation MaterialInformation [8] = {
        { .name = "Gold", .color = {1.0f, 0.765557f, 0.336057f}, .roughness = 0.1f, .metallic = 1.0f },
        { .name = "Copper", .color = {0.955008f, 0.637427f, 0.538163f}, .roughness = 0.1f, .metallic = 1.0f },
        { .name = "Chromium", .color = {0.549585f, 0.556114f, 0.554256f}, .roughness = 0.1f, .metallic = 1.0f },
        { .name = "Nickel", .color = {0.659777f, 0.608679f, 0.525649f}, .roughness = 0.1f, .metallic = 1.0f },
        { .name = "Titanium", .color = {0.541931f, 0.496791f, 0.449419f}, .roughness = 0.1f, .metallic = 1.0f },
        { .name = "Cobalt", .color = {0.662124f, 0.654864f, 0.633732f}, .roughness = 0.1f, .metallic = 1.0f },
        { .name = "Platinum", .color = {0.672411f, 0.637331f, 0.585456f}, .roughness = 0.1f, .metallic = 1.0f },
        // Testing materials
        //{ .name = "White", .color = {1.0f, 1.0f, 1.0f}, .roughness = 0.1f, .metallic = 1.0f },
        { .name = "Custom", .color = {1.0f, 0.0f, 0.0f}, .roughness = 0.5f, .metallic = 0.1f },
        //{ .name = "Blue", .color = {0.0f, 0.0f, 1.0f}, .roughness = 0.1f, .metallic = 1.0f },
        //{ .name = "Black", .color = {0.0f, 0.0f, 0.0f}, .roughness = 0.1f, .metallic = 1.0f },
    };

    MFA::RenderFrontend::UniformBufferGroup m_material_buffer_group;
    struct MaterialBuffer {
        float albedo[3];        // BaseColor
        alignas(4) float metallic;
        alignas(4) float roughness;
        //alignas(4) float ao;               // Emission  
    } m_material_data {};

    MFA::RenderFrontend::UniformBufferGroup m_light_view_buffer_group;
    struct LightViewBuffer {
        alignas(16) float camPos[3];
        alignas(4) int32_t lightCount;
        alignas(16) float lightPositions[3];
        //alignas(16) float lightColors[3];
    } m_light_view_data {};

    MFA::RenderFrontend::UniformBufferGroup m_transformation_buffer_group;
    struct TransformationBuffer {
        alignas(16 * 4) float rotation[16];
        alignas(16 * 4) float transform[16];
        alignas(16 * 4) float perspective[16];
    } m_translate_data {};

    MFA::RenderFrontend::GpuModel m_sphere {};

    VkDescriptorSetLayout_T * m_descriptor_set_layout = nullptr;
    MFA::RenderFrontend::DrawPipeline m_draw_pipeline {};
   // https://seblagarde.wordpress.com/2011/08/17/feeding-a-physical-based-lighting-mode/
    float m_sphere_rotation [3] {0, 0, 0};
    float m_sphere_position[3] {0, 0, -6};
    int m_selected_material_index = 0;
    float m_sphere_color[3] {0.0f, 0.0f, 0.0f};

    float m_sphere_metallic = 1.0f;
    float m_sphere_roughness = 0.1f;
    //float m_sphere_emission = 0.0f; // For now, it's a constant value

    float const m_camera_position[3] {0.0f, 0.0f, 0.0f};   // For now, it's a constant value
 
    int32_t m_light_count = 1; // For now, it's a constant value
    float m_light_position[3] {0.0f, 0.0f, -2.0f};
    //float m_light_colors[3] {1.0f, 1.0f, 1.0f};

    std::vector<VkDescriptorSet_T *> m_sphere_descriptor_sets {};
};
