struct PSIn {
    float4 position : SV_POSITION;                  // Refers to world position
};

struct PSOut {
    // float color : SV_Target0;
    float depth : SV_Depth;
};

struct LightBuffer {
    float4 lightPosition;
    float4 projectionMaximumDistance;       // abs(Far - near)
};

ConstantBuffer <LightBuffer> lBuffer : register (b4, space0);

PSOut main(PSIn input) {

    // get distance between fragment and light source
    float lightDistance = length(input.position.xyz - lBuffer.lightPosition.xyz);
    
    // map to [0;1] range by dividing by far_plane
    lightDistance = lightDistance / lBuffer.projectionMaximumDistance;
    
    // write this as modified depth
    float depth = lightDistance;

    PSOut output;
    output.depth = depth;
	return output;
}