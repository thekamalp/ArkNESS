// exp_background_ps.hlsl
// Renders the background tiles
// this is custom shader for MMC5 when using 16K patterns

cbuffer nametable {
	uint4 nametable[4 * 64];
};

cbuffer exp_nametable {
	uint4 exp_nametable[4 * 64];
};

//cbuffer pattern {
//	uint4 pattern0[256 * 16];
//	uint4 pattern1[256 * 16];
//	uint4 pattern2[256 * 16];
//	uint4 pattern3[256 * 16];
//};

Texture2D <uint4> exp_pattern;

cbuffer palette {
	float4 palette[16];
};

struct VS_OUTPUT {
	float4 pos : SV_POSITION;
	float4 texcoord : TEXCOORD0;
};


float4 main(VS_OUTPUT i) : SV_TARGET
{
	float4 o_color;
	//uint2 n_pos = floor(0.5*(i_pos.xy - float2(64.5, 0.5)));
	float2 f_pos = i.texcoord.xy;
	f_pos.y += 256.0 * (i.texcoord.y < 0);
	uint2 n_pos = (uint2) floor(f_pos);
	uint max_y = (uint) floor(i.texcoord.z);
	n_pos.y += 16 * (n_pos.y >= max_y);
	uint screen_offset = ((n_pos.y & 0x100) >> 1) | ((n_pos.x & 0x100) >> 2);
	n_pos.x = n_pos.x & 0xFF;
	n_pos.y = n_pos.y & 0xFF;

	// get name table index and shift
	uint name_table_index = screen_offset | ((n_pos.y >> 2) & 0x3E) | ((n_pos.x >> 7) & 0x1);
	uint name_table_sub_index = (n_pos.x >> 5) & 0x3;
	uint name_table_shift = (n_pos.x & 0x18);

	// get pattern index
	uint4 name4 = nametable[name_table_index];
	uint4 exp_name4 = exp_nametable[name_table_index];

	uint attrib = (exp_name4[name_table_sub_index] >> name_table_shift) & 0xFF;
	uint3 pattern_coord;
	pattern_coord.x = (name4[name_table_sub_index] >> name_table_shift) & 0xFF;
	//pattern_coord.x = ((attrib & 0x3f) << 8) | pattern_coord.x;
	pattern_coord.y = attrib & 0x3f;
	pattern_coord.z = 0;
	attrib = (attrib >> 4) & 0x0c;

	uint4 pat4 = exp_pattern.Load(pattern_coord);

	// get the two planes, based on msb of y within the tile (bit 2)
	uint2 pat2 = (n_pos.y & 0x4) ? pat4.ga : pat4.rb;
	uint pattern_shift = ((n_pos.y & 0x3) << 3) | (0x7 - (n_pos.x & 0x7));

	// get palette index
	uint palette_index = ((pat2.r >> pattern_shift) & 0x1) | (((pat2.g >> pattern_shift) & 0x1) << 1) | attrib;

	o_color = palette[palette_index];

	return o_color;
}