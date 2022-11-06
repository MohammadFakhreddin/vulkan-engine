#ifndef PBR_HLSL
#define PBR_HLSL

#include "./Math.hlsl"
#include "./Normal.hlsl"
#include "./DirectionalLightBuffer.hlsl"
#include "./DirectionalLightShadow.hlsl"
#include "./PointLightShadow.hlsl"
#include "./PointLightBuffer.hlsl"
#include "./DirectionalLightBuffer.hlsl"
#include "./MaxTextureCount.hlsl"

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

//-------------------------------------------------------------------------

float3 ReinhardToneMapping(float3 color)
{
    return color / (color + float3(1.0f));
}

//-------------------------------------------------------------------------

float3 GammaCorrect(float3 color)
{
    return pow(color, float3(1.0f/2.2f)); 
}

//-------------------------------------------------------------------------

float3 AmbientOcclusion(float3 baseColor, float ambientFactor)
{
    return baseColor * ambientFactor;
};

//-------------------------------------------------------------------------

struct EmissionParams
{
    float3 emissiveFactor;
    int textureIndex;
    sampler textureSampler; 
    float2 uv;
};

float3 EmissiveColor(
    Texture2D textures[MAX_TEXTURE_COUNT],
    EmissionParams params,
    float3 baseColor
)
{
    float3 ao = (1.0, 1.0, 1.0);
    if (params.textureIndex >= 0) {
        ao = textures[params.textureIndex].Sample(params.textureSampler, params.uv);
    }
    return baseColor * ao * params.emissiveFactor;
};

//-------------------------------------------------------------------------

struct OcclusionParams 
{
    int textureIndex;
    sampler textureSampler;
    float2 uv;
};

float OcclusionColor(
    Texture2D textures[MAX_TEXTURE_COUNT],
    OcclusionParams params
)
{
    float occlusionFactor = 1.0f;
    if (params.textureIndex >= 0) 
    {
        occlusionFactor = textures[params.textureIndex].Sample(params.textureSampler, params.uv).r;
    }
    return occlusionFactor;
};

//-------------------------------------------------------------------------

struct MetallicRoughnessParams 
{
    float metallicFactor;
    float roughnessFactor;
    int textureIndex;
    sampler textureSampler;
    float2 uv;
};

float2 MetallicRoughness(
    Texture2D textures[MAX_TEXTURE_COUNT],
    MetallicRoughnessParams params
)
{
    float metallic;
    float roughness;
    if (params.textureIndex >= 0) {
        float4 metallicRoughness = textures[params.textureIndex].Sample(params.textureSampler, params.uv);
        metallic = metallicRoughness.b;
        roughness = metallicRoughness.g;
    } else {
        metallic = params.metallicFactor;
        roughness = params.roughnessFactor;
    }
    return float2(metallic, roughness);
}

//-------------------------------------------------------------------------

struct BaseColorParams {
    float4 colorFactor;
    int textureIndex;           // -1 for null
    sampler textureSampler; 
    float2 uv;
};

float4 BaseColor(
    Texture2D textures[MAX_TEXTURE_COUNT],
    BaseColorParams params
)
{
    // TODO Why do we have pow here ?
    float4 baseColor = params.textureIndex >= 0
        ? pow(textures[params.textureIndex].Sample(params.textureSampler, params.uv).rgba, 2.2f) 
        : params.colorFactor.rgba;
    
    return baseColor;
}

//-------------------------------------------------------------------------

struct PixelNormalParams {
    float3 worldNormal;
    float3 worldTangent;
    float3 worldBiTangent;
    int normalTextureIndex;
    sampler textureSampler;
    float2 uv;
};

float3 PixelNormal(
    Texture2D textures[MAX_TEXTURE_COUNT],
    PixelNormalParams params
)
{
    float3 pixelNormal;
    if (params.normalTextureIndex < 0) 
    {
        pixelNormal = params.worldNormal;
    } else 
    {
        pixelNormal = CalculateNormalFromTexture(
            params.worldNormal,
            params.worldTangent,
            params.worldBiTangent,
            params.uv,
            textures[params.normalTextureIndex],
            params.textureSampler
        );
    }
    return pixelNormal;
}

//-------------------------------------------------------------------------

float4 PBR_ComputeColor(
    Texture2D textures[MAX_TEXTURE_COUNT],
    // Alpha
    int alphaMode,
    float alphaCutoff,
    // Base color
    BaseColorParams baseColorParams,
    // Normal
    PixelNormalParams pixelNormalParams,
    // Metallic roughness
    MetallicRoughnessParams metallicRoughnessParams,
    // Occlusion
    OcclusionParams occlusionParams,
    // Emission
    EmissionParams emissionParams,
    // Ambient
    float ambientFactor,
    // Position
    float3 cameraPosition,
    float3 worldPosition,
    float projectionFarToNearDistance,
    // Directional light
    Texture2DArray directionalLightShadowMap,
    sampler directionalLightSampler,
    float3 directionalLightPosition[MAX_DIRECTIONAL_LIGHT_COUNT],
    uint directionalLightCount,
    DirectionalLight directionalLights [MAX_DIRECTIONAL_LIGHT_COUNT],
    // Point light
    TextureCubeArray pointLightShadowMap,
    sampler pointLightSampler,
    uint pointLightCount,
    float pointLightConstantAttenuation,
    PointLight pointLights [MAX_POINT_LIGHT_COUNT]
)
{
    // TODO Why do we have pow here ?
    float4 baseColor = BaseColor(
        textures, 
        baseColorParams
    );
    
    // Alpha mask
    if (alphaMode == 1 && baseColor.a < alphaCutoff) {
        discard;
    }

    float2 metallicRoughness = MetallicRoughness(
        textures,
        metallicRoughnessParams
    );
    float metallic = metallicRoughness.x;
    float roughness = metallicRoughness.y;

	float3 surfaceNormal = PixelNormal(
        textures, 
        pixelNormalParams
    );
	float3 normalizedSurfaceNormal = normalize(surfaceNormal.xyz);
    float3 viewVector = cameraPosition - worldPosition;
	float3 normalizedViewVector = normalize(viewVector);

    // Specular contribution
	float3 Lo = float3(0.0, 0.0, 0.0);
    int lightIndex = 0;
    for (lightIndex = 0; lightIndex < directionalLightCount; lightIndex++)
    {
        DirectionalLight directionalLight = directionalLights[lightIndex];
        float3 lightVector = directionalLight.direction;
        
        float shadow = DirectionalLightShadow(
            directionalLightShadowMap,
            directionalLightSampler,
            directionalLightPosition[lightIndex], 
            lightIndex
        );
        if (shadow < 1.0f)
        {
            Lo += BRDF(
                lightVector, 
                normalizedViewVector, 
                normalizedSurfaceNormal, 
                metallic, 
                roughness, 
                baseColor.rgb, 
                1.0f,
                0.0f,
                0.0f,
                1.0f,
                directionalLight.color
            ) * (1.0f - shadow);
        }
    }

    float viewVectorLength = length(viewVector);

	for (lightIndex = 0; lightIndex < pointLightCount; lightIndex++)        // Point light count
    {   
		PointLight pointLight = pointLights[lightIndex];
        float3 lightDistanceVector = pointLight.position.xyz - worldPosition;
        float lightVectorSquareLength = lightDistanceVector.x * lightDistanceVector.x + 
            lightDistanceVector.y * lightDistanceVector.y + 
            lightDistanceVector.z * lightDistanceVector.z;
        if (lightVectorSquareLength <= pointLight.maxSquareDistance)
        {
            float lightVectorLength = sqrt(lightVectorSquareLength);
            float3 normalizedLightVector = float3(
                lightDistanceVector.x / lightVectorLength, 
                lightDistanceVector.y / lightVectorLength, 
                lightDistanceVector.z / lightVectorLength
            );
            float shadow = PointLightShadow(
                projectionFarToNearDistance,
                pointLightShadowMap,
                pointLightSampler,
                lightDistanceVector, 
                lightVectorLength, 
                viewVectorLength, 
                lightIndex
            );
            if (shadow < 1.0f) {
                Lo += BRDF(
                    normalizedLightVector, 
                    normalizedViewVector, 
                    normalizedSurfaceNormal, 
                    metallic, 
                    roughness, 
                    baseColor.rgb, 
                    pointLightConstantAttenuation,
                    pointLight.linearAttenuation,
                    pointLight.quadraticAttenuation,
                    lightVectorLength,
                    pointLight.color
                ) * (1.0 - shadow);
            }
        }
	};
    
    Lo *= OcclusionColor(
        textures, 
        occlusionParams
    );

    // Combine with ambient
    float3 color = float3(0.0, 0.0, 0.0);
    
    color += EmissiveColor(
        textures, 
        emissionParams,
        baseColor.rgb
    );

    color += AmbientOcclusion(baseColor.rgb, ambientFactor);

    color += Lo;

    // reinhard tone mapping    --> Try to implement more advanced hdr (Passing exposure parameter is also a good option)
	color = ReinhardToneMapping(color);
    // Gamma correct
    color = GammaCorrect(color); 

    return float4(color, baseColor.a);
}

//-------------------------------------------------------------------------

#endif