#include "Common.hlsli"

float CalcShadowFactor(Texture2D sMap, float4 shadowPosH) {
	shadowPosH.xyz /= shadowPosH.w;
	shadowPosH.y = -shadowPosH.y;

	float2 shadowPosXY = (shadowPosH.xy *0.5 + 0.5);
	float depth = shadowPosH.z;

	uint width, height, numMips;
	sMap.GetDimensions(0, width, height, numMips);

	float dx = 1.0f / (float)width;

	float percentLit = 0.0f;
	const float2 offsets[9] = {
		float2(-dx,  -dx), float2(0.0f,  -dx), float2(dx,  -dx),
		float2(-dx, 0.0f), float2(0.0f, 0.0f), float2(dx, 0.0f),
		float2(-dx,  +dx), float2(0.0f,  +dx), float2(dx,  +dx)
	};

	[unroll]
	for (int i = 0; i < 9; ++i)
	{
		float lit = sMap.SampleCmpLevelZero(g_shadowComparisonSampler, shadowPosXY + offsets[i], depth);

		percentLit += lit;
	}

	percentLit /= 9.0f;

	return percentLit;
}

float CalcShadowFactor(TextureCube sMap, float4 hPos[6],float3 posW) {
	float3 lightVec = glights[START_POINT_LIGHTS].Position - posW;
	lightVec *= float3(1.0f, -1.0f, -1.0f);

	float3 lAbs = abs(lightVec);
	bool
		xy = lAbs.x > lAbs.y,
		xz = lAbs.x > lAbs.z,
		yz = lAbs.y > lAbs.z;
	
	uint lIndex = (!xy *  yz * 2) + (!xz * !yz * 4);
	const float tsign = (-sign(lightVec[lIndex/2])) + 1.0f;
	lIndex += uint(tsign) / 2;

	const float depth = hPos[lIndex].z / hPos[lIndex].w;
	float lit = sMap.SampleCmpLevelZero(g_shadowComparisonSampler, lightVec, depth);

	return lit;
}

float4 main(PSInput input) : SV_TARGET {
	float4 diffuseAlbedo = g_texture.Sample(g_sampler, input.uv) * gdiffuseAlbedo;

	clip(diffuseAlbedo.a - 0.1f);

	input.normalW = normalize(input.normalW);

	float3 normalMapSample = g_normal.Sample(g_sampler, input.uv).rgb;
	float3 bumpedNormalW = NormalSampleToWorldSpace(normalMapSample, input.normalW, input.tangentW);

	float3 toEyeW = normalize(geyePosition.xyz - input.positionW);

	float4 ambient = gambientLight * diffuseAlbedo;

	float specularVal = g_specular.Sample(g_sampler, input.uv).r;

	float shadowFactors[MaxLights];
	[unroll]
	for (int i = 0; i < MaxLights; ++i) {
		shadowFactors[i] = 1.0f;
	}

#ifdef RECEIVE_SHADOWS
	shadowFactors[START_DIR_LIGHTS] = CalcShadowFactor(g_directionalShadowMap, input.shadowDirectionalPosH);
	shadowFactors[START_POINT_LIGHTS] = CalcShadowFactor(g_pointShadowMap, input.shadowPointPosHs, input.positionW);
#endif

	const float shininess = max((1.0f - groughness) * specularVal, 0.00001f);
	Material mat = { diffuseAlbedo, gfresnelR0.xyz, shininess };
	float4 directLight = ComputeLighting(glights, mat, input.positionW, bumpedNormalW, toEyeW, shadowFactors);

	float4 litColor = ambient + directLight;

	litColor.a = diffuseAlbedo.a;

	return litColor;
}