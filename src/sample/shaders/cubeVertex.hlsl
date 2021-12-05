struct View
{
	float4x4 model;
	float4x4 view;
	float4x4 mvp;
};

struct VS_INPUT
{
	float3 Pos : POSITION;
	float2 Uv : TEXCOORD0;
};

struct VS_OUTPUT
{
	float4 Pos : SV_POSITION;
	float2 Uv : TEXCOORD0;
};

ConstantBuffer<View> modelView : register(b0);

VS_OUTPUT main(VS_INPUT input)
{
	VS_OUTPUT output;
	output.Pos = mul(float4(input.Pos, 1), modelView.mvp);
	output.Uv = input.Uv;

	return output;
}