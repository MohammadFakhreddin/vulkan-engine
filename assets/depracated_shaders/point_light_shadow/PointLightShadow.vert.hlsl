#include "../CameraBuffer.hlsl"
#include "../SkinJointsBuffer.hlsl"

struct VSIn {
    float4 worldPosition;
};

struct VSOut {
    float4 worldPosition : POSITION0;
};

CAMERA_BUFFER(cameraBuffer)

VSOut main(VSIn input) {
    VSOut output;
    output.worldPosition = input.worldPosition;
    return output;
}