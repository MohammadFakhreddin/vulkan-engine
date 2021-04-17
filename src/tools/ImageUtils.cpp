#include "ImageUtils.hpp"

#include "../engine/FoundationAsset.hpp"
#include "../engine/BedrockLog.hpp"
#include "../engine/BedrockAssert.hpp"
#include "../engine/BedrockMemory.hpp"

#include "../libs/stb_image/stb_image.h"
#include "engine/BedrockFileSystem.hpp"

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

namespace MFA::Utils::DDSTexture {

uintmax_t ComputeMipmapLen(
    uint32_t const width,
    uint32_t const height,
    TextureFormat const format
) {
    uintmax_t mipmap_len = 0;
    enum class TextureType
    {
        BC,
        Raw,
        Unsupported
    };
    auto texture_type = TextureType::Unsupported;
    uint8_t bc_bytes_per_block = 0;
    uint8_t raw_bytes_per_pixel = 0;
    switch (format)
    {
        case TextureFormat::BC7_UNorm_Linear_RGB:
        case TextureFormat::BC7_UNorm_sRGB_RGB:
        case TextureFormat::BC7_UNorm_Linear_RGBA:
        case TextureFormat::BC7_UNorm_sRGB_RGBA:
        case TextureFormat::BC6H_SFloat_Linear_RGB:
        case TextureFormat::BC6H_UFloat_Linear_RGB:
        case TextureFormat::BC5_UNorm_Linear_RG:
        case TextureFormat::BC5_SNorm_Linear_RG:
            texture_type = TextureType::BC;
            bc_bytes_per_block = 16;
        break;
        case TextureFormat::BC4_UNorm_Linear_R:
        case TextureFormat::BC4_SNorm_Linear_R:
            texture_type = TextureType::BC;
            bc_bytes_per_block = 8;
        break;
        case TextureFormat::UNCOMPRESSED_UNORM_R8G8B8A8_LINEAR:
        case TextureFormat::UNCOMPRESSED_UNORM_R8G8B8A8_SRGB:
        case TextureFormat::UNCOMPRESSED_UNORM_R8G8_LINEAR:
        case TextureFormat::UNCOMPRESSED_UNORM_R8_LINEAR:
        case TextureFormat::UNCOMPRESSED_UNORM_R8_SRGB:
            texture_type = TextureType::Raw;
            raw_bytes_per_pixel = AssetSystem::Texture::FormatTable[static_cast<uint8_t>(format)].bits_total / 8;
        break;
        default:
            MFA_CRASH("Format not supported");
            // Means that format is unsupported by engine
        break;
    };
    switch (texture_type)
    {
        case TextureType::BC:
            MFA_ASSERT((8 == bc_bytes_per_block) || (16 == bc_bytes_per_block));
            mipmap_len = Math::Max<uint32_t>(1, ( (width + 3) / 4 ) ) * Math::Max<uint32_t>(1, ( (height + 3) / 4 ) ) * bc_bytes_per_block;
        break;
        case TextureType::Raw:
            MFA_ASSERT(raw_bytes_per_pixel > 0);
            mipmap_len = width * height * raw_bytes_per_pixel;
        break;
        case TextureType::Unsupported:
        default:
        break;
    };
    return mipmap_len;
}

LoadResult Load (Data & out_image_data, char const * path) {
    LoadResult ret = LoadResult::INVALID;
    auto * file = FileSystem::OpenFile(path, FileSystem::Usage::Read);
    if(FileSystem::IsUsable(file)){
        auto const file_size = FileSystem::FileSize(file);
        MFA_ASSERT(file_size > 0);
        out_image_data.asset = Memory::Alloc(file_size);
        auto const ready_bytes = FileSystem::Read(file, out_image_data.asset);
        if(ready_bytes == out_image_data.asset.len) {
            auto const * dds_header = reinterpret_cast<DDS_Header *>(out_image_data.asset.ptr);
            if(124 == dds_header->dw_size && 32 == dds_header->dds_pixel_format.dw_size){
                
                UnionOfUint32 reversed_magic {};
                reversed_magic.digit0 = dds_header->magic.digit3;
                reversed_magic.digit1 = dds_header->magic.digit2;
                reversed_magic.digit2 = dds_header->magic.digit1;
                reversed_magic.digit3 = dds_header->magic.digit0;

                if(0x44445320 == reversed_magic.number){
                    
                    /*
                     *   Note:
                     *   When you write .dds files, you should set the DDSD_CAPS and DDSD_PIXELFORMAT flags, and for mipmapped textures you should also set the DDSD_MIPMAPCOUNT flag.
                     *   However, when you read a .dds file, you should not rely on the DDSD_CAPS, DDSD_PIXELFORMAT, and DDSD_MIPMAPCOUNT flags being set because some writers of such a file might not set these flags.
                     */
                    //auto const has_caps = dds_header->dw_flags & 0x1;
                    auto const has_height = dds_header->dw_flags & 0x2;
                    MFA_ASSERT(has_height);
                    auto const has_width = dds_header->dw_flags & 0x4;
                    MFA_ASSERT(has_width);
                    //auto const has_pitch = dds_header->dw_flags & 0x8;
                    //auto const has_pixel_format = dds_header->dw_flags & 0x1000;
                    //auto const has_mipmap_count = dds_header->dw_flags & 0x20000;
                    //auto const has_linear_size = dds_header->dw_flags & 0x80000;
                    //auto const has_depth = dds_header->dw_flags & 0x800000;

                    auto const has_alpha = dds_header->dds_pixel_format.dw_flags & 0x1;            // Texture contains alpha data; dwRGBAlphaBitMask contains valid data.
                    //auto const has_alpha_legacy = dds_header->dds_pixel_format.dw_flags & 0x2;              // Used in some older DDS files for alpha channel only uncompressed data (dwRGBBitCount contains the alpha channel bitcount; dwABitMask contains valid data)
                    auto const has_four_cc = dds_header->dds_pixel_format.dw_flags & 0x4;          // Texture contains compressed RGB data; dwFourCC contains valid data.
                    auto const has_rgb = dds_header->dds_pixel_format.dw_flags & 0x40;                      // Texture contains uncompressed RGB data; dwRGBBitCount and the RGB masks (dwRBitMask, dwGBitMask, dwBBitMask) contain valid data.           
                    //auto const has_yuv_legacy = dds_header->dds_pixel_format.dw_flags & 0x200;              // Used in some older DDS files for YUV uncompressed data (dwRGBBitCount contains the YUV bit count; dwRBitMask contains the Y mask, dwGBitMask contains the U mask, dwBBitMask contains the V mask)
                    //auto const has_luminance_legacy = dds_header->dds_pixel_format.dw_flags & 0x20000;      // Used in some older DDS files for single channel color uncompressed data (dwRGBBitCount contains the luminance channel bit count; dwRBitMask contains the channel mask). Can be combined with DDPF_ALPHAPIXELS for a two channel DDS file.
                    if(has_four_cc || has_rgb){
                        MFA_ASSERT(((has_four_cc > 0) != (has_rgb > 0)));
                        auto const format = dds_header->dds_pixel_format.dw_four_cc.character;
                        // We currently only support DX10 format
                        if('D' == format[0] && 'X' == format[1] && '1' == format[2] && '0' == format[3]){
                            
                            DDS_Header_DXT10 * dx10_header = reinterpret_cast<DDS_Header_DXT10 *>(out_image_data.asset.ptr + sizeof(DDS_Header));
                            // TODO Check for array size as well
                            // We are downsizing dw_mip_map_count from u32_t to u8_t
                            out_image_data.mipmap_count = Math::Max<uint8_t>(static_cast<uint8_t>(dds_header->dw_mip_map_count), 1);
                            out_image_data.dimension = [dx10_header]()-> uint8_t
                            {
                                uint8_t dimensions = 0;
                                switch (dx10_header->resource_dimension) {
                                    case D3D10_ResourceDimension::D3D10_RESOURCE_DIMENSION_TEXTURE1D:
                                    dimensions = 1;
                                    break;
                                    case D3D10_ResourceDimension::D3D10_RESOURCE_DIMENSION_TEXTURE2D:
                                    dimensions = 2;
                                    break;
                                    case D3D10_ResourceDimension::D3D10_RESOURCE_DIMENSION_TEXTURE3D:
                                    dimensions = 3;
                                    break;
                                    case D3D10_ResourceDimension::D3D10_RESOURCE_DIMENSION_BUFFER:
                                    case D3D10_ResourceDimension::D3D10_RESOURCE_DIMENSION_UNKNOWN:
                                    MFA_NOT_IMPLEMENTED_YET("Mohammad Fakhreddin");
                                };
                                return dimensions;
                            }();
                            MFA_ASSERT((out_image_data.dimension < 3 || out_image_data.depth > 1)); // Only 3d textures can have depth
                            out_image_data.format = [&dx10_header, has_alpha]() -> TextureFormat
                            {
                                TextureFormat format = TextureFormat::INVALID;
                                switch (dx10_header->dxgi_format)
                                {
                                    case DXGI_Format::DXGI_FORMAT_BC7_UNORM:
                                        format = has_alpha
                                            ? TextureFormat::BC7_UNorm_Linear_RGBA
                                            : TextureFormat::BC7_UNorm_Linear_RGB;
                                        break;
                                    case DXGI_Format::DXGI_FORMAT_BC7_UNORM_SRGB:
                                        format = has_alpha
                                            ? TextureFormat::BC7_UNorm_sRGB_RGBA
                                            : TextureFormat::BC7_UNorm_sRGB_RGB;
                                        break;
                                    case DXGI_Format::DXGI_FORMAT_BC5_UNORM:
                                        format = TextureFormat::BC5_UNorm_Linear_RG;
                                        break;
                                    case DXGI_Format::DXGI_FORMAT_BC5_SNORM:
                                        format = TextureFormat::BC5_SNorm_Linear_RG;
                                        break;
                                    case DXGI_Format::DXGI_FORMAT_BC4_UNORM:
                                        format = TextureFormat::BC4_UNorm_Linear_R;
                                        break;
                                    case DXGI_Format::DXGI_FORMAT_BC4_SNORM:
                                        format = TextureFormat::BC4_SNorm_Linear_R;
                                        break;
                                    case DXGI_Format::DXGI_FORMAT_R8G8B8A8_UNORM:
                                        format = TextureFormat::UNCOMPRESSED_UNORM_R8G8B8A8_LINEAR;
                                        break;
                                    case DXGI_Format::DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
                                        format = TextureFormat::UNCOMPRESSED_UNORM_R8G8B8A8_SRGB;
                                        break;
                                    case DXGI_Format::DXGI_FORMAT_R8G8_UNORM:
                                        format = TextureFormat::UNCOMPRESSED_UNORM_R8G8_LINEAR;
                                        break;
                                    case DXGI_Format::DXGI_FORMAT_R8_UNORM:
                                        format = TextureFormat::UNCOMPRESSED_UNORM_R8_LINEAR;
                                        break;
                                    case DXGI_Format::DXGI_FORMAT_R8_SNORM:
                                        format = TextureFormat::UNCOMPRESSED_UNORM_R8_SRGB;
                                        break;
                                    default:
                                        break;
                                };
                                return format;
                            }();
                            if(TextureFormat::INVALID != out_image_data.format) {
                                out_image_data.valid = true;
                                out_image_data.data_offset_in_asset = {
                                    out_image_data.asset.ptr + sizeof(DDS_Header) + sizeof(DDS_Header_DXT10),
                                    out_image_data.asset.len - sizeof(DDS_Header) - sizeof(DDS_Header_DXT10)
                                };
                                out_image_data.depth = Math::Max<uint8_t>(static_cast<uint8_t>(dds_header->dw_depth), 1);
                                MFA_ASSERT(out_image_data.mipmap_count >= 1);
                                out_image_data.mipmaps = Memory::Alloc(sizeof(Mipmap) * out_image_data.mipmap_count);
                                {// Generating mipmap offset from data
                                    auto * mipmap_array = out_image_data.mipmaps.as<Mipmap>();
                                    auto * data_current_ptr = out_image_data.data_offset_in_asset.ptr;
                                    uintmax_t mipmaps_total_len = 0;
                                    auto current_width = dds_header->dw_width;
                                    auto current_height = dds_header->dw_height;
                                    for(int i=0;i<out_image_data.mipmap_count;i++)
                                    {
                                        MFA_ASSERT(current_width >= 1);
                                        MFA_ASSERT(current_height >= 1);
                                        mipmap_array[i].data = Blob{
                                            data_current_ptr,
                                            ComputeMipmapLen(current_width, current_height, out_image_data.format)
                                        };
                                        MFA_ASSERT(mipmap_array[i].data.len > 0);
                                        mipmap_array[i].width = current_width;
                                        MFA_ASSERT(mipmap_array[i].width > 0);
                                        mipmap_array[i].height = current_height;
                                        MFA_ASSERT(mipmap_array[i].height > 0);
                                        current_width /= 2;
                                        current_height /= 2;
                                        data_current_ptr += mipmap_array[i].data.len;
                                        mipmaps_total_len += mipmap_array[i].data.len;
                                    }
                                    // TO Check if data
                                    MFA_ASSERT(mipmaps_total_len == out_image_data.data_offset_in_asset.len);
                                }
                                ret = LoadResult::Success;
                            } else {
                                ret = LoadResult::NotSupported_Format;
                            }
                        } else {
                            ret = LoadResult::NotSupported_DirectXVersion;
                        }
                    } else {
                        ret = LoadResult::NotSupported_Flags;
                    }
                } else {
                    ret = LoadResult::CorruptData_InvalidMagicNumber;
                }
            } else {
                ret = LoadResult::CorruptData_InvalidHeaderSize;
            }
        } else {
            ret = LoadResult::CorruptData_ReadBytesDoesNotMatch;
        }
    } else {
        ret = LoadResult::FileNotFound;
    }
    return ret;
}

bool Unload(Data * imageData) {
    bool ret = false;
    if(imageData != nullptr && imageData->valid) {
        imageData->valid = false;
        if(imageData->asset.ptr != nullptr) {
            Memory::Free(imageData->asset);
            imageData->asset = {};
        }
        if(imageData->mipmaps.ptr != nullptr) {
            Memory::Free(imageData->mipmaps);
            imageData->mipmaps = {};
        }
        ret = true;
    }
    return ret;
}

}

namespace MFA::Utils::KTXTexture {
    
}
