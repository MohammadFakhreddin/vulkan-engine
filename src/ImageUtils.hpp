#include <stb_image/stb_image.h>
#include <stb_image/stb_image_resize.h>

namespace MFA::Utils {
    namespace UncompressedTexture {

static constexpr const char * SystemName = "UncompressedImage";

struct Data {
    int32_t width = 0;
    int32_t height = 0;
    int32_t stbi_components = 0;
    Blob stbi_pixels;
    YUGA::TextureFormat format = TextureFormat::Invalid;
    Blob pixels;
    int32_t components = 0;
    [[nodiscard]]
    bool valid() const {
        return YUGEN_PTR_VALID(stbi_pixels.ptr) && 
            stbi_pixels.len > 0 && 
            width > 0 && 
            height > 0 && 
            stbi_components > 0;
    }
};


    } // UncompressedTexture
}