// fill_ps.hlsl
// Renders background color

cbuffer palette {
	float4 palette[16];
};

float4 main() : SV_TARGET
{
	return palette[0];
}