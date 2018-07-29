#include "Common.hlsli"

struct VSInputTrim {
	float4 position : POSITION;
	float2 uv : TEXCOORD;
};

struct PSInputTrim {
	float4 positionH : SV_POSITION;
	float2 uv : TEXCOORD;
};

PSInputTrim main( VSInputTrim input ) {
	PSInputTrim result;

	float4x4 wvpMat = mul(lightPassVP, worldMat);

	float4x4 transposedWVP = transpose(wvpMat);

	result.positionH = mul(input.position, transposedWVP);
	result.uv = input.uv;

	return result;
}