#include "TextureViewerScene.hpp"

#include "engine/BedrockMemory.hpp"
#include "libs/tiny_ktx/tinyktx.h"
#include "tools/Importer.hpp"
#include "engine/BedrockFileSystem.hpp"

namespace RF = MFA::RenderFrontend;
namespace RB = MFA::RenderBackend;
namespace AS = MFA::AssetSystem;
namespace Importer = MFA::Importer;
namespace FS = MFA::FileSystem;
namespace AS = MFA::AssetSystem;

static void tinyktxCallbackError(void *user, char const *msg) {
    MFA_LOG_ERROR("Tiny_Ktx ERROR: %s", msg);
}
static void *tinyktxCallbackAlloc(void *user, const size_t size) {
    return MFA::Memory::Alloc(size).ptr;
}
static void tinyktxCallbackFree(void *user, void *data) {
    MFA::Memory::PtrFree(data);
}
// TODO Start from here, Complete all callbacks
static size_t tinyktxCallbackRead(void *user, void* data, const size_t size) {
    auto * handle = static_cast<FS::FileHandle *>(user);
    return FS::Read(handle, MFA::Blob {data, size});
}
static bool tinyktxCallbackSeek(void *user, const int64_t offset) {
    auto * handle = static_cast<FS::FileHandle *>(user);
    return FS::Seek(handle, static_cast<int>(offset), FS::Origin::Start);

}
static int64_t tinyktxCallbackTell(void *user) {
    auto * handle = static_cast<FS::FileHandle *>(user);
    int64_t location = 0;
    bool success = FS::Tell(handle, location);
    MFA_ASSERT(success);
    return location;
}

AS::Texture Image_LoadKTX(FS::FileHandle * handle) {

    using namespace MFA;
    using TextureFormat = AssetSystem::Texture::Format;

    AS::Texture result {};

    TinyKtx_Callbacks callbacks {
            &tinyktxCallbackError,
            &tinyktxCallbackAlloc,
            &tinyktxCallbackFree,
            tinyktxCallbackRead,
            &tinyktxCallbackSeek,
            &tinyktxCallbackTell
    };

    auto * ctx =  TinyKtx_CreateContext( &callbacks, handle);
    MFA_DEFER {
        TinyKtx_DestroyContext(ctx);
    };
    
    auto const readHeaderResult = TinyKtx_ReadHeader(ctx);
    MFA_ASSERT(readHeaderResult);

    U16 width = static_cast<U16>(TinyKtx_Width(ctx));
    U16 height = static_cast<U16>(TinyKtx_Height(ctx));
    U16 depth = Math::Max<U16>(static_cast<U16>(TinyKtx_Depth(ctx)), 1);
    const U8 sliceCount = Math::Max<U8>(static_cast<U8>(TinyKtx_ArraySlices(ctx)), 1);
    // TODO
    //TinyKtx_Is1D()
    //TinyKtx_Is2D()
    //TinyKtx_Is3D()
    //TinyKtx_IsCubemap()
    
    TinyKtx_Format const tinyKtxFormat = TinyKtx_GetFormat(ctx);
    TextureFormat const format = [tinyKtxFormat]() -> TextureFormat
    {
        TextureFormat conversionResult = TextureFormat::INVALID;
        switch (tinyKtxFormat) 
        {
            case TKTX_BC7_SRGB_BLOCK:
                conversionResult = TextureFormat::BC7_UNorm_sRGB_RGBA;
            break;
            case TKTX_BC7_UNORM_BLOCK:
                conversionResult = TextureFormat::BC7_UNorm_Linear_RGBA;
            break;
            case TKTX_BC5_UNORM_BLOCK:
                conversionResult = TextureFormat::BC5_UNorm_Linear_RG;
            break;
            case TKTX_BC5_SNORM_BLOCK:
                conversionResult = TextureFormat::BC5_SNorm_Linear_RG;
            break;
            case TKTX_BC4_UNORM_BLOCK:
                conversionResult = TextureFormat::BC4_UNorm_Linear_R;
            break;
            case TKTX_BC4_SNORM_BLOCK:
                conversionResult = AssetSystem::Texture::Format::BC4_SNorm_Linear_R;
            break;
            case TKTX_R8G8B8A8_UNORM:
                conversionResult = AssetSystem::Texture::Format::UNCOMPRESSED_UNORM_R8G8B8A8_LINEAR;
            break;
            case TKTX_R8G8B8A8_SRGB:
                conversionResult = TextureFormat::UNCOMPRESSED_UNORM_R8G8B8A8_SRGB;
            break;
            case TKTX_R8G8_UNORM:
                conversionResult = TextureFormat::UNCOMPRESSED_UNORM_R8G8_LINEAR;
            break;
            case TKTX_R8_UNORM:
                conversionResult = TextureFormat::UNCOMPRESSED_UNORM_R8_LINEAR;
            break;
            case TKTX_R8_SNORM:
                conversionResult = TextureFormat::UNCOMPRESSED_UNORM_R8_SRGB;
            break;
            default:
                break;
        }
        return conversionResult;
    }();

    if(tinyKtxFormat != TKTX_UNDEFINED) {
        // TODO Maybe I have to reevaluate my values
        const U8 mipmapCount = static_cast<U8>(TinyKtx_NumberOfMipmaps(ctx));
        uint32_t totalImageSize = 0;

        for(auto i = 0u; i < mipmapCount; ++i) {
            totalImageSize += TinyKtx_ImageSize(ctx, i);
        }
        
        result.initForWrite(
            format, 
            sliceCount, 
            mipmapCount, 
            depth, 
            nullptr, 
            Memory::Alloc(totalImageSize)
        );

        for(auto i = 0u; i < mipmapCount; ++i) {
            MFA_ASSERT(width >= 1);
            MFA_ASSERT(height >= 1);
            MFA_ASSERT(depth >= 1);

            auto const imageSize = TinyKtx_ImageSize(ctx, i);
            auto const * imageDataPtr = TinyKtx_ImageRawData(ctx, i);
            
            result.addMipmap(
                AS::Texture::Dimensions {
                    .width = width,
                    .height = height,
                    .depth = depth
                }, 
                CBlob {imageDataPtr, imageSize}
            );

            width = Math::Max<U16>(width / 2, 1);
            height = Math::Max<U16>(height / 2, 1);
            depth = Math::Max<U16>(depth / 2, 1);
        }
    }

    return result;
}

void TextureViewerScene::Init() {
    auto * fileHandle = MFA::FileSystem::OpenFile(
        "../assets/models/sponza/2185409758123873465.ktx", 
        MFA::FileSystem::Usage::Read
    );
    auto cpuTexture = Image_LoadKTX(fileHandle);
    MFA_ASSERT(cpuTexture.isValid());
    mGpuTexture = RF::CreateTexture(cpuTexture);
    //Importer::FreeTexture(&texture);

    // Vertex shader
    auto cpu_vertex_shader = Importer::ImportShaderFromSPV(
        "../assets/shaders/texture_viewer/TextureViewer.vert.spv", 
        MFA::AssetSystem::Shader::Stage::Vertex, 
        "main"
    );
    MFA_ASSERT(cpu_vertex_shader.isValid());
    auto gpu_vertex_shader = RF::CreateShader(cpu_vertex_shader);
    MFA_ASSERT(gpu_vertex_shader.valid());
    MFA_DEFER {
        RF::DestroyShader(gpu_vertex_shader);
        Importer::FreeShader(&cpu_vertex_shader);
    };
    
    // Fragment shader
    auto cpu_fragment_shader = Importer::ImportShaderFromSPV(
        "../assets/shaders/texture_viewer/TextureViewer.frag.spv",
        MFA::AssetSystem::Shader::Stage::Fragment,
        "main"
    );
    auto gpu_fragment_shader = RF::CreateShader(cpu_fragment_shader);
    MFA_ASSERT(cpu_fragment_shader.isValid());
    MFA_ASSERT(gpu_fragment_shader.valid());
    MFA_DEFER {
        RF::DestroyShader(gpu_fragment_shader);
        Importer::FreeShader(&cpu_fragment_shader);
    };

    std::vector<RB::GpuShader> shaders {gpu_vertex_shader, gpu_fragment_shader};

    // TODO We need nearest and linear filters
    mSamplerGroup = RF::CreateSampler();

    createDescriptorSetLayout();

    createDrawPipeline(static_cast<MFA::U8>(shaders.size()), shaders.data());
    
}

void TextureViewerScene::Shutdown() {
    RF::DestroyTexture(mGpuTexture);
    Importer::FreeTexture(mGpuTexture.cpu_texture());
}

void TextureViewerScene::OnDraw(
    MFA::U32 delta_time, 
    RF::DrawPass & draw_pass
) {
    
}

void TextureViewerScene::OnUI(
    MFA::U32 delta_time, 
    RF::DrawPass & draw_pass
) {
    
}
