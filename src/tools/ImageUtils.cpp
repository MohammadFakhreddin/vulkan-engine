#include "ImageUtils.hpp"

#include "../engine/FoundationAsset.hpp"
#include "../engine/BedrockLog.hpp"
#include "../engine/BedrockAssert.hpp"
#include "../engine/BedrockMemory.hpp"

#include "../libs/stb_image/stb_image.h"

namespace MFA::Utils::UncompressedTexture {

LoadResult Load(Data & outImageData, const char * path, bool const prefer_srgb) {
    using namespace AssetSystem;
    LoadResult ret = LoadResult::Invalid;
    auto * readData = stbi_load(
        path,
        &outImageData.width,
        &outImageData.height,
        &outImageData.stbi_components,
        0
    );
    if(readData != nullptr) {
        MFA_ASSERT(outImageData.width > 0);
        MFA_ASSERT(outImageData.height > 0);
        MFA_ASSERT(outImageData.stbi_components > 0);
        outImageData.stbi_pixels.ptr = readData;
        outImageData.stbi_pixels.len = static_cast<int64_t>(outImageData.width) * 
            outImageData.height * 
            outImageData.stbi_components * 
            sizeof(uint8_t);
        outImageData.components = outImageData.stbi_components;
        if (prefer_srgb) {
            switch (outImageData.stbi_components) {
                case 1:
                    outImageData.format = TextureFormat::UNCOMPRESSED_UNORM_R8_SRGB;
                    break;
                case 2:
                case 3:
                case 4:
                    outImageData.format = TextureFormat::UNCOMPRESSED_UNORM_R8G8B8A8_SRGB;
                    outImageData.components = 4;
                    break;
                default: MFA_NOT_IMPLEMENTED_YET("Mohammad Fakhreddin");
            }
        } else {
            switch (outImageData.stbi_components) {
                case 1:
                    outImageData.format = TextureFormat::UNCOMPRESSED_UNORM_R8_LINEAR;
                    break;
                case 2:
                    outImageData.format = TextureFormat::UNCOMPRESSED_UNORM_R8G8_LINEAR;
                    break;
                case 3:
                case 4:
                    outImageData.format = TextureFormat::UNCOMPRESSED_UNORM_R8G8B8A8_LINEAR;
                    outImageData.components = 4;
                    break;
                default: MFA_LOG_WARN("Unhandled component count: %d", outImageData.stbi_components);
            }
        }
        MFA_ASSERT(outImageData.components >= static_cast<U32>(outImageData.stbi_components));
        if(outImageData.components == outImageData.stbi_components)
        {
            outImageData.pixels = outImageData.stbi_pixels;
        } else {
            auto const size = static_cast<size_t>(outImageData.width) * 
                outImageData.height * 
                outImageData.components * 
                sizeof(uint8_t);
            // TODO We need allocation system (Leak checking)
            outImageData.pixels = Memory::Alloc(size);
            MFA_ASSERT(outImageData.pixels.ptr != nullptr);
            auto * pixels_array = outImageData.pixels.as<uint8_t>();
            auto const * stbi_pixels_array = outImageData.stbi_pixels.as<uint8_t>();
            for(int pixel_index = 0; pixel_index < outImageData.width * outImageData.height ; pixel_index ++ )
            {
                for(U32 component_index = 0; component_index < outImageData.components; component_index ++ )
                {
                    pixels_array[pixel_index * outImageData.components + component_index] = static_cast<I64>(component_index) < outImageData.stbi_components
                        ? stbi_pixels_array[pixel_index * outImageData.stbi_components + component_index]
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

bool Unload(Data * imageData) {
    bool ret = false;
    MFA_ASSERT((imageData->stbi_pixels.ptr != nullptr) == (imageData->pixels.ptr != nullptr));
    MFA_ASSERT((imageData->stbi_pixels.ptr != nullptr) == imageData->valid());
    if(imageData != nullptr && imageData->stbi_pixels.ptr != nullptr) {
        stbi_image_free(imageData->stbi_pixels.ptr);
        if(imageData->components != imageData->stbi_components) {
            Memory::Free(imageData->pixels);
        }
        imageData->pixels = {};
        imageData->stbi_pixels = {};
        ret = true;
    }
    return ret;
}

} // MFA::Utils
