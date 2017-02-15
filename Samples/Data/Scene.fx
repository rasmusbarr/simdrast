
Texture2D DiffuseTexture : register(t0);
SamplerState DiffuseSampler : register(s0);
Texture2D ShadowTextures[4] : register(t1);
SamplerState ShadowSampler : register(s1);

cbuffer Constants : register(b0) {
    matrix ModelViewProj;
	matrix ShadowTransform[4];
	float4 Light[4];
	float3 View;
	int ShadowCount;
};

struct VS_INPUT {
    float4 Pos : POSITION;
	float3 Normal : NORMAL;
    float2 Tex : TEXCOORD0;
};

struct PS_INPUT {
    float4 Pos : SV_POSITION;
	float3 Normal : NORMAL;
    float2 Tex : TEXCOORD0;
	float4 WorldPos : TEXCOORD1;
};

PS_INPUT VS(VS_INPUT input) {
    PS_INPUT output = (PS_INPUT)0;
    output.Pos = mul(ModelViewProj, input.Pos);
	output.Pos.z = (output.Pos.z + output.Pos.w)*0.5f; // From GL-style z.
	output.Normal = input.Normal;
    output.Tex = input.Tex;
	output.WorldPos = input.Pos;
    return output;
}

float4 PS(PS_INPUT input) : SV_Target {
	float radiance = 0.0f;
	float3 n = normalize(input.Normal);

	for (int i = 0; i < ShadowCount; ++i) {
		float nDotL = max(dot(n, Light[i].xyz), 0.0f);

		float3 viewDir = normalize(View - input.WorldPos.xyz);
		float3 h = normalize(viewDir + Light[i].xyz);
		float specLighting = pow(max(dot(h, n), 0.0f), 32.0f)*0.5f;
		nDotL += specLighting;

		float4 shadow = mul(ShadowTransform[i], input.WorldPos);
		shadow.z = (shadow.z + shadow.w)*0.5f; // From GL-style z-clip.

		float width, height;
		unsigned int mipCount;
		ShadowTextures[i].GetDimensions(0, width, height, mipCount);

		float2 dim = float2(width, height);
		float2 dimInv = float2(1.0f/width, 1.0f/height);

		float2 coord = shadow.xy*0.5f + 0.5f;
		coord.y = 1.0f-coord.y;
		coord *= dim;
		coord -= 0.5f;

		float2 s = coord;
		coord = floor(coord);
		s -= coord;

		float ref = shadow.z - 1e-2f;

		float2 c00 = (coord + float2(0.0f, 0.0f) + 0.5f) * dimInv;
		float2 c01 = (coord + float2(0.0f, 1.0f) + 0.5f) * dimInv;
		float2 c10 = (coord + float2(1.0f, 0.0f) + 0.5f) * dimInv;
		float2 c11 = (coord + float2(1.0f, 1.0f) + 0.5f) * dimInv;

		float r00 = ShadowTextures[i].Sample(ShadowSampler, c00).x > ref;
		float r01 = ShadowTextures[i].Sample(ShadowSampler, c01).x > ref;
		float r10 = ShadowTextures[i].Sample(ShadowSampler, c10).x > ref;
		float r11 = ShadowTextures[i].Sample(ShadowSampler, c11).x > ref;

		r00 = lerp(r00, r10, s.x);
		r01 = lerp(r01, r11, s.x);
		r00 = lerp(r00, r01, s.y);

		nDotL *= (r00 + 1.0f)*0.5f;
		radiance += nDotL;
	}

	radiance *= 1.0f / ShadowCount;
	radiance += 0.25f;
	
	float4 c = float4(radiance, radiance, radiance, 1.0f);

#ifndef NO_DIFFUSE
    return c * DiffuseTexture.Sample(DiffuseSampler, input.Tex);
#else
	return c;
#endif
}
