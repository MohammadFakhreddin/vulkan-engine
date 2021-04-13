#pragma once

#include "../engine/BedrockCommon.hpp"
#include "../engine/FoundationAsset.hpp"

namespace MFA::Utils {
using TextureFormat = AssetSystem::TextureFormat;
    namespace UncompressedTexture {

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
        return stbi_pixels.ptr != nullptr && 
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
uintmax_t ComputeMipmapLen(
    uint32_t width,
    uint32_t height,
    TextureFormat format
);

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

[[nodiscard]] LoadResult Load (Data & out_image_data, char const * path);

bool Unload(Data * imageData);

    } // DDSTexture

    namespace KTXTexture {
        // TODO Start from here
    } // KTXTexture
} // MFA::Utils