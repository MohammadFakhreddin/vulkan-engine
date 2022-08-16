#include "UI_System.hpp"

#include "engine/BedrockMemory.hpp"
#include "tools/Importer.hpp"
#include "engine/render_system/RenderFrontend.hpp"
#if defined(__ANDROID__) || defined(__IOS__)
#include "engine/InputManager.hpp"
#endif
#include "engine/BedrockAssert.hpp"
#include "engine/BedrockMatrix.hpp"
#include "engine/BedrockSignal.hpp"
#include "engine/render_system/render_passes/display_render_pass/DisplayRenderPass.hpp"
#include "engine/asset_system/AssetTypes.hpp"
#include "engine/scene_manager/SceneManager.hpp"

#include "libs/imgui/imgui.h"
#include "libs/imgui/imgui_stdlib.h"
#ifdef __DESKTOP__
#include "libs/sdl/SDL.hpp"
#endif

namespace MFA::UI_System
{

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

    //static VkDeviceSize g_BufferMemoryAlignment = 256;

    struct State
    {
        std::shared_ptr<RT::SamplerGroup> fontSampler{};
        std::shared_ptr<RT::DescriptorSetLayoutGroup> descriptorSetLayout{};
        VkDescriptorPool descriptorPool{};
        RT::DescriptorSetGroup descriptorSetGroup{};
        std::shared_ptr<RT::PipelineGroup> pipeline{};
        std::shared_ptr<RT::GpuTexture> fontTexture{};
        bool hasFocus = false;
        Signal<> UIRecordSignal{};
        std::vector<std::shared_ptr<RT::BufferAndMemory>> vertexBuffers{};
        std::vector<std::shared_ptr<RT::BufferAndMemory>> indexBuffers{};
#if defined(__ANDROID__) || defined(__IOS__)
        IM::MousePosition previousMousePositionX = 0.0f;
        IM::MousePosition previousMousePositionY = 0.0f;
#endif
#ifdef __DESKTOP__
        MSDL::SDL_Cursor * mouseCursors[ImGuiMouseCursor_COUNT]{};
        RF::SDLEventWatchId eventWatchId = -1;
#endif
        SignalId resizeSignalId = InvalidSignalId;
    };

    static State * state = nullptr;

#ifdef __ANDROID__
    static android_app * androidApp = nullptr;
#endif

    struct PushConstants
    {
        float scale[2];
        float translate[2];
    };

#ifdef __DESKTOP__
    static int EventWatch(void * data, MSDL::SDL_Event * event)
    {
        ImGuiIO & io = ImGui::GetIO();
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
            const int key = event->key.keysym.scancode;
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
    static int32_t getDensityDpi(android_app * app)
    {
        AConfiguration * config = AConfiguration_new();
        AConfiguration_fromAssetManager(config, app->activity->assetManager);
        int32_t density = AConfiguration_getDensity(config);
        AConfiguration_delete(config);
        return density;
    }
#endif

    //-------------------------------------------------------------------------------------------------

    static void onResize()
    {
        ImGuiIO & io = ImGui::GetIO();
        MFA_ASSERT(io.Fonts->IsBuilt() && "Font atlas not built! It is generally built by the renderer backend. Missing call to renderer _NewFrame() function? e.g. ImGui_ImplOpenGL3_NewFrame().");

        // Setup display size (every frame to accommodate for window resizing)
        int32_t window_width, window_height;
        int32_t drawable_width, drawable_height;
        RF::GetDrawableSize(window_width, window_height);

#if defined(__DESKTOP__)
        if (RF::GetWindowFlags() & MSDL::SDL_WINDOW_MINIMIZED)
        {
            window_width = window_height = 0;
        }
#endif

        RF::GetDrawableSize(drawable_width, drawable_height);
        io.DisplaySize = ImVec2(static_cast<float>(window_width), static_cast<float>(window_height));
        io.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);
    }

    //-------------------------------------------------------------------------------------------------

    static void createDescriptorSetLayout()
    {
        std::vector<VkDescriptorSetLayoutBinding> binding{
            VkDescriptorSetLayoutBinding{
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .descriptorCount = 1,
                .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
                .pImmutableSamplers = &state->fontSampler->sampler,
            }
        };

        state->descriptorSetLayout = RF::CreateDescriptorSetLayout(
            static_cast<uint8_t>(binding.size()),
            binding.data()
        );
    }

    //-------------------------------------------------------------------------------------------------

    static void createPipeline()
    {
        // Constants: we are using 'vec2 offset' and 'vec2 scale' instead of a full 3d projection matrix
        std::vector<VkPushConstantRange> pushConstantRanges{};

        pushConstantRanges.emplace_back(VkPushConstantRange {
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
            .offset = 0,
            .size = 4 * sizeof(float)
        });

        // Create Descriptor Set:
        state->descriptorSetGroup = RF::CreateDescriptorSets(
            state->descriptorPool,
            1,
            *state->descriptorSetLayout
        );


        auto const pipelineLayout = RF::CreatePipelineLayout(
            1,
            &state->descriptorSetLayout->descriptorSetLayout,
            static_cast<uint32_t>(pushConstantRanges.size()),
            pushConstantRanges.data()
        );

        // Vertex shader
        auto const vertexShader = RF::CreateShader(Importer::ImportShaderFromSPV(
            CBlobAliasOf(vertex_shader_spv),
            AS::ShaderStage::Vertex,
            "main"
        ));
        
        // Fragment shader
        auto const fragmentShader = RF::CreateShader(Importer::ImportShaderFromSPV(
            CBlobAliasOf(fragment_shader_spv),
            AS::ShaderStage::Fragment,
            "main"
        ));

        std::vector<RT::GpuShader const *> shaderStages {
            vertexShader.get(),
            fragmentShader.get()
        };

        VkVertexInputBindingDescription vertex_binding_description{};
        vertex_binding_description.stride = sizeof(ImDrawVert);
        vertex_binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        std::vector<VkVertexInputAttributeDescription> inputAttributeDescription(3);
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

        std::vector<VkDynamicState> const dynamicStates{ VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
        VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo{};
        dynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicStateCreateInfo.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
        dynamicStateCreateInfo.pDynamicStates = dynamicStates.data();

        RT::CreateGraphicPipelineOptions pipelineOptions{};
        pipelineOptions.frontFace = VK_FRONT_FACE_CLOCKWISE;
        pipelineOptions.dynamicStateCreateInfo = &dynamicStateCreateInfo;
        pipelineOptions.depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        pipelineOptions.depthStencil.depthTestEnable = false;
        pipelineOptions.depthStencil.depthWriteEnable = false;
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
        pipelineOptions.useStaticViewportAndScissor = false;
        pipelineOptions.cullMode = VK_CULL_MODE_NONE;
        // TODO I wish we could render ui without MaxSamplesCount
        pipelineOptions.rasterizationSamples = RF::GetMaxSamplesCount();

        state->pipeline = RF::CreateGraphicPipeline(
            RF::GetDisplayRenderPass()->GetVkRenderPass(),
            static_cast<uint8_t>(shaderStages.size()),
            shaderStages.data(),
            pipelineLayout,
            1, 
            &vertex_binding_description,
            static_cast<uint8_t>(inputAttributeDescription.size()),
            inputAttributeDescription.data(),
            pipelineOptions
        );
    }

    //-------------------------------------------------------------------------------------------------

    static void createFontTexture()
    {
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
        ImGuiIO & io = ImGui::GetIO();

        uint8_t * pixels = nullptr;
        int32_t width, height;
        io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
        MFA_ASSERT(pixels != nullptr);
        MFA_ASSERT(width > 0);
        MFA_ASSERT(height > 0);
        size_t const components_count = 4;
        size_t const depth = 1;
        size_t const slices = 1;
        size_t const image_size = width * height * components_count * sizeof(uint8_t);

        Importer::ImportTextureOptions importTextureOptions{};
        importTextureOptions.tryToGenerateMipmaps = false;
        importTextureOptions.preferSrgb = false;

        auto const textureAsset = Importer::ImportInMemoryTexture(
            "FontTextures",
            CBlob{ pixels, image_size },
            width,
            height,
            AssetSystem::TextureFormat::UNCOMPRESSED_UNORM_R8G8B8A8_LINEAR,
            components_count,
            depth,
            slices,
            importTextureOptions
        );
        // TODO Support from in memory import of images inside importer
        state->fontTexture = RF::CreateTexture(*textureAsset);
    }

    //-------------------------------------------------------------------------------------------------

#ifdef __DESKTOP__
    void prepareKeyMapForDesktop()
    {
        ImGuiIO & io = ImGui::GetIO();
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
    }
#endif

    //-------------------------------------------------------------------------------------------------

    static void adjustGlobalFontScale()
    {
        ImGuiIO & io = ImGui::GetIO();
#if defined(__ANDROID__)
        // Scaling fonts    // TODO Find a better way
        io.FontGlobalScale = 3.5f;
#elif defined(__IOS__)
        io.FontGlobalScale = 3.0f;
#else
        io.FontGlobalScale = 1.0f;
#endif
    }

    //-------------------------------------------------------------------------------------------------

    static void updateDescriptorSets()
    {
        // Update the Descriptor Set:
        for (auto & descriptorSet : state->descriptorSetGroup.descriptorSets)
        {
            auto const imageInfo = VkDescriptorImageInfo{
                .sampler = state->fontSampler->sampler,
                .imageView = state->fontTexture->imageView->imageView,
                .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
            };
            auto writeDescriptorSet = VkWriteDescriptorSet{
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
    }

    //-------------------------------------------------------------------------------------------------

    void Init()
    {
        state = new State();
        
        // Setup Dear ImGui context
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();

        // FontSampler
        state->fontSampler = RF::CreateSampler(RT::CreateSamplerParams {
            .minLod = -1000,
            .maxLod = 1000,
            .maxAnisotropy = 1.0f,
        });

        state->descriptorPool = RF::CreateDescriptorPool(RF::GetMaxFramesPerFlight());

        createDescriptorSetLayout();

        createPipeline();

        createFontTexture();

#ifdef __DESKTOP__
        prepareKeyMapForDesktop();
#endif

        adjustGlobalFontScale();

        updateDescriptorSets();

#ifdef __DESKTOP__
        state->eventWatchId = RF::AddEventWatch(EventWatch);
#endif

        onResize();

        state->resizeSignalId = RF::AddResizeEventListener([]()->void {onResize();});

        state->vertexBuffers.resize(RF::GetMaxFramesPerFlight());
        state->indexBuffers.resize(RF::GetMaxFramesPerFlight());
    }

    //-------------------------------------------------------------------------------------------------

    static void UpdateMousePositionAndButtons()
    {
        auto & io = ImGui::GetIO();

#ifdef __DESKTOP__
        // Set OS mouse position if requested (rarely used, only when ImGuiConfigFlags_NavEnableSetMousePos is enabled by user)
        if (io.WantSetMousePos)
        {
            RF::WarpMouseInWindow(static_cast<int32_t>(io.MousePos.x), static_cast<int32_t>(io.MousePos.y));
        }
        else
        {
            io.MousePos = ImVec2(-FLT_MAX, -FLT_MAX);
        }
        int mx, my;
        uint32_t const mouse_buttons = MSDL::SDL_GetMouseState(&mx, &my);
        io.MouseDown[0] = (mouse_buttons & SDL_BUTTON(SDL_BUTTON_LEFT)) != 0;  // If a mouse press event came, always pass it as "mouse held this frame", so we don't miss click-release events that are shorter than 1 frame.
        io.MouseDown[1] = (mouse_buttons & SDL_BUTTON(SDL_BUTTON_RIGHT)) != 0;
        io.MouseDown[2] = (mouse_buttons & SDL_BUTTON(SDL_BUTTON_MIDDLE)) != 0;
        if (RF::GetWindowFlags() & MSDL::SDL_WINDOW_INPUT_FOCUS)
        {
            io.MousePos = ImVec2(static_cast<float>(mx), static_cast<float>(my));
        }
#elif defined(__ANDROID__) || defined(__IOS__)
        if (IM::IsMouseLocationValid())
        {
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
    static void UpdateMouseCursor()
    {
        auto & io = ImGui::GetIO();

        if (io.ConfigFlags & ImGuiConfigFlags_NoMouseCursorChange)
        {
            return;
        }
        ImGuiMouseCursor imgui_cursor = ImGui::GetMouseCursor();
        if (io.MouseDrawCursor || imgui_cursor == ImGuiMouseCursor_None)
        {
            // Hide OS mouse cursor if imgui is drawing it or if it wants no cursor
            MSDL::SDL_ShowCursor(MSDL::SDL_FALSE);
        }
        else
        {
            // Show OS mouse cursor
            MSDL::SDL_SetCursor(state->mouseCursors[imgui_cursor] ? state->mouseCursors[imgui_cursor] : state->mouseCursors[ImGuiMouseCursor_Arrow]);
            MSDL::SDL_ShowCursor(MSDL::SDL_TRUE);
        }
    }
#endif

    bool Render(
        float const deltaTimeInSec,
        RT::CommandRecordState & recordState
    )
    {
        ImGuiIO & io = ImGui::GetIO();
        MFA_ASSERT(io.Fonts->IsBuilt() && "Font atlas not built! It is generally built by the renderer backend. Missing call to renderer _NewFrame() function? e.g. ImGui_ImplOpenGL3_NewFrame().");

        io.DeltaTime = deltaTimeInSec;
        UpdateMousePositionAndButtons();
#if defined(__DESKTOP__)
        UpdateMouseCursor();
#endif

        auto const * drawData = ImGui::GetDrawData();
        if(drawData == nullptr)
        {
            return false;
        }

        // Setup desired Vulkan state
        // Bind pipeline and descriptor sets:
        RF::BindPipeline(recordState, *state->pipeline);

        RF::BindDescriptorSet(
            recordState,
            RenderFrontend::UpdateFrequency::PerFrame,
            state->descriptorSetGroup.descriptorSets[0]
        );
        
        // Avoid rendering when minimized, scale coordinates for retina displays (screen coordinates != framebuffer coordinates)
        float const frameBufferWidth = drawData->DisplaySize.x * drawData->FramebufferScale.x;
        float const frameBufferHeight = drawData->DisplaySize.y * drawData->FramebufferScale.y;
        if (frameBufferWidth > 0 && frameBufferHeight > 0)
        {
            if (drawData->TotalVtxCount > 0)
            {
                // TODO We can create vertices for ui system in post render
                // Create or resize the vertex/index buffers
                size_t const vertexSize = drawData->TotalVtxCount * sizeof(ImDrawVert);
                size_t const indexSize = drawData->TotalIdxCount * sizeof(ImDrawIdx);
                auto const vertexData = Memory::Alloc(vertexSize);
                auto const indexData = Memory::Alloc(indexSize);
                {
                    auto * vertexPtr = reinterpret_cast<ImDrawVert *>(vertexData->memory.ptr);
                    auto * indexPtr = reinterpret_cast<ImDrawIdx *>(indexData->memory.ptr);
                    for (int n = 0; n < drawData->CmdListsCount; n++)
                    {
                        const ImDrawList * cmd = drawData->CmdLists[n];
                        ::memcpy(vertexPtr, cmd->VtxBuffer.Data, cmd->VtxBuffer.Size * sizeof(ImDrawVert));
                        ::memcpy(indexPtr, cmd->IdxBuffer.Data, cmd->IdxBuffer.Size * sizeof(ImDrawIdx));
                        vertexPtr += cmd->VtxBuffer.Size;
                        indexPtr += cmd->IdxBuffer.Size;
                    }
                }

                auto & vertexBuffer = state->vertexBuffers[recordState.frameIndex];
                auto & indexBuffer = state->indexBuffers[recordState.frameIndex];

                if (vertexBuffer == nullptr || vertexBuffer->size < vertexSize)
                {
                    vertexBuffer = RF::CreateBuffer(
                        vertexSize,
                        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
                    );
                }

                RF::UpdateHostVisibleBuffer(*vertexBuffer, vertexData->memory);

                if (indexBuffer == nullptr || indexBuffer->size < indexSize)
                {
                    indexBuffer = RF::CreateBuffer(
                        indexSize,
                        VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
                    );
                }

                RF::UpdateHostVisibleBuffer(*indexBuffer, indexData->memory);

                RF::BindIndexBuffer(
                    recordState,
                    *indexBuffer,
                    0,
                    sizeof(ImDrawIdx) == 2 ? VK_INDEX_TYPE_UINT16 : VK_INDEX_TYPE_UINT32
                );
                RF::BindVertexBuffer(
                    recordState,
                    *vertexBuffer
                );

                // Setup viewport:
                VkViewport const viewport
                {
                    .x = 0,
                    .y = 0,
                    .width = frameBufferWidth,
                    .height = frameBufferHeight,
                    .minDepth = 0.0f,
                    .maxDepth = 1.0f,
                };
                RF::SetViewport(recordState, viewport);
                
                // Setup scale and translation:
                // Our visible imgui space lies from draw_data->DisplayPps (top left) to draw_data->DisplayPos+data_data->DisplaySize (bottom right). DisplayPos is (0,0) for single viewport apps.
                {
                    PushConstants constants{};
                    constants.scale[0] = 2.0f / drawData->DisplaySize.x;
                    constants.scale[1] = 2.0f / drawData->DisplaySize.y;
                    constants.translate[0] = -1.0f - drawData->DisplayPos.x * constants.scale[0];
                    constants.translate[1] = -1.0f - drawData->DisplayPos.y * constants.scale[1];
                    RF::PushConstants(
                        recordState,
                        AS::ShaderStage::Vertex,
                        0,
                        CBlobAliasOf(constants)
                    );
                }

                // Will project scissor/clipping rectangles into frame-buffer space
                ImVec2 const clip_off = drawData->DisplayPos;         // (0,0) unless using multi-viewports
                ImVec2 const clip_scale = drawData->FramebufferScale; // (1,1) unless using retina display which are often (2,2)

                // Render command lists
                // (Because we merged all buffers into a single one, we maintain our own offset into them)
                int global_vtx_offset = 0;
                int global_idx_offset = 0;
                for (int n = 0; n < drawData->CmdListsCount; n++)
                {
                    const ImDrawList * cmd_list = drawData->CmdLists[n];
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
                                VkRect2D scissor{};
                                scissor.offset.x = static_cast<int32_t>(clip_rect.x);
                                scissor.offset.y = static_cast<int32_t>(clip_rect.y);
                                scissor.extent.width = static_cast<uint32_t>(clip_rect.z - clip_rect.x);
                                scissor.extent.height = static_cast<uint32_t>(clip_rect.w - clip_rect.y);
                                RF::SetScissor(recordState, scissor);
                            }

                            // Draw
                            RF::DrawIndexed(
                                recordState,
                                pcmd->ElemCount,
                                1,
                                pcmd->IdxOffset + global_idx_offset,
                                pcmd->VtxOffset + global_vtx_offset
                            );
                        }
                    }
                    global_idx_offset += cmd_list->IdxBuffer.Size;
                    global_vtx_offset += cmd_list->VtxBuffer.Size;
                }
            }
        }

        return true;

    }

    //-------------------------------------------------------------------------------------------------

    void PostRender(float deltaTimeInSec)
    {
        ImGui::NewFrame();
        state->hasFocus = false;
        state->UIRecordSignal.Emit();
        ImGui::Render();
    }

    //-------------------------------------------------------------------------------------------------

    void BeginWindow(char const * windowName)
    {
        ImGui::Begin(windowName);
    }

    //-------------------------------------------------------------------------------------------------

    void EndWindow()
    {
        if (ImGui::IsWindowFocused())
        {
            state->hasFocus = true;
        }
        ImGui::End();
    }

    //-------------------------------------------------------------------------------------------------

    int Register(std::function<void()> const & listener)
    {

        MFA_ASSERT(listener != nullptr);
        return state->UIRecordSignal.Register(listener);
    }

    //-------------------------------------------------------------------------------------------------

    bool UnRegister(int const listenerId)
    {
        return state->UIRecordSignal.UnRegister(listenerId);
    }

    //-------------------------------------------------------------------------------------------------

    void SetNextItemWidth(float const nextItemWidth)
    {
        ImGui::SetNextItemWidth(nextItemWidth);
    }

    //-------------------------------------------------------------------------------------------------

    void Text(char const * label, ...)
    {
        va_list args;
        va_start(args, label);
        ImGui::TextV(label, args);
        va_end(args);
    }

    //-------------------------------------------------------------------------------------------------

    void InputFloat1(char const * label, float * value)
    {
        ImGui::InputFloat(label, value);
    }

    //-------------------------------------------------------------------------------------------------

    void InputFloat2(char const * label, float * value)
    {
        ImGui::InputFloat2(label, value);
    }

    //-------------------------------------------------------------------------------------------------

    void InputFloat3(char const * label, float * value)
    {
        ImGui::InputFloat3(label, value);
    }

    //-------------------------------------------------------------------------------------------------

    void InputFloat4(char const * label, float * value)
    {
        ImGui::InputFloat3(label, value);
    }

    //-------------------------------------------------------------------------------------------------

    // TODO Maybe we could cache unchanged vertices
    bool Combo(
        char const * label,
        int32_t * selectedItemIndex,
        char const ** items,
        int32_t const itemsCount
    )
    {
        return ImGui::Combo(
            label,
            selectedItemIndex,
            items,
            itemsCount
        );
    }

    //-------------------------------------------------------------------------------------------------
    // Based on https://eliasdaler.github.io/using-imgui-with-sfml-pt2/
    static auto vector_getter = [](void * vec, int idx, const char ** out_text)
    {
        auto & vector = *static_cast<std::vector<std::string>*>(vec);
        if (idx < 0 || idx >= static_cast<int>(vector.size())) { return false; }
        *out_text = vector.at(idx).c_str();
        return true;
    };

    // TODO: Use new api
    bool Combo(
        const char * label,
        int * selectedItemIndex,
        std::vector<std::string> & values
    )
    {
        if (values.empty())
        {
            return false;
        }
        return ImGui::Combo(
            label,
            selectedItemIndex,
            vector_getter,
            &values,
            static_cast<int>(values.size())
        );
    }

    //-------------------------------------------------------------------------------------------------

    void SliderInt(
        char const * label,
        int * value,
        int const minValue,
        int const maxValue
    )
    {
        ImGui::SliderInt(
            label,
            value,
            minValue,
            maxValue
        );
    }

    //-------------------------------------------------------------------------------------------------

    void SliderFloat(
        char const * label,
        float * value,
        float const minValue,
        float const maxValue
    )
    {
        ImGui::SliderFloat(
            label,
            value,
            minValue,
            maxValue
        );
    }

    //-------------------------------------------------------------------------------------------------

    void Checkbox(char const * label, bool * value)
    {
        ImGui::Checkbox(label, value);
    }
    
    //-------------------------------------------------------------------------------------------------

    bool Checkbox(char const * label, bool & value)
    {
        bool changed = false;
        bool tempValue = value;
        Checkbox(label, &tempValue);
        if (tempValue != value)
        {
            value = tempValue;
            changed = true;
        }
        return changed;
    }

    //-------------------------------------------------------------------------------------------------

    void Spacing()
    {
        ImGui::Spacing();
    }

    //-------------------------------------------------------------------------------------------------

    void Button(char const * label, std::function<void()> const & onPress)
    {
        if (ImGui::Button(label))
        {
            MFA_ASSERT(onPress != nullptr);
            SceneManager::AssignMainThreadTask([onPress]()->void{
                onPress();
            });
        }
    }

    //-------------------------------------------------------------------------------------------------

    void InputText(char const * label, std::string & outValue)
    {
        ImGui::InputText(label, &outValue);
    }

    //-------------------------------------------------------------------------------------------------

    bool HasFocus()
    {
        return state->hasFocus;
    }

    //-------------------------------------------------------------------------------------------------

    void Shutdown()
    {
        RF::RemoveResizeEventListener(state->resizeSignalId);
        RF::DestroyDescriptorPool(state->descriptorPool);
#ifdef __DESKTOP__
        RF::RemoveEventWatch(state->eventWatchId);
#endif

        delete state;
        state = nullptr;
    }

    //-------------------------------------------------------------------------------------------------

    bool IsItemActive()
    {
        return ImGui::IsItemActive();
    }

    //-------------------------------------------------------------------------------------------------

    bool TreeNode(char const * name)
    {
        return ImGui::TreeNode(name);
    }

    //-------------------------------------------------------------------------------------------------

    void TreePop()
    {
        ImGui::TreePop();
    }

    //-------------------------------------------------------------------------------------------------

#ifdef __ANDROID__
    void SetAndroidApp(android_app * pApp)
    {
        androidApp = pApp;
}
#endif

    //-------------------------------------------------------------------------------------------------

}
