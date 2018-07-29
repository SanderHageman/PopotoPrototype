#define MaxLights 16

struct Light {
	float3 Strength;
	float FalloffStart;
	float3 Direction;
	float FalloffEnd;
	float3 Position;
	float SpotPower;
};

struct Material {
	float4 DiffuseAlbedo;
	float3 FresnelR0;
	float Shininess;
};

float CalcAttenuation(float d, float falloffStart, float falloffEnd) {
	return saturate((falloffEnd - d) / (falloffEnd - falloffStart));
}

float3 SchlickFresnel(float3 R0, float3 normal, float3 lightVec) {
	float cosIncidentAngle = saturate(dot(normal, lightVec));
	float f0 = 1.0f - cosIncidentAngle;
	return R0 + (1.0f - R0)*pow(f0, 5);
}

float3 BlinnPhong(float3 lightStrength, float3 lightVec, float3 normal, float3 toEye, Material mat) {
	const float m = mat.Shininess * 256.0f;
	float3 halfVec = normalize(toEye + lightVec);

	float roughnessFactor = 
		(m + 8.0f) *
		pow(
			saturate(	dot(halfVec, normal)), 
			m)
		/ 8.0f;

	float3 fresnelFactor = SchlickFresnel(mat.FresnelR0, halfVec, lightVec);

	float3 specAlbedo = fresnelFactor * roughnessFactor;

	specAlbedo = specAlbedo / (specAlbedo + 1.0f);

	return (mat.DiffuseAlbedo.rgb + specAlbedo) * lightStrength;
}

float3 ComputeDirectionalLight(Light L, Material mat, float3 normal, float3 toEye) {
	float3 lightVec = -L.Direction;

	float ndotl = max(dot(lightVec, normal), 0.0f);
	float3 lightStrength = L.Strength * ndotl;

	return BlinnPhong(lightStrength, lightVec, normal, toEye, mat);
}

float3 ComputeSpotLight(Light L, Material mat, float3 pos, float3 normal, float3 toEye) {
	float3 lightVec = L.Position - pos;
	float d = length(lightVec);
	if (d > L.FalloffEnd)
		return 0.0f;

	lightVec /= d;

	float ndotl = max(dot(lightVec, normal), 0.0f);
	float3 lightStrength = L.Strength * ndotl;

	float att = CalcAttenuation(d, L.FalloffStart, L.FalloffEnd);
	lightStrength *= att;

	float spotFactor = pow(max(dot(-lightVec, L.Direction), 0.0f), L.SpotPower);
	lightStrength *= spotFactor;

	return BlinnPhong(lightStrength, lightVec, normal, toEye, mat);
}

float3 ComputePointLight(Light L, Material mat, float3 pos, float3 normal, float3 toEye) {
	float3 lightVec = L.Position - pos;
	float distance = length(lightVec);

	// If the distance is further than the light could possibly shine, we return early
	if (distance > L.FalloffEnd) {
		return 0.0f;
	}

	lightVec /= distance;
	
	float ndotl = saturate(
		dot(lightVec, normal));

	float att = CalcAttenuation(
		distance, 
		L.FalloffStart, 
		L.FalloffEnd);

	float3 lightStrength = L.Strength * ndotl * att;

	return BlinnPhong(lightStrength, lightVec, normal, toEye, mat);
}

float4 ComputeLighting(Light gLights[MaxLights], Material mat, float3 pos, float3 normal, float3 toEye, float shadowFactors[MaxLights]) {
	float3 result = 0.0f;

	const int dirlightStart = START_DIR_LIGHTS;
	const int dirlightEnd = dirlightStart + NUM_DIR_LIGHTS;

	const int pointlightStart = START_POINT_LIGHTS;
	const int pointlightEnd = pointlightStart + NUM_POINT_LIGHTS;

	const int spotlightStart = START_SPOT_LIGHTS;
	const int spotlightEnd = spotlightStart + NUM_SPOT_LIGHTS;

#if (NUM_DIR_LIGHTS > 0)
	for (int i = dirlightStart; i < dirlightEnd; ++i) {
		result += shadowFactors[i] * ComputeDirectionalLight(gLights[i], mat, normal, toEye);
	}
#endif

#if (NUM_POINT_LIGHTS > 0)
	for (int j = pointlightStart; j < pointlightEnd; ++j) {
		result += shadowFactors[j] * ComputePointLight(gLights[j], mat, pos, normal, toEye);
	}
#endif

#if (NUM_SPOT_LIGHTS > 0)
	for (int k = spotlightStart; k < spotlightEnd + NUM_SPOT_LIGHTS; ++k) {
		result += shadowFactors[k] * ComputeSpotLight(gLights[k], mat, pos, normal, toEye);
	}
#endif

	return float4(result, 0.0f);
}