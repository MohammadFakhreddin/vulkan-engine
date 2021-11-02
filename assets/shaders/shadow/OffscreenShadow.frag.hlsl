struct PSIn {
    float4 position : SV_POSITION;                  
    float4 worldPosition: POSITION0;
};

struct PSOut {
    float depth : SV_DEPTH;
};

struct PointLight
{
    float3 position;
    float placeholder0;
    float3 color;
    float maxSquareDistance;
    float linearAttenuation;
    float quadraticAttenuation;
    float2 placeholder1;     
    float4x4 viewProjectionMatrices[6];
};

#define MAX_POINT_LIGHT_COUNT 10

struct PointLightsBufferData
{
    uint count;
    float constantAttenuation;
    float2 placeholder;

    PointLight items [MAX_POINT_LIGHT_COUNT];                                       // Max light
};

ConstantBuffer <PointLightsBufferData> pointLightsBuffer: register(b1, space0);

// Maybe we can have a separate file for this data type
struct PushConsts
{   
    float4x4 model;
    float4x4 inverseNodeTransform;
    int faceIndex;
    int skinIndex;
    uint lightIndex;
    int placeholder0;
};

[[vk::push_constant]]
cbuffer {
    PushConsts pushConsts;
};

struct CameraBuffer {
    float4x4 viewProjection;
    float3 cameraPosition;
    float projectFarToNearDistance;
};

ConstantBuffer <CameraBuffer> cameraBuffer : register (b0, space0);

PSOut main(PSIn input) {

    // get distance between fragment and light source
    float lightDistance = length(input.worldPosition.xyz - pointLightsBuffer.items[pushConsts.lightIndex].position.xyz);
    
    // map to [0;1] range by dividing by far_plane
    lightDistance = lightDistance / cameraBuffer.projectFarToNearDistance;
    
    // write this as modified depth
    float depth = lightDistance;

    PSOut output;
    output.depth = depth;
	return output;
}