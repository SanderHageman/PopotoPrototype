struct PSInputTrim {
	float4 positionH : SV_POSITION;
	float2 uv : TEXCOORD;
};

Texture2D g_texture : register(t0);
SamplerState g_sampler : register(s0);

void main(PSInputTrim input) {
	float4 diffuseAlbedo = g_texture.Sample(g_sampler, input.uv);
	clip(diffuseAlbedo.a - 0.1f);
}