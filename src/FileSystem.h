#ifndef FILE_SYSTEM_HEADER
#define FILE_SYSTEM_HEADER

#include <cassert>
#include <combaseapi.h>
#include <fstream>
#include <iostream>
#include <filesystem>

#include <tiny_obj_loader/tiny_obj_loader.h>

#include <stb_image/open_gl_stb_image.h>

class FileSystem{
public:
    struct RawTexture
    {
        ~RawTexture()
        {
            FreeTexture(*this);
            raw_pixels = nullptr;
            if(original_number_of_channels != number_of_channels)
            {
                delete[] pixels;
            }
            pixels = nullptr;
        }
        uint8_t * raw_pixels = nullptr;
        uint8_t * pixels = nullptr;
        int32_t width = -1;
        int32_t height = -1;
        int32_t original_number_of_channels = -1;
        int32_t const number_of_channels = 4;
        int64_t image_size() const
        {
            return int64_t(width) * height * number_of_channels * sizeof(uint8_t);
        }
        bool isValid() const
        {
            return nullptr != raw_pixels && width > 0 && height > 0 && original_number_of_channels > 0;
        }
    };
    static void
    LoadTexture (RawTexture & out, char const * texture_address){
        out.raw_pixels = stbi_load(
            texture_address, 
            &out.width, 
            &out.height, 
            &out.original_number_of_channels, 
            STBI_rgb
        );
        if(out.original_number_of_channels != out.number_of_channels)
        {
            assert(3 == out.original_number_of_channels);
            out.pixels = new uint8_t[out.image_size()];
            for(int i = 0 ; i < out.width * out.height ; i++)
            {
                out.pixels[4 * i] = out.raw_pixels[3 * i];
                out.pixels[4 * i + 1] = out.raw_pixels[3 * i + 1];
                out.pixels[4 * i + 2] = out.raw_pixels[3 * i + 2];
                out.pixels[4 * i + 3] = 255;
            }
            assert(255 == out.pixels[out.width * out.height * out.number_of_channels - 1]);
        } else
        {
            out.pixels = out.raw_pixels;
        }
    }   
    static bool
    FreeTexture (RawTexture & texture)
    {
        bool ret = false;
        if(texture.isValid()){
            stbi_image_free(texture.raw_pixels);
            texture.raw_pixels = nullptr;
            ret = true;
        }
        return ret;
    }
    class DDSTexture
    {
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
     */
    struct DDS_PixelFormat {
        DWORD           dw_size;                             
        DWORD           dw_flags;
        union
        {
            DWORD       number;
            char        character[sizeof(DWORD)];
        } dw_four_cc;
        DWORD           dw_rgb_bit_count;
        DWORD           dw_r_bit_mask;
        DWORD           dw_g_bit_mask;
        DWORD           dw_b_bit_mask;
        DWORD           dw_a_bit_mask;
    };
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
    struct DDS_Header {
        UnionOfUint32   magic;
        DWORD           dw_size;
        DWORD           dw_flags;
        DWORD           dw_height;
        DWORD           dw_width;
        DWORD           dw_pitch_or_linear_size;
        DWORD           dw_depth;
        DWORD           dw_mip_map_count;
        DWORD           dw_reserved1[11];
        DDS_PixelFormat dds_pixel_format;
        DWORD           dw_caps;
        DWORD           dw_caps2;
        DWORD           dw_caps3;
        DWORD           dw_caps4;
        DWORD           dw_reserved2;
    };
    static_assert(sizeof(DDS_Header) == 128);
    static_assert(sizeof(DWORD) == sizeof(uint32_t));
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
    enum D3D10_ResourceDimension {
        D3D10_RESOURCE_DIMENSION_UNKNOWN,                       // Resource is of unknown type
        D3D10_RESOURCE_DIMENSION_BUFFER,                        // Resource is a buffer
        D3D10_RESOURCE_DIMENSION_TEXTURE1D,                     // Resource is a 1D texture
        D3D10_RESOURCE_DIMENSION_TEXTURE2D,                     // Resource is a 2D texture
        D3D10_RESOURCE_DIMENSION_TEXTURE3D                      // Resource is a 3D texture
    };
    struct DDS_Header_DXT10 {
        DXGI_Format              dxgi_format;
        D3D10_ResourceDimension  resource_dimension;
        UINT                     misc_flag;
        UINT                     array_size;
        UINT                     misc_flags2;
    };
    static_assert(sizeof(UINT) == 4);
    static_assert(sizeof(DDS_Header_DXT10) == 20);
    public:
        struct Mipmap
        {
            byte * ptr;
            uintmax_t len;
            uint32_t width;         // Width in pixels
            uint32_t height;        // Height in pixels
        };
        static uintmax_t ComputeBC7MipmapLen(uint32_t width, uint32_t height)
        {
            return std::max<uint32_t>(1, ( (width + 3) / 4 ) ) * std::max<uint32_t>(1, ( (height + 3) / 4 ) ) * 16;
        }
        DDSTexture(char const * path)
        {
            // Open the stream to 'lock' the file.
            std::ifstream f(path, std::ios::in | std::ios::binary);

            if(std::filesystem::exists(path)){

                // Obtain the size of the file.
                m_asset_len = std::filesystem::file_size(path);

                // Create a buffer.
                m_asset = new byte[m_asset_len];

                // Read the whole file into the buffer.
                f.read(reinterpret_cast<char*>(m_asset), m_asset_len);

                f.close();

                auto const * header = reinterpret_cast<DDS_Header *>(m_asset);
                if(124 == header->dw_size && 32 == header->dds_pixel_format.dw_size){
                    
                    UnionOfUint32 reversed_magic {};
                    reversed_magic.digit0 = header->magic.digit3;
                    reversed_magic.digit1 = header->magic.digit2;
                    reversed_magic.digit2 = header->magic.digit1;
                    reversed_magic.digit3 = header->magic.digit0;

                    if(0x44445320 == reversed_magic.number){
                        auto const format = header->dds_pixel_format.dw_four_cc.character;
                        // We only support DX10 format
                        if('D' == format[0] && 'X' == format[1] && '1' == format[2] && '0' == format[3]){
                            m_valid = true;

                            DDS_Header_DXT10 * dx10_header = reinterpret_cast<DDS_Header_DXT10 *>(m_asset + sizeof(DDS_Header));

                            /*
                             *   Note:
                             *   When you write .dds files, you should set the DDSD_CAPS and DDSD_PIXELFORMAT flags, and for mipmapped textures you should also set the DDSD_MIPMAPCOUNT flag.
                             *   However, when you read a .dds file, you should not rely on the DDSD_CAPS, DDSD_PIXELFORMAT, and DDSD_MIPMAPCOUNT flags being set because some writers of such a file might not set these flags.
                             */
                            auto const has_caps = header->dw_flags & 0x1;
                            auto const has_height = header->dw_flags & 0x2;
                            assert(has_height);
                            auto const has_width = header->dw_flags & 0x4;
                            assert(has_width);
                            auto const has_pitch = header->dw_flags & 0x8;
                            auto const has_pixel_format = header->dw_flags & 0x1000;
                            auto const has_mipmap_count = header->dw_flags & 0x20000;
                            auto const has_linear_size = header->dw_flags & 0x80000;
                            auto const has_depth = header->dw_flags & 0x800000;

                            auto const has_alpha = header->dds_pixel_format.dw_flags & 0x1;                     // Texture contains alpha data; dwRGBAlphaBitMask contains valid data.
                            auto const has_alpha_legacy = header->dds_pixel_format.dw_flags & 0x2;              // Used in some older DDS files for alpha channel only uncompressed data (dwRGBBitCount contains the alpha channel bitcount; dwABitMask contains valid data)
                            auto const has_four_cc = header->dds_pixel_format.dw_flags & 0x4;                   // Texture contains compressed RGB data; dwFourCC contains valid data.
                            auto const has_rgb = header->dds_pixel_format.dw_flags & 0x40;                      // Texture contains uncompressed RGB data; dwRGBBitCount and the RGB masks (dwRBitMask, dwGBitMask, dwBBitMask) contain valid data.           
                            auto const has_yuv_legacy = header->dds_pixel_format.dw_flags & 0x200;              // Used in some older DDS files for YUV uncompressed data (dwRGBBitCount contains the YUV bit count; dwRBitMask contains the Y mask, dwGBitMask contains the U mask, dwBBitMask contains the V mask)
                            auto const has_luminance_legacy = header->dds_pixel_format.dw_flags & 0x20000;      // Used in some older DDS files for single channel color uncompressed data (dwRGBBitCount contains the luminance channel bit count; dwRBitMask contains the channel mask). Can be combined with DDPF_ALPHAPIXELS for a two channel DDS file.

                            m_mipmap_count = std::max<uint32_t>(header->dw_mip_map_count, 1);
                            m_dimension = [dx10_header]()-> uint8_t
                            {
                                uint8_t ret = 0;
                                switch (dx10_header->resource_dimension)
                                {
                                    case D3D10_RESOURCE_DIMENSION_TEXTURE1D:
                                    ret = 1;
                                    break;
                                    case D3D10_RESOURCE_DIMENSION_TEXTURE2D:
                                    ret = 2;
                                    break;
                                    case D3D10_RESOURCE_DIMENSION_TEXTURE3D:
                                    ret = 3;
                                    break;
                                    case D3D10_RESOURCE_DIMENSION_BUFFER:
                                    case D3D10_RESOURCE_DIMENSION_UNKNOWN:
                                    std::cout << "Error: Unsupported format:" << dx10_header->resource_dimension;
                                    ret = 0;
                                };
                                return ret;
                            }();
                            m_format = [dx10_header]() -> VkFormat
                            {
                                VkFormat ret;
                                switch (dx10_header->dxgi_format)
                                {
                                    case DXGI_Format::DXGI_FORMAT_BC7_UNORM:
                                    ret = VK_FORMAT_BC7_UNORM_BLOCK;
                                    break;
                                    case DXGI_Format::DXGI_FORMAT_BC7_UNORM_SRGB:
                                    ret = VK_FORMAT_BC7_SRGB_BLOCK;
                                    break;
                                    case DXGI_Format::DXGI_FORMAT_R8G8B8A8_UNORM:
                                    ret = VK_FORMAT_R8G8B8A8_UNORM;
                                    break;
                                    case DXGI_Format::DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
                                    ret = VK_FORMAT_R8G8B8A8_SRGB;
                                    break;
                                    default:
                                    std::cout << "Unhandled format detected," << uint32_t(dx10_header->dxgi_format) << std::endl;
                                };
                                return ret;
                            }();
                            m_depth = std::max<uint32_t>(header->dw_depth, 1);
                            assert(m_mipmap_count >= 1);
                            m_mipmaps = new Mipmap[m_mipmap_count];
                            {
                                auto data_current_ptr = m_asset + sizeof(DDS_Header) + sizeof(DDS_Header_DXT10);
                                auto current_width = header->dw_width;
                                auto current_height = header->dw_height;
                                for(int i=0;i<m_mipmap_count;i++)
                                {
                                    assert(current_width >= 1);
                                    assert(current_height >= 1);
                                    m_mipmaps[i].ptr = data_current_ptr;
                                    m_mipmaps[i].len = ComputeBC7MipmapLen(current_width,current_height);
                                    m_mipmaps[i].width = current_width;
                                    m_mipmaps[i].height = current_height;
                                    current_width /= 2;
                                    current_height /= 2;
                                    data_current_ptr += m_mipmaps[i].len;
                                }
                            }
                            std::cout
                                << "--------DDS Info-----Path:" << path                                         << std::endl
                                << "magic:"                     << header->magic.number                         << std::endl
                                << "dwSize:"                    << header->dw_size                              << std::endl
                                << "dwFlags:"                   << header->dw_flags                             << std::endl
                                << "dwHeight:"                  << header->dw_height                            << std::endl
                                << "dwWidth:"                   << header->dw_height                            << std::endl
                                << "dwPitchOrLinearSize:"       << header->dw_height                            << std::endl
                                << "dwDepth:"                   << header->dw_height                            << std::endl
                                << "dwMipMapCount:"             << header->dw_mip_map_count                     << std::endl
                                << "dwReserved1:"               << header->dw_reserved1                         << std::endl
                                << "ddspf.dwSize:"              << header->dds_pixel_format.dw_size             << std::endl
                                << "ddspf.dwFlags:"             << header->dds_pixel_format.dw_flags            << std::endl
                                << "ddspf.dwFourCC:"            << header->dds_pixel_format.dw_four_cc.number   << std::endl
                                << "ddspf.dwRGBBitCount:"       << header->dds_pixel_format.dw_rgb_bit_count    << std::endl
                                << "ddspf.dwRBitMask:"          << header->dds_pixel_format.dw_r_bit_mask       << std::endl
                                << "ddspf.dwGBitMask:"          << header->dds_pixel_format.dw_g_bit_mask       << std::endl
                                << "ddspf.dwBBitMask:"          << header->dds_pixel_format.dw_b_bit_mask       << std::endl
                                << "ddspf.dwABitMask:"          << header->dds_pixel_format.dw_a_bit_mask       << std::endl
                                << "dwCaps:"                    << header->dw_caps                              << std::endl
                                << "dwCaps2:"                   << header->dw_caps2                             << std::endl
                                << "dwCaps3:"                   << header->dw_caps3                             << std::endl
                                << "dwCaps4:"                   << header->dw_caps4                             << std::endl
                                << "dwReserved2:"               << header->dw_reserved2                         << std::endl
                                << "has_caps:"                  << has_caps                                     << std::endl
                                << "has_height:"                << has_height                                   << std::endl
                                << "has_width:"                 << has_width                                    << std::endl
                                << "has_pitch:"                 << has_pitch                                    << std::endl
                                << "has_pixel_format:"          << has_pixel_format                             << std::endl
                                << "has_mipmap_count:"          << has_mipmap_count                             << std::endl
                                << "has_linear_size:"           << has_linear_size                              << std::endl
                                << "has_depth:"                 << has_depth                                    << std::endl
                                << "-------------------------End------------------------"                       << std::endl;
                        } else
                        {
                            std::cout << "We only support DX10 format" << std::endl;
                        }
                    }
                }
            }
        }
        // TODO Delete or implement Move, Copy constructors
        ~DDSTexture()
        {
            m_valid = false;
            if(nullptr != m_asset)
            {
                delete[] m_asset;
                m_asset = nullptr;
            }
            if(nullptr != m_mipmaps)
            {
                delete[] m_mipmaps;
                m_mipmaps = nullptr;
            }
        }
        bool valid() const
        {
            return m_valid;
        }
        Mipmap pixels(uint8_t const mip_level) const
        {
            assert(mip_level < m_mipmap_count);
            return m_mipmaps[mip_level];
        }
        uint8_t mipmap_count() const
        {
            return m_mipmap_count;
        }
        VkFormat format() const
        {
            return m_format;
        }
    private:
        uint8_t         m_mipmap_count = 0;     // Number of mipmaps
        uint8_t         m_depth = 0;
        uint8_t         m_dimension = 0;
        VkFormat        m_format;
        bool            m_valid = false;
        byte *          m_asset = nullptr;
        uintmax_t       m_asset_len = 0;
        Mipmap *        m_mipmaps = nullptr;
    };
    static bool
    LoadObj(MTypes::Mesh & out_mesh, char const * path)
    {
        using Byte = char;
        bool ret = false;
        if(std::filesystem::exists(path)){

            std::ifstream file {path};

            bool is_counter_clockwise = false;
            {//Check if normal vectors are reverse
              std::string first_line;
              std::getline(file, first_line);
              if(first_line.find("ccw") != std::string::npos){
                is_counter_clockwise = true;
              }
            }

            tinyobj::attrib_t attributes;
            std::vector<tinyobj::shape_t> shapes;
            std::string error;
            auto load_obj_result = tinyobj::LoadObj(&attributes, &shapes, nullptr, &error, &file);
            if(load_obj_result)
            {
                if(shapes.empty())
                {
                    std::cerr << "Object has no shape" << std::endl;
                } else if(0 != attributes.vertices.size() % 3)
                {
                    std::cerr << "Vertices must be dividable by 3" << std::endl;
                } else if(0 != attributes.texcoords.size() % 2){
                    std::cerr << "Attributes must be dividable by 3" << std::endl;
                } else if (0 != shapes[0].mesh.indices.size() % 3)
                {
                    std::cerr << "Indices must be dividable by 3" << std::endl;
                } else if(attributes.vertices.size() / 3 != attributes.texcoords.size() / 2){
                    std::cerr << "Vertices and texture coordinates must have same size" << std::endl;
                } else
                {
                    ret = true;
                    struct Position
                    {
                        MTypes::TypeOfPosition pos[3];
                    };
                    std::vector<Position> positions;
                    struct TexCoord
                    {
                        MTypes::TypeOfTexCoord uv[2];
                    };
                    std::vector<TexCoord> coords;
                    {// Vertices
                        positions.resize(attributes.vertices.size() / 2);
                        for(
                            uintmax_t vertex_index = 0; 
                            vertex_index < attributes.vertices.size() / 3; 
                            vertex_index ++
                        )
                        {
                            positions[vertex_index].pos[0] = attributes.vertices[vertex_index * 3 + 0];
                            positions[vertex_index].pos[1] = attributes.vertices[vertex_index * 3 + 1];
                            positions[vertex_index].pos[2] = attributes.vertices[vertex_index * 3 + 2];
                        }
                        coords.resize(attributes.texcoords.size() / 2);
                        for(
                            uintmax_t tex_index = 0; 
                            tex_index < attributes.texcoords.size() / 2; 
                            tex_index ++
                        )
                        {
                            coords[tex_index].uv[0] = attributes.texcoords[tex_index * 2 + 0];
                            coords[tex_index].uv[1] = attributes.texcoords[tex_index * 2 + 1];
                        }
                    }
                    out_mesh.vertices.resize(positions.size());
                    {// Indices
                        out_mesh.indices.resize(shapes[0].mesh.indices.size());
                        for(
                            uintmax_t indices_index = 0;
                            indices_index < shapes[0].mesh.indices.size();
                            indices_index += 1
                        )
                        {
                            auto const vertex_index = shapes[0].mesh.indices[indices_index].vertex_index;
                            auto const uv_index = shapes[0].mesh.indices[indices_index].texcoord_index;
                            out_mesh.indices[indices_index] = shapes[0].mesh.indices[indices_index].vertex_index;
                            ::memcpy(out_mesh.vertices[vertex_index].pos, positions[vertex_index].pos, sizeof(Position));
                            ::memcpy(out_mesh.vertices[vertex_index].tex_coord, coords[uv_index].uv, sizeof(TexCoord));
                            out_mesh.vertices[vertex_index].tex_coord[1] = 1.0f - out_mesh.vertices[vertex_index].tex_coord[1];
                        }
                    }
                } 
            } else if (!error.empty() && error.substr(0, 4) != "WARN")
            {
                std::cerr << "LoadObj returned error:" << error << " File:" << path << std::endl;
            } else
            {
                std::cerr << "LoadObj failed" << std::endl;
            }
        }
        return ret;
    };
};

#endif
