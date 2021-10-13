struct PSIn {
    float4 position : SV_POSITION;                  
};

struct PSOut {
    uint color : SV_Target0;
};

PSOut main(PSIn input) {
    // float depth = input.position.z / input.position.w;
    PSOut output;
    output.color = 1;
    // output.depth = depth;
	return output;
}