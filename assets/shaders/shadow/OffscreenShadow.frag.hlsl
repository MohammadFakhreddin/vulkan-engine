struct PSIn {
    float4 position : SV_POSITION;                  
    float4 worldPosition: POSITION0;
};

struct PSOut {
    float depth : SV_DEPTH;
};

struct LightBuffer {
    float3 lightPosition; //TODO We will have multiple lights and visibility info
    float placeholder0;
    float3 lightColor;
    float placeholder1;
};

ConstantBuffer <LightBuffer> lightBuffer : register (b2, space0);

struct CameraBuffer {
    float4x4 viewProjection;
    float3 cameraPosition;
    float projectFarToNearDistance;
};

ConstantBuffer <CameraBuffer> cameraBuffer : register (b0, space0);

PSOut main(PSIn input) {

    // get distance between fragment and light source
    float lightDistance = length(input.worldPosition.xyz - lightBuffer.lightPosition.xyz);
    
    // map to [0;1] range by dividing by far_plane
    lightDistance = lightDistance / cameraBuffer.projectFarToNearDistance;
    
    // write this as modified depth
    float depth = lightDistance;

    PSOut output;
    output.depth = depth;
	return output;
}