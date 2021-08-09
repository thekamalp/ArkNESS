// screen_vs.hlsl
// takes NES pixel coordinates and transforms to normalized viewport coordinates
// assumes a 640x480 resolution screen, and each NES pixel is written out as a 2x2
// renders 256x240 screen

cbuffer ppu {
	uint4 ppu;
};

struct VS_OUTPUT {
	float4 pos : SV_POSITION;
	float4 texcoord : TEXCOORD0;
};


VS_OUTPUT main(float2 pos : POSITION)
{
	VS_OUTPUT o;
	float2 scroll = uint2((ppu.z >> 16) & 0xFF, (ppu.z >> 24) & 0xFF);
	uint max_y = 240 + 256 * (scroll.y >= 240) + ((ppu.x & 0x2) << 7);
	scroll.x = scroll.x + ((ppu.x & 0x1) << 8);
	scroll.y = scroll.y + ((ppu.x & 0x2) << 7);
	float2 start = float2(-128.0, -120.0);
	float2 win_scale = float2(160.0, -120.0);
	o.pos.xy = (pos + start) / win_scale;
	o.pos.zw = float2(0.5, 1.0);
	o.texcoord.xy = scroll + pos;
	o.texcoord.z = max_y;
	o.texcoord.w = 1.0;
	return o;
}

