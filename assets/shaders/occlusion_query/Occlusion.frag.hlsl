#include "../TexturesBuffer.hlsl"
#include "../PrimitiveInfoBuffer.hlsl"
#include "../TextureSampler.hlsl"

struct PSIn {
    float4 position : SV_POSITION;   
    float2 baseColorTexCoord : TEXCOORD0;            
};

struct PushConsts
{
    float4x4 model;
    float4x4 inverseNodeTransform;
	int skinIndex;
    uint primitiveIndex;
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

    float4 baseColor = primitiveInfo.baseColorTextureIndex >= 0
        ? textures[primitiveInfo.baseColorTextureIndex].Sample(textureSampler, input.baseColorTexCoord).rgba
        : primitiveInfo.baseColorFactor.rgba;

    if (baseColor.a < 1.0f)
    {
        discard;
    }
}