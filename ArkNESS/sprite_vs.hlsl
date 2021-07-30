// sprite_vs.hlsl
// takes NES pixel coordinates and transforms to normalized viewport coordinates
// assumes a 640x480 resolution screen, and each NES pixel is written out as a 2x2
// renders 256x240 screen
// also passes texture coordinates to allow flipping of sprite

cbuffer ppu {
	uint2 ppu;
};

cbuffer sprite {
	uint4 sprite[16];
};

struct VS_OUTPUT {
	float4 pos : SV_POSITION;
	float4 texcoord : TEXCOORD0;
};

VS_OUTPUT main( float2 pos : POSITION, uint instance : SV_InstanceID )
{
	VS_OUTPUT o;
	uint sprite_index = instance / 4;
	uint sprite_offset = instance & 0x3;
	uint sprite_data = sprite[sprite_index][sprite_offset];

	float2 sprite_start = float2((sprite_data >> 24) & 0xFF, (sprite_data & 0xFF) + 1);
	float2 start = float2(-128.0, -120.0) + sprite_start;
	float2 win_scale = float2(160.0, -120.0);
	o.pos.xy = ((pos + start) / win_scale);
	//if ((sprite_data & 0xFF) >= 0xef) o.pos.xy = float2(0.0, 0.0);
	o.pos.zw = float2(1.0, 1.0);
	o.texcoord.x = ((sprite_data >> 22) & 0x1) ? 8.0f - pos.x : pos.x;
	o.texcoord.y = ((sprite_data >> 23) & 0x1) ? 8.0f - pos.y : pos.y;
	o.texcoord.y = (ppu.x & 0x20) ? 2 * o.texcoord.y : o.texcoord.y;

	// get the palette index - upper 2 bits from sprite data structure
	uint palette_index = (sprite_data & 0x30000) >> 14;

	// get pattern index
	// first check if we're in 8x8 or 8x16 mode
	uint pattern_index = ((ppu.x << 3) & sprite_data & 0x100) | (~(ppu.x << 3) & (ppu.x << 5) & 0x100);
	//pattern_index = pattern_index| (sprite_data >> 8) & 0xFF;
	// use the second byte of sprite to get the rest of the index, masking off the bottom bit if we're 8x16 mode
	pattern_index = pattern_index | ((sprite_data >> 8) & (0xfe | (~(ppu.x >> 5) & 0x1)));

	o.texcoord.z = palette_index;
	o.texcoord.w = pattern_index;

	return o;
}