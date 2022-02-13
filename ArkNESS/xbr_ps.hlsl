// xbr_ps.hlsl
// Upscale image using a modified xBR algorithm
// This shader upscales by arbitrary size, not just fixed
// multiples of the original resolution

#pragma pack_matrix( row_major )

struct VS_OUTPUT {
	float4 pos : SV_POSITION;
	float2 texcoord : TEXCOORD0;
};

cbuffer cbuf : register(b0) {
	float4x4 color_xform;
};

Texture2D <float4> tex : register(t0);

SamplerState sampleNearest : register(s0);

static const float3x3 yuv_xform = float3x3(0.299, 0.587, 0.144,
										  -0.169, -0.331, 0.5,
										   0.5, -0.419, -0.081);

static const float2 screen = float2(256.0, 240.0);

float dist(float4 a, float4 b)
{
	float3 rgb_delta;
	rgb_delta.r = abs(a.r - b.r);
	rgb_delta.g = abs(a.g - b.g);
	rgb_delta.b = abs(a.b - b.b);

	float3 yuv_delta = mul(yuv_xform, rgb_delta);
	return 48.0 * yuv_delta.r + 7.0 * yuv_delta.g + 6 * yuv_delta.b;
}

float4 edge_detect(float4 E, float4 B, float4 D, float4 C, float4 G, float4 F, float4 H,
	float4 F4, float4 I, float4 H5, float4 I4, float4 I5)
{
	float4 new_edge_color = (dist(E, F) <= dist(E, H)) ? F : H;

	float wd_red = dist(E, C) + dist(E, G) + dist(I, F4) + dist(I, H5) + 4.0 * dist(H, F);
	float wd_blue = dist(H, D) + dist(H, I5) + dist(F, I4) + dist(F, B) + 4.0 * dist(E, I);
	return (wd_red < wd_blue) ? new_edge_color : E;
}

float edge_weight(float2 norm_pix_pt, float2 norm_pix_dist, float2 scale)
{
	// only doing level 1 weight for now
	float2 corner = norm_pix_pt;
	float l;
	l = dot(norm_pix_pt, scale);
	if (l < 0.5) {
		// compute lower right corner
		corner += norm_pix_dist;
		l = dot(corner, scale);
		if (l < 0.5) {
			// corner is fully inside edge
			return 1.0;
		} else {
			//float w = 0.5 - norm_pix_pt.x - norm_pix_pt.y;
			//w = w / (norm_pix_dist.x + norm_pix_dist.y);
			//w = clamp(w, 0.0, 1.0);
			//w = w * w;
			//w = 0.5 + 0.5 * w;
			return 0.5;// 1.0 - w;
		}
		//return 1.0;
	} else {
		// compute upper left corner
		corner -= norm_pix_pt;
		l = dot(corner, scale);
		if (l < 0.5) {
			//float w = norm_pix_pt.x + norm_pix_pt.y - 0.5;
			//w = w / (norm_pix_dist.x + norm_pix_dist.y);
			//w = clamp(w, 0.0, 1.0);
			//w = w * w;
			//w = 0.5 + 0.5 * w;
			return 0.5;// w;
		} else {
			// corner fully outside edge
			return 0.0;
		}
		//return 0.0;
	}
//	return 0.5;
}

float4 main(VS_OUTPUT inp) : SV_TARGET
{
	float2 screen_point = inp.texcoord * screen;
	float2 pixel_point = frac(screen_point);
	int3 tex_point = int3(screen_point, 0);
	float2 pix_dist = 1.0 / screen;
	float2 norm_pix_dist;
	norm_pix_dist.x = ddx(inp.texcoord.x);
	norm_pix_dist.y = ddy(inp.texcoord.y);
	norm_pix_dist *= screen / 2.0;
	int3 right_vec, down_vec;
	float2 norm_pix_pt;
	if (pixel_point.x >= 0.5) {
		norm_pix_pt.x = 1.0 - pixel_point.x;
		if (pixel_point.y >= 0.5) {
			norm_pix_pt.y = 1.0 - pixel_point.y;
			right_vec = int3(1, 0, 0);
			down_vec = int3(0, 1, 0);
			//right_vec = float2(pix_dist.x, 0.0);
			//down_vec = float2(0.0, pix_dist.y);
		} else {
			norm_pix_pt.y = pixel_point.y;
			right_vec = int3(0, -1, 0);
			down_vec = int3(1, 0, 0);
			//right_vec = float2(0.0, -pix_dist.y);
			//down_vec = float2(pix_dist.x, 0.0);
		}
	} else {
		norm_pix_pt.x = pixel_point.x;
		if (pixel_point.y >= 0.5) {
			norm_pix_pt.y = 1.0 - pixel_point.y;
			right_vec = int3(0, 1, 0);
			down_vec = int3(-1, 0, 0);
			//right_vec = float2(0.0, pix_dist.y);
			//down_vec = float2(-pix_dist.x, 0.0);
		} else {
			norm_pix_pt.y = pixel_point.y;
			right_vec = int3(-1, 0, 0);
			down_vec = int3(0, -1, 0);
			//right_vec = float2(-pix_dist.x, 0.0);
			//down_vec = float2(0.0, -pix_dist.y);
		}
	}

	//float4 E = tex.Sample(sampleNearest, inp.texcoord);
	//float4 B = tex.Sample(sampleNearest, inp.texcoord - down_vec);
	//float4 D = tex.Sample(sampleNearest, inp.texcoord - right_vec);
	//float4 C = tex.Sample(sampleNearest, inp.texcoord + right_vec - down_vec);
	//float4 G = tex.Sample(sampleNearest, inp.texcoord - right_vec + down_vec);
	//float4 F = tex.Sample(sampleNearest, inp.texcoord + right_vec);
	//float4 H = tex.Sample(sampleNearest, inp.texcoord + down_vec);
	//float4 F4 = tex.Sample(sampleNearest, inp.texcoord + 2.0*right_vec);
	//float4 I = tex.Sample(sampleNearest, inp.texcoord + right_vec + down_vec);
	//float4 H5 = tex.Sample(sampleNearest, inp.texcoord + 2.0*down_vec);
	//float4 I4 = tex.Sample(sampleNearest, inp.texcoord + 2.0 * right_vec + down_vec);
	//float4 I5 = tex.Sample(sampleNearest, inp.texcoord + right_vec + 2.0 * down_vec);

	float4 E = tex.Load(tex_point);
	float4 B = tex.Load(tex_point - down_vec);
	float4 D = tex.Load(tex_point - right_vec);
	float4 C = tex.Load(tex_point + right_vec - down_vec);
	float4 G = tex.Load(tex_point - right_vec + down_vec);
	float4 F = tex.Load(tex_point + right_vec);
	float4 H = tex.Load(tex_point + down_vec);
	float4 F4 = tex.Load(tex_point + 2 * right_vec);
	float4 I = tex.Load(tex_point + right_vec + down_vec);
	float4 H5 = tex.Load(tex_point + 2 * down_vec);
	float4 I4 = tex.Load(tex_point + 2 * right_vec + down_vec);
	float4 I5 = tex.Load(tex_point + right_vec + 2 * down_vec);

	float4 edge_color = edge_detect(E, B, D, C, G, F, H, F4, I, H5, I4, I5);

	float2 scale = float2(1.0, 1.0);
	//scale.x = (F == G) ? 0.5 : 1.0;
	//scale.y = (C == H) ? 0.5 : 1.0;
	float weight = edge_weight(norm_pix_pt, norm_pix_dist, scale);

	//uint3 coord = uint3(inp.pos.xy, 0);
	//return tex.Load(coord);
	float4 color = weight* edge_color + (1.0 - weight) * E;
	float4 xformed_color = mul(color_xform, color);
	//xformed_color = (weight == 0.5 && E != edge_color) ? float4(1.0, 0.0, 0.0, 1.0) : xformed_color;
	return xformed_color;
}
