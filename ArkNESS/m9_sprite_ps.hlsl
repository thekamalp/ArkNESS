// m9_sprite_ps.hlsl
// Renders the sprite tiles for mapper9
// This mapper has an extra pattern table for background and sprites

cbuffer cbuf : register(b0) {
	uint4 ppu;
	uint4 sprite[16];
	uint4 pattern[512];
	float4 palette[32];
	uint4 nametable[256];
	uint4 alt_pattern[512];
	uint4 nametable_msb[32];
	uint4 sprite_msb[2 * 240];
};

struct VS_OUTPUT {
	float4 pos : SV_POSITION;
	float4 texcoord : TEXCOORD0;
	bool frontface : SV_IsFrontFace;
};

float4 main(/*float4 i_pos : SV_Position*/ VS_OUTPUT i) : SV_TARGET
{
	float4 o_color;
	if (i.frontface) {
		uint2 n_pos = (uint2) floor(i.texcoord);
		uint2 s_pos = (uint2) floor(i.pos.xy);

		uint palette_index = i.texcoord.z;
		uint pattern_index = i.texcoord.w;

		// if we're 8x16 mode, use bit 3 of y for lsb
		// note that bit 3 may only be set when in 8x16 mode, so we don't have to check the ppu register state
		pattern_index = pattern_index | (0x1 & (n_pos.y >> 3));

		// determine if we need to use the alternative pattern
		uint sprite_msb_index = (s_pos.y << 1) | ((s_pos.x >> 7) & 0x1);
		uint sprite_msb_sub_index = ((s_pos.x >> 5) & 0x3);
		uint sprite_msb_shift = (s_pos.x & 0x1f);
		uint4 sprite_msb4 = sprite_msb[sprite_msb_index];
		uint use_alt_pattern = (sprite_msb4[sprite_msb_sub_index] >> sprite_msb_shift) & 0x1;

		// get pattern
		uint4 pat4_old = pattern[pattern_index];
		uint4 pat4_alt = alt_pattern[pattern_index];
		uint4 pat4 = (use_alt_pattern != 0) ? pat4_alt : pat4_old;

		// get the two planes, based on msb of y within the tile (bit 2)
		uint2 pat2 = (n_pos.y & 0x4) ? pat4.ga : pat4.rb;
		uint pattern_shift = ((n_pos.y & 0x3) << 3) | (0x7 - (n_pos.x & 0x7));

		// get palette index
		palette_index = palette_index | ((pat2.r >> pattern_shift) & 0x1) | (((pat2.g >> pattern_shift) & 0x1) << 1);

		o_color = palette[palette_index];
	} else {
		o_color = float4(0.0, 0.0, 0.0, 1.0);
	}

	return o_color;
}