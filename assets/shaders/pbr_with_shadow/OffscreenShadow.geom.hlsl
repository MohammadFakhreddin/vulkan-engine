struct VSInput {
    float4 position : SV_POSITION;          // Refers to world position
};

struct VSOutput
{
	float4 position : SV_POSITION;
};

struct GSOutput
{
	float4 FragPos : SV_POSITION;
	int Layer : SV_RenderTargetArrayIndex;
};

struct ShadowMatricesBuffer {
    float4x4 viewMatrices[6];
};

ConstantBuffer <ShadowMatricesBuffer> smBuffer : register (b3, space0);

[maxvertexcount(18)]        // Number of generated vertices
void main(triangle VSOutput input[3], uint InvocationID : SV_GSInstanceID, inout TriangleStream<GSOutput> outStream)
{
    for(int face = 0; face < 6; ++face)
    {
        for(int i = 0; i < 3; ++i)      // for each triangle vertex
        {
            GSOutput output = (GSOutput)0;
            output.FragPos = mul(smBuffer.viewMatrices[face], input[i].position);
            output.Layer = face;        // Specifies which face of cube we render on.
            outStream.Append(output);;
        }    
        outStream.RestartStrip();
    }
}