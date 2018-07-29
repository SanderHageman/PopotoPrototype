/*
	Directional:
		[0, 1]
	Point:
		[2]
	Spot:
		-
*/

#ifndef NUM_DIR_LIGHTS
	#define NUM_DIR_LIGHTS 2
	#define START_DIR_LIGHTS 0
#endif

#ifndef NUM_POINT_LIGHTS
	#define NUM_POINT_LIGHTS 1
	#define START_POINT_LIGHTS 2
#endif

#ifndef NUM_SPOT_LIGHTS
	#define NUM_SPOT_LIGHTS 0
	#define START_SPOT_LIGHTS 0
#endif

#include "DefaultLight.hlsli"

struct VSInput {
	float4 position : POSITION;
	float4 normal : NORMAL;
	float4 tangent : TANGENT;
	float2 uv : TEXCOORD;
};

struct PSInput {
	float4 positionH : SV_POSITION;
	float4 shadowDirectionalPosH : POSITION0;
	float4 shadowPointPosHs[6] : POSITION1;
	float3 positionW : POSITION7;
	float3 normalW: NORMAL;
	float3 tangentW: TANGENT;
	float2 uv : TEXCOORD;
};

cbuffer ObjectConstantBuffer : register(b0) {
	float4x4 worldMat;
};

cbuffer MaterialConstantBuffer : register(b1) {
	float4 gdiffuseAlbedo;
	float4 gfresnelR0;
	float groughness;
};

cbuffer LightPassConstantBuffer : register(b2) {
	float4x4 lightPassVP;
};

cbuffer MainPassConstantBuffer : register(b3) {
	float4 geyePosition;
	float4 gambientLight;
	float4x4 vpMat;
	float4x4 directionalLightVpMat;
	float4x4 pointLightVpMat[6];
	Light glights[MaxLights];
};

Texture2D g_texture : register(t0);
Texture2D g_normal : register(t1);
Texture2D g_specular : register(t2);
Texture2D g_directionalShadowMap : register(t3);
TextureCube g_pointShadowMap : register(t4);

SamplerState g_sampler : register(s0);
SamplerComparisonState g_shadowComparisonSampler : register(s1);

float3 NormalSampleToWorldSpace(float3 normalMapSample, float3 unitNormalW, float3 tangentW) {
	// Uncompress the components
	float3 normalT = 2.0f * normalMapSample - 1.0f;

	// Build orthonormal basis
	float3 N = unitNormalW;
	float3 T = normalize(tangentW - dot(tangentW, N) * N);
	float3 B = cross(N, T);

	float3x3 TBN = float3x3(T, B, N);

	// Transform from tangent into worldspace
	float3 bumpedNormalW = mul(normalT, TBN);
	return bumpedNormalW;
}