// copy_ps.hlsl
// Copies from render surfac to back buffer

struct VS_OUTPUT {
	float4 pos : SV_POSITION;
	float2 texcoord : TEXCOORD0;
};

Texture2D <float4> tex : register(t0);

SamplerState sampleLinear : register(s0);

float4 main(VS_OUTPUT inp) : SV_TARGET
{
	//uint3 coord = uint3(inp.pos.xy, 0);
	//return tex.Load(coord);
	return tex.Sample(sampleLinear, inp.texcoord);
}