#include "ImageUtils.hpp"

#include "FoundationAsset.hpp"
#include "BedrockLog.hpp"
#include "BedrockAssert.hpp"

#include <stb_image/stb_image.h>
#include <stb_image/stb_image_resize.h>

namespace MFA::Utils {
    namespace UncompressedTexture {

LoadResult Load(Data & out_image_data, const char * path, bool const use_srgb) {
    using namespace Asset;
    LoadResult ret = LoadResult::Invalid;
    auto * read_data = stbi_load(
        path,
        &out_image_data.width,
        &out_image_data.height,
        &out_image_data.stbi_components,
        0
    );
    if(MFA_PTR_VALID(read_data)) {
        MFA_ASSERT(out_image_data.width > 0);
        MFA_ASSERT(out_image_data.height > 0);
        MFA_ASSERT(out_image_data.stbi_components > 0);
        out_image_data.stbi_pixels.ptr = read_data;
        out_image_data.stbi_pixels.len = static_cast<int64_t>(out_image_data.width) * 
            out_image_data.height * 
            out_image_data.stbi_components * 
            sizeof(uint8_t);
        out_image_data.components = out_image_data.stbi_components;
        if (use_srgb) {
            switch (out_image_data.stbi_components) {
                case 1:
                    out_image_data.format = TextureFormat::UNCOMPRESSED_UNORM_R8_SRGB;
                    break;
                case 2:
                case 3:
                case 4:
                    out_image_data.format = TextureFormat::UNCOMPRESSED_UNORM_R8G8B8A8_SRGB;
                    out_image_data.components = 4;
                    break;
                default: MFA_NOT_IMPLEMENTED_YET("Mohammad Fakhreddin");
            }
        } else {
            switch (out_image_data.stbi_components) {
                case 1:
                    out_image_data.format = TextureFormat::UNCOMPRESSED_UNORM_R8_LINEAR;
                    break;
                case 2:
                    out_image_data.format = TextureFormat::UNCOMPRESSED_UNORM_R8G8_LINEAR;
                    break;
                case 3:
                case 4:
                    out_image_data.format = TextureFormat::UNCOMPRESSED_UNORM_R8G8B8A8_LINEAR;
                    out_image_data.components = 4;
                    break;
                default: MFA_LOG_WARN("Unhandled component count: %d", out_image_data.stbi_components);
            }
        }
        MFA_ASSERT(out_image_data.components >= out_image_data.stbi_components);
        if(out_image_data.components == out_image_data.stbi_components)
        {
            out_image_data.pixels = out_image_data.stbi_pixels;
        } else {
            auto const size = static_cast<size_t>(out_image_data.width) * 
                out_image_data.height * 
                out_image_data.components * 
                sizeof(uint8_t);
            out_image_data.pixels = Blob {::malloc(size), size};
            MFA_ASSERT(MFA_PTR_VALID(out_image_data.pixels.ptr));
            auto const pixels_array = out_image_data.pixels.as<uint8_t>();
            auto const stbi_pixels_array = out_image_data.stbi_pixels.as<uint8_t>();
            for(int pixel_index = 0; pixel_index < out_image_data.width * out_image_data.height ; pixel_index ++ )
            {
                for(int component_index = 0; component_index < out_image_data.components; component_index ++ )
                {
                    pixels_array[pixel_index * out_image_data.components + component_index] = component_index < out_image_data.stbi_components
                        ? stbi_pixels_array[pixel_index * out_image_data.stbi_components + component_index]
                        : 255u;
                }
            }
        }
        ret = LoadResult::Success;
    } else {
        ret = LoadResult::FileNotExists;
    }
    return ret;
}

bool Unload(Data * image_data) {
    bool ret = false;
    MFA_ASSERT(MFA_PTR_VALID(image_data->stbi_pixels.ptr) == MFA_PTR_VALID(image_data->pixels.ptr));
    MFA_ASSERT(MFA_PTR_VALID(image_data->stbi_pixels.ptr) == image_data->valid());
    if(MFA_PTR_VALID(image_data) && MFA_PTR_VALID(image_data->stbi_pixels.ptr)) {
        stbi_image_free(image_data->stbi_pixels.ptr);
        if(image_data->components != image_data->stbi_components)
        {
            ::free(image_data->pixels.ptr);
        }
        image_data->pixels = {};
        image_data->stbi_pixels = {};
        ret = true;
    }
    return ret;
}

    } // UncompressedTexture
} // MFA::Utils