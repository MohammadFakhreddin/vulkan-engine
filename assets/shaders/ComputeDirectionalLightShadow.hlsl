#ifndef COMPUTE_DIRECTIONAL_LIGHT_HLSL
#define COMPUTE_DIRECTIONAL_LIGHT_HLSL

#include "./TextureSampler.hlsl"
#include "./DirectionalLightShadowMap.hlsl"

const float DIR_ShadowBias = 0.0005;

#define DIR_SHADOW_TEXTURE_SIZE 2048.0f
#define DIR_SHADOW_PER_SAMPLE 0.111f //1.0f / 9.0f;

TEXTURE_SAMPLER(DIR_shadowSampler)

DIRECTIONAL_LIGHT_SHADOW_MAP(DIR_shadowMap)

float directionalLightShadowCalculation(float4 directionLightPosition, int lightIndex)
{
    // TODO Use this formula for shadow bias
    //float bias = max(0.05 * (1.0 - dot(normal, lightDir)), 0.005);  

    float shadow = 0.0f;
    // perform perspective divide
    float3 projCoords = directionLightPosition.xyz; //In ortho w is always 1:directionLightPosition.xyz / directionLightPosition.w;          // I think w is 1 most of the times!
    // transform from [-1,1] to [0,1] range Note: We are already in 0 to 1
    projCoords.x = projCoords.x * 0.5 + 0.5;                // I think we need this because we write from 0 to 1 into the images
    projCoords.y = projCoords.y * 0.5 + 0.5;                // I think we need this because we write from 0 to 1 into the images

    float currentDepth = projCoords.z;      // depth is alreay between 0 to 1 

    // if (currentDepth > 1 || currentDepth < 0 )
    // {
    //     return 1.0f;
    // }

    // Without pcf
    float closestDepth = DIR_shadowMap.Sample(DIR_shadowSampler, float3(projCoords.x, projCoords.y, lightIndex)).r; 
    if(currentDepth - DIR_ShadowBias >= closestDepth) {      // Maybe we could have stored the square of closest depth instead
        shadow = 1.0f;
    }

    // With pcf
    // float2 texelSize = 1.0f / DIR_SHADOW_TEXTURE_SIZE;
    // for(int x = -1; x <= 1; ++x)
    // {
    //     for(int y = -1; y <= 1; ++y)
    //     {
    //         float2 uv = projCoords.xy + float2(x, y) * texelSize;
    //         // get closest depth value from light's perspective (using [0,1] range fragPosLight as coords)
    //         float closestDepth = DIR_shadowMap.Sample(DIR_shadowSampler, float3(uv.x, uv.y, lightIndex)).r; 
    //         if(currentDepth - DIR_ShadowBias > closestDepth) {      // Maybe we could have stored the square of closest depth instead
    //             shadow += DIR_SHADOW_PER_SAMPLE;
    //         }
    //     }
    // }
    return shadow;
}

#endif