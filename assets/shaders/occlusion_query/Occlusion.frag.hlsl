struct PSIn {
    float4 position : SV_POSITION;                  
};

struct PSOut {
    uint color : SV_Target0;
};

PSOut main(PSIn input) {
    PSOut output;
    output.color = 1;
    return output;
}