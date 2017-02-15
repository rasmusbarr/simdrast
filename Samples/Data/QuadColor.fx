
cbuffer Constants : register(b0) {
    float4 Color;
};

struct VS_INPUT {
    float4 Pos : POSITION;
    float2 Tex : TEXCOORD0;
};

struct PS_INPUT {
    float4 Pos : SV_POSITION;
};

PS_INPUT VS(VS_INPUT input) {
    PS_INPUT output = (PS_INPUT)0;
    output.Pos = input.Pos;
    return output;
}

float4 PS(PS_INPUT input) : SV_Target {
    return Color;
}
