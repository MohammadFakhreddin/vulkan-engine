#ifndef NORMAL_HLSL
#define NORMAL_HLSL

float3 CalculateNormalFromTexture(
    float3 worldNormal, 
    float3 worldTangent,
    float3 worldBiTangent, 
    float2 normalTexCoord, 
    Texture2D normalTexture, 
    sampler textureSampler
)
{
    float3 tangentNormal = normalTexture.Sample(textureSampler, normalTexCoord).rgb * 2.0 - 1.0;
    float3x3 TBN = transpose(float3x3(worldTangent, worldBiTangent, worldNormal));
    float3 pixelNormal = mul(TBN, tangentNormal).xyz;
    return pixelNormal;
};

#endif