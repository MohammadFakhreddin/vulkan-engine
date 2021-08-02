struct PSIn {
    float4 position : SV_POSITION;                  
    float4 worldPosition: POSITION0;
};

struct PSOut {
    // float color : SV_Target0;
    float depth : SV_DEPTH;
};

struct LightBuffer {
    float4 lightPosition;
    float projectionMaximumDistance;       // abs(Far - near)
};

ConstantBuffer <LightBuffer> lBuffer : register (b4, space0);

PSOut main(PSIn input) {

    // get distance between fragment and light source
    float lightDistance = length(input.worldPosition.xyz - lBuffer.lightPosition.xyz);
    
    // map to [0;1] range by dividing by far_plane
    lightDistance = lightDistance / lBuffer.projectionMaximumDistance;
    
    // write this as modified depth
    float depth = lightDistance;

    PSOut output;
    output.depth = depth;
	return output;

    // PSOut output;
    // output.color = 0.0f;
	// return output;
}