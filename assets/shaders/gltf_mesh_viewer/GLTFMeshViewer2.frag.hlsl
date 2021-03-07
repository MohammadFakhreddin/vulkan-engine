struct PSIn {
    float4 position : SV_POSITION;
	float2 baseColorTexCoord : TEXCOORD0;
    float2 metallicRoughnessTexCoord : TEXCOORD1;
    float2 normalTexCoord: TEXCOORD2;
    float3 worldPos: POSITION0;
	float3 normal : NORMAL0;
	float4 tangent: TEXCOORD3;
};

struct PSOut{
    float4 color:SV_Target0;
};


sampler baseColorSampler : register(s1, space0);
Texture2D baseColorTexture : register(t1, space0);

sampler metallicRoughnessSampler : register(s2, space0);
Texture2D metallicRoughnessTexture : register(t2, space0);

sampler normalSampler : register(s3, space0);
Texture2D normalTexture : register(t3, space0);

struct LightViewBuffer {
    float3 lightPosition;
    float3 camPos;
};

ConstantBuffer <LightViewBuffer> lvBuff : register (b4, space0);

struct RotationBuffer {
	float4x4 rotation;
};

ConstantBuffer <RotationBuffer> rBuffer : register (b5, space0);

const float PI = 3.14159265359;

const float attenuationFactor = 200.0f;

const float3 lightColor = float3(252.0f/256.0f, 212.0f/256.0f, 64.0f/256.0f);
// float3 lightColor = float3(1.0, 1.0, 1.0);
	
// This function computes ratio between amount of light that reflect and refracts
// Reflection contrbutes to specular while refraction contributes to diffuse light
/*
    F0 parameter which is known as the surface reflection at zero incidence or how much the surface reflects 
    if looking directly at the surface. The F0 varies per material and is tinted on metals as we find in 
    large material databases. In the PBR metallic workflow we make the simplifying assumption that most dielectric surfaces 
    look visually correct with a constant F0 of 0.04, while we do specify F0 for metallic surfaces as then given by the albedo value. 
    This translates to code as follows:
*/
// Fresnel function ----------------------------------------------------
float3 F_Schlick(float cosTheta, float metallic, float3 baseColor)
{
	float3 F0 = lerp(float3(0.04, 0.04, 0.04), baseColor.rgb, metallic); // * material.specular
	float3 F = F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
	return F;
}

// Normal Distribution function --------------------------------------
float D_GGX(float dotNH, float roughness)
{
	float alpha = roughness * roughness;
	float alpha2 = alpha * alpha;
	float denom = dotNH * dotNH * (alpha2 - 1.0) + 1.0;
	return (alpha2)/(PI * denom*denom);
}

// Geometric Shadowing function --------------------------------------
float G_SchlicksmithGGX(float dotNL, float dotNV, float roughness)
{
	float r = (roughness + 1.0);
	float k = (r*r) / 8.0;
	float GL = dotNL / (dotNL * (1.0 - k) + k);
	float GV = dotNV / (dotNV * (1.0 - k) + k);
	return GL * GV;
}

// Specular BRDF composition --------------------------------------------

float3 BRDF(float3 L, float3 V, float3 N, float metallic, float roughness, float3 baseColor, float3 worldPos)
{
    // Precalculate vectors and dot products
	float3 H = normalize (V + L);
    float dotNV = clamp(dot(N, V), 0.0, 1.0);
	float dotNL = clamp(dot(N, L), 0.0, 1.0);
	float dotLH = clamp(dot(L, H), 0.0, 1.0);
	float dotNH = clamp(dot(N, H), 0.0, 1.0);

    float3 color = float3(0, 0, 0);

    float distance    = length(lvBuff.lightPosition - worldPos);
    float attenuation = attenuationFactor / (distance * distance);
    float3 radiance     = lightColor * attenuation;        
    
    // cook-torrance brdf
    float NDF = D_GGX(dotNH, roughness);        
    float G = G_SchlicksmithGGX(dotNL, dotNV, roughness);  
    float3 F = F_Schlick(dotNV, metallic, baseColor);;       
    
    float3 kS = F;
    float3 kD = float3(1.0) - kS;
    kD *= 1.0 - metallic;	  
    
    float3 numerator    = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0);
    float3 specular     = numerator / max(denominator, 0.001);  
        
    // add to outgoing radiance Lo
    float NdotL = max(dot(N, L), 0.0);                
    color += (kD * baseColor.rgb / PI + specular) * radiance * NdotL; 

	return color;
}

// TODO Use this to compute normal correctly
float3 calculateNormal(PSIn input)
{
	float3 tangentNormal = normalTexture.Sample(normalSampler, input.normalTexCoord).rgb * 2.0 - 1.0;

	float3 N = normalize(input.normal);
	float3 T = normalize(input.tangent);
	float3 B = normalize(cross(N, T));
	float3x3 TBN = transpose(float3x3(T, B, N));

	float3 modelNormal = mul(TBN, tangentNormal);
	float4 worldNormal = mul(rBuffer.rotation, float4(modelNormal, 0.0f));
	return normalize(worldNormal.xyz);
}

PSOut main(PSIn input) {
	float3 baseColor = pow(baseColorTexture.Sample(baseColorSampler, input.baseColorTexCoord).rgb, 2.2f);
    float4 metallicRoughness = metallicRoughnessTexture.Sample(metallicRoughnessSampler, input.metallicRoughnessTexCoord);
    float metallic = metallicRoughness.b;
    float roughness = max(metallicRoughness.g, 0.5);
	float3 normal = calculateNormal(input);
	// float4 worldNormal = normal;
	float3 N = normalize(normal.xyz);
	float3 V = normalize(lvBuff.camPos - input.worldPos);
    
    // Specular contribution
	float3 Lo = float3(0.0, 0.0, 0.0);
	for (int i = 0; i < 1; i++) {   // Light count
		float3 L = normalize(lvBuff.lightPosition.xyz - input.worldPos);
		Lo += BRDF(L, V, N, metallic, roughness, baseColor, input.worldPos);
	};

	// Combine with ambient
	float3 color = baseColor * 0.01; // TODO Add emissive color here
	color += Lo;

	// Gamma correct
    color = color / (color + float3(1.0f));
    color = pow(color, float3(1.0f/2.2f)); 

    PSOut output;
    output.color = float4(color, 1.0);
	return output;
}
