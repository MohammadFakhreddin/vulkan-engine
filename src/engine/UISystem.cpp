#include "UISystem.hpp"

#include "BedrockMemory.hpp"
#include "../tools/Importer.hpp"
#include "libs/imgui/imgui.h"

#include <SDL2/SDL.h>

namespace MFA::UISystem {

namespace RB = RenderBackend;
namespace Importer = Importer;

//-----------------------------------------------------------------------------
// SHADERS
//-----------------------------------------------------------------------------

// glsl_shader.vert, compiled with:
// # glslangValidator -V -x -o glsl_shader.vert.u32 glsl_shader.vert
/*
#version 450 core
layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aUV;
layout(location = 2) in vec4 aColor;
layout(push_constant) uniform uPushConstant { vec2 uScale; vec2 uTranslate; } pc;

out gl_PerVertex { vec4 gl_Position; };
layout(location = 0) out struct { vec4 Color; vec2 UV; } Out;

void main()
{
    Out.Color = aColor;
    Out.UV = aUV;
    gl_Position = vec4(aPos * pc.uScale + pc.uTranslate, 0, 1);
}
*/
static uint32_t vertex_shader_spv[] =
{
    0x07230203,0x00010000,0x00080001,0x0000002e,0x00000000,0x00020011,0x00000001,0x0006000b,
    0x00000001,0x4c534c47,0x6474732e,0x3035342e,0x00000000,0x0003000e,0x00000000,0x00000001,
    0x000a000f,0x00000000,0x00000004,0x6e69616d,0x00000000,0x0000000b,0x0000000f,0x00000015,
    0x0000001b,0x0000001c,0x00030003,0x00000002,0x000001c2,0x00040005,0x00000004,0x6e69616d,
    0x00000000,0x00030005,0x00000009,0x00000000,0x00050006,0x00000009,0x00000000,0x6f6c6f43,
    0x00000072,0x00040006,0x00000009,0x00000001,0x00005655,0x00030005,0x0000000b,0x0074754f,
    0x00040005,0x0000000f,0x6c6f4361,0x0000726f,0x00030005,0x00000015,0x00565561,0x00060005,
    0x00000019,0x505f6c67,0x65567265,0x78657472,0x00000000,0x00060006,0x00000019,0x00000000,
    0x505f6c67,0x7469736f,0x006e6f69,0x00030005,0x0000001b,0x00000000,0x00040005,0x0000001c,
    0x736f5061,0x00000000,0x00060005,0x0000001e,0x73755075,0x6e6f4368,0x6e617473,0x00000074,
    0x00050006,0x0000001e,0x00000000,0x61635375,0x0000656c,0x00060006,0x0000001e,0x00000001,
    0x61725475,0x616c736e,0x00006574,0x00030005,0x00000020,0x00006370,0x00040047,0x0000000b,
    0x0000001e,0x00000000,0x00040047,0x0000000f,0x0000001e,0x00000002,0x00040047,0x00000015,
    0x0000001e,0x00000001,0x00050048,0x00000019,0x00000000,0x0000000b,0x00000000,0x00030047,
    0x00000019,0x00000002,0x00040047,0x0000001c,0x0000001e,0x00000000,0x00050048,0x0000001e,
    0x00000000,0x00000023,0x00000000,0x00050048,0x0000001e,0x00000001,0x00000023,0x00000008,
    0x00030047,0x0000001e,0x00000002,0x00020013,0x00000002,0x00030021,0x00000003,0x00000002,
    0x00030016,0x00000006,0x00000020,0x00040017,0x00000007,0x00000006,0x00000004,0x00040017,
    0x00000008,0x00000006,0x00000002,0x0004001e,0x00000009,0x00000007,0x00000008,0x00040020,
    0x0000000a,0x00000003,0x00000009,0x0004003b,0x0000000a,0x0000000b,0x00000003,0x00040015,
    0x0000000c,0x00000020,0x00000001,0x0004002b,0x0000000c,0x0000000d,0x00000000,0x00040020,
    0x0000000e,0x00000001,0x00000007,0x0004003b,0x0000000e,0x0000000f,0x00000001,0x00040020,
    0x00000011,0x00000003,0x00000007,0x0004002b,0x0000000c,0x00000013,0x00000001,0x00040020,
    0x00000014,0x00000001,0x00000008,0x0004003b,0x00000014,0x00000015,0x00000001,0x00040020,
    0x00000017,0x00000003,0x00000008,0x0003001e,0x00000019,0x00000007,0x00040020,0x0000001a,
    0x00000003,0x00000019,0x0004003b,0x0000001a,0x0000001b,0x00000003,0x0004003b,0x00000014,
    0x0000001c,0x00000001,0x0004001e,0x0000001e,0x00000008,0x00000008,0x00040020,0x0000001f,
    0x00000009,0x0000001e,0x0004003b,0x0000001f,0x00000020,0x00000009,0x00040020,0x00000021,
    0x00000009,0x00000008,0x0004002b,0x00000006,0x00000028,0x00000000,0x0004002b,0x00000006,
    0x00000029,0x3f800000,0x00050036,0x00000002,0x00000004,0x00000000,0x00000003,0x000200f8,
    0x00000005,0x0004003d,0x00000007,0x00000010,0x0000000f,0x00050041,0x00000011,0x00000012,
    0x0000000b,0x0000000d,0x0003003e,0x00000012,0x00000010,0x0004003d,0x00000008,0x00000016,
    0x00000015,0x00050041,0x00000017,0x00000018,0x0000000b,0x00000013,0x0003003e,0x00000018,
    0x00000016,0x0004003d,0x00000008,0x0000001d,0x0000001c,0x00050041,0x00000021,0x00000022,
    0x00000020,0x0000000d,0x0004003d,0x00000008,0x00000023,0x00000022,0x00050085,0x00000008,
    0x00000024,0x0000001d,0x00000023,0x00050041,0x00000021,0x00000025,0x00000020,0x00000013,
    0x0004003d,0x00000008,0x00000026,0x00000025,0x00050081,0x00000008,0x00000027,0x00000024,
    0x00000026,0x00050051,0x00000006,0x0000002a,0x00000027,0x00000000,0x00050051,0x00000006,
    0x0000002b,0x00000027,0x00000001,0x00070050,0x00000007,0x0000002c,0x0000002a,0x0000002b,
    0x00000028,0x00000029,0x00050041,0x00000011,0x0000002d,0x0000001b,0x0000000d,0x0003003e,
    0x0000002d,0x0000002c,0x000100fd,0x00010038
};

// glsl_shader.frag, compiled with:
// # glslangValidator -V -x -o glsl_shader.frag.u32 glsl_shader.frag
/*
#version 450 core
layout(location = 0) out vec4 fColor;
layout(set=0, binding=0) uniform sampler2D sTexture;
layout(location = 0) in struct { vec4 Color; vec2 UV; } In;
void main()
{
    fColor = In.Color * texture(sTexture, In.UV.st);
}
*/
static uint32_t fragment_shader_spv[] =
{
    0x07230203,0x00010000,0x00080001,0x0000001e,0x00000000,0x00020011,0x00000001,0x0006000b,
    0x00000001,0x4c534c47,0x6474732e,0x3035342e,0x00000000,0x0003000e,0x00000000,0x00000001,
    0x0007000f,0x00000004,0x00000004,0x6e69616d,0x00000000,0x00000009,0x0000000d,0x00030010,
    0x00000004,0x00000007,0x00030003,0x00000002,0x000001c2,0x00040005,0x00000004,0x6e69616d,
    0x00000000,0x00040005,0x00000009,0x6c6f4366,0x0000726f,0x00030005,0x0000000b,0x00000000,
    0x00050006,0x0000000b,0x00000000,0x6f6c6f43,0x00000072,0x00040006,0x0000000b,0x00000001,
    0x00005655,0x00030005,0x0000000d,0x00006e49,0x00050005,0x00000016,0x78655473,0x65727574,
    0x00000000,0x00040047,0x00000009,0x0000001e,0x00000000,0x00040047,0x0000000d,0x0000001e,
    0x00000000,0x00040047,0x00000016,0x00000022,0x00000000,0x00040047,0x00000016,0x00000021,
    0x00000000,0x00020013,0x00000002,0x00030021,0x00000003,0x00000002,0x00030016,0x00000006,
    0x00000020,0x00040017,0x00000007,0x00000006,0x00000004,0x00040020,0x00000008,0x00000003,
    0x00000007,0x0004003b,0x00000008,0x00000009,0x00000003,0x00040017,0x0000000a,0x00000006,
    0x00000002,0x0004001e,0x0000000b,0x00000007,0x0000000a,0x00040020,0x0000000c,0x00000001,
    0x0000000b,0x0004003b,0x0000000c,0x0000000d,0x00000001,0x00040015,0x0000000e,0x00000020,
    0x00000001,0x0004002b,0x0000000e,0x0000000f,0x00000000,0x00040020,0x00000010,0x00000001,
    0x00000007,0x00090019,0x00000013,0x00000006,0x00000001,0x00000000,0x00000000,0x00000000,
    0x00000001,0x00000000,0x0003001b,0x00000014,0x00000013,0x00040020,0x00000015,0x00000000,
    0x00000014,0x0004003b,0x00000015,0x00000016,0x00000000,0x0004002b,0x0000000e,0x00000018,
    0x00000001,0x00040020,0x00000019,0x00000001,0x0000000a,0x00050036,0x00000002,0x00000004,
    0x00000000,0x00000003,0x000200f8,0x00000005,0x00050041,0x00000010,0x00000011,0x0000000d,
    0x0000000f,0x0004003d,0x00000007,0x00000012,0x00000011,0x0004003d,0x00000014,0x00000017,
    0x00000016,0x00050041,0x00000019,0x0000001a,0x0000000d,0x00000018,0x0004003d,0x0000000a,
    0x0000001b,0x0000001a,0x00050057,0x00000007,0x0000001c,0x00000017,0x0000001b,0x00050085,
    0x00000007,0x0000001d,0x00000012,0x0000001c,0x0003003e,0x00000009,0x0000001d,0x000100fd,
    0x00010038
};

static VkDeviceSize g_BufferMemoryAlignment = 256;

struct State {
    RF::SamplerGroup font_sampler {};
    VkDescriptorSetLayout_T * descriptor_set_layout = nullptr;
    RB::GpuShader vertex_shader {};
    RB::GpuShader fragment_shader {};
    std::vector<VkDescriptorSet_T *> descriptor_sets {};
    RF::DrawPipeline draw_pipeline {};
    RB::GpuTexture font_texture {};
    bool mouse_pressed[3] {false};
    SDL_Cursor *  mouse_cursors[ImGuiMouseCursor_COUNT] {};
    std::vector<RF::MeshBuffers> mesh_buffers {};
    std::vector<bool> mesh_buffers_validation_status {};
    bool hasFocus = false;
    RF::EventWatchId eventWatchId = -1;
};

static State * state = nullptr;

struct PushConstants {
    float scale[2];
    float translate[2];
};

static int EventWatch(void* data, SDL_Event* event) {
    ImGuiIO& io = ImGui::GetIO();
    switch (event->type)
    {
    case SDL_TEXTINPUT:
        {
            io.AddInputCharactersUTF8(event->text.text);
            return true;
        }
    case SDL_KEYDOWN:
    case SDL_KEYUP:
        {
            int key = event->key.keysym.scancode;
            IM_ASSERT(key >= 0 && key < IM_ARRAYSIZE(io.KeysDown));
            io.KeysDown[key] = (event->type == SDL_KEYDOWN);
            io.KeyShift = ((SDL_GetModState() & KMOD_SHIFT) != 0);
            io.KeyCtrl = ((SDL_GetModState() & KMOD_CTRL) != 0);
            io.KeyAlt = ((SDL_GetModState() & KMOD_ALT) != 0);
#ifdef _WIN32
            io.KeySuper = false;
#else
            io.KeySuper = ((SDL_GetModState() & KMOD_GUI) != 0);
#endif
            return true;
        }
    }
    return false;
}

void Init() {
    state = new State();
    auto const swap_chain_images_count = RF::SwapChainImagesCount();
    state->mesh_buffers.resize(swap_chain_images_count);
    state->mesh_buffers_validation_status.resize(swap_chain_images_count);
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    state->font_sampler = RF::CreateSampler(
        RB::CreateSamplerParams {
            .min_lod = -1000,
            .max_lod = 1000,
            .max_anisotropy = 1.0f
        }
    );

    {// Descriptor set layout
        std::vector<VkDescriptorSetLayoutBinding> binding {1};
        binding[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        binding[0].descriptorCount = 1;
        binding[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        binding[0].pImmutableSamplers = &state->font_sampler.sampler;
        state->descriptor_set_layout = RF::CreateDescriptorSetLayout(
            static_cast<uint8_t>(binding.size()),
            binding.data()
        );
    }

    // Create Descriptor Set:
    state->descriptor_sets = RF::CreateDescriptorSets(state->descriptor_set_layout); // Original number was 1 , Now it creates as many as swap_chain_image_count

    {// Vertex shader
        auto const shader_asset = Importer::ImportShaderFromSPV(
            CBlobAliasOf(vertex_shader_spv),
            AssetSystem::Shader::Stage::Vertex,
            "main"
        );
        state->vertex_shader = RF::CreateShader(shader_asset);
        // TODO Implement them in shutdown as well
    }

    {// Fragment shader
        auto const shader_asset = Importer::ImportShaderFromSPV(
            CBlobAliasOf(fragment_shader_spv),
            AssetSystem::Shader::Stage::Fragment,
            "main"
        );
        state->fragment_shader = RF::CreateShader(shader_asset);
    }

    {
        // Constants: we are using 'vec2 offset' and 'vec2 scale' instead of a full 3d projection matrix
        std::vector<VkPushConstantRange> push_constants {
            VkPushConstantRange {
                .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
                .offset = 0,
                .size = sizeof(push_constants)
            }
        };
        
        std::vector<RB::GpuShader> shader_stages {state->vertex_shader, state->fragment_shader};

        VkVertexInputBindingDescription vertex_binding_description {};
        vertex_binding_description.stride = sizeof(ImDrawVert);
        vertex_binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        std::vector<VkVertexInputAttributeDescription> input_attribute_description {3};
        input_attribute_description[0].location = 0;
        input_attribute_description[0].binding = vertex_binding_description.binding;
        input_attribute_description[0].format = VK_FORMAT_R32G32_SFLOAT;
        input_attribute_description[0].offset = offsetof(ImDrawVert, pos);
        input_attribute_description[1].location = 1;
        input_attribute_description[1].binding = vertex_binding_description.binding;
        input_attribute_description[1].format = VK_FORMAT_R32G32_SFLOAT;
        input_attribute_description[1].offset = offsetof(ImDrawVert, uv);
        input_attribute_description[2].location = 2;
        input_attribute_description[2].binding = vertex_binding_description.binding;
        input_attribute_description[2].format = VK_FORMAT_R8G8B8A8_UNORM;
        input_attribute_description[2].offset = offsetof(ImDrawVert, col);

        std::vector<VkDynamicState> dynamic_states = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
        auto dynamic_state_create_info = VkPipelineDynamicStateCreateInfo {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
            .dynamicStateCount = static_cast<uint32_t>(dynamic_states.size()),
            .pDynamicStates = dynamic_states.data(),
        };

        state->draw_pipeline = RF::CreateDrawPipeline(
            static_cast<uint8_t>(shader_stages.size()),
            shader_stages.data(),
            1,
            &state->descriptor_set_layout,
            vertex_binding_description,
            static_cast<uint8_t>(input_attribute_description.size()),
            input_attribute_description.data(),
            RB::CreateGraphicPipelineOptions {
                .font_face = VK_FRONT_FACE_COUNTER_CLOCKWISE,
                .dynamic_state_create_info = &dynamic_state_create_info,
                .depth_stencil {
                    .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
                    .depthTestEnable = false,
                    .depthBoundsTestEnable = false,
                    .stencilTestEnable = false
                },
                .color_blend_attachments {
                    .blendEnable = VK_TRUE,
                    .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
                    .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
                    .colorBlendOp = VK_BLEND_OP_ADD,
                    .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
                    .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
                    .alphaBlendOp = VK_BLEND_OP_ADD,
                    .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
                },
                .push_constants_range_count = static_cast<uint8_t>(push_constants.size()),
                .push_constant_ranges = push_constants.data(),
                .use_static_viewport_and_scissor = false
            }
        );
    }

    {// Creating fonts textures
        // Load Fonts
        // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
        // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
        // - If the file cannot be loaded, the function will return NULL. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
        // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
        // - Read 'docs/FONTS.md' for more instructions and details.
        // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
        //io.Fonts->AddFontDefault();
        //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
        //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
        //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
        //io.Fonts->AddFontFromFileTTF("../../misc/fonts/ProggyTiny.ttf", 10.0f);
        //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());
        //IM_ASSERT(font != NULL);
        
        ImGuiIO & io = ImGui::GetIO();

        // Setup Dear ImGui style
        ImGui::StyleColorsDark();
    
        Byte * pixels = nullptr;
        int32_t width, height;
        io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
        MFA_ASSERT(pixels != nullptr);
        MFA_ASSERT(width > 0);
        MFA_ASSERT(height > 0);
        uint8_t const components_count = 4;
        uint8_t const depth = 1;
        uint8_t const slices = 1;
        size_t const image_size = width * height * components_count * sizeof(Byte);
        auto texture_asset = Importer::ImportInMemoryTexture(
            CBlob {pixels, image_size},
            width,
            height,
            AssetSystem::TextureFormat::UNCOMPRESSED_UNORM_R8G8B8A8_LINEAR,
            components_count,
            depth,
            slices,
            Importer::ImportTextureOptions {.tryToGenerateMipmaps = false, .preferSrgb = false}
        );
        // TODO Support from in memory import of images inside importer
        state->font_texture = RF::CreateTexture(texture_asset);

        // Keyboard mapping. ImGui will use those indices to peek into the io.KeysDown[] array.
        io.KeyMap[ImGuiKey_Tab] = SDL_SCANCODE_TAB;
        io.KeyMap[ImGuiKey_LeftArrow] = SDL_SCANCODE_LEFT;
        io.KeyMap[ImGuiKey_RightArrow] = SDL_SCANCODE_RIGHT;
        io.KeyMap[ImGuiKey_UpArrow] = SDL_SCANCODE_UP;
        io.KeyMap[ImGuiKey_DownArrow] = SDL_SCANCODE_DOWN;
        io.KeyMap[ImGuiKey_PageUp] = SDL_SCANCODE_PAGEUP;
        io.KeyMap[ImGuiKey_PageDown] = SDL_SCANCODE_PAGEDOWN;
        io.KeyMap[ImGuiKey_Home] = SDL_SCANCODE_HOME;
        io.KeyMap[ImGuiKey_End] = SDL_SCANCODE_END;
        io.KeyMap[ImGuiKey_Insert] = SDL_SCANCODE_INSERT;
        io.KeyMap[ImGuiKey_Delete] = SDL_SCANCODE_DELETE;
        io.KeyMap[ImGuiKey_Backspace] = SDL_SCANCODE_BACKSPACE;
        io.KeyMap[ImGuiKey_Space] = SDL_SCANCODE_SPACE;
        io.KeyMap[ImGuiKey_Enter] = SDL_SCANCODE_RETURN;
        io.KeyMap[ImGuiKey_Escape] = SDL_SCANCODE_ESCAPE;
        io.KeyMap[ImGuiKey_KeyPadEnter] = SDL_SCANCODE_KP_ENTER;
        io.KeyMap[ImGuiKey_A] = SDL_SCANCODE_A;
        io.KeyMap[ImGuiKey_C] = SDL_SCANCODE_C;
        io.KeyMap[ImGuiKey_V] = SDL_SCANCODE_V;
        io.KeyMap[ImGuiKey_X] = SDL_SCANCODE_X;
        io.KeyMap[ImGuiKey_Y] = SDL_SCANCODE_Y;
        io.KeyMap[ImGuiKey_Z] = SDL_SCANCODE_Z;
    }

    state->eventWatchId = RF::AddEventWatch(EventWatch);
}

static void UpdateMousePositionAndButtons() {
    auto & io = ImGui::GetIO();

    // Set OS mouse position if requested (rarely used, only when ImGuiConfigFlags_NavEnableSetMousePos is enabled by user)
    if (io.WantSetMousePos) {
        RF::WarpMouseInWindow(static_cast<int32_t>(io.MousePos.x), static_cast<int32_t>(io.MousePos.y));
    } else {
        io.MousePos = ImVec2(-FLT_MAX, -FLT_MAX);
    }
    int mx, my;
    Uint32 const mouse_buttons = SDL_GetMouseState(&mx, &my);
    io.MouseDown[0] = state->mouse_pressed[0] || (mouse_buttons & SDL_BUTTON(SDL_BUTTON_LEFT)) != 0;  // If a mouse press event came, always pass it as "mouse held this frame", so we don't miss click-release events that are shorter than 1 frame.
    io.MouseDown[1] = state->mouse_pressed[1] || (mouse_buttons & SDL_BUTTON(SDL_BUTTON_RIGHT)) != 0;
    io.MouseDown[2] = state->mouse_pressed[2] || (mouse_buttons & SDL_BUTTON(SDL_BUTTON_MIDDLE)) != 0;
    state->mouse_pressed[0] = state->mouse_pressed[1] = state->mouse_pressed[2] = false;
    if (RF::GetWindowFlags() & SDL_WINDOW_INPUT_FOCUS) {
        io.MousePos = ImVec2(static_cast<float>(mx), static_cast<float>(my));
    }

    //// TODO Move this to input manager
    //auto const * sdlKeysDown = RF::GetKeyboardState();
    //if (sdlKeysDown[SDL_SCANCODE_0]) {
    //    io.AddInputCharacter('0');
    //}
    //if (sdlKeysDown[SDL_SCANCODE_1]) {
    //    io.AddInputCharacter('1');
    //}
    //if (sdlKeysDown[SDL_SCANCODE_2]) {
    //    io.AddInputCharacter('2');
    //}
    //if (sdlKeysDown[SDL_SCANCODE_3]) {
    //    io.AddInputCharacter('3');
    //}
    //if (sdlKeysDown[SDL_SCANCODE_4]) {
    //    io.AddInputCharacter('4');
    //}
    //if (sdlKeysDown[SDL_SCANCODE_5]) {
    //    io.AddInputCharacter('5');
    //}
    //if (sdlKeysDown[SDL_SCANCODE_6]) {
    //    io.AddInputCharacter('6');
    //}
    //if (sdlKeysDown[SDL_SCANCODE_7]) {
    //    io.AddInputCharacter('7');
    //}
    //if (sdlKeysDown[SDL_SCANCODE_8]) {
    //    io.AddInputCharacter('8');
    //}
    //if (sdlKeysDown[SDL_SCANCODE_9]) {
    //    io.AddInputCharacter('9');
    //}
}

static void UpdateMouseCursor() {
    auto & io = ImGui::GetIO();

    if (io.ConfigFlags & ImGuiConfigFlags_NoMouseCursorChange) {
        return;
    }
    ImGuiMouseCursor imgui_cursor = ImGui::GetMouseCursor();
    if (io.MouseDrawCursor || imgui_cursor == ImGuiMouseCursor_None) {
        // Hide OS mouse cursor if imgui is drawing it or if it wants no cursor
        SDL_ShowCursor(SDL_FALSE);
    } else {
        // Show OS mouse cursor
        SDL_SetCursor(state->mouse_cursors[imgui_cursor] ? state->mouse_cursors[imgui_cursor] : state->mouse_cursors[ImGuiMouseCursor_Arrow]);
        SDL_ShowCursor(SDL_TRUE);
    }
}

void OnNewFrame(
    float const deltaTimeInSec,
    RF::DrawPass & drawPass,
    RecordUICallback const & recordUiCallback
) {
    ImGuiIO& io = ImGui::GetIO();
    IM_ASSERT(io.Fonts->IsBuilt() && "Font atlas not built! It is generally built by the renderer backend. Missing call to renderer _NewFrame() function? e.g. ImGui_ImplOpenGL3_NewFrame().");

    // Setup display size (every frame to accommodate for window resizing)
    int32_t window_width, window_height;
    int32_t drawable_width, drawable_height;
    RF::GetWindowSize(window_width, window_height);
    if (RF::GetWindowFlags() & SDL_WINDOW_MINIMIZED) {
        window_width = window_height = 0;
    }
    RF::GetDrawableSize(drawable_width, drawable_height);
    io.DisplaySize = ImVec2(static_cast<float>(window_width), static_cast<float>(window_height));
    if (window_width > 0 && window_height > 0) {
        io.DisplayFramebufferScale = ImVec2(
            static_cast<float>(drawable_width) / static_cast<float>(window_width), 
            static_cast<float>(drawable_height) / static_cast<float>(window_height)
        );
    }
    io.DeltaTime = deltaTimeInSec;
    UpdateMousePositionAndButtons();
    UpdateMouseCursor();
    // Start the Dear ImGui frame
    ImGui::NewFrame();

    state->hasFocus = false;
    if(recordUiCallback != nullptr) {
        recordUiCallback();
    }

    ImGui::Render();
    auto * draw_data = ImGui::GetDrawData();
    // Avoid rendering when minimized, scale coordinates for retina displays (screen coordinates != framebuffer coordinates)
    auto const frame_buffer_width = static_cast<float>(draw_data->DisplaySize.x * draw_data->FramebufferScale.x);
    auto const frame_buffer_height = static_cast<float>(draw_data->DisplaySize.y * draw_data->FramebufferScale.y);
    if (frame_buffer_width > 0 && frame_buffer_height > 0) {
        if (draw_data->TotalVtxCount > 0) {
            // Create or resize the vertex/index buffers
            size_t const vertex_size = draw_data->TotalVtxCount * sizeof(ImDrawVert);
            size_t const index_size = draw_data->TotalIdxCount * sizeof(ImDrawIdx);
            auto vertex_data = MFA::Memory::Alloc(vertex_size);
            MFA_DEFER {MFA::Memory::Free(vertex_data);};
            auto index_data = MFA::Memory::Alloc(index_size);
            MFA_DEFER {MFA::Memory::Free(index_data);};
            {
                auto * vertex_ptr = reinterpret_cast<ImDrawVert *>(vertex_data.ptr);
                auto * index_ptr = reinterpret_cast<ImDrawIdx *>(index_data.ptr);
                for (int n = 0; n < draw_data->CmdListsCount; n++)
                {
                    const ImDrawList * cmd_list = draw_data->CmdLists[n];
                    ::memcpy(vertex_ptr, cmd_list->VtxBuffer.Data, cmd_list->VtxBuffer.Size * sizeof(ImDrawVert));
                    ::memcpy(index_ptr, cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx));
                    vertex_ptr += cmd_list->VtxBuffer.Size;
                    index_ptr += cmd_list->IdxBuffer.Size;
                }
            }
            if(state->mesh_buffers_validation_status[drawPass.imageIndex]) {
                RF::DestroyMeshBuffers(state->mesh_buffers[drawPass.imageIndex]);
                state->mesh_buffers_validation_status[drawPass.imageIndex] = false;
            }
            state->mesh_buffers[drawPass.imageIndex].verticesBuffer = RF::CreateVertexBuffer(CBlob {vertex_data.ptr, vertex_data.len});
            state->mesh_buffers[drawPass.imageIndex].indicesBuffer = RF::CreateIndexBuffer(CBlob {index_data.ptr, index_data.len});
            state->mesh_buffers_validation_status[drawPass.imageIndex] = true;
            // Setup desired Vulkan state
            // Bind pipeline and descriptor sets:
            {
                RF::BindDrawPipeline(drawPass, state->draw_pipeline);
                RF::BindDescriptorSet(drawPass, state->descriptor_sets[drawPass.imageIndex]);
            }

            RF::BindIndexBuffer(
                drawPass,
                state->mesh_buffers[drawPass.imageIndex].indicesBuffer,
                0,
                sizeof(ImDrawIdx) == 2 ? VK_INDEX_TYPE_UINT16 : VK_INDEX_TYPE_UINT32
            );
            RF::BindVertexBuffer(
                drawPass,
                state->mesh_buffers[drawPass.imageIndex].verticesBuffer
            );

            // Update the Descriptor Set:
            {
                VkDescriptorImageInfo desc_image[1] = {};
                desc_image[0].sampler = state->font_sampler.sampler;
                desc_image[0].imageView = state->font_texture.image_view();
                desc_image[0].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                std::vector<VkWriteDescriptorSet> write_desc {1};
                write_desc[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                write_desc[0].descriptorCount = 1;
                write_desc[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                write_desc[0].pImageInfo = desc_image;
                RF::UpdateDescriptorSets(
                    static_cast<uint8_t>(state->descriptor_sets.size()),
                    state->descriptor_sets.data(),
                    static_cast<uint8_t>(write_desc.size()),
                    write_desc.data()
                );
            }

            // Setup viewport:
            {
                VkViewport viewport;
                viewport.x = 0;
                viewport.y = 0;
                viewport.width = frame_buffer_width;
                viewport.height = frame_buffer_height;
                viewport.minDepth = 0.0f;
                viewport.maxDepth = 1.0f;
                RF::SetViewport(drawPass, viewport);
            }

            // Setup scale and translation:
            // Our visible imgui space lies from draw_data->DisplayPps (top left) to draw_data->DisplayPos+data_data->DisplaySize (bottom right). DisplayPos is (0,0) for single viewport apps.
            {
                PushConstants constants {};
                constants.scale[0] = 2.0f / draw_data->DisplaySize.x;
                constants.scale[1] = 2.0f / draw_data->DisplaySize.y;
                constants.translate[0] = -1.0f - draw_data->DisplayPos.x * constants.scale[0];
                constants.translate[1] = -1.0f - draw_data->DisplayPos.y * constants.scale[1];
                RF::PushConstants(
                    drawPass,
                    AssetSystem::Shader::Stage::Vertex,
                    0,
                    CBlobAliasOf(constants)
                );
            }

            // Will project scissor/clipping rectangles into frame-buffer space
            ImVec2 const clip_off = draw_data->DisplayPos;         // (0,0) unless using multi-viewports
            ImVec2 const clip_scale = draw_data->FramebufferScale; // (1,1) unless using retina display which are often (2,2)

            // Render command lists
            // (Because we merged all buffers into a single one, we maintain our own offset into them)
            int global_vtx_offset = 0;
            int global_idx_offset = 0;
            for (int n = 0; n < draw_data->CmdListsCount; n++)
            {
                const ImDrawList* cmd_list = draw_data->CmdLists[n];
                for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++)
                {
                    const ImDrawCmd * pcmd = &cmd_list->CmdBuffer[cmd_i];
                    
                    // Project scissor/clipping rectangles into frame-buffer space
                    ImVec4 clip_rect;
                    clip_rect.x = (pcmd->ClipRect.x - clip_off.x) * clip_scale.x;
                    clip_rect.y = (pcmd->ClipRect.y - clip_off.y) * clip_scale.y;
                    clip_rect.z = (pcmd->ClipRect.z - clip_off.x) * clip_scale.x;
                    clip_rect.w = (pcmd->ClipRect.w - clip_off.y) * clip_scale.y;

                    if (clip_rect.x < frame_buffer_width && clip_rect.y < frame_buffer_height && clip_rect.z >= 0.0f && clip_rect.w >= 0.0f)
                    {
                        // Negative offsets are illegal for vkCmdSetScissor
                        if (clip_rect.x < 0.0f)
                            clip_rect.x = 0.0f;
                        if (clip_rect.y < 0.0f)
                            clip_rect.y = 0.0f;

                        // Apply scissor/clipping rectangle
                        VkRect2D scissor {
                            .offset {
                                .x = static_cast<int32_t>(clip_rect.x),
                                .y = static_cast<int32_t>(clip_rect.y)
                            },
                            .extent {
                                .width = static_cast<uint32_t>(clip_rect.z - clip_rect.x),
                                .height = static_cast<uint32_t>(clip_rect.w - clip_rect.y)
                            }
                        };
                        RF::SetScissor(drawPass, scissor);
                        
                        // Draw
                        RF::DrawIndexed(
                            drawPass,
                            pcmd->ElemCount,
                            1,
                            pcmd->IdxOffset + global_idx_offset, 
                            pcmd->VtxOffset + global_vtx_offset, 
                            0
                        );
                    }
                }
                global_idx_offset += cmd_list->IdxBuffer.Size;
                global_vtx_offset += cmd_list->VtxBuffer.Size;
            }
        }
    }
}

void BeginWindow(char const * windowName) {
    ImGui::Begin(windowName);
}

void EndWindow() {
    if (ImGui::IsWindowFocused()) {
        state->hasFocus = true;
    }
    ImGui::End();
}

void SetNextItemWidth(float const nextItemWidth) {
    ImGui::SetNextItemWidth(nextItemWidth);
}

void InputFloat(char const * label, float * value) {
    ImGui::InputFloat(label, value);
}

void InputFloat2(char const * label, float value[2]) {
    ImGui::InputFloat2(label, value);
}

void InputFloat3(char const * label, float value[3]) {
    ImGui::InputFloat3(label, value);    
}

// TODO Maybe we could cache unchanged vertices
void Combo(
    char const * label, 
    int32_t * selectedItemIndex, 
    char const ** items, 
    int32_t const itemsCount
) {
    ImGui::Combo(
        label,
        selectedItemIndex,
        items,
        itemsCount
    );
}

void SliderInt(
    char const * label, 
    int * value, 
    int const minValue, 
    int const maxValue
) {
    ImGui::SliderInt(
        label,
        value,
        minValue,
        maxValue
    );
}

void SliderFloat(
    char const * label, 
    float * value, 
    float const minValue, 
    float const maxValue
) {
    ImGui::SliderFloat(
        label, 
        value, 
        minValue, 
        maxValue
    ); 
}

void Checkbox(char const * label, bool * value) {
    ImGui::Checkbox(label, value);
}

bool HasFocus() {
    return state->hasFocus;
}

void Shutdown() {
    MFA_ASSERT(state->mesh_buffers.size() == state->mesh_buffers_validation_status.size());
    for(auto i = 0; i < state->mesh_buffers_validation_status.size(); i++) {
        if(true == state->mesh_buffers_validation_status[i]) {
            RF::DestroyMeshBuffers(state->mesh_buffers[i]);
            state->mesh_buffers_validation_status[i] = false;
        }
    }
    RF::DestroyTexture(state->font_texture);
    Importer::FreeTexture(state->font_texture.cpu_texture());

    RF::DestroyDrawPipeline(state->draw_pipeline);
    // TODO We can remove shader after creating pipeline
    RF::DestroyShader(state->fragment_shader);
    Importer::FreeShader(state->fragment_shader.cpuShader());
    RF::DestroyShader(state->vertex_shader);
    Importer::FreeShader(state->vertex_shader.cpuShader());

    RF::DestroyDescriptorSetLayout(state->descriptor_set_layout);
    RF::DestroySampler(state->font_sampler);

    RF::RemoveEventWatch(state->eventWatchId);

    delete state;
    state = nullptr;
}

}
