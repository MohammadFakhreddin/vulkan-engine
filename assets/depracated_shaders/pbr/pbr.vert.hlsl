struct VSIn {
    float3 LocalPosition: POSITION0;
    float3 LocalNormal: NORMAL0;
};

struct VSOut {
    float3 WorldPos: POSITION0;
    float3 WorldNormal: NORMAL0;
    float4 Position: SV_POSITION;
};

struct Transformation {
    float4x4 Rotation;
    float4x4 Transformation;
    float4x4 Projection;
};

ConstantBuffer <Transformation> tBuffer: register(b0, space0);

VSOut main(VSIn input) {
    VSOut output;

    output.WorldNormal = mul(tBuffer.Rotation, float4(input.LocalNormal, 1.0f)).xyz;

    float4 rotationResult = mul(tBuffer.Rotation, float4(input.LocalPosition, 1.0f));
    float4 WorldPos = mul(tBuffer.Transformation, rotationResult);

    output.WorldPos = WorldPos.xyz;

    output.Position = mul(tBuffer.Projection, WorldPos);

    return output;
}