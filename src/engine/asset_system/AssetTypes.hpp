#pragma once

#include <cstdint>

namespace MFA::AssetSystem
{

    enum class AssetType : uint8_t
    {
        INVALID = 0,
        Texture = 1,
        Mesh = 2,
        Shader = 3,
        Material = 4
    };

    // Also we might have opportunity not to order translucent primitives in depth pre pass and shadow passes
    enum class AlphaMode : uint8_t
    {
        Opaque = 0,
        Mask = 1,
        Blend = 2
    };

    enum class ShaderStage : uint8_t
    {
        Invalid,
        Vertex,
        Geometry,
        Fragment,
        Compute
    };

    enum class TextureFormat : uint8_t
    {
        INVALID = 0,
        UNCOMPRESSED_UNORM_R8_LINEAR = 1,
        UNCOMPRESSED_UNORM_R8G8_LINEAR = 2,
        UNCOMPRESSED_UNORM_R8G8B8A8_LINEAR = 3,
        UNCOMPRESSED_UNORM_R8_SRGB = 4,
        UNCOMPRESSED_UNORM_R8G8B8A8_SRGB = 5,

        BC7_UNorm_Linear_RGB = 6,
        BC7_UNorm_Linear_RGBA = 7,
        BC7_UNorm_sRGB_RGB = 8,
        BC7_UNorm_sRGB_RGBA = 9,

        BC6H_UFloat_Linear_RGB = 10,
        BC6H_SFloat_Linear_RGB = 11,

        BC5_UNorm_Linear_RG = 12,
        BC5_SNorm_Linear_RG = 13,

        BC4_UNorm_Linear_R = 14,
        BC4_SNorm_Linear_R = 15,

        Count
    };

    using Position = float[3];
    using Normal = float[3];
    using UV = float[2];
    using Color = uint8_t[3];
    using Tangent = float[4]; // XYZW vertex tangents where the w component is a sign value (-1 or +1) indicating handedness of the tangent basis. I need to change XYZ order if handness is different from mine
    using TextureIndex = int16_t;
    using Index = uint32_t;

    struct SamplerConfig
    {
        bool const isValid = false;
        enum class SampleMode
        {
            Linear,
            Nearest
        };
        SampleMode const sampleMode = SampleMode::Linear;
        int const magFilter = 0;
        int const minFilter = 0;
        int const wrapS = 0;
        int const wrapT = 0;
    };

}

namespace MFA
{
    namespace AS = AssetSystem;
}
