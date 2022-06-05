#ifndef COMPUTE_POINT_LIGHT_HLSL
#define COMPUTE_POINT_LIGHT_HLSL

#include "./CameraBuffer.hlsl"

const float PL_ShadowBias = 0.05;

const int PL_ShadowSamplesCount = 20;
const float PL_ShadowPerSample = 1.0f / float(PL_ShadowSamplesCount);
const float3 PL_SampleOffsetDirections[20] = {
    float3(1,  1,  1),
    float3(1,  -1,  1),
    float3(-1,  -1,  1),
    float3(-1,  1,  1),
    float3(1,  1,  -1),
    float3(1,  -1,  -1),
    float3(-1,  -1,  -1),
    float3(-1,  1,  -1),
    float3(1,  1,  0),
    float3(1,  -1,  0),
    float3(-1,  -1,  0),
    float3(-1,  1,  0),
    float3(1,  0,  1),
    float3(-1,  0,  1),
    float3(1,  0,  -1),
    float3(-1,  0,  -1),
    float3(0,  1,  1),
    float3(0,  -1,  1),
    float3(0,  -1,  -1),
    float3(0,  1,  -1)
};

// CAMERA_BUFFER(PL_cameraBuffer)
// ConstantBuffer <CameraData> PL_cameraBuffer: register(b0, space0);

// POINT_LIGHT_SHADOW_MAP(PL_shadowMap)
// TextureCubeArray PL_shadowMap : register(t5, space0);

// TEXTURE_SAMPLER(PL_textureSampler)
// sampler PL_textureSampler : register(s3, space0);

// Normalized light vector will cause incorrect sampling!
float computePointLightShadow(
    float projectionFarToNearDistance,
    TextureCubeArray shadowMap,
    sampler shadowSampler,
    float3 lightVector,
    float lightDistance,
    float viewDistance,
    int lightIndex
)
{

    // TODO Use this formula for shadow bias
    //float bias = max(0.05 * (1.0 - dot(normal, lightDir)), 0.005);  

    // UNITY_SAMPLE_TEXCUBEARRAY(_MainTex, float4(i.uv, _SliceIndex));
    // get vector between fragment position and light position
    // Because we sampled the shadow cubemap from light to each position now we need to reverse the worldPosition to light vector
    float3 lightToFrag = -1 * lightVector;
    
    /*
    // Old way
    // use the light to fragment vector to sample from the depth map    
    float closestDepth = shadowMapTexture.Sample(textureSampler, lightToFrag).r;
    // it is currently in linear range between [0,1]. Re-transform back to original value
    closestDepth *= cameraBuffer.projectFarToNearDistance;
    // now get current linear depth as the length between the fragment and light position
    float currentDepth = lightDistance;
    // now test for shadows
    float shadow = currentDepth -  shadowBias > closestDepth ? 1.0 : 0.0;
    */
    // With PCF
    
    float shadow = 0.0f;
    float currentDepth = lightDistance;
    float diskRadius = (1.0f + (viewDistance / projectionFarToNearDistance)) / 75.0f;  
    for(int i = 0; i < PL_ShadowSamplesCount; ++i)
    {
        float3 uv = lightToFrag + PL_SampleOffsetDirections[i] * diskRadius;
        float closestDepth = shadowMap.Sample(shadowSampler, float4(uv.x, uv.y, uv.z, lightIndex)).r;
        closestDepth *= projectionFarToNearDistance;
        if(currentDepth - PL_ShadowBias > closestDepth) {      // Maybe we could have stored the square of closest depth instead
            shadow += PL_ShadowPerSample;
        }
    }

    return shadow;
}

#endif