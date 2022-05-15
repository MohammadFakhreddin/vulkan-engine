#include "../TexturesBuffer.hlsl"
#include "../PrimitiveInfoBuffer.hlsl"
#include "../TextureSampler.hlsl"

struct PSIn {
    float4 position : SV_POSITION;   
    float2 baseColorTexCoord : TEXCOORD0;            
};

struct PushConsts
{
    uint primitiveIndex;
    int placeholder0;
    int placeholder1;
    int placeholder2;
};

[[vk::push_constant]]
cbuffer {
    PushConsts pushConsts;
};

TEXTURE_SAMPLER(textureSampler)

TEXTURES_BUFFER(textures)

PRIMITIVE_INFO(primitiveInfoBuffer)

void main(PSIn input) {
    PrimitiveInfo primitiveInfo = primitiveInfoBuffer.primitiveInfo[pushConsts.primitiveIndex];
    if (primitiveInfo.alphaMode == 1) {
        float4 baseColor = primitiveInfo.baseColorTextureIndex >= 0
            ? textures[primitiveInfo.baseColorTextureIndex].Sample(textureSampler, input.baseColorTexCoord).rgba
            : primitiveInfo.baseColorFactor.rgba;

        if (baseColor.a < primitiveInfo.alphaCutoff)
        {
            discard;
        }
    }
}