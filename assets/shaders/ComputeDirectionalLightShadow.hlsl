#ifndef COMPUTE_DIRECTIONAL_LIGHT_HLSL
#define COMPUTE_DIRECTIONAL_LIGHT_HLSL

#include "./TextureSampler.hlsl"
#include "./DirectionalLightShadowMap.hlsl"

const float DIR_ShadowBias = 0.05;

#define DIR_SHADOW_TEXTURE_SIZE 1024.0f
#define DIR_SHADOW_PER_SAMPLE 0.25f //1.0f / 4.0f;

TEXTURE_SAMPLER(DIR_shadowSampler)

DIRECTIONAL_LIGHT_SHADOW_MAP(DIR_shadowMap)

float directionalLightShadowCalculation(float4 directionLightPosition, int lightIndex)
{
    float shadow = 0.0f;
    float currentDepth = directionLightPosition.z;
    // perform perspective divide
    float3 projCoords = directionLightPosition.xyz / directionLightPosition.w;
    // transform to [0,1] range
    projCoords = projCoords * 0.5 + 0.5;
    
    float2 texelSize = 1.0f / DIR_SHADOW_TEXTURE_SIZE;
    for(int x = -1; x <= 1; ++x)
    {
        for(int y = -1; y <= 1; ++y)
        {
            float2 uv = projCoords.xy + float2(x, y) * texelSize;
            // get closest depth value from light's perspective (using [0,1] range fragPosLight as coords)
            float closestDepth = DIR_shadowMap.Sample(DIR_shadowSampler, float3(uv.x, uv.y, lightIndex)).r; 
            if(currentDepth - DIR_ShadowBias > closestDepth) {      // Maybe we could have stored the square of closest depth instead
                shadow += DIR_SHADOW_PER_SAMPLE;
            }
        }
    }
    return shadow;
}

#endif