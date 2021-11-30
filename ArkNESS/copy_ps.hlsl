// copy_ps.hlsl
// Copies from render surfac to back buffer

#pragma pack_matrix( row_major )

struct VS_OUTPUT {
	float4 pos : SV_POSITION;
	float2 texcoord : TEXCOORD0;
};

cbuffer cbuf : register(b0) {
	float4x4 color_xform;
};

Texture2D <float4> tex : register(t0);

SamplerState sampleLinear : register(s0);

float4 main(VS_OUTPUT inp) : SV_TARGET
{
	//uint3 coord = uint3(inp.pos.xy, 0);
	//return tex.Load(coord);
	float4 color = tex.Sample(sampleLinear, inp.texcoord);
	float4 xformed_color = mul(color_xform, color);
	return xformed_color;
}
