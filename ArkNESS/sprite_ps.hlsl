// sprite_ps.hlsl
// Renders a sprite

cbuffer pattern {
	uint4 pattern[512];
};

cbuffer palette {
	float4 palette[16];
};

//cbuffer ppu {
//	uint2 ppu;
//};
//
//cbuffer sprite {
//	uint sprite;
//};

struct VS_OUTPUT {
	float4 pos : SV_POSITION;
	float4 texcoord : TEXCOORD0;
};

float4 main(/*float4 i_pos : SV_Position*/ VS_OUTPUT i ) : SV_TARGET
{
	float4 o_color;
	uint2 n_pos = (uint2) floor(i.texcoord); //  floor(0.5 * (i_pos.xy - float2(64.5, 0.5))); //floor(texcoord.xy);
	//n_pos.x = n_pos.x - ((sprite >> 24) & 0xFF);
	//n_pos.y = n_pos.y - (sprite & 0xFF);

	//// get the palette index - upper 2 bits from sprite data structure
	//uint palette_index = (sprite & 0x30000) >> 14;
	//
	// get pattern index
	// first check if we're in 8x8 or 8x16 mode
	//uint pattern_index = ((ppu.x << 3) & sprite & 0x100) | (~(ppu.x << 3) & (ppu.x << 5) & 0x100);
	////pattern_index = pattern_index| (sprite >> 8) & 0xFF;
	//// use the second byte of sprite to get the rest of the index, masking off the bottom bit if we're 8x16 mode
	//pattern_index = pattern_index | ((sprite >> 8) & (0xfe | (~(ppu.x >> 5) & 0x1)));
	//// if we're 8x16 mode, use bit 3 of y for lsb
	//pattern_index = pattern_index | ((ppu.x >> 5) & 0x1 & (n_pos.y >> 3));

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

	return o_color;
}