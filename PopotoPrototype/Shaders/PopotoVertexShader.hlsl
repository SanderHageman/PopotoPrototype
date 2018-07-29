#include "Common.hlsli"

float4 calculateHPos(float4x4 vpMat, float4x4 worldMat, float4 posL) {
	const float4x4 wvpMat = mul(vpMat, worldMat);
	return mul(posL, transpose(wvpMat));
}

PSInput main(VSInput input){
	PSInput result;

	float4 posW = mul(input.position, worldMat);
	result.positionW = posW.xyz;

	float4x4 wvpMat = mul(vpMat, worldMat);
	float4x4 transposedWVP = transpose(wvpMat);

	result.positionH = mul(input.position, transposedWVP);
	result.normalW = mul(input.normal, worldMat).xyz;
	result.tangentW = mul(input.tangent, worldMat).xyz;
	result.uv = input.uv;

	//float4x4 shadowDirectionalWvpMat = mul(directionalLightVpMat, worldMat);
	result.shadowDirectionalPosH = calculateHPos(directionalLightVpMat, worldMat, input.position);// mul(input.position, transpose(shadowDirectionalWvpMat));

	////float4x4 shadowPointWvpMat = mul(pointLightVpMat, worldMat);
	////result.shadowPointPosH = mul(input.position, transpose(shadowPointWvpMat));

	[unroll]
	for (int i = 0; i < 6; ++i) {
		result.shadowPointPosHs[i] = calculateHPos(pointLightVpMat[i], worldMat, input.position);
	}

	return result;
}