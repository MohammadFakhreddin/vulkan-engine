#include "../CameraBuffer.hlsl"
#include "../SkinJointsBuffer.hlsl"

struct VSIn {
    float4 worldPosition;
};

struct VSOut {
    float4 worldPosition : POSITION0;
};

VSOut main(VSIn input) {
    VSOut output;
    // Position
    output.worldPosition = input.worldPosition;
    return output;
}