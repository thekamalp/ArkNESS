// screen_vs.hlsl
// takes NES pixel coordinates and transforms to normalized viewport coordinates
// assumes a 640x480 resolution screen, and each NES pixel is written out as a 2x2
// renders 256x240 screen

float4 main(float2 pos : POSITION) : SV_POSITION
{
	float2 start = float2(-128.0, -120.0);
	float2 win_scale = float2(160.0, -120.0);
	float4 o_pos;
	o_pos.xy = (pos + start) / win_scale;
	o_pos.zw = float2(1.0, 1.0);
	return o_pos;
}

