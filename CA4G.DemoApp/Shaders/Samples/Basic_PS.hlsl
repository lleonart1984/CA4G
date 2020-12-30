struct PS_IN {
	float4 Proj : SV_POSITION;
	float3 P : POSITION; // World-Space Position
	float3 N : NORMAL; // World-Space Normal
	float2 C : TEXCOORD;
};

float4 main(PS_IN pIn) : SV_TARGET
{
	return float4(pIn.N, 1);
}