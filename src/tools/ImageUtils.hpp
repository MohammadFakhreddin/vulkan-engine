#pragma once

#include "../engine/BedrockCommon.hpp"
#include "../engine/BedrockAssert.hpp"
#include "../engine/FoundationAsset.hpp"
#include "../engine/BedrockMath.hpp"
#include "../engine/BedrockMemory.hpp"
#include "../engine/BedrockFileSystem.hpp"

namespace MFA::Utils {
using TextureFormat = AssetSystem::TextureFormat;
    namespace UncompressedTexture {

static constexpr const char * SystemName = "UncompressedImage";

struct Data {
    I32 width = 0;
    I32 height = 0;
    I32 stbi_components = 0;
    Blob stbi_pixels;
    AssetSystem::TextureFormat format = TextureFormat::INVALID;
    Blob pixels;
    U32 components = 0;
    [[nodiscard]]
    bool valid() const {
        return MFA_PTR_VALID(stbi_pixels.ptr) && 
            stbi_pixels.len > 0 && 
            width > 0 && 
            height > 0 && 
            stbi_components > 0;
    }
};

enum class LoadResult {
    Invalid,
    Success,
    FileNotExists
    // Format not supported
};

LoadResult Load(Data & outImageData, const char * path, bool prefer_srgb);

bool Unload(Data * imageData);

    } // UncompressedTexture
        namespace DDSTexture {
static inline constexpr const char * SystemName = "Importer::DDSTexture";
/*
    *  DDS source : 
    *  https://docs.microsoft.com/en-us/windows/win32/direct3ddds/dds-header
    *  https://docs.microsoft.com/en-us/windows/win32/direct3ddds/dds-pixelformat
    *  https://github.com/microsoft/DirectXTex/blob/master/DirectXTex/DDS.h
    *  Check for magic numbers as well for correctness
    *  https://stackoverflow.com/questions/51974508/how-can-i-make-sure-that-a-directdraw-surface-has-a-correct-file-format
    *  Formats are stored here
    *  https://docs.microsoft.com/en-us/windows/win32/api/dxgiformat/ne-dxgiformat-dxgi_format
    *  Resource for mipmap sizes
    *  https://docs.microsoft.com/en-us/windows/win32/direct3ddds/dds-file-layout-for-textures
    *  Correct implementation for DDSTexture
    *  // TODO Use this to fix missing pieces
    *  https://docs.microsoft.com/en-us/windows/uwp/gaming/complete-code-for-ddstextureloader
    *  Information about component count
    *  https://docs.microsoft.com/en-us/windows/win32/direct3d10/d3d10-graphics-programming-guide-resources-block-compression#bc4
    */
static_assert(sizeof(uint32_t) == 4);
#pragma pack (push, 1)
struct DDS_PixelFormat {
    uint32_t       dw_size;                             
    uint32_t       dw_flags;
    union
    {
        uint32_t   number;
        char       character[sizeof(uint32_t)];
    }              dw_four_cc;
    uint32_t       dw_rgb_bit_count;
    uint32_t       dw_r_bit_mask;
    uint32_t       dw_g_bit_mask;
    uint32_t       dw_b_bit_mask;
    uint32_t       dw_a_bit_mask;
};
#pragma pack (pop)  
static_assert(sizeof(DDS_PixelFormat) == 32);
union UnionOfUint32
{
    uint32_t        number;
    struct {
        uint8_t     digit0;
        uint8_t     digit1;
        uint8_t     digit2;
        uint8_t     digit3;
    };    
};
#pragma pack (push, 1)
struct DDS_Header {
    UnionOfUint32   magic;
    uint32_t        dw_size;
    uint32_t        dw_flags;
    uint32_t        dw_height;
    uint32_t        dw_width;
    uint32_t        dw_pitch_or_linear_size;
    uint32_t        dw_depth;
    uint32_t        dw_mip_map_count;
    uint32_t        dw_reserved1[11];
    DDS_PixelFormat dds_pixel_format;
    uint32_t        dw_caps;
    uint32_t        dw_caps2;
    uint32_t        dw_caps3;
    uint32_t        dw_caps4;
    uint32_t        dw_reserved2;
};
#pragma pack (pop)
static_assert(sizeof(DDS_Header) == 128);
enum class DXGI_Format {
    DXGI_FORMAT_UNKNOWN,
    DXGI_FORMAT_R32G32B32A32_TYPELESS,
    DXGI_FORMAT_R32G32B32A32_FLOAT,
    DXGI_FORMAT_R32G32B32A32_UINT,
    DXGI_FORMAT_R32G32B32A32_SINT,
    DXGI_FORMAT_R32G32B32_TYPELESS,
    DXGI_FORMAT_R32G32B32_FLOAT,
    DXGI_FORMAT_R32G32B32_UINT,
    DXGI_FORMAT_R32G32B32_SINT,
    DXGI_FORMAT_R16G16B16A16_TYPELESS,
    DXGI_FORMAT_R16G16B16A16_FLOAT,
    DXGI_FORMAT_R16G16B16A16_UNORM,
    DXGI_FORMAT_R16G16B16A16_UINT,
    DXGI_FORMAT_R16G16B16A16_SNORM,
    DXGI_FORMAT_R16G16B16A16_SINT,
    DXGI_FORMAT_R32G32_TYPELESS,
    DXGI_FORMAT_R32G32_FLOAT,
    DXGI_FORMAT_R32G32_UINT,
    DXGI_FORMAT_R32G32_SINT,
    DXGI_FORMAT_R32G8X24_TYPELESS,
    DXGI_FORMAT_D32_FLOAT_S8X24_UINT,
    DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS,
    DXGI_FORMAT_X32_TYPELESS_G8X24_UINT,
    DXGI_FORMAT_R10G10B10A2_TYPELESS,
    DXGI_FORMAT_R10G10B10A2_UNORM,
    DXGI_FORMAT_R10G10B10A2_UINT,
    DXGI_FORMAT_R11G11B10_FLOAT,
    DXGI_FORMAT_R8G8B8A8_TYPELESS,
    DXGI_FORMAT_R8G8B8A8_UNORM,
    DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
    DXGI_FORMAT_R8G8B8A8_UINT,
    DXGI_FORMAT_R8G8B8A8_SNORM,
    DXGI_FORMAT_R8G8B8A8_SINT,
    DXGI_FORMAT_R16G16_TYPELESS,
    DXGI_FORMAT_R16G16_FLOAT,
    DXGI_FORMAT_R16G16_UNORM,
    DXGI_FORMAT_R16G16_UINT,
    DXGI_FORMAT_R16G16_SNORM,
    DXGI_FORMAT_R16G16_SINT,
    DXGI_FORMAT_R32_TYPELESS,
    DXGI_FORMAT_D32_FLOAT,
    DXGI_FORMAT_R32_FLOAT,
    DXGI_FORMAT_R32_UINT,
    DXGI_FORMAT_R32_SINT,
    DXGI_FORMAT_R24G8_TYPELESS,
    DXGI_FORMAT_D24_UNORM_S8_UINT,
    DXGI_FORMAT_R24_UNORM_X8_TYPELESS,
    DXGI_FORMAT_X24_TYPELESS_G8_UINT,
    DXGI_FORMAT_R8G8_TYPELESS,
    DXGI_FORMAT_R8G8_UNORM,
    DXGI_FORMAT_R8G8_UINT,
    DXGI_FORMAT_R8G8_SNORM,
    DXGI_FORMAT_R8G8_SINT,
    DXGI_FORMAT_R16_TYPELESS,
    DXGI_FORMAT_R16_FLOAT,
    DXGI_FORMAT_D16_UNORM,
    DXGI_FORMAT_R16_UNORM,
    DXGI_FORMAT_R16_UINT,
    DXGI_FORMAT_R16_SNORM,
    DXGI_FORMAT_R16_SINT,
    DXGI_FORMAT_R8_TYPELESS,
    DXGI_FORMAT_R8_UNORM,
    DXGI_FORMAT_R8_UINT,
    DXGI_FORMAT_R8_SNORM,
    DXGI_FORMAT_R8_SINT,
    DXGI_FORMAT_A8_UNORM,
    DXGI_FORMAT_R1_UNORM,
    DXGI_FORMAT_R9G9B9E5_SHAREDEXP,
    DXGI_FORMAT_R8G8_B8G8_UNORM,
    DXGI_FORMAT_G8R8_G8B8_UNORM,
    DXGI_FORMAT_BC1_TYPELESS,
    DXGI_FORMAT_BC1_UNORM,
    DXGI_FORMAT_BC1_UNORM_SRGB,
    DXGI_FORMAT_BC2_TYPELESS,
    DXGI_FORMAT_BC2_UNORM,
    DXGI_FORMAT_BC2_UNORM_SRGB,
    DXGI_FORMAT_BC3_TYPELESS,
    DXGI_FORMAT_BC3_UNORM,
    DXGI_FORMAT_BC3_UNORM_SRGB,
    DXGI_FORMAT_BC4_TYPELESS,
    DXGI_FORMAT_BC4_UNORM,
    DXGI_FORMAT_BC4_SNORM,
    DXGI_FORMAT_BC5_TYPELESS,
    DXGI_FORMAT_BC5_UNORM,
    DXGI_FORMAT_BC5_SNORM,
    DXGI_FORMAT_B5G6R5_UNORM,
    DXGI_FORMAT_B5G5R5A1_UNORM,
    DXGI_FORMAT_B8G8R8A8_UNORM,
    DXGI_FORMAT_B8G8R8X8_UNORM,
    DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM,
    DXGI_FORMAT_B8G8R8A8_TYPELESS,
    DXGI_FORMAT_B8G8R8A8_UNORM_SRGB,
    DXGI_FORMAT_B8G8R8X8_TYPELESS,
    DXGI_FORMAT_B8G8R8X8_UNORM_SRGB,
    DXGI_FORMAT_BC6H_TYPELESS,
    DXGI_FORMAT_BC6H_UF16,
    DXGI_FORMAT_BC6H_SF16,
    DXGI_FORMAT_BC7_TYPELESS,
    DXGI_FORMAT_BC7_UNORM,
    DXGI_FORMAT_BC7_UNORM_SRGB,
    DXGI_FORMAT_AYUV,
    DXGI_FORMAT_Y410,
    DXGI_FORMAT_Y416,
    DXGI_FORMAT_NV12,
    DXGI_FORMAT_P010,
    DXGI_FORMAT_P016,
    DXGI_FORMAT_420_OPAQUE,
    DXGI_FORMAT_YUY2,
    DXGI_FORMAT_Y210,
    DXGI_FORMAT_Y216,
    DXGI_FORMAT_NV11,
    DXGI_FORMAT_AI44,
    DXGI_FORMAT_IA44,
    DXGI_FORMAT_P8,
    DXGI_FORMAT_A8P8,
    DXGI_FORMAT_B4G4R4A4_UNORM,
    DXGI_FORMAT_P208,
    DXGI_FORMAT_V208,
    DXGI_FORMAT_V408,
    DXGI_FORMAT_SAMPLER_FEEDBACK_MIN_MIP_OPAQUE,
    DXGI_FORMAT_SAMPLER_FEEDBACK_MIP_REGION_USED_OPAQUE,
    DXGI_FORMAT_FORCE_UINT
};
enum class D3D10_ResourceDimension {
    D3D10_RESOURCE_DIMENSION_UNKNOWN,                       // Resource is of unknown type
    D3D10_RESOURCE_DIMENSION_BUFFER,                        // Resource is a buffer
    D3D10_RESOURCE_DIMENSION_TEXTURE1D,                     // Resource is a 1D texture
    D3D10_RESOURCE_DIMENSION_TEXTURE2D,                     // Resource is a 2D texture
    D3D10_RESOURCE_DIMENSION_TEXTURE3D                      // Resource is a 3D texture
};
#pragma pack (push, 1)
struct DDS_Header_DXT10 {
    DXGI_Format              dxgi_format;
    D3D10_ResourceDimension  resource_dimension;
    uint32_t                 misc_flag;
    uint32_t                 array_size;
    uint32_t                 misc_flags2;
};
#pragma pack (pop)
static_assert(sizeof(DDS_Header_DXT10) == 20);

struct Mipmap {
    Blob                    data;
    uint32_t                width = 0;              // Width in pixels
    uint32_t                height = 0;             // Height in pixels
};

struct Data {
    uint8_t mipmap_count = 0;                       // Number of mipmaps
    uint8_t depth = 0;
    uint8_t dimension = 0;
    TextureFormat format = AssetSystem::TextureFormat::INVALID;
    bool valid = false;
    Blob asset {};
    Blob data_offset_in_asset {};
    Blob mipmaps {};                                //!< Mipmaps level in DDS is in reverse order compared to YUGA, For example in DDS biggest mipmap is 0 but on the contrast smallest mipmap (1X1) has level of zero on FFF
};

[[nodiscard]]
uintmax_t inline
ComputeMipmapLen(
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

enum class LoadResult {
    INVALID,
    Success,
    FileNotFound,
    CorruptData_InvalidHeaderSize,
    CorruptData_ReadBytesDoesNotMatch,
    CorruptData_InvalidMagicNumber,
    NotSupported_Flags,
    NotSupported_DirectXVersion,
    NotSupported_Format
};

[[nodiscard]] LoadResult inline
Load (Data & out_image_data, char const * path) {
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

bool inline
Unload(Data * imageData) {
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
    } // DDSTexture
} // MFA::Utils