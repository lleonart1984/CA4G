
/// Common header to any library that will use the scene retained as follow.
/// Provides functions to get intersection information
/// Augment material and surfel with texture mapping (including bump mapping)

#include "Definitions.h"

StructuredBuffer<Vertex> VertexBuffer			: register(t0, space1);	// All vertices in scene.
StructuredBuffer<int3> IndexBuffer				: register(t1, space1);	// All indices in scene.
StructuredBuffer<float4x3> Transforms			: register(t2, space1); // All materials.
StructuredBuffer<Material> Materials			: register(t3, space1); // All materials.
Texture2D<float4> Textures[500]					: register(t4, space1); // All textures used

RWTexture2D<float3> Output						: register(u0, space1); // Final Output image from the ray-trace

cbuffer PerGeometry : register(b0, space1){
	int StartTriangle;
	int GeometryVertexOffset;
	int TransformIndex;
	int MaterialIndex;
}

// Used for texture mapping
SamplerState gSmp : register(s0, space1);

// Given a surfel will modify the normal with texture maps, using
// Bump mapping and masking textures.
// Material info is updated as well.
void AugmentHitInfoWithTextureMapping(inout Vertex surfel, inout Material material, float ddx, float ddy) {
	float4 DiffTex = material.Texture_Index.x >= 0 ? Textures[material.Texture_Index.x].SampleGrad(gSmp, surfel.C, ddx, ddy) : float4(1, 1, 1, 1);
	float3 SpecularTex = material.Texture_Index.y >= 0 ? Textures[material.Texture_Index.y].SampleGrad(gSmp, surfel.C, ddx, ddy).xyz : material.Specular;
	float3 BumpTex = material.Texture_Index.z >= 0 ? Textures[material.Texture_Index.z].SampleGrad(gSmp, surfel.C, ddx, ddy).xyz : float3(0.5, 0.5, 1);
	float3 MaskTex = material.Texture_Index.w >= 0 ? Textures[material.Texture_Index.w].SampleGrad(gSmp, surfel.C, ddx, ddy).xyz : 1;

	float3x3 TangentToWorld = { surfel.T, surfel.B, surfel.N };
	// Change normal according to bump map
	surfel.N = normalize(mul(BumpTex * 2 - 1, TangentToWorld));

	material.Diffuse *= DiffTex.xyz * (MaskTex.x * DiffTex.w); // set transparent if necessary.
	material.Specular.xyz = max(material.Specular.xyz, SpecularTex);
}

void GetIndices(out int transformIndex, out int materialIndex, out int triangleIndex, out int vertexOffset) {
	transformIndex = TransformIndex;
	materialIndex = MaterialIndex;
	vertexOffset = GeometryVertexOffset;
	triangleIndex = StartTriangle + PrimitiveIndex();
}

void GetHitInfo(
	float3 barycentrics, 
	int materialIndex,
	int triangleIndex, 
	int vertexOffset,
	int transformIndex,
	out Vertex surfel, out Material material, float ddx, float ddy)
{
	int3 indices = vertexOffset + IndexBuffer[triangleIndex];
	Vertex v1 = VertexBuffer[indices.x];
	Vertex v2 = VertexBuffer[indices.y];
	Vertex v3 = VertexBuffer[indices.z];
	Vertex s = {
		v1.P * barycentrics.x + v2.P * barycentrics.y + v3.P * barycentrics.z,
		v1.N * barycentrics.x + v2.N * barycentrics.y + v3.N * barycentrics.z,
		v1.C * barycentrics.x + v2.C * barycentrics.y + v3.C * barycentrics.z,
		v1.T * barycentrics.x + v2.T * barycentrics.y + v3.T * barycentrics.z,
		v1.B * barycentrics.x + v2.B * barycentrics.y + v3.B * barycentrics.z
	};

	surfel = Transform(s, Transforms[transformIndex]);

	material = Materials[materialIndex];

	AugmentHitInfoWithTextureMapping(surfel, material, ddx, ddy);
}
