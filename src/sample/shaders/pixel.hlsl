
struct PS_INPUT
{
	float4 Pos : SV_POSITION;
	float3 Color : COLOR;
};

float4 main(PS_INPUT input) : SV_TARGET
{
	float4 color = float4(input.Color, 1.0);
	return color;
}
