struct PS_INPUT
{
	float4 Pos : SV_POSITION;
	float2 Uv : TEXCOORD0;
};

Texture2D tex : register(t0);
SamplerState Bilinear : register(s0);

float4 main(PS_INPUT input) : SV_TARGET
{
	float4 color = tex.Sample(Bilinear, input.Uv);
	return color;
}