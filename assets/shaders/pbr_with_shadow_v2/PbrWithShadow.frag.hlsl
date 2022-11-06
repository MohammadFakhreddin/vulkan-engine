#include "../DirectionalLightShadow.hlsl"
#include "../PointLightShadow.hlsl"
#include "../PointLightBuffer.hlsl"
#include "../CameraBuffer.hlsl"
#include "../PrimitiveInfoBuffer.hlsl"
#include "../DirectionalLightBuffer.hlsl"
#include "../MaxTextureCount.hlsl"
#include "../Normal.hlsl"
#include "../PBR.hlsl"

struct PSIn {
    float4 position : SV_POSITION;
    float3 worldPos: POSITION0;
    
    float2 baseColorTexCoord : TEXCOORD0;
    float2 metallicRoughnessTexCoord : TEXCOORD1;
    
    float2 normalTexCoord: TEXCOORD2;
    float3 worldNormal;// : NORMAL0;
    float3 worldTangent;//: TEXCOORD3;
    float3 worldBiTangent;// : TEXCOORD4;

    float2 emissiveTexCoord: TEXCOORD3;
    float2 occlusionTexCoord: TEXCOORD4;

    float3 directionLightPosition[MAX_DIRECTIONAL_LIGHT_COUNT];
};

struct PSOut {
    float4 color:SV_Target0;
};

// ConstantBuffer <CameraData> cameraBuffer: register(b0, space0);

ConstantBuffer <DirectionalLightBufferData> directionalLightBuffer: register(b1, space0);

ConstantBuffer <PointLightsBufferData> pointLightsBuffer: register(b2, space0); 

sampler textureSampler : register(s3, space0);

Texture2DArray DIR_shadowMap : register(t4, space0);

TextureCubeArray PL_shadowMap : register(t5, space0);

ConstantBuffer <PrimitiveInfoBuffer> primitiveInfoBuffer : register (b0, space1);

Texture2D textures[MAX_TEXTURE_COUNT] : register(t1, space1);  // TODO: Maybe I should decrease the textures count

struct PushConsts
{
    uint primitiveIndex : packoffset(c0);
    float3 cameraPosition : packoffset(c0.y);
    float projectFarToNearDistance : packoffset(c1);
    float3 placeholder : packoffset(c1.y);
};

[[vk::push_constant]]
cbuffer {
    PushConsts pushConsts;
};

// TODO: Pass this value as settings
const float ambientOcclusion = 0.008f;

// TODO Strength in ambient occulusion and Scale for normals
PSOut main(PSIn input) {

    PSOut output;

    PrimitiveInfo primitiveInfo = primitiveInfoBuffer.primitiveInfo[pushConsts.primitiveIndex];

    BaseColorParams baseColorParams;
    baseColorParams.colorFactor = primitiveInfo.baseColorFactor;
    baseColorParams.textureIndex = primitiveInfo.baseColorTextureIndex;
    baseColorParams.textureSampler = textureSampler;
    baseColorParams.uv = input.baseColorTexCoord;

    PixelNormalParams pixelNormalParams;
    pixelNormalParams.worldNormal = input.worldNormal;
    pixelNormalParams.worldTangent = input.worldTangent;
    pixelNormalParams.worldBiTangent = input.worldBiTangent;
    pixelNormalParams.normalTextureIndex = primitiveInfo.normalTextureIndex;
    pixelNormalParams.textureSampler = textureSampler;
    pixelNormalParams.uv = input.normalTexCoord;

    MetallicRoughnessParams metallicRoughnessParams;
    metallicRoughnessParams.metallicFactor = primitiveInfo.metallicFactor;
    metallicRoughnessParams.roughnessFactor = primitiveInfo.roughnessFactor;
    metallicRoughnessParams.textureIndex = primitiveInfo.metallicRoughnessTextureIndex;
    metallicRoughnessParams.textureSampler = textureSampler;
    metallicRoughnessParams.uv = input.metallicRoughnessTexCoord;

    OcclusionParams occlusionParams;
    occlusionParams.textureIndex = primitiveInfo.occlusionTextureIndex;
    occlusionParams.textureSampler = textureSampler;
    occlusionParams.uv = input.occlusionTexCoord;

    EmissionParams emissionParams;
    emissionParams.emissiveFactor = primitiveInfo.emissiveFactor;
    emissionParams.textureIndex = primitiveInfo.emissiveTextureIndex;
    emissionParams.textureSampler = textureSampler;
    emissionParams.uv = input.emissiveTexCoord;

    output.color = PBR_ComputeColor(
        textures,
        
        primitiveInfo.alphaMode,
        primitiveInfo.alphaCutoff,
        
        baseColorParams,
        
        pixelNormalParams,
        
        metallicRoughnessParams,
        
        occlusionParams,
        
        emissionParams,
        
        ambientOcclusion,
        
        pushConsts.cameraPosition,
        input.worldPos,
        pushConsts.projectFarToNearDistance,

        DIR_shadowMap,
        textureSampler,
        input.directionLightPosition,
        directionalLightBuffer.count,
        directionalLightBuffer.items,
        
        PL_shadowMap,
        textureSampler,
        pointLightsBuffer.count,
        pointLightsBuffer.constantAttenuation,
        pointLightsBuffer.items
    );

    return output;
}
