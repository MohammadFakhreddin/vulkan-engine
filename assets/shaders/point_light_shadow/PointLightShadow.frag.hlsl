#include "../PointLightBuffer.hlsl"
#include "../CameraBuffer.hlsl"

struct PSIn {
    float4 position : SV_POSITION;                  
    float4 worldPosition: POSITION0;
};

struct PSOut {
    float depth : SV_DEPTH;
};

POINT_LIGHT(pointLightsBuffer)

CAMERA_BUFFER(cameraBuffer)

struct PushConsts
{   
    float4x4 model;
    float4x4 inverseNodeTransform;
    int skinIndex;
    int lightIndex;
    int placeholder1;
    int placeholder2;
};

[[vk::push_constant]]
cbuffer {
    PushConsts pushConsts;
};

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