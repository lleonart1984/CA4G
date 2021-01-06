// Top level structure with the scene
RaytracingAccelerationStructure Scene : register(t0, space0);

RWTexture2D<float4> Output : register(u0);

cbuffer Transforms : register(b0)
{
	float4x4 FromProjectionToWorld;
};

struct RayPayload // Only used for raycasting
{
	int Index;
};

/// Use RTX TraceRay to detect a single intersection. No recursion is necessary
bool Intersect(float3 P, float3 D, out int tIndex) {
	RayPayload payload = (RayPayload)0;
	RayDesc ray;
	ray.Origin = P;
	ray.Direction = D;
	ray.TMin = 0;
	ray.TMax = 100.0;
	TraceRay(Scene, RAY_FLAG_FORCE_OPAQUE, 0xFF, 0, 1, 0, ray, payload);
	tIndex = payload.Index;
	return tIndex >= 0;
}

[shader("closesthit")]
void OnClosestHit(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attr)
{
	payload.Index = PrimitiveIndex();
}

[shader("miss")]
void OnMiss(inout RayPayload payload)
{
	payload.Index = -1;
}

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
	float3 P = viewP.xyz;
	float3 D = normalize(viewT.xyz - viewP.xyz);
	
	int index = -1;
	if (Intersect(P, D, index))
	{
		Output[raysIndex] = float4(1, 1, 0, 1);
	}
	else
	{
		Output[raysIndex] = float4(D, 1);
	}
}

