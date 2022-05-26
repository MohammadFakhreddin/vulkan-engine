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
    int lightIndex;
    int faceIndex;
    int placeholder0;
    int placeholder1;
};

[[vk::push_constant]]
cbuffer {
    PushConsts pushConsts;
};

PSOut main(PSIn input) {
    // TODO: Do we really need the fragment stage ? The depth gets auto written anyway
    // TODO: Check this part
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