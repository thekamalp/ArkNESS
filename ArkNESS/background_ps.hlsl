// background_ps.hlsl
// Renders the background tiles

cbuffer nametable {
	uint4 nametable[4 * 64];
};

cbuffer pattern {
	uint4 pattern[256];
};

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

	// get attrib index and shift (bit index with 32b)
	uint attrib_index = screen_offset | (60 + (n_pos.y >> 6));
	uint attrib_sub_index = ((n_pos.y >> 4) & 0x2) | ((n_pos.x >> 7) & 0x1);
	uint attrib_shift = (((n_pos.x & 0x10) >> 3) | ((n_pos.y & 0x10) >> 2) | ((n_pos.x & 0x60) >> 2));

	// get name table index and shift
	uint name_table_index = screen_offset | ((n_pos.y >> 2) & 0x3E) | ((n_pos.x >> 7) & 0x1);
	uint name_table_sub_index = (n_pos.x >> 5) & 0x3;
	uint name_table_shift = (n_pos.x & 0x18);

	// get pattern index
	uint4 name4 = nametable[name_table_index];
//		(name_table_sel == 0) ? nametable0[name_table_index] :
//		(name_table_sel == 1) ? nametable1[name_table_index] :
//		(name_table_sel == 2) ? nametable2[name_table_index] : nametable3[name_table_index];
	uint4 attrib4 = nametable[attrib_index];
//		(name_table_sel == 0) ? attribtable0[attrib_index] :
//		(name_table_sel == 1) ? attribtable1[attrib_index] :
//		(name_table_sel == 2) ? attribtable2[attrib_index] : attribtable3[attrib_index];
	uint pattern_index = (name4[name_table_sub_index] >> name_table_shift) & 0xFF;
	uint4 pat4 = pattern[pattern_index];
	// get the two planes, based on msb of y within the tile (bit 2)
	uint2 pat2 = (n_pos.y & 0x4) ? pat4.ga : pat4.rb;
	uint pattern_shift = ((n_pos.y & 0x3) << 3) | (0x7 - (n_pos.x & 0x7));

	// get palette index
	uint palette_index = ((pat2.r >> pattern_shift) & 0x1) | (((pat2.g >> pattern_shift) & 0x1) << 1) | (((attrib4[attrib_sub_index] >> attrib_shift) & 0x3) << 2);

	o_color = palette[palette_index];

	return o_color;
}