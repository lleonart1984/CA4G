RWTexture2D<float4> Output : register(u0);

cbuffer Transforms : register(b0)
{
	float4x4 FromProjectionToWorld;
};

struct RayPayload // Only used for raycasting
{
	int Index;
};

[shader("raygeneration")]
void RayGen() {
	uint2 raysIndex = DispatchRaysIndex();
	uint2 raysDimensions = DispatchRaysDimensions();
	float2 coord = (raysIndex.xy + 0.5) / raysDimensions;
	float4 ndcP = float4(2 * coord - 1, 0, 1);
	ndcP.y *= -1;
	float4 ndcT = ndcP + float4(0, 0, 1, 0);
	float4 viewP = mul(ndcP, FromProjectionToWorld);
	viewP.xyz /= viewP.w;
	float4 viewT = mul(ndcT, FromProjectionToWorld);
	viewT.xyz /= viewT.w;
	float3 O = viewP.xyz;
	float3 D = normalize(viewT.xyz - viewP.xyz);

	Output[raysIndex] = float4(D, 1);
}

[shader("miss")]
void Miss(inout RayPayload payload)
{
}