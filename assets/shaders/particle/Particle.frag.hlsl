struct PSIn {
    float4 position : SV_POSITION;
    float2 centerPosition;
    float pointRadius;
    int textureIndex;
    // float2 uv : TEXCOORD0;
    float3 color : COLOR0;
    float alpha : COLOR1;
};

struct PSOut {
    float4 color:SV_Target0;
};

#define MAX_POSSIBLE_TEXTURE 5

sampler baseColorSampler : register(s1, space0);
Texture2D baseColorTexture[MAX_POSSIBLE_TEXTURE] : register(t2, space1);

PSOut main(PSIn input) {
    PSOut output;

    float4 color;
    if (input.textureIndex >= 0) {
        float2 uv;
        uv.x = ((input.position.x - input.centerPosition.x) / input.pointRadius) * 0.5f + 0.5f;
        uv.y = ((input.position.y - input.centerPosition.y) / input.pointRadius) * 0.5f + 0.5f;
        color = baseColorTexture[input.textureIndex].Sample(baseColorSampler, uv).rgba;
    } else {
        color = float4(input.color, 1.0f);
    }

    float distanceAlpha = (length(input.position.xy - input.centerPosition.xy) / input.pointRadius);

    float4 finalColor = float4(
        color.rgb, 
        color.a * input.alpha * distanceAlpha
    );

    if (finalColor.a <= 0.0f) {
        discard;
    }

    output.color = finalColor;

    return output;
}