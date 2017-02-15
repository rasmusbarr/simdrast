
Texture2D DiffuseTexture : register(t0);
SamplerState DiffuseSampler : register(s0);

struct VS_INPUT {
    float4 Pos : POSITION;
    float2 Tex : TEXCOORD0;
};

struct PS_INPUT {
    float4 Pos : SV_POSITION;
    float2 Tex : TEXCOORD0;
};

PS_INPUT VS(VS_INPUT input) {
    PS_INPUT output = (PS_INPUT)0;
    output.Pos = input.Pos;
    output.Tex = input.Tex;
    return output;
}

float4 PS(PS_INPUT input) : SV_Target {
    return DiffuseTexture.Sample(DiffuseSampler, input.Tex);
}
