struct PSIn {
    float4 position : SV_POSITION;
    float3 worldPos: POSITION0;
    
    float2 baseColorTexCoord : TEXCOORD0;
    float2 metallicRoughnessTexCoord : TEXCOORD1;
    
    float2 normalTexCoord: TEXCOORD2;
    float3 worldNormal : NORMAL0;
    float3 worldTangent: TEXCOORD3;
    float3 worldBiTangent : TEXCOORD4;

    float2 emissiveTexCoord: TEXCOORD5;

};

struct PSOut {
    float4 color:SV_Target0;
};

struct PrimitiveInfo {
    float4 baseColorFactor: COLOR0;
    float3 emissiveFactor: COLOR3;
    float placeholder0;
    
    int baseColorTextureIndex;
    
    float metallicFactor: COLOR1;
    float roughnessFactor: COLOR2;
    int metallicRoughnessTextureIndex;

    int normalTextureIndex;  

    int emissiveTextureIndex;
    
    // TODO Occlusion texture
    
    int hasSkin;    // TODO Move hasSkin to vertex

    int placeholder1;
};

struct PrimitiveInfoBuffer {
    PrimitiveInfo primitiveInfo[];
};

ConstantBuffer <PrimitiveInfoBuffer> smBuff : register (b2, space0);

struct LightViewBuffer {
    float3 lightPosition;
    float3 camPos;              // It shouldn't be always 0, I might need to 
    float3 lightColor;          // Light color can be from 0 to inf (Sun for example can exceed 1.0f)
    float nearToFarPlaneDistance;
};

ConstantBuffer <LightViewBuffer> lvBuff : register (b3, space0);

sampler shadowMapSampler : register(s4, space0);
TextureCube shadowMapTexture : register(t4, space0);

sampler textureSampler : register(s5, space0);

Texture2D textures[64] : register(t6, space0);

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

const float PI = 3.14159265359;

// TODO Each light source should have its own attenuation, TODO Get this values from cpp code (From I was doing things wrong website)
// Fow now I assume we have a light source with r = 1.0f
const float lightSphereRadius = 1.0f;
const float constantAttenuation = 1.0f;
const float linearAttenuation = 2.0f / lightSphereRadius;
const float quadraticAttenuation = 1.0f / (lightSphereRadius * lightSphereRadius);

const float alphaMaskCutoff = 0.1f;

const float ambientOcclusion = 0.02f;

const float shadowBias = 0.05;
const int shadowSamples = 20;
const float shadowPerSample = 1.0f / float(shadowSamples);

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
    float3 worldPos
)
{
    // Precalculate vectors and dot products
	float3 H = normalize (V + L);
    float dotNV = clamp(dot(N, V), 0.0, 1.0);
	float dotNL = clamp(dot(N, L), 0.0, 1.0);
	float dotLH = clamp(dot(L, H), 0.0, 1.0);
	float dotNH = clamp(dot(N, H), 0.0, 1.0);

    float3 color = float3(0, 0, 0);

    float distance = length(lvBuff.lightPosition - worldPos);
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
    float3 radiance = lvBuff.lightColor * attenuation;        
    
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
float3 calculateNormal(PSIn input, int normalTextureIndex)
{
    float3 pixelNormal;
    if (normalTextureIndex < 0) {
        pixelNormal = input.worldNormal;
    } else {
        float3 tangentNormal = textures[normalTextureIndex].Sample(textureSampler, input.normalTexCoord).rgb * 2.0 - 1.0;
        
        float3x3 TBN = transpose(float3x3(input.worldTangent, input.worldBiTangent, input.worldNormal));
        // float3x3 TBN = float3x3(input.worldTangent, input.worldBiTangent, input.worldNormal));
        pixelNormal = mul(TBN, tangentNormal).xyz;
    }
    return pixelNormal;
};

float shadowCalculation(float3 lightVector, float viewDistance)
{
    // get vector between fragment position and light position
    // Because we sampled the shadow cubemap from light to each position now we need to reverse the worldPosition to light vector
    float3 lightToFrag = -1 * lightVector;
    
    // // Old way
    // // use the light to fragment vector to sample from the depth map    
    // float closestDepth = shadowMapTexture.Sample(shadowMapSampler, lightToFrag).r;
    // // it is currently in linear range between [0,1]. Re-transform back to original value
    // closestDepth *= lvBuff.nearToFarPlaneDistance;
    // // now get current linear depth as the length between the fragment and light position
    // float currentDepth = length(lightToFrag);
    // // now test for shadows
    // float shadow = currentDepth -  bias > closestDepth ? 1.0 : 0.0;

    // TODO We can make this array an buffer
    float3 sampleOffsetDirections[20];
    sampleOffsetDirections[0] = float3(1,  1,  1);
    sampleOffsetDirections[1] = float3(1,  -1,  1);
    sampleOffsetDirections[2] = float3(-1,  -1,  1);
    sampleOffsetDirections[3] = float3(-1,  1,  1);
    sampleOffsetDirections[4] = float3(1,  1,  -1);
    sampleOffsetDirections[5] = float3(1,  -1,  -1);
    sampleOffsetDirections[6] = float3(-1,  -1,  -1);
    sampleOffsetDirections[7] = float3(-1,  1,  -1);
    sampleOffsetDirections[8] = float3(1,  1,  0);
    sampleOffsetDirections[9] = float3(1,  -1,  0);
    sampleOffsetDirections[10] = float3(-1,  -1,  0);
    sampleOffsetDirections[11] = float3(-1,  1,  0);
    sampleOffsetDirections[12] = float3(1,  0,  1);
    sampleOffsetDirections[13] = float3(-1,  0,  1);
    sampleOffsetDirections[14] = float3(1,  0,  -1);
    sampleOffsetDirections[15] = float3(-1,  0,  -1);
    sampleOffsetDirections[16] = float3(0,  1,  1);
    sampleOffsetDirections[17] = float3(0,  -1,  1);
    sampleOffsetDirections[18] = float3(0,  -1,  -1);
    sampleOffsetDirections[19] = float3(0,  1,  -1);
    
    float shadow = 0.0f;
    float currentDepth = length(lightToFrag);
    float diskRadius = (1.0f + (viewDistance / lvBuff.nearToFarPlaneDistance)) / 75.0f;  
    for(int i = 0; i < shadowSamples; ++i)
    {
        float closestDepth = shadowMapTexture.Sample(shadowMapSampler, lightToFrag + sampleOffsetDirections[i] * diskRadius).r;
        closestDepth *= lvBuff.nearToFarPlaneDistance;
        if(currentDepth - shadowBias > closestDepth) {
            shadow += shadowPerSample;
        }
    }

    return shadow;
}
// TODO Strength in ambient occulusion and Scale for normals
PSOut main(PSIn input) {
    PrimitiveInfo primitiveInfo = smBuff.primitiveInfo[pushConsts.primitiveIndex];

    float4 baseColor = primitiveInfo.baseColorTextureIndex >= 0
        ? pow(textures[primitiveInfo.baseColorTextureIndex].Sample(textureSampler, input.baseColorTexCoord).rgba, 2.2f)
        : primitiveInfo.baseColorFactor.rgba;

    // Alpha mask
    if (baseColor.a < alphaMaskCutoff) {
        discard;
    }
	
    float metallic = 0.0f;
    float roughness = 0.0f;
    // TODO: Is usages of occlusion correct ?
    // TODO Handle occlusionTexture and its strength
    if (primitiveInfo.metallicRoughnessTextureIndex >= 0) {
        float4 metallicRoughness = textures[primitiveInfo.metallicRoughnessTextureIndex].Sample(textureSampler, input.metallicRoughnessTexCoord);
        metallic = metallicRoughness.b;
        roughness = metallicRoughness.g;
    } else {
        metallic = primitiveInfo.metallicFactor;
        roughness = primitiveInfo.roughnessFactor;
    }

	float3 normal = calculateNormal(input, primitiveInfo.normalTextureIndex);

	float3 N = normalize(normal.xyz);
    
	float3 V = normalize(lvBuff.camPos - input.worldPos);
    
    // Specular contribution
	float3 Lo = float3(0.0, 0.0, 0.0);
	for (int i = 0; i < 1; i++) {   // Light count
		float3 L = lvBuff.lightPosition.xyz - input.worldPos;
        float shadow = shadowCalculation(L, length(lvBuff.camPos - input.worldPos));
    	Lo += BRDF(normalize(L), V, N, metallic, roughness, baseColor.rgb, input.worldPos) * (1.0 - shadow);
	};

    // Combine with ambient
    float3 color = float3(0.0, 0.0, 0.0);
    
    if (primitiveInfo.emissiveTextureIndex >= 0) {
        float3 ao = textures[primitiveInfo.emissiveTextureIndex].Sample(textureSampler, input.emissiveTexCoord);
        color += float3(baseColor.r * ao.r, baseColor.g * ao.g, baseColor.b * ao.b) * primitiveInfo.emissiveFactor;
    } else {
        color += baseColor.rgb * primitiveInfo.emissiveFactor; // * 0.3
    }
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
