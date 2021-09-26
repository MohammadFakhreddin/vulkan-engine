struct PSIn {
    float4 position : SV_POSITION;                  
};

struct PSOut {
    float depth : SV_DEPTH;
};

PSOut main(PSIn input) {
    PSOut output;
    output.depth = input.position.z;
	return output;
}