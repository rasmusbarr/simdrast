
cbuffer Constants : register(b0) {
    matrix ModelViewProj;
};

struct VS_INPUT {
    float4 Pos : POSITION;
};

struct PS_INPUT {
    float4 Pos : SV_POSITION;
};

PS_INPUT VS(VS_INPUT input) {
    PS_INPUT output = (PS_INPUT)0;
    output.Pos = mul(ModelViewProj, input.Pos);
	output.Pos.z = (output.Pos.z + output.Pos.w)*0.5f; // From GL-style z-clip.
    return output;
}

float4 PS(PS_INPUT input) : SV_Target {
	float z = input.Pos.z;
    return float4(z, z, z, z);
}
