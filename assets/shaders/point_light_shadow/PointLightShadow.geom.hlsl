struct VSOutput
{
    float4 position: SV_POSITION;
    float4 worldPosition : POSITION0;
};

struct GSOutput
{
    float4 position: SV_POSITION;
    float4 worldPosition : POSITION0;
	int Layer : SV_RenderTargetArrayIndex;
};

// Maybe we can have a separate file for this data type
struct PointLight
{
    float3 position;
    float placeholder0;
    float3 color;
    float maxSquareDistance;
    float linearAttenuation;
    float quadraticAttenuation;
    float2 placeholder1;     
    float4x4 viewProjectionMatrices[6];
};

#define MAX_POINT_LIGHT_COUNT 10

struct PointLightsBufferData
{
    uint count;
    float constantAttenuation;
    float2 placeholder;

    PointLight items [MAX_POINT_LIGHT_COUNT];                                       // Max light
};

ConstantBuffer <PointLightsBufferData> pointLightsBuffer: register(b1, space0);

struct PushConsts
{   
    float4x4 model;
    float4x4 inverseNodeTransform;
    int skinIndex;
    uint lightIndex;
    int placeholder0;
    int placeholder1;
};

[[vk::push_constant]]
cbuffer {
    PushConsts pushConsts;
};

[maxvertexcount(18)]                        // Number of generated vertices
void main(triangle VSOutput input[3], /*uint InvocationID : SV_GSInstanceID,*/ inout TriangleStream<GSOutput> outStream)
{
    // TODO Maybe we could do process for all lights here as well
    for(int faceIndex = 0; faceIndex < 6; ++faceIndex)
    {
        for(int i = 0; i < 3; ++i)          // for each triangle vertex
        {
            GSOutput output = (GSOutput)0;
            output.worldPosition = input[i].worldPosition;
            output.position = mul(pointLightsBuffer.items[pushConsts.lightIndex].viewProjectionMatrices[faceIndex], input[i].worldPosition);
            output.Layer = faceIndex;            // Specifies which face of cube we render on.
            outStream.Append(output);;      // Emit vertex
        }
        outStream.RestartStrip();           // End primitive
    }
}