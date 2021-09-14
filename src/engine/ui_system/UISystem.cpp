#include "UISystem.hpp"

#include "engine/BedrockMemory.hpp"
#include "tools/Importer.hpp"
#include "engine/render_system/RenderFrontend.hpp"
#include "UIRecordObject.hpp"
#if defined(__ANDROID__) || defined(__IOS__)
#include "engine/InputManager.hpp"
#endif

#include "engine/render_system/render_passes/display_render_pass/DisplayRenderPass.hpp"
#include "libs/imgui/imgui.h"

#ifdef __DESKTOP__
#include "libs/sdl/SDL.hpp"
#endif

namespace MFA::UISystem {

#if defined(__ANDROID__) || defined(__IOS__)
namespace IM = InputManager;
#endif
//-----------------------------------------------------------------------------
// SHADERS
//-----------------------------------------------------------------------------

// glsl_shader.vert, compiled with:
// # glslangValidator -V -x -o glsl_shader.vert.u32 glsl_shader.vert
/*
#version 450 core
layout(location = 0) in vec2 aPos;d
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
    RT::SamplerGroup fontSampler {};
    VkDescriptorSetLayout descriptorSetLayout {};
    RT::GpuShader vertexShader {};
    RT::GpuShader fragmentShader {};
    RT::DescriptorSetGroup descriptorSetGroup {};
    RT::PipelineGroup drawPipeline {};
    RT::GpuTexture fontTexture {};
    std::vector<RT::MeshBuffers> meshBuffers {};
    std::vector<bool> meshBuffersValidationStatus {};
    bool hasFocus = false;
    std::vector<UIRecordObject *> mRecordObjects {};
#if defined(__ANDROID__) || defined(__IOS__)
    IM::MousePosition previousMousePositionX = 0.0f;
    IM::MousePosition previousMousePositionY = 0.0f;
#endif
#ifdef __DESKTOP__
    MSDL::SDL_Cursor *  mouseCursors[ImGuiMouseCursor_COUNT] {};
    RF::SDLEventWatchId eventWatchId = -1;
#endif
};

static State * state = nullptr;

#ifdef __ANDROID__
static android_app * androidApp = nullptr;
#endif

struct PushConstants {
    float scale[2];
    float translate[2];
};

#ifdef __DESKTOP__
static int EventWatch(void* data, MSDL::SDL_Event* event) {
    ImGuiIO& io = ImGui::GetIO();
    switch (event->type)
    {
    case MSDL::SDL_TEXTINPUT:
        {
            io.AddInputCharactersUTF8(event->text.text);
            return true;
        }
    case MSDL::SDL_KEYDOWN:
    case MSDL::SDL_KEYUP:
        {
            int key = event->key.keysym.scancode;
            IM_ASSERT(key >= 0 && key < IM_ARRAYSIZE(io.KeysDown));
            io.KeysDown[key] = (event->type == MSDL::SDL_KEYDOWN);
            io.KeyShift = ((MSDL::SDL_GetModState() & MSDL::KMOD_SHIFT) != 0);
            io.KeyCtrl = ((MSDL::SDL_GetModState() & MSDL::KMOD_CTRL) != 0);
            io.KeyAlt = ((MSDL::SDL_GetModState() & MSDL::KMOD_ALT) != 0);
#ifdef __PLATFORM_WIN__
            io.KeySuper = false;
#else
            io.KeySuper = ((MSDL::SDL_GetModState() & MSDL::KMOD_GUI) != 0);
#endif
            return true;
        }
    }
    return false;
}
#endif

#ifdef __ANDROID__
static int32_t getDensityDpi(android_app * app) {
    AConfiguration* config = AConfiguration_new();
    AConfiguration_fromAssetManager(config, app->activity->assetManager);
    int32_t density = AConfiguration_getDensity(config);
    AConfiguration_delete(config);
    return density;
}
#endif

void Init() {
    state = new State();
    auto const swapChainImagesCount = RF::GetSwapChainImagesCount();
    state->meshBuffers.resize(swapChainImagesCount);
    state->meshBuffersValidationStatus.resize(swapChainImagesCount);
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    {// FontSampler
        RT::CreateSamplerParams params {};
        params.min_lod = -1000;
        params.max_lod = 1000;
        params.max_anisotropy = 1.0f;
        state->fontSampler = RF::CreateSampler(params);
    }

    {// Descriptor set layout
        std::vector<VkDescriptorSetLayoutBinding> binding {1};
        binding[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        binding[0].descriptorCount = 1;
        binding[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        binding[0].pImmutableSamplers = &state->fontSampler.sampler;
        state->descriptorSetLayout = RF::CreateDescriptorSetLayout(
            static_cast<uint8_t>(binding.size()),
            binding.data()
        );
    }

    // Create Descriptor Set:
    state->descriptorSetGroup = RF::CreateDescriptorSets(state->descriptorSetLayout); // Original number was 1 , Now it creates as many as swap_chain_image_count

    {// Vertex shader
        auto const shader_asset = Importer::ImportShaderFromSPV(
            CBlobAliasOf(vertex_shader_spv),
            AssetSystem::Shader::Stage::Vertex,
            "main"
        );
        state->vertexShader = RF::CreateShader(shader_asset);
        // TODO Implement them in shutdown as well
    }

    {// Fragment shader
        auto const shader_asset = Importer::ImportShaderFromSPV(
            CBlobAliasOf(fragment_shader_spv),
            AssetSystem::Shader::Stage::Fragment,
            "main"
        );
        state->fragmentShader = RF::CreateShader(shader_asset);
    }

    {
        // Constants: we are using 'vec2 offset' and 'vec2 scale' instead of a full 3d projection matrix
        std::vector<VkPushConstantRange> pushConstantRanges {};
        
        VkPushConstantRange pushConstantRange {};
        pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        pushConstantRange.offset = 0;
        pushConstantRange.size = 4 * sizeof(float);
        pushConstantRanges.emplace_back(pushConstantRange);

        //state->vertexShader, state->fragmentShader
        std::vector<RT::GpuShader const *> shaderStages {&state->vertexShader, &state->fragmentShader};
        
        VkVertexInputBindingDescription vertex_binding_description {};
        vertex_binding_description.stride = sizeof(ImDrawVert);
        vertex_binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        std::vector<VkVertexInputAttributeDescription> inputAttributeDescription (3);
        inputAttributeDescription[0].location = 0;
        inputAttributeDescription[0].binding = vertex_binding_description.binding;
        inputAttributeDescription[0].format = VK_FORMAT_R32G32_SFLOAT;
        inputAttributeDescription[0].offset = offsetof(ImDrawVert, pos);
        inputAttributeDescription[1].location = 1;
        inputAttributeDescription[1].binding = vertex_binding_description.binding;
        inputAttributeDescription[1].format = VK_FORMAT_R32G32_SFLOAT;
        inputAttributeDescription[1].offset = offsetof(ImDrawVert, uv);
        inputAttributeDescription[2].location = 2;
        inputAttributeDescription[2].binding = vertex_binding_description.binding;
        inputAttributeDescription[2].format = VK_FORMAT_R8G8B8A8_UNORM;
        inputAttributeDescription[2].offset = offsetof(ImDrawVert, col);

        std::vector<VkDynamicState> dynamicStates { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
        VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo {};
        dynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicStateCreateInfo.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
        dynamicStateCreateInfo.pDynamicStates = dynamicStates.data();
        
        RT::CreateGraphicPipelineOptions pipelineOptions {};
        pipelineOptions.frontFace = VK_FRONT_FACE_CLOCKWISE;
        pipelineOptions.dynamicStateCreateInfo = &dynamicStateCreateInfo;
        pipelineOptions.depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        pipelineOptions.depthStencil.depthTestEnable = false;
        pipelineOptions.depthStencil.depthBoundsTestEnable = false;
        pipelineOptions.depthStencil.stencilTestEnable = false;
        pipelineOptions.colorBlendAttachments.blendEnable = VK_TRUE;
        pipelineOptions.colorBlendAttachments.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        pipelineOptions.colorBlendAttachments.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        pipelineOptions.colorBlendAttachments.colorBlendOp = VK_BLEND_OP_ADD;
        pipelineOptions.colorBlendAttachments.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        pipelineOptions.colorBlendAttachments.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        pipelineOptions.colorBlendAttachments.alphaBlendOp = VK_BLEND_OP_ADD;
        pipelineOptions.colorBlendAttachments.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        pipelineOptions.pushConstantsRangeCount = static_cast<uint8_t>(pushConstantRanges.size());
        pipelineOptions.pushConstantRanges = pushConstantRanges.data();
        pipelineOptions.useStaticViewportAndScissor = false;
        pipelineOptions.cullMode = VK_CULL_MODE_NONE;
        // TODO I wish we could render ui without MaxSamplesCount
        pipelineOptions.rasterizationSamples = RF::GetMaxSamplesCount();
        
        state->drawPipeline = RF::CreatePipeline(
            RF::GetDisplayRenderPass(),
            static_cast<uint8_t>(shaderStages.size()),
            shaderStages.data(),
            1,
            &state->descriptorSetLayout,
            vertex_binding_description,
            static_cast<uint8_t>(inputAttributeDescription.size()),
            inputAttributeDescription.data(),
            pipelineOptions
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

        uint8_t * pixels = nullptr;
        int32_t width, height;
        io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
        MFA_ASSERT(pixels != nullptr);
        MFA_ASSERT(width > 0);
        MFA_ASSERT(height > 0);
        uint8_t const components_count = 4;
        uint8_t const depth = 1;
        uint8_t const slices = 1;
        size_t const image_size = width * height * components_count * sizeof(uint8_t);

        Importer::ImportTextureOptions importTextureOptions {};
        importTextureOptions.tryToGenerateMipmaps = false;
        importTextureOptions.preferSrgb = false;

        auto texture_asset = Importer::ImportInMemoryTexture(
            CBlob {pixels, image_size},
            width,
            height,
            AssetSystem::TextureFormat::UNCOMPRESSED_UNORM_R8G8B8A8_LINEAR,
            components_count,
            depth,
            slices,
            importTextureOptions
        );
        // TODO Support from in memory import of images inside importer
        state->fontTexture = RF::CreateTexture(texture_asset);

#ifdef __DESKTOP__
        // Keyboard mapping. ImGui will use those indices to peek into the io.KeysDown[] array.
        io.KeyMap[ImGuiKey_Tab] = MSDL::SDL_SCANCODE_TAB;
        io.KeyMap[ImGuiKey_LeftArrow] = MSDL::SDL_SCANCODE_LEFT;
        io.KeyMap[ImGuiKey_RightArrow] = MSDL::SDL_SCANCODE_RIGHT;
        io.KeyMap[ImGuiKey_UpArrow] = MSDL::SDL_SCANCODE_UP;
        io.KeyMap[ImGuiKey_DownArrow] = MSDL::SDL_SCANCODE_DOWN;
        io.KeyMap[ImGuiKey_PageUp] = MSDL::SDL_SCANCODE_PAGEUP;
        io.KeyMap[ImGuiKey_PageDown] = MSDL::SDL_SCANCODE_PAGEDOWN;
        io.KeyMap[ImGuiKey_Home] = MSDL::SDL_SCANCODE_HOME;
        io.KeyMap[ImGuiKey_End] = MSDL::SDL_SCANCODE_END;
        io.KeyMap[ImGuiKey_Insert] = MSDL::SDL_SCANCODE_INSERT;
        io.KeyMap[ImGuiKey_Delete] = MSDL::SDL_SCANCODE_DELETE;
        io.KeyMap[ImGuiKey_Backspace] = MSDL::SDL_SCANCODE_BACKSPACE;
        io.KeyMap[ImGuiKey_Space] = MSDL::SDL_SCANCODE_SPACE;
        io.KeyMap[ImGuiKey_Enter] = MSDL::SDL_SCANCODE_RETURN;
        io.KeyMap[ImGuiKey_Escape] = MSDL::SDL_SCANCODE_ESCAPE;
        io.KeyMap[ImGuiKey_KeyPadEnter] = MSDL::SDL_SCANCODE_KP_ENTER;
        io.KeyMap[ImGuiKey_A] = MSDL::SDL_SCANCODE_A;
        io.KeyMap[ImGuiKey_C] = MSDL::SDL_SCANCODE_C;
        io.KeyMap[ImGuiKey_V] = MSDL::SDL_SCANCODE_V;
        io.KeyMap[ImGuiKey_X] = MSDL::SDL_SCANCODE_X;
        io.KeyMap[ImGuiKey_Y] = MSDL::SDL_SCANCODE_Y;
        io.KeyMap[ImGuiKey_Z] = MSDL::SDL_SCANCODE_Z;
#endif

#if defined(__ANDROID__)
        // Scaling fonts    // TODO Find a better way
        io.FontGlobalScale = 3.5f;
#elif defined(__IOS__)
        io.FontGlobalScale = 3.0f;
#endif
    }

    // Update the Descriptor Set:
    for (auto & descriptorSet : state->descriptorSetGroup.descriptorSets) {
        auto const imageInfo = VkDescriptorImageInfo {
            .sampler = state->fontSampler.sampler,
            .imageView = state->fontTexture.imageView(),
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        };
        auto writeDescriptorSet = VkWriteDescriptorSet {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = descriptorSet,
            .dstBinding = 0,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .pImageInfo = &imageInfo,
        };
        RF::UpdateDescriptorSets(
            1,
            &writeDescriptorSet
        );
    }

#ifdef __DESKTOP__
    state->eventWatchId = RF::AddEventWatch(EventWatch);
#endif

}

static void UpdateMousePositionAndButtons() {
    auto & io = ImGui::GetIO();

#ifdef __DESKTOP__
    // Set OS mouse position if requested (rarely used, only when ImGuiConfigFlags_NavEnableSetMousePos is enabled by user)
    if (io.WantSetMousePos) {
        RF::WarpMouseInWindow(static_cast<int32_t>(io.MousePos.x), static_cast<int32_t>(io.MousePos.y));
    } else {
        io.MousePos = ImVec2(-FLT_MAX, -FLT_MAX);
    }
    int mx, my;
    uint32_t const mouse_buttons = MSDL::SDL_GetMouseState(&mx, &my);
    io.MouseDown[0] = (mouse_buttons & SDL_BUTTON(SDL_BUTTON_LEFT)) != 0;  // If a mouse press event came, always pass it as "mouse held this frame", so we don't miss click-release events that are shorter than 1 frame.
    io.MouseDown[1] = (mouse_buttons & SDL_BUTTON(SDL_BUTTON_RIGHT)) != 0;
    io.MouseDown[2] = (mouse_buttons & SDL_BUTTON(SDL_BUTTON_MIDDLE)) != 0;
    if (RF::GetWindowFlags() & MSDL::SDL_WINDOW_INPUT_FOCUS) {
        io.MousePos = ImVec2(static_cast<float>(mx), static_cast<float>(my));
    }
#elif defined(__ANDROID__) || defined(__IOS__)
    if (IM::IsMouseLocationValid()) {
        state->previousMousePositionX = IM::GetMouseX();
        state->previousMousePositionY = IM::GetMouseY();
    }
    io.MousePos = ImVec2(state->previousMousePositionX, state->previousMousePositionY);

    io.MouseDown[0] = IM::IsLeftMouseDown();
    io.MouseDown[1] = false;
    io.MouseDown[2] = false;
#else
    #error Os not supported
#endif
}

#ifdef __DESKTOP__
static void UpdateMouseCursor() {
    auto & io = ImGui::GetIO();

    if (io.ConfigFlags & ImGuiConfigFlags_NoMouseCursorChange) {
        return;
    }
    ImGuiMouseCursor imgui_cursor = ImGui::GetMouseCursor();
    if (io.MouseDrawCursor || imgui_cursor == ImGuiMouseCursor_None) {
        // Hide OS mouse cursor if imgui is drawing it or if it wants no cursor
        MSDL::SDL_ShowCursor(MSDL::SDL_FALSE);
    } else {
        // Show OS mouse cursor
        MSDL::SDL_SetCursor(state->mouseCursors[imgui_cursor] ? state->mouseCursors[imgui_cursor] : state->mouseCursors[ImGuiMouseCursor_Arrow]);
        MSDL::SDL_ShowCursor(MSDL::SDL_TRUE);
    }
}
#endif

void OnNewFrame(
    float const deltaTimeInSec,
    RT::DrawPass & drawPass
) {
    ImGuiIO& io = ImGui::GetIO();
    IM_ASSERT(io.Fonts->IsBuilt() && "Font atlas not built! It is generally built by the renderer backend. Missing call to renderer _NewFrame() function? e.g. ImGui_ImplOpenGL3_NewFrame().");
    
    // Setup display size (every frame to accommodate for window resizing)
    int32_t window_width, window_height;
    int32_t drawable_width, drawable_height;
    RF::GetDrawableSize(window_width, window_height);

#if defined(__DESKTOP__)
    if (RF::GetWindowFlags() & MSDL::SDL_WINDOW_MINIMIZED) {
        window_width = window_height = 0;
    }
#endif

    RF::GetDrawableSize(drawable_width, drawable_height);
    io.DisplaySize = ImVec2(static_cast<float>(window_width), static_cast<float>(window_height));
    if (window_width > 0 && window_height > 0) {
        //io.DisplayFramebufferScale = ImVec2(
        //    static_cast<float>(drawable_width) / static_cast<float>(window_width), 
        //    static_cast<float>(drawable_height) / static_cast<float>(window_height)
        //);
        io.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);
    }
    io.DeltaTime = deltaTimeInSec;
    UpdateMousePositionAndButtons();
#if defined(__DESKTOP__)
    UpdateMouseCursor();
#endif
    // Start the Dear ImGui frame
    ImGui::NewFrame();
    
    state->hasFocus = false;

    for (auto recordObject : state->mRecordObjects) {
        recordObject->Record();
    }
    
    ImGui::Render();

    auto * draw_data = ImGui::GetDrawData();
    // Avoid rendering when minimized, scale coordinates for retina displays (screen coordinates != framebuffer coordinates)
    float const frameBufferWidth = draw_data->DisplaySize.x * draw_data->FramebufferScale.x;
    float const frameBufferHeight = draw_data->DisplaySize.y * draw_data->FramebufferScale.y;
    if (frameBufferWidth > 0 && frameBufferHeight > 0) {
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
            // TODO Write function to get mesh buffer and its status
            if(state->meshBuffersValidationStatus[drawPass.frameIndex]) {
                RF::DestroyMeshBuffers(state->meshBuffers[drawPass.frameIndex]);
                state->meshBuffersValidationStatus[drawPass.frameIndex] = false;
            }
            state->meshBuffers[drawPass.frameIndex].verticesBuffer = RF::CreateVertexBuffer(CBlob {vertex_data.ptr, vertex_data.len});
            state->meshBuffers[drawPass.frameIndex].indicesBuffer = RF::CreateIndexBuffer(CBlob {index_data.ptr, index_data.len});
            state->meshBuffersValidationStatus[drawPass.frameIndex] = true;
            // Setup desired Vulkan state
            // Bind pipeline and descriptor sets:
            {
                RF::BindDrawPipeline(drawPass, state->drawPipeline);
                RF::BindDescriptorSet(drawPass, state->descriptorSetGroup.descriptorSets[drawPass.frameIndex]);
            }

            RF::BindIndexBuffer(
                drawPass,
                state->meshBuffers[drawPass.frameIndex].indicesBuffer,
                0,
                sizeof(ImDrawIdx) == 2 ? VK_INDEX_TYPE_UINT16 : VK_INDEX_TYPE_UINT32
            );
            RF::BindVertexBuffer(
                drawPass,
                state->meshBuffers[drawPass.frameIndex].verticesBuffer
            );
            // Setup viewport:
            {
                VkViewport viewport;
                viewport.x = 0;
                viewport.y = 0;
                viewport.width = frameBufferWidth;
                viewport.height = frameBufferHeight;
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

                    if (clip_rect.x < frameBufferWidth && clip_rect.y < frameBufferHeight && clip_rect.z >= 0.0f && clip_rect.w >= 0.0f)
                    {
                        // Negative offsets are illegal for vkCmdSetScissor
                        if (clip_rect.x < 0.0f)
                            clip_rect.x = 0.0f;
                        if (clip_rect.y < 0.0f)
                            clip_rect.y = 0.0f;

                        {// Apply scissor/clipping rectangle
                            VkRect2D scissor {};
                            scissor.offset.x = static_cast<int32_t>(clip_rect.x);
                            scissor.offset.y = static_cast<int32_t>(clip_rect.y);
                            scissor.extent.width = static_cast<uint32_t>(clip_rect.z - clip_rect.x);
                            scissor.extent.height = static_cast<uint32_t>(clip_rect.w - clip_rect.y);
                            RF::SetScissor(drawPass, scissor);
                        }
                        
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

void Register(UIRecordObject * recordObject) {
    MFA_ASSERT(recordObject != nullptr);
    state->mRecordObjects.emplace_back(recordObject);
}

void UnRegister(UIRecordObject * recordObject) {
    for (size_t i = 0; i < state->mRecordObjects.size(); ++i) {
        if (*state->mRecordObjects[i] == *recordObject) {
            state->mRecordObjects.erase(state->mRecordObjects.begin() + i);
            break;
        }
    }
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

void Spacing() {
    ImGui::Spacing();
}

void Button(char const * label, std::function<void()> const & onPress) {
    if (ImGui::Button(label)) {
        MFA_ASSERT(onPress != nullptr);
        onPress();
    }
}

bool HasFocus() {
    return state->hasFocus;
}

void Shutdown() {
    MFA_ASSERT(state->meshBuffers.size() == state->meshBuffersValidationStatus.size());
    for(auto i = 0; i < state->meshBuffersValidationStatus.size(); i++) {
        if(true == state->meshBuffersValidationStatus[i]) {
            RF::DestroyMeshBuffers(state->meshBuffers[i]);
            state->meshBuffersValidationStatus[i] = false;
        }
    }
    RF::DestroyTexture(state->fontTexture);
    Importer::FreeTexture(state->fontTexture.cpuTexture());

    RF::DestroyPipelineGroup(state->drawPipeline);
    // TODO We can remove shader after creating pipeline
    RF::DestroyShader(state->fragmentShader);
    Importer::FreeShader(state->fragmentShader.cpuShader());
    RF::DestroyShader(state->vertexShader);
    Importer::FreeShader(state->vertexShader.cpuShader());

    RF::DestroyDescriptorSetLayout(state->descriptorSetLayout);
    RF::DestroySampler(state->fontSampler);

#ifdef __DESKTOP__
    RF::RemoveEventWatch(state->eventWatchId);
#endif

    delete state;
    state = nullptr;
}

bool IsItemActive() {
    return ImGui::IsItemActive();
}

#ifdef __ANDROID__
void SetAndroidApp(android_app * pApp)
{
    androidApp = pApp;
}
#endif

}
