#pragma once

#include "AssetBase.hpp"
#include "engine/BedrockMemory.hpp"
#include "AssetTypes.hpp"

#include <cstdint>
#include <memory>
#include <vector>

namespace MFA::AssetSystem
{
    class Texture final : public Base
    {
    public:

        using Format = TextureFormat;

        struct Dimensions
        {
            uint32_t width = 0;
            uint32_t height = 0;
            uint16_t depth = 0;
        };

        struct MipmapInfo
        {
            uint64_t offset{};
            uint32_t size{};
            Dimensions dimension{};
        };

    private:
        struct InternalFormatTableType
        {
            Format texture_format;
            uint8_t compression;                            // 0: uncompressed, 1: basis?, ...
            uint8_t component_count;                        // 1..4
            uint8_t component_format;                       // 0: UNorm, 1: SNorm, 2: UInt, 3: SInt, 4: UFloat, 5: SFloat
            uint8_t color_space;                            // 0: Linear, 1: sRGB
            uint8_t bits_r, bits_g, bits_b, bits_a;         // each 0..32
            uint8_t bits_total;                             // 1..128
        };
    public:
        static constexpr InternalFormatTableType FormatTable[] = {
            {Format::INVALID                                 , 0, 0, 0, 0, 0, 0, 0, 0,  0},

            {Format::UNCOMPRESSED_UNORM_R8_LINEAR            , 0, 1, 0, 0, 8, 0, 0, 0,  8},
            {Format::UNCOMPRESSED_UNORM_R8G8_LINEAR          , 0, 2, 0, 0, 8, 8, 0, 0, 16},
            {Format::UNCOMPRESSED_UNORM_R8G8B8A8_LINEAR      , 0, 4, 0, 0, 8, 8, 8, 8, 32},
            {Format::UNCOMPRESSED_UNORM_R8_SRGB              , 0, 1, 0, 1, 8, 0, 0, 0,  8},
            {Format::UNCOMPRESSED_UNORM_R8G8B8A8_SRGB        , 0, 4, 0, 1, 8, 8, 8, 8, 32},

            {Format::BC7_UNorm_Linear_RGB                    , 7, 3, 0, 0, 8, 8, 8, 0,  8},
            {Format::BC7_UNorm_Linear_RGBA                   , 7, 4, 0, 0, 8, 8, 8, 8,  8},
            {Format::BC7_UNorm_sRGB_RGB                      , 7, 3, 0, 1, 8, 8, 8, 0,  8},
            {Format::BC7_UNorm_sRGB_RGBA                     , 7, 4, 0, 1, 8, 8, 8, 8,  8},

            {Format::BC6H_UFloat_Linear_RGB                  , 6, 3, 4, 0, 16, 16, 16, 0, 8},
            {Format::BC6H_SFloat_Linear_RGB                  , 6, 3, 5, 0, 16, 16, 16, 0, 8},

            {Format::BC5_UNorm_Linear_RG                     , 5, 2, 0, 0, 8, 8, 0, 0, 8},
            {Format::BC5_SNorm_Linear_RG                     , 5, 2, 0, 1, 8, 8, 0, 0, 8},

            {Format::BC4_UNorm_Linear_R                      , 5, 2, 0, 0, 8, 8, 0, 0, 16},
            {Format::BC4_SNorm_Linear_R                      , 5, 2, 0, 1, 8, 8, 0, 0, 16},

        };
        static_assert(ArrayCount(FormatTable) == static_cast<unsigned>(Format::Count));

    public:

        explicit Texture(std::string nameId);
        ~Texture() override;

        Texture(Texture const &) noexcept = delete;
        Texture(Texture &&) noexcept = delete;
        Texture & operator= (Texture const & rhs) noexcept = delete;
        Texture & operator= (Texture && rhs) noexcept = delete;

        static uint8_t ComputeMipCount(Dimensions const & dimensions);

        /*
         * This function result is only correct for uncompressed data
         */
        [[nodiscard]]
        static size_t MipSizeBytes(
            Format format,
            uint16_t slices,
            Dimensions const & mipLevelDimension
        );

        // TODO Consider moving this function to util_image
        /*
         * This function result is only correct for uncompressed data
         */
         // NOTE: 0 is the *smallest* mipmap level, and "mip_count - 1" is the *largest*.
        [[nodiscard]]
        static Dimensions MipDimensions(
            uint8_t mipLevel,
            uint8_t mipCount,
            Dimensions originalImageDims
        );

        /*
        * This function result is only correct for uncompressed data
        * Returns space required for both mipmaps and TextureHeader
        */
        [[nodiscard]]
        static size_t CalculateUncompressedTextureRequiredDataSize(
            Format format,
            uint16_t slices,
            Dimensions const & dims,
            uint8_t mipCount
        );

        // TODO Constructor might have been enough
        void initForWrite(
            Format format,
            uint16_t slices,
            uint16_t depth,
            std::shared_ptr<SmartBlob> buffer
        );

        void addMipmap(
            Dimensions const & dimension,
            CBlob const & data
        );

        [[nodiscard]]
        size_t mipOffsetInBytes(uint8_t mip_level, uint8_t slice_index = 0) const;

        [[nodiscard]]
        bool isValid() const;

        [[nodiscard]]
        CBlob GetBuffer() const noexcept;

        [[nodiscard]]
        Format GetFormat() const noexcept;

        [[nodiscard]]
        uint16_t GetSlices() const noexcept;

        [[nodiscard]]
        uint8_t GetMipCount() const noexcept;

        [[nodiscard]]
        MipmapInfo const & GetMipmap(uint8_t const mipLevel) const noexcept;

        [[nodiscard]]
        MipmapInfo const * GetMipmaps() const noexcept;

        [[nodiscard]]
        uint16_t GetDepth() const noexcept
        {
            return mDepth;
        }

        [[nodiscard]]
        std::string const & GetNameId() const noexcept
        {
            return mNameId;
        }

    private:

        std::string mNameId {};

        Format mFormat = Format::INVALID;

        uint16_t mSlices = 0;

        uint8_t mMipCount = 0;

        uint16_t mDepth = 0;

        std::vector<MipmapInfo> mMipmapInfos{};

        std::shared_ptr<SmartBlob> mBuffer{};

        uint64_t mCurrentOffset = 0;

        int mPreviousMipWidth = -1;

        int mPreviousMipHeight = -1;

    };

}

namespace MFA
{
    namespace AS = AssetSystem;
}
