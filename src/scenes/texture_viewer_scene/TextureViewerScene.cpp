#include "TextureViewerScene.hpp"

#include "libs/tiny_ktx/tinyktx2.h"
#include "tools/Importer.hpp"

namespace RF = MFA::RenderFrontend;
namespace RB = MFA::RenderBackend;
namespace AS = MFA::AssetSystem;
namespace Importer = MFA::Importer;


static void tinyktxCallbackError(void *user, char const *msg) {
	LOGERROR("Tiny_Ktx ERROR: %s", msg);
}
static void *tinyktxCallbackAlloc(void *user, size_t size) {
	return MEMORY_MALLOC(size);
}
static void tinyktxCallbackFree(void *user, void *data) {
	MEMORY_FREE(data);
}
static size_t tinyktxCallbackRead(void *user, void* data, size_t size) {
	auto handle = (VFile_Handle) user;
	return VFile_Read(handle, data, size);
}
static bool tinyktxCallbackSeek(void *user, int64_t offset) {
	auto handle = (VFile_Handle) user;
	return VFile_Seek(handle, offset, VFile_SD_Begin);

}
static int64_t tinyktxCallbackTell(void *user) {
	auto handle = (VFile_Handle) user;
	return VFile_Tell(handle);
}

AL2O3_EXTERN_C Image_ImageHeader const *Image_LoadKTX(VFile_Handle handle) {
	TinyKtx_Callbacks callbacks {
			&tinyktxCallbackError,
			&tinyktxCallbackAlloc,
			&tinyktxCallbackFree,
			tinyktxCallbackRead,
			&tinyktxCallbackSeek,
			&tinyktxCallbackTell
	};

	auto ctx =  TinyKtx_CreateContext( &callbacks, handle);
	TinyKtx_ReadHeader(ctx);
	uint32_t w = TinyKtx_Width(ctx);
	uint32_t h = TinyKtx_Height(ctx);
	uint32_t d = TinyKtx_Depth(ctx);
	uint32_t s = TinyKtx_ArraySlices(ctx);
	ImageFormat fmt = ImageFormatToTinyKtxFormat(TinyKtx_GetFormat(ctx));
	if(fmt == ImageFormat_UNDEFINED) {
		TinyKtx_DestroyContext(ctx);
		return nullptr;
	}

	Image_ImageHeader const* topImage = nullptr;
	Image_ImageHeader const* prevImage = nullptr;
	for(auto i = 0u; i < TinyKtx_NumberOfMipmaps(ctx);++i) {
		auto image = Image_CreateNoClear(w, h, d, s, fmt);
		if(i == 0) topImage = image;

		if(Image_ByteCountOf(image) != TinyKtx_ImageSize(ctx, i)) {
			LOGERROR("KTX file %s mipmap %i size error", VFile_GetName(handle), i);
			Image_Destroy(topImage);
			TinyKtx_DestroyContext(ctx);
			return nullptr;
		}
		memcpy(Image_RawDataPtr(image), TinyKtx_ImageRawData(ctx, i), Image_ByteCountOf(image));
		if(prevImage) {
			auto p = (Image_ImageHeader *)prevImage;
			p->nextType = Image_NextType::Image_IT_MipMaps;
			p->nextImage = image;
		}
		if(w > 1) w = w / 2;
		if(h > 1) h = h / 2;
		if(d > 1) d = d / 2;
		prevImage = image;
	}

	TinyKtx_DestroyContext(ctx);
	return topImage;
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
