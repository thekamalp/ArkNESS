// sprite_ps.hlsl
// Renders a sprite

cbuffer cbuf : register(b0) {
	uint4 ppu;
	uint4 sprite[16];
	uint4 pattern[512];
	float4 palette[32];
};

struct VS_OUTPUT {
	float4 pos : SV_POSITION;
	float4 texcoord : TEXCOORD0;
	bool frontface : SV_IsFrontFace;
};

float4 main(/*float4 i_pos : SV_Position*/ VS_OUTPUT i ) : SV_TARGET
{
	float4 o_color;
	if(i.frontface) {
		uint2 n_pos = (uint2) floor(i.texcoord);

		uint palette_index = i.texcoord.z;
		uint pattern_index = i.texcoord.w;

		// if we're 8x16 mode, use bit 3 of y for lsb
		// note that bit 3 may only be set when in 8x16 mode, so we don't have to check the ppu register state
		pattern_index = pattern_index | (0x1 & (n_pos.y >> 3));

		// get pattern
		uint4 pat4 = pattern[pattern_index];
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