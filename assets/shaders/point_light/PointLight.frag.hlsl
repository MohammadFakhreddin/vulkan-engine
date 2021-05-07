struct PSIn {
    float4 position : SV_POSITION;
};

struct PSOut {
    float4 color:SV_Target0;
};

struct SubMeshInfo {
    float4 baseColorFactor: COLOR0;
};

ConstantBuffer <SubMeshInfo> smBuff : register (b2, space0);

PSOut main(PSIn input) {
    PSOut output;
    output.color = smBuff.baseColorFactor;
    return output;
}