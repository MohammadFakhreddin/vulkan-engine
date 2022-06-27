#include "ImageUtils.hpp"

#include "engine/BedrockLog.hpp"
#include "engine/BedrockAssert.hpp"
#include "engine/BedrockMemory.hpp"
#include "tools/Importer.hpp"
#include "engine/BedrockFileSystem.hpp"
#include "engine/BedrockMath.hpp"

#include "libs/stb_image/stb_image.h"
#include "libs/stb_image/stb_image_resize.h"
#include "libs/tiny_ktx/tinyktx.h"

// TODO: Separate this file into multiple ones
namespace MFA::Utils::UncompressedTexture
{

    LoadResult Load(Data & outImageData, std::string const & path, bool const prefer_srgb)
    {
        using namespace AssetSystem;
        LoadResult ret = LoadResult::Invalid;

        auto const rawFile = Importer::ReadRawFile(path);
        if (rawFile.valid() == false)
        {
            return ret;
        }
        auto * readData = stbi_load_from_memory(
            rawFile.data->memory.ptr,
            static_cast<int>(rawFile.data->memory.len),
            &outImageData.width,
            &outImageData.height,
            &outImageData.stbi_components,
            0
        );
        if (readData != nullptr)
        {
            MFA_ASSERT(outImageData.width > 0);
            MFA_ASSERT(outImageData.height > 0);
            MFA_ASSERT(outImageData.stbi_components > 0);
            outImageData.stbi_pixels = std::make_shared<SmartBlob>(Blob{
                readData,
                static_cast<size_t>(outImageData.width) *
                    outImageData.height *
                    outImageData.stbi_components *
                    sizeof(uint8_t)
            });
            outImageData.components = outImageData.stbi_components;
            if (prefer_srgb)
            {
                switch (outImageData.stbi_components)
                {
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
            }
            else
            {
                switch (outImageData.stbi_components)
                {
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
            MFA_ASSERT(outImageData.components >= static_cast<uint32_t>(outImageData.stbi_components));
            if (static_cast<int>(outImageData.components) == outImageData.stbi_components)
            {
                outImageData.pixels = outImageData.stbi_pixels;
            }
            else
            {
                auto const size = static_cast<size_t>(outImageData.width) *
                    outImageData.height *
                    outImageData.components *
                    sizeof(uint8_t);
                // TODO We need allocation system (Leak checking)
                outImageData.pixels = Memory::Alloc(size);
                MFA_ASSERT(outImageData.pixels->memory.ptr != nullptr);
                auto * pixels_array = outImageData.pixels->memory.as<uint8_t>();
                auto const * stbi_pixels_array = outImageData.stbi_pixels->memory.as<uint8_t>();
                for (int pixel_index = 0; pixel_index < outImageData.width * outImageData.height; pixel_index++)
                {
                    for (uint32_t component_index = 0; component_index < outImageData.components; component_index++)
                    {
                        pixels_array[pixel_index * outImageData.components + component_index] = static_cast<int64_t>(component_index) < outImageData.stbi_components
                            ? stbi_pixels_array[pixel_index * outImageData.stbi_components + component_index]
                            : 255u;
                    }
                }
            }
            ret = LoadResult::Success;
        }
        else
        {
            ret = LoadResult::FileNotExists;
        }
        return ret;
    }

    bool Resize(ResizeInputParams const & params)
    {
        MFA_ASSERT(params.inputImageWidth > 0);
        MFA_ASSERT(params.inputImageHeight > 0);
        MFA_ASSERT(params.inputImagePixels.ptr != nullptr);
        MFA_ASSERT(params.inputImagePixels.len > 0);

        MFA_ASSERT(params.componentsCount > 0);

        MFA_ASSERT(params.outputWidth > 0);
        MFA_ASSERT(params.outputHeight > 0);
        MFA_ASSERT(params.outputImagePixels.ptr != nullptr);
        MFA_ASSERT(params.outputImagePixels.len > 0);

        auto const resizeResult = params.useSRGB ? stbir_resize_uint8_srgb(
            params.inputImagePixels.ptr,
            params.inputImageWidth,
            params.inputImageHeight,
            0,
            params.outputImagePixels.ptr,
            params.outputWidth,
            params.outputHeight,
            0,
            params.componentsCount,
            params.componentsCount > 3 ? 3 : STBIR_ALPHA_CHANNEL_NONE,
            0
        ) : stbir_resize_uint8(
            params.inputImagePixels.ptr,
            params.inputImageWidth,
            params.inputImageHeight,
            0,
            params.outputImagePixels.ptr,
            params.outputWidth,
            params.outputHeight,
            0,
            params.componentsCount
        );
        return resizeResult > 0;
    }

} // MFA::Utils

namespace MFA::Utils::DDSTexture
{

    size_t ComputeMipmapLen(
        uint32_t const width,
        uint32_t const height,
        TextureFormat const format
    )
    {
        size_t mipmap_len = 0;
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
            mipmap_len = Math::Max<uint32_t>(1, ((width + 3) / 4)) * Math::Max<uint32_t>(1, ((height + 3) / 4)) * bc_bytes_per_block;
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

    LoadResult Load(Data & outImageData, std::string const & path)
    {
        LoadResult ret = LoadResult::INVALID;
        auto file = FileSystem::OpenFile(path, FileSystem::Usage::Read);
        if (MFA_VERIFY(file != nullptr))
        {
            auto const fileSize = file->size();
            MFA_ASSERT(fileSize > 0);
            outImageData.asset = Memory::Alloc(fileSize);
            auto const readBytes = file->read(outImageData.asset->memory);
            if (readBytes == outImageData.asset->memory.len)
            {
                auto const * dds_header = reinterpret_cast<DDS_Header *>(outImageData.asset->memory.ptr);
                if (124 == dds_header->dw_size && 32 == dds_header->dds_pixel_format.dw_size)
                {

                    UnionOfUint32 reversed_magic{};
                    reversed_magic.digit0 = dds_header->magic.digit3;
                    reversed_magic.digit1 = dds_header->magic.digit2;
                    reversed_magic.digit2 = dds_header->magic.digit1;
                    reversed_magic.digit3 = dds_header->magic.digit0;

                    if (0x44445320 == reversed_magic.number)
                    {

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
                        if (has_four_cc || has_rgb)
                        {
                            MFA_ASSERT(((has_four_cc > 0) != (has_rgb > 0)));
                            auto const format = dds_header->dds_pixel_format.dw_four_cc.character;
                            // We currently only support DX10 format
                            if ('D' == format[0] && 'X' == format[1] && '1' == format[2] && '0' == format[3])
                            {

                                auto * dx10_header = reinterpret_cast<DDS_Header_DXT10 *>(outImageData.asset->memory.ptr + sizeof(DDS_Header));
                                // TODO Check for array size as well
                                // We are downsizing dw_mip_map_count from u32_t to u8_t
                                outImageData.mipmap_count = Math::Max<uint8_t>(static_cast<uint8_t>(dds_header->dw_mip_map_count), 1);
                                outImageData.dimension = [dx10_header]()-> uint8_t
                                {
                                    uint8_t dimensions = 0;
                                    switch (dx10_header->resource_dimension)
                                    {
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
                                MFA_ASSERT((outImageData.dimension < 3 || outImageData.depth > 1)); // Only 3d textures can have depth
                                outImageData.format = [&dx10_header, has_alpha]() -> TextureFormat
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
                                if (TextureFormat::INVALID != outImageData.format)
                                {
                                    outImageData.valid = true;
                                    // TODO Recheck this part
                                    outImageData.data_offset_in_asset.ptr = outImageData.asset->memory.ptr + sizeof(DDS_Header) + sizeof(DDS_Header_DXT10);
                                    outImageData.data_offset_in_asset.len = outImageData.asset->memory.len - sizeof(DDS_Header) - sizeof(DDS_Header_DXT10);

                                    outImageData.depth = Math::Max<uint8_t>(static_cast<uint8_t>(dds_header->dw_depth), 1);
                                    MFA_ASSERT(outImageData.mipmap_count >= 1);
                                    outImageData.mipmaps = Memory::Alloc(sizeof(Mipmap) * outImageData.mipmap_count);
                                    {// Generating mipmap offset from data
                                        auto * mipmap_array = outImageData.mipmaps->memory.as<Mipmap>();
                                        auto * data_current_ptr = outImageData.data_offset_in_asset.ptr;
                                        uintmax_t mipmaps_total_len = 0;
                                        auto current_width = dds_header->dw_width;
                                        auto current_height = dds_header->dw_height;
                                        for (int i = 0; i < outImageData.mipmap_count; i++)
                                        {
                                            MFA_ASSERT(current_width >= 1);
                                            MFA_ASSERT(current_height >= 1);
                                            mipmap_array[i].data = Blob{
                                                data_current_ptr,
                                                ComputeMipmapLen(current_width, current_height, outImageData.format)
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
                                        MFA_ASSERT(mipmaps_total_len == outImageData.data_offset_in_asset.len);
                                    }
                                    ret = LoadResult::Success;
                                }
                                else
                                {
                                    ret = LoadResult::NotSupported_Format;
                                }
                            }
                            else
                            {
                                ret = LoadResult::NotSupported_DirectXVersion;
                            }
                        }
                        else
                        {
                            ret = LoadResult::NotSupported_Flags;
                        }
                    }
                    else
                    {
                        ret = LoadResult::CorruptData_InvalidMagicNumber;
                    }
                }
                else
                {
                    ret = LoadResult::CorruptData_InvalidHeaderSize;
                }
            }
            else
            {
                ret = LoadResult::CorruptData_ReadBytesDoesNotMatch;
            }
        }
        else
        {
            ret = LoadResult::FileNotFound;
        }
        return ret;
    }

}

namespace MFA::Utils::KTXTexture
{

    static void tinyktxCallbackError(void * user, char const * msg)
    {
        MFA_LOG_ERROR("Tiny_Ktx ERROR: %s", msg);
    }
    // I don't like allocation here
    static void * tinyktxCallbackAlloc(void * user, const size_t size)
    {
        //return MFA::Memory::Alloc(size).ptr;
        return ::malloc(size);
    }
    static void tinyktxCallbackFree(void * user, void * data)
    {
        ::free(data);
        //MFA::Memory::PtrFree(data);
    }

    static size_t tinyktxCallbackRead(void * userData, void * data, const size_t size)
    {
#if defined(__DESKTOP__) || defined(__IOS__)
        auto * file = static_cast<FS::FileHandle *>(userData);
        return file->read(MFA::Blob{ data, size });
#elif defined(__ANDROID__)
        auto * handle = static_cast<FS::AndroidAssetHandle *>(userData);
        return FS::Android_ReadAsset(handle, MFA::Blob{ data, size });
#else
#error Os not handled
#endif
    }

    static bool tinyktxCallbackSeek(void * userData, const int64_t offset)
    {
#if defined(__DESKTOP__) || defined(__IOS__)
        auto * file = static_cast<FS::FileHandle *>(userData);
        return file->seek(static_cast<int>(offset), FS::Origin::Start);
#elif defined(__ANDROID__)
        auto * handle = static_cast<FS::AndroidAssetHandle *>(userData);
        return FS::Android_Seek(handle, static_cast<int>(offset), FS::Origin::Start);
#else
#error Os not handled
#endif
    }

    //-------------------------------------------------------------------------------------------------

    static int64_t tinyktxCallbackTell(void * userData)
    {
#if defined(__DESKTOP__) || defined(__IOS__)
        auto * file = static_cast<FS::FileHandle *>(userData);
        int64_t location = 0;
        bool success = file->tell(location);
        MFA_ASSERT(success);
        return location;
#elif defined(__ANDROID__)
        auto * handle = static_cast<FS::AndroidAssetHandle *>(userData);
        int64_t location = 0;
        bool success = FS::Android_Tell(handle, location);
        MFA_ASSERT(success);
        return location;
#else
#error Os not handled
#endif
    }

    //-------------------------------------------------------------------------------------------------

    Data::Data(
        int32_t width_,
        int32_t height_,
        uint16_t depth_,
        uint8_t sliceCount_,
        AS::TextureFormat format_,
        uint8_t mipmapCount_,
        TinyKtx_Context * context_,
        uint64_t totalImageSize_,
        const std::shared_ptr<FS::FileHandle> & fileHandle_
    )
        : width(width_)
        , height(height_)
        , depth(depth_)
        , sliceCount(sliceCount_)
        , format(format_)
        , mipmapCount(mipmapCount_)
        , totalImageSize(totalImageSize_)
        , fileHandle(fileHandle_)
        , mContext(context_)
    {}

    //-------------------------------------------------------------------------------------------------

    bool Data::isValid() const noexcept
    {
        return width > 0 &&
        height > 0 &&
        depth > 0 &&
        sliceCount > 0 &&
        format != TextureFormat::INVALID &&
        mipmapCount > 0 &&
        mContext != nullptr &&
        totalImageSize > 0;
    }

    //-------------------------------------------------------------------------------------------------

    Data::~Data()
    {
        if (mContext != nullptr)
        {
            TinyKtx_DestroyContext(mContext);
        }
    }

    //-------------------------------------------------------------------------------------------------

    TinyKtx_Context * Data::GetContext() const
    {
        return mContext;
    }

    //-------------------------------------------------------------------------------------------------

    std::shared_ptr<Data> Load(LoadResult & loadResult, std::string const & path)
    {

        std::shared_ptr<Data> data = nullptr;

        loadResult = LoadResult::Invalid;

#if defined(__DESKTOP__) || defined(__IOS__)
        auto fileHandle = FS::OpenFile(path, FS::Usage::Read);
#elif defined(__ANDROID__)
        auto * fileHandle = FS::Android_OpenAsset(path);
#else
#error Os is not handled
#endif
        if (!MFA_VERIFY(fileHandle != nullptr))
        {
            loadResult = LoadResult::FileNotExists;
            return nullptr;
        }

        TinyKtx_Callbacks callbacks{
            &tinyktxCallbackError,
            &tinyktxCallbackAlloc,
            &tinyktxCallbackFree,
            tinyktxCallbackRead,
            &tinyktxCallbackSeek,
            &tinyktxCallbackTell
        };

        auto * ctx = TinyKtx_CreateContext(&callbacks, fileHandle.get());

        auto const readHeaderResult = TinyKtx_ReadHeader(ctx);
        MFA_ASSERT(readHeaderResult);

        auto const width = static_cast<uint16_t>(TinyKtx_Width(ctx));
        auto const height = static_cast<uint16_t>(TinyKtx_Height(ctx));
        auto const depth = Math::Max<uint16_t>(static_cast<uint16_t>(TinyKtx_Depth(ctx)), 1);
        auto const sliceCount = Math::Max<uint8_t>(static_cast<uint8_t>(TinyKtx_ArraySlices(ctx)), 1);
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
        } ();

        if (format != TextureFormat::INVALID)
        {

            auto mipmapCount = static_cast<uint8_t>(TinyKtx_NumberOfMipmaps(ctx));

            uint64_t totalImageSize = 0;

            int previousImageSize = -1;
            for (auto i = 0u; i < mipmapCount; ++i)
            {
                int const imageSize = TinyKtx_ImageSize(ctx, i);
                MFA_ASSERT(imageSize > 0);
                totalImageSize += imageSize;
                MFA_ASSERT(previousImageSize == -1 || imageSize < previousImageSize);
                previousImageSize = imageSize;
            }

            data = std::make_shared<Data>(
                width,
                height,
                depth,
                sliceCount,
                format,
                mipmapCount,
                ctx,
                totalImageSize,
                fileHandle
            );
            

            MFA_ASSERT(data->isValid());

            loadResult = LoadResult::Success;
        }
        else
        {
            TinyKtx_DestroyContext(ctx);
        }

        return data;
    }

    CBlob GetMipBlob(Data * imageData, int const mipIndex)
    {
        MFA_ASSERT(imageData != nullptr);
        MFA_ASSERT(imageData->isValid());
        MFA_ASSERT(mipIndex >= 0);
        MFA_ASSERT(imageData->mipmapCount > mipIndex);

        auto const imageSize = TinyKtx_ImageSize(imageData->GetContext(), mipIndex);
        MFA_ASSERT(imageSize > 0);

        auto const * imagePtr = TinyKtx_ImageRawData(imageData->GetContext(), mipIndex);
        MFA_ASSERT(imagePtr != nullptr);

        return CBlob{ imagePtr, imageSize };
    }

}
