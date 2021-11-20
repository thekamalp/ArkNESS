// fill_ps.hlsl
// Renders background color

cbuffer cbuf : register(b0) {
	uint4 ppu;
	uint4 sprite[16];
	uint4 pattern[512];
	float4 palette[32];
};

float4 main() : SV_TARGET
{
	return palette[0];
}