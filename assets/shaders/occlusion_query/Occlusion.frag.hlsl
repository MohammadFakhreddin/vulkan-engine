#include "../PrimitiveInfoBuffer.hlsl"
#include "../MaxTextureCount.hlsl"

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

sampler textureSampler : register(s3, space0);

Texture2D textures[MAX_TEXTURE_COUNT] : register(t1, space1);

ConstantBuffer <PrimitiveInfoBuffer> primitiveInfoBuffer : register (b0, space1);

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