struct PSIn {
    float4 position : SV_POSITION;                  
};

struct PSOut {
    float depth : SV_DEPTH;
};

PSOut main(PSIn input) {
    float depth = input.position.z / input.position.w;
    PSOut output;
    output.depth = depth;
	return output;
}