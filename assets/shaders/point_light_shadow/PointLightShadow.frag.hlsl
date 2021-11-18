#include "../PointLightBuffer.hlsl"
#include "../CameraBuffer.hlsl"

struct PSIn {
    float4 position : SV_POSITION;                  
    float4 worldPosition: POSITION0;
    int lightIndex;
};

struct PSOut {
    float depth : SV_DEPTH;
};

POINT_LIGHT(pointLightsBuffer)

CAMERA_BUFFER(cameraBuffer)

PSOut main(PSIn input) {

    // get distance between fragment and light source
    float lightDistance = length(input.worldPosition.xyz - pointLightsBuffer.items[input.lightIndex].position.xyz);
    
    // map to [0;1] range by dividing by far_plane
    lightDistance = lightDistance / cameraBuffer.projectFarToNearDistance;
    
    // write this as modified depth
    float depth = lightDistance;

    PSOut output;
    output.depth = depth;
	return output;
}