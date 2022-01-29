// Based on https://learnopengl.com/PBR/Lighting

struct PSIn {
    float3 WorldPos: POSITION0;
    float3 WorldNormal: NORMAL0;
    float4 Position: SV_POSITION;
};

struct PSOut {
    float4 FragColor: SV_Target0;
};

struct MaterialBuffer {
    float3 albedo;          // BaseColor
    float metallic;
    float roughness;
    // float ao;               // Emission  
};

ConstantBuffer <MaterialBuffer> matBuff : register (b1, space0);

struct LightViewBuffer {
    float3 camPos;
    int lightCount;
    float3 lightPositions[1];
    // float3 lightColors[1];
};

ConstantBuffer <LightViewBuffer> lvBuff : register (b2, space0);

const float PI = 3.14159265359;

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
float3 F_Schlick(float cosTheta, float metallic)
{
	float3 F0 = lerp(float3(0.04, 0.04, 0.04), matBuff.albedo, metallic); // * material.specular
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

float3 BRDF(float3 L, float3 V, float3 N, float metallic, float roughness)
{
	// Precalculate vectors and dot products
	float3 H = normalize (V + L);
	float dotNV = clamp(dot(N, V), 0.0, 1.0);
	float dotNL = clamp(dot(N, L), 0.0, 1.0);
	float dotLH = clamp(dot(L, H), 0.0, 1.0);
	float dotNH = clamp(dot(N, H), 0.0, 1.0);

	// Light color fixed
	float3 lightColor = float3(1.0, 1.0, 1.0);

	float3 color = float3(0.0, 0.0, 0.0);

	if (dotNL > 0.0)
	{
		// float rroughness = max(0.05, roughness);
		// D = Normal distribution (Distribution of the microfacets)
		float D = D_GGX(dotNH, roughness);
		// G = Geometric shadowing term (Microfacets shadowing)
		float G = G_SchlicksmithGGX(dotNL, dotNV, roughness);
		// F = Fresnel factor (Reflectance depending on angle of incidence)
		float3 F = F_Schlick(dotNV, metallic);

		float3 spec = D * F * G / (4.0 * dotNL * dotNV);

		color += spec * dotNL * lightColor;
	}

	return color;
}

PSOut main(PSIn input) {

	// PSOut output2;
    // output2.FragColor = float4(input.WorldNormal.xyz, 1.0f);
    // return output2;

	float3 N = normalize(input.WorldNormal);
	float3 V = normalize(lvBuff.camPos - input.WorldPos);

	float roughness = matBuff.roughness;

	// Specular contribution
	float3 Lo = float3(0.0, 0.0, 0.0);
	for (int i = 0; i < lvBuff.lightCount; i++) {
		float3 L = normalize(lvBuff.lightPositions[i].xyz - input.WorldPos);
		Lo += BRDF(L, V, N, matBuff.metallic, roughness);
	};

	// Combine with ambient
	float3 color = matBuff.albedo * 0.02;
	color += Lo;

	// Gamma correct
	color = pow(color, float3(0.4545, 0.4545, 0.4545));

    PSOut output;
    output.FragColor = float4(color, 1.0);
	return output;
};