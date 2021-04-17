#include "TextureViewerScene.hpp"

#include "engine/BedrockMemory.hpp"
#include "libs/tiny_ktx/tinyktx2.h"
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
static void *tinyktxCallbackAlloc(void *user, size_t size) {
    return MFA::Memory::Alloc(size).ptr;
}
static void tinyktxCallbackFree(void *user, void *data) {
    MFA::Memory::PtrFree(data);
}
// TODO Start from here, Complete all callbacks
static size_t tinyktxCallbackRead(void *user, void* data, size_t size) {
    auto * handle = static_cast<FS::FileHandle *>(user);
    return FS::Read(handle, MFA::Blob {data, size});
}
static bool tinyktxCallbackSeek(void *user, int64_t offset) {
    auto * handle = static_cast<FS::FileHandle *>(user);
    return FS::Seek(handle, offset, FS::Origin::Start);

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

    AS::Texture result {};

    TinyKtx2_Callbacks callbacks {
            &tinyktxCallbackError,
            &tinyktxCallbackAlloc,
            &tinyktxCallbackFree,
            tinyktxCallbackRead,
            &tinyktxCallbackSeek,
            &tinyktxCallbackTell
    };

    auto * ctx =  TinyKtx2_CreateContext( &callbacks, handle);
    MFA_DEFER {
        TinyKtx2_DestroyContext(ctx);
    };
    
    auto const readHeaderResult = TinyKtx2_ReadHeader(ctx);
    MFA_ASSERT(readHeaderResult);

    uint32_t width = TinyKtx2_Width(ctx);
    uint32_t height = TinyKtx2_Height(ctx);
    uint32_t depth = TinyKtx2_Depth(ctx);
    const uint32_t sliceCount = TinyKtx2_ArraySlices(ctx);
    
    auto const tinyKtxFormat = TinyKtx_GetFormat(ctx);

    if(tinyKtxFormat != TKTX_UNDEFINED) {
        const auto mipmapCount = TinyKtx2_NumberOfMipmaps(ctx);
        uint32_t totalImageSize = 0;

        for(auto i = 0u; i < mipmapCount; ++i) {
            totalImageSize += TinyKtx2_ImageSize(ctx, i);
        }

        // TODO We need convert function here

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

            const auto imageSize = TinyKtx2_ImageSize(ctx, i);
            auto * imageDataPtr = TinyKtx2_ImageRawData(ctx, i);
            
            result.addMipmap(
                AS::Texture::Dimensions {
                    .width = width,
                    .height = height,
                    .depth = depth
                }, 
                CBlob {imageDataPtr, imageSize}
            );

            width = width / 2;
            height = height / 2;
            depth = depth / 2;
        }
    }

    return result;
}

void TextureViewerScene::Init() {
    ktxTexture* ktxTexture = nullptr;
    KTX_error_code result = ktxTexture_CreateFromNamedFile(
        "../assets/models/sponza/2185409758123873465.ktx", 
        KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, 
        &ktxTexture
    );
    MFA_ASSERT(result == KTX_SUCCESS);
}

void TextureViewerScene::Shutdown() {
    
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
