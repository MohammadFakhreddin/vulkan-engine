#include "AssetTexture.hpp"

#include "engine/BedrockAssert.hpp"
#include "engine/BedrockMath.hpp"

namespace MFA::AssetSystem
{
        //-------------------------------------------------------------------------------------------------

    Texture::Texture(std::string nameId) : mNameId(std::move(nameId)) {}
    Texture::~Texture() = default;

    //-------------------------------------------------------------------------------------------------

    uint8_t Texture::ComputeMipCount(Dimensions const & dimensions)
    {
        uint32_t const max_dimension = Math::Max(
            dimensions.width,
            Math::Max(
                dimensions.height,
                static_cast<uint32_t>(dimensions.depth)
            )
        );
        for (uint8_t i = 0; i < 32; ++i)
            if ((1ULL << i) >= max_dimension)
                return 1 + i;
        return 33;
    }

    //-------------------------------------------------------------------------------------------------

    size_t Texture::MipSizeBytes(
        Format format,
        uint16_t const slices,
        Dimensions const & mipLevelDimension
    )
    {
        auto const & d = mipLevelDimension;
        size_t const p = FormatTable[static_cast<unsigned>(format)].bits_total / 8;
        return p * slices * d.width * d.height * d.depth;
    }

    //-------------------------------------------------------------------------------------------------

    Texture::Dimensions Texture::MipDimensions(
        uint8_t const mipLevel,
        uint8_t const mipCount,
        Dimensions const originalImageDims
    )
    {
        Dimensions ret = {};
        if (mipLevel < mipCount)
        {
            uint32_t const pow = mipLevel;
            uint32_t const add = (1 << pow) - 1;
            ret.width = (originalImageDims.width + add) >> pow;
            ret.height = (originalImageDims.height + add) >> pow;
            ret.depth = static_cast<uint16_t>((originalImageDims.depth + add) >> pow);
        }
        return ret;
    }

    //-------------------------------------------------------------------------------------------------

    size_t Texture::CalculateUncompressedTextureRequiredDataSize(
        Format const format,
        uint16_t const slices,
        Dimensions const & dims,
        uint8_t const mipCount
    )
    {
        size_t ret = 0;
        for (uint8_t mip_level = 0; mip_level < mipCount; mip_level++)
        {
            ret += MipSizeBytes(
                format,
                slices,
                MipDimensions(mip_level, mipCount, dims)
            );
        }
        return ret;
    }

    //-------------------------------------------------------------------------------------------------

    void Texture::initForWrite(
        const Format format,
        const uint16_t slices,
        const uint16_t depth,
        std::shared_ptr<SmartBlob> buffer
    )
    {
        MFA_ASSERT(format != Format::INVALID);
        mFormat = format;
        MFA_ASSERT(slices > 0);
        mSlices = slices;
        MFA_ASSERT(depth > 0);
        mDepth = depth;
        MFA_ASSERT(buffer != nullptr);
        mBuffer = std::move(buffer);
    }

    //-------------------------------------------------------------------------------------------------

    void Texture::addMipmap(
        Dimensions const & dimension,
        CBlob const & data
    )
    {
        MFA_ASSERT(data.ptr != nullptr);
        MFA_ASSERT(data.len > 0);

        MFA_ASSERT(mPreviousMipWidth == -1 || mPreviousMipWidth > static_cast<int>(dimension.width));
        MFA_ASSERT(mPreviousMipHeight == -1 || mPreviousMipHeight > static_cast<int>(dimension.height));
        mPreviousMipWidth = static_cast<int>(dimension.width);
        mPreviousMipHeight = static_cast<int>(dimension.height);

        auto const dataLen = static_cast<uint32_t>(data.len);
        {
            MipmapInfo mipmapInfo{};
            mipmapInfo.offset = mCurrentOffset;
            mipmapInfo.size = dataLen;
            mipmapInfo.dimension = dimension;
            mMipmapInfos.emplace_back(mipmapInfo);
        }
        uint64_t const nextOffset = mCurrentOffset + dataLen;
        MFA_ASSERT(mBuffer->memory.ptr != nullptr);
        MFA_ASSERT(nextOffset <= mBuffer->memory.len);
        ::memcpy(mBuffer->memory.ptr + mCurrentOffset, data.ptr, data.len);
        mCurrentOffset = nextOffset;

        ++mMipCount;
    }

    //-------------------------------------------------------------------------------------------------

    size_t Texture::mipOffsetInBytes(uint8_t const mip_level, uint8_t const slice_index) const
    {
        size_t ret = 0;
        if (mip_level < mMipCount && slice_index < mSlices)
        {
            ret = mMipmapInfos[mip_level].offset + slice_index * mMipmapInfos[mip_level].size;
        }
        return ret;
    }

    //-------------------------------------------------------------------------------------------------

    bool Texture::isValid() const
    {
        return
            mFormat != Format::INVALID &&
            mSlices > 0 &&
            mMipCount > 0 &&
            mMipmapInfos.empty() == false &&
            mBuffer->memory.ptr != nullptr &&
            mBuffer->memory.len > 0;
    }

    //-------------------------------------------------------------------------------------------------

    CBlob Texture::GetBuffer() const noexcept
    {
        return mBuffer->memory;
    }

    //-------------------------------------------------------------------------------------------------

    Texture::Format Texture::GetFormat() const noexcept
    {
        return mFormat;
    }

    //-------------------------------------------------------------------------------------------------

    uint16_t Texture::GetSlices() const noexcept
    {
        return mSlices;
    }

    //-------------------------------------------------------------------------------------------------

    uint8_t Texture::GetMipCount() const noexcept
    {
        return mMipCount;
    }

    //-------------------------------------------------------------------------------------------------

    Texture::MipmapInfo const & Texture::GetMipmap(uint8_t const mipLevel) const noexcept
    {
        return mMipmapInfos[mipLevel];
    }

    //-------------------------------------------------------------------------------------------------

    Texture::MipmapInfo const * Texture::GetMipmaps() const noexcept
    {
        return mMipmapInfos.data();
    }

    //-------------------------------------------------------------------------------------------------

}
