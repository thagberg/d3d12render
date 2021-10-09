
struct VS_INPUT
{
	float4 Pos : POSITION;
	float3 Color : COLOR;
};

struct VS_OUTPUT
{
	float4 Pos : SV_POSITION;
	float3 Color : COLOR;
};

VS_OUTPUT main(VS_INPUT input)
{
	VS_OUTPUT output = input;
	return output;
}
