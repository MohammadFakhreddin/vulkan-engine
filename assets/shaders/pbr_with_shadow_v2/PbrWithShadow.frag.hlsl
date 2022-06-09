// #include "../ComputeDirectionalLightShadow.hlsl"
// #include "../ComputePointLightShadow.hlsl"
#include "../PointLightBuffer.hlsl"
#include "../CameraBuffer.hlsl"
#include "../PrimitiveInfoBuffer.hlsl"
#include "../DirectionalLightBuffer.hlsl"
#include "../MaxTextureCount.hlsl"

struct PSIn {
    float4 position : SV_POSITION;
    // float3 worldPos: POSITION0;
    
    float2 baseColorTexCoord : TEXCOORD0;
    // float2 metallicRoughnessTexCoord : TEXCOORD1;
    
    // float2 normalTexCoord: TEXCOORD2;
    // float3 worldNormal : NORMAL0;
    // float3 worldTangent: TEXCOORD3;
    // float3 worldBiTangent : TEXCOORD4;

    // float2 emissiveTexCoord: TEXCOORD1;
    // float2 occlusionTexCoord: TEXCOORD2;

    // float4 directionLightPosition[3];
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
    uint primitiveIndex;
    float3 cameraPosition;
};

[[vk::push_constant]]
cbuffer {
    PushConsts pushConsts;
};

const float PI = 3.14159265359;

// TODO Each light source should have its own attenuation, TODO Get this values from cpp code (From I was doing things wrong website)
// Fow now I assume we have a light source with r = 1.0f
// const float lightSphereRadius = 1.0f;
// const float constantAttenuation = 1.0f;
// const float linearAttenuation = 2.0f / lightSphereRadius;
// const float quadraticAttenuation = 1.0f / (lightSphereRadius * lightSphereRadius);

const float ambientOcclusion = 0.008f;

// This function computes ratio between amount of light that reflect and refracts
// Reflection contrbutes to specular while refraction contributes to diffuse light
/*
    F0 parameter which is known as the surface reflection at zero incidence or how much the surface reflects 
    if looking directly at the surface. The F0 varies per material and is tinted on metals as we find in 
    large material databases. In the PBR metallic workflow we make the simplifying assumption that most dielectric surfaces 
    look visually correct with a constant F0 of 0.04, while we do specify F0 for metallic surfaces as then given by the albedo value. 
    This translates to code as follows:
*/
// Fresnel function ----------------------------------------------------
float3 F_Schlick(float cosTheta, float metallic, float3 baseColor)
{
	float3 F0 = lerp(float3(0.04, 0.04, 0.04), baseColor.rgb, metallic); // * material.specular
	float3 F = F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
	return F;
}

// Normal Distribution function --------------------------------------
float D_GGX(float dotNH, float roughness)
{
	float alpha = roughness * roughness;
	float alpha2 = alpha * alpha;
	float denom = dotNH * dotNH * (alpha2 - 1.0) + 1.0;
	return (alpha2)/(PI * denom*denom);
}

// This function computes the self shadowing effect
// Geometric Shadowing function --------------------------------------
float G_SchlicksmithGGX(float dotNL, float dotNV, float roughness)
{
	float r = (roughness + 1.0);
	float k = (r*r) / 8.0;
	float GL = dotNL / (dotNL * (1.0 - k) + k);
	float GV = dotNV / (dotNV * (1.0 - k) + k);
	return GL * GV;
}

// Specular BRDF composition --------------------------------------------

float3 BRDF(
    float3 L,
    float3 V,
    float3 N,
    float metallic,
    float roughness,
    float3 baseColor,
    float3 worldPos,
    float constantAttenuation,
    float linearAttenuation,
    float quadraticAttenuation,
    float lightVectorLength,
    float3 lightColor
)
{
    // Precalculate vectors and dot products
	float3 H = normalize (V + L);
    float dotNV = clamp(dot(N, V), 0.0, 1.0);
	float dotNL = clamp(dot(N, L), 0.0, 1.0);
	float dotLH = clamp(dot(L, H), 0.0, 1.0);
	float dotNH = clamp(dot(N, H), 0.0, 1.0);

    float3 color = float3(0, 0, 0);

    float distance = lightVectorLength;//length(lightBuffer.lightPosition - worldPos);
    // TODO Consider using more advanced attenuation https://github.com/lettier/3d-game-shaders-for-beginners/blob/master/sections/lighting.md
    // float attenuation =
    //     1
    //   / ( p3d_LightSource[i].constantAttenuation
    //     + p3d_LightSource[i].linearAttenuation
    //     * lightDistance
    //     + p3d_LightSource[i].quadraticAttenuation
    //     * (lightDistance * lightDistance)
    //     );
    // https://imdoingitwrong.wordpress.com/2011/01/31/light-attenuation/
    float attenuation =
        1.0f
      / ( 
            constantAttenuation
            + linearAttenuation * distance
            + quadraticAttenuation * distance * distance
        );
    float3 radiance = lightColor * attenuation;        
    
    // cook-torrance brdf
    float NDF = D_GGX(dotNH, roughness);        
    float G = G_SchlicksmithGGX(dotNL, dotNV, roughness);  
    float3 F = F_Schlick(dotNV, metallic, baseColor);;       
    
    float3 kS = F;
    float3 kD = float3(1.0) - kS;
    kD *= 1.0 - metallic;	  
    
    float3 numerator    = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0);
    float3 specular     = numerator / max(denominator, 0.001);  
        
    // add to outgoing radiance Lo
    float NdotL = max(dot(N, L), 0.0);                
    color += (kD * baseColor.rgb / PI + specular) * radiance * NdotL; 

	return color;
}

// TODO Use this to compute normal correctly
// float3 calculateNormal(PSIn input, int normalTextureIndex)
// {
//     float3 pixelNormal;
//     if (normalTextureIndex < 0) {
//         pixelNormal = input.worldNormal;
//     } else {
//         float3 tangentNormal = textures[normalTextureIndex].Sample(textureSampler, input.normalTexCoord).rgb * 2.0 - 1.0;
        
//         float3x3 TBN = transpose(float3x3(input.worldTangent, input.worldBiTangent, input.worldNormal));
//         // float3x3 TBN = float3x3(input.worldTangent, input.worldBiTangent, input.worldNormal));
//         pixelNormal = mul(TBN, tangentNormal).xyz;
//     }
//     return pixelNormal;
// };


// TODO Strength in ambient occulusion and Scale for normals
PSOut main(PSIn input) {
    PrimitiveInfo primitiveInfo = primitiveInfoBuffer.primitiveInfo[pushConsts.primitiveIndex];

    // TODO Why do we have pow here ?
    float4 baseColor = primitiveInfo.baseColorTextureIndex >= 0
        ? pow(textures[primitiveInfo.baseColorTextureIndex].Sample(textureSampler, input.baseColorTexCoord).rgba, 2.2f) 
        : primitiveInfo.baseColorFactor.rgba;

    // Alpha mask
    if (primitiveInfo.alphaMode == 1 && baseColor.a < primitiveInfo.alphaCutoff) {
        discard;
    }
	
    // float metallic = 0.0f;
    // float roughness = 0.0f;
    // // TODO: Is usages of occlusion correct ?
    // // TODO Handle occlusionTexture and its strength
    // if (primitiveInfo.metallicRoughnessTextureIndex >= 0) {
    //     float4 metallicRoughness = textures[primitiveInfo.metallicRoughnessTextureIndex].Sample(textureSampler, input.metallicRoughnessTexCoord);
    //     metallic = metallicRoughness.b;
    //     roughness = metallicRoughness.g;
    // } else {
    //     metallic = primitiveInfo.metallicFactor;
    //     roughness = primitiveInfo.roughnessFactor;
    // }

	// float3 surfaceNormal = calculateNormal(input, primitiveInfo.normalTextureIndex);
	// float3 normalizedSurfaceNormal = normalize(surfaceNormal.xyz);
    // float3 viewVector = cameraBuffer.cameraPosition - input.worldPos;
	// float3 normalizedViewVector = normalize(viewVector);

    // Specular contribution
	float3 Lo = float3(0.0, 0.0, 0.0);
    // int lightIndex = 0;
    // for (lightIndex = 0; lightIndex < directionalLightBuffer.count; lightIndex++)
    // {
    //     DirectionalLight directionalLight = directionalLightBuffer.items[lightIndex];
    //     float3 lightVector = directionalLight.direction;
        
    //     float shadow = computeDirectionalLightShadow(
    //         DIR_shadowMap,
    //         textureSampler,
    //         input.directionLightPosition[lightIndex], 
    //         lightIndex
    //     );
    //     // float shadow = 0.0f;
    //     if (shadow < 1.0f)
    //     {
    //         Lo += BRDF(
    //             lightVector, 
    //             normalizedViewVector, 
    //             normalizedSurfaceNormal, 
    //             metallic, 
    //             roughness, 
    //             baseColor.rgb, 
    //             input.worldPos,
    //             1.0f,
    //             0.0f,
    //             0.0f,
    //             1.0f,
    //             directionalLight.color
    //         ) * (1.0f - shadow);
    //     }
    // }
    
    // float viewVectorLength = length(viewVector);

	// for (lightIndex = 0; lightIndex < pointLightsBuffer.count; lightIndex++)        // Point light count
    // {   
	// 	PointLight pointLight = pointLightsBuffer.items[lightIndex];
    //     float3 lightDistanceVector = pointLight.position.xyz - input.worldPos;
    //     float lightVectorSquareLength = lightDistanceVector.x * lightDistanceVector.x + 
    //         lightDistanceVector.y * lightDistanceVector.y + 
    //         lightDistanceVector.z * lightDistanceVector.z;
    //     if (lightVectorSquareLength <= pointLight.maxSquareDistance)
    //     {
    //         float lightVectorLength = sqrt(lightVectorSquareLength);
    //         float3 normalizedLightVector = float3(
    //             lightDistanceVector.x / lightVectorLength, 
    //             lightDistanceVector.y / lightVectorLength, 
    //             lightDistanceVector.z / lightVectorLength
    //         );
    //         float shadow = computePointLightShadow(
    //             cameraBuffer.projectFarToNearDistance,
    //             PL_shadowMap,
    //             textureSampler,
    //             lightDistanceVector, 
    //             lightVectorLength, 
    //             viewVectorLength, 
    //             lightIndex
    //         );
    //         if (shadow < 1.0f) {
    //             Lo += BRDF(
    //                 normalizedLightVector, 
    //                 normalizedViewVector, 
    //                 normalizedSurfaceNormal, 
    //                 metallic, 
    //                 roughness, 
    //                 baseColor.rgb, 
    //                 input.worldPos,
    //                 pointLightsBuffer.constantAttenuation,
    //                 pointLight.linearAttenuation,
    //                 pointLight.quadraticAttenuation,
    //                 lightVectorLength,
    //                 pointLight.color
    //             ) * (1.0 - shadow);
    //         }
    //     }
	// };
    
    // if (primitiveInfo.occlusionTextureIndex >= 0) {
    //     float occlusionFactor = textures[primitiveInfo.occlusionTextureIndex].Sample(
    //         textureSampler, 
    //         input.occlusionTexCoord
    //     ).r;
    //     Lo *= occlusionFactor;
    // }

    // Combine with ambient
    float3 color = float3(0.0, 0.0, 0.0);
    
    // if (primitiveInfo.emissiveTextureIndex >= 0) {
    //     float3 ao = textures[primitiveInfo.emissiveTextureIndex].Sample(textureSampler, input.emissiveTexCoord);
    //     color += float3(baseColor.r * ao.r, baseColor.g * ao.g, baseColor.b * ao.b) * primitiveInfo.emissiveFactor;
    // } else {
    //     color += baseColor.rgb * primitiveInfo.emissiveFactor; // * 0.3
    // }

    color += baseColor.rgb * ambientOcclusion;

    color += Lo;
    
    // reinhard tone mapping    --> Try to implement more advanced hdr (Passing exposure parameter is also a good option)
	color = color / (color + float3(1.0f));
    // Gamma correct
    color = pow(color, float3(1.0f/2.2f)); 

    PSOut output;
    output.color = float4(color, baseColor.a);
	return output;


}
