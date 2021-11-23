// copy_vs.hlsl
// copies render surface to back buffer

struct VS_OUTPUT {
	float4 pos : SV_POSITION;
	float2 texcoord : TEXCOORD0;
};


VS_OUTPUT main(float2 pos : POSITION)
{
	VS_OUTPUT o;
	float2 start = float2(-128.0, -120.0);
	float2 win_scale = float2(128.0, -120.0);
	o.pos.xy = (pos + start) / win_scale;
	o.pos.zw = float2(0.5, 1.0);
	win_scale.y = -win_scale.y;
	o.texcoord = 0.5* pos / win_scale;
	return o;
}
