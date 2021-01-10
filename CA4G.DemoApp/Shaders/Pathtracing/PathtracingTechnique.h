#pragma once


#include "ca4g.h"
#include "../../GUITraits.h"

using namespace CA4G;

class PathtracingTechnique : public Technique, public IManageScene {

public:
	~PathtracingTechnique() {}

	struct RTXPathtracing : public RaytracingPipelineBindings {

		RTXPathtracing() {}

		struct Program : public RTProgram<RTXPathtracing> {

			void Setup() {
				__set Payload(28);

				__load Shader(Context()->Generating);
				__load Shader(Context()->Missing);
				__load Shader(Context()->Hitting);
			}

			void Bindings(gObj<RaytracingBinder> binder) {
				binder _set Space(1);
				binder _set UAV(0, Context()->OutputImage);
				binder _set UAV(1, Context()->Accumulation);
				binder _set UAV(2, Context()->Complexity);
				binder _set SRV(0, Context()->VertexBuffer);
				binder _set SRV(1, Context()->IndexBuffer);
				binder _set SRV(2, Context()->Transforms);
				binder _set SRV(3, Context()->Materials);
				binder _set SRV(4, Context()->VolMaterials);
				binder _set SRV_Array(5, Context()->Textures, Context()->TextureCount);
				binder _set SMP_Static(0, Sampler::Linear());
				binder _set CBV(1, Context()->AccumulativeInfo);

				binder _set Space(0);

				binder _set ADS(0, Context()->Scene);
				binder _set CBV(0, Context()->Lighting);
				binder _set CBV(1, Context()->ProjectionToWorld);
			}

			void HitGroup_Bindings(gObj<RaytracingBinder> binder) {
				binder _set Space(1);
				binder _set CBV(0, Context()->PerGeometry);
			}
		};
		gObj<Program> MainProgram;

		void Setup() {
			__load Code(ShaderLoader::FromFile("./Shaders/Pathtracing/Pathtracing_RT.cso"));
			Generating = __create Shader<RayGenerationHandle>(L"RayGen");
			Missing = __create Shader<MissHandle>(L"OnMiss");
			auto closestHit = __create Shader<ClosestHitHandle>(L"OnClosestHit");
			Hitting = __create HitGroup(closestHit, nullptr, nullptr);

			__load Program(MainProgram);
		}

		gObj<RayGenerationHandle> Generating;
		gObj<HitGroupHandle> Hitting;
		gObj<MissHandle> Missing;

		// Space 1 (CommonRT.h, CommonPT.h)
		gObj<Buffer> VertexBuffer;
		gObj<Buffer> IndexBuffer;
		gObj<Buffer> Transforms;
		gObj<Buffer> Materials;
		gObj<Buffer> VolMaterials;
		gObj<Texture2D>* Textures;
		int TextureCount;
		gObj<Texture2D> OutputImage;
		gObj<Texture2D> Accumulation;
		gObj<Texture2D> Complexity;
		struct PerGeometryInfo {
			int StartTriangle;
			int GeometryVertexOffset;
			int TransformIndex;
			int MaterialIndex;
		} PerGeometry;

		int2 AccumulativeInfo;

		// Space 0
		gObj<InstanceCollection> Scene;
		gObj<Buffer> Lighting;
		gObj<Buffer> ProjectionToWorld;
	};
	gObj<RTXPathtracing> pipeline;

	struct LightingCB {
		float3 LightPosition;
		float rem0;
		float3 LightIntensity;
		float rem1;
		float3 LightDirection;
		float rem2;
	};

	// Used to build bottom level ADS
	gObj<Buffer> GeometryTransforms;

	// Inherited via Technique
	virtual void OnLoad() override {

		auto desc = scene->getScene();

		pipeline = __create Pipeline<RTXPathtracing>();

		GeometryTransforms = __create Buffer_SRV<float4x3>(desc->getTransformsBuffer().Count);

		int globalGeometryCount = 0;
		for (int i = 0; i < desc->Instances().Count; i++)
			globalGeometryCount += desc->Instances().Data[i].Count;

		// Allocate Memory for scene elements
		pipeline->VertexBuffer = __create Buffer_SRV<SceneVertex>(desc->Vertices().Count);
		pipeline->IndexBuffer = __create Buffer_SRV<int>(desc->Indices().Count);
		pipeline->Transforms = __create Buffer_SRV<float4x3>(globalGeometryCount);
		pipeline->Materials = __create Buffer_SRV<SceneMaterial>(desc->Materials().Count);
		pipeline->VolMaterials = __create Buffer_SRV<VolumeMaterial>(desc->Materials().Count);
		pipeline->TextureCount = desc->getTextures().Count;
		pipeline->Textures = new gObj<Texture2D>[desc->getTextures().Count];
		for (int i = 0; i < pipeline->TextureCount; i++)
			pipeline->Textures[i] = __create Texture2D_SRV(desc->getTextures().Data[i]);

		pipeline->OutputImage = __create Texture2D_UAV<RGBA>(render_target->Width, render_target->Height, 1, 1);
		pipeline->Accumulation = __create Texture2D_UAV<float4>(render_target->Width, render_target->Height, 1, 1);
		
		pipeline->Lighting = __create Buffer_CB<LightingCB>();
		pipeline->ProjectionToWorld = __create Buffer_CB<float4x4>();

		__dispatch member_collector(LoadAssets);

		__dispatch member_collector(CreateRTXScene);
	}

	void LoadAssets(gObj<GraphicsManager> manager) {
		SceneElement elements = scene->Updated(sceneVersion);
		UpdateBuffers(manager, elements);
	}

	virtual void OnDispatch() override {
		// Update dirty elements
		__dispatch member_collector(UpdateAssets);

		// Draw current Frame
		__dispatch member_collector(DrawScene);
	}

	void UpdateAssets(gObj<RaytracingManager> manager) {
		SceneElement elements = scene->Updated(sceneVersion);
		UpdateBuffers(manager, elements);

		if (+(elements & SceneElement::InstanceTransforms))
			UpdateRTXScene(manager);

		if (elements != SceneElement::None)
			pipeline->AccumulativeInfo.x = 0; // restart frame for Pathtracing...
	}

	void UpdateBuffers(gObj<GraphicsManager> manager, SceneElement elements) {
		auto desc = scene->getScene();

		if (+(elements & SceneElement::Vertices))
		{
			pipeline->VertexBuffer _copy FromPtr(desc->Vertices().Data);
			manager _load AllToGPU(pipeline->VertexBuffer);
		}

		if (+(elements & SceneElement::Indices))
		{
			pipeline->IndexBuffer _copy FromPtr(desc->Indices().Data);
			manager _load AllToGPU(pipeline->IndexBuffer);
		}

		if (+(elements & SceneElement::Materials))
		{
			pipeline->Materials _copy FromPtr(desc->Materials().Data);
			pipeline->VolMaterials _copy FromPtr(desc->VolumeMaterials().Data);
			manager _load AllToGPU(pipeline->Materials);
			manager _load AllToGPU(pipeline->VolMaterials);
		}

		if (+(elements & SceneElement::Textures)) {
			for (int i = 0; i < pipeline->TextureCount; i++)
				manager _load AllToGPU(pipeline->Textures[i]);
		}

		if (+(elements & SceneElement::Camera))
		{
			float4x4 proj, view;
			scene->getCamera().GetMatrices(render_target->Width, render_target->Height, view, proj);
			pipeline->ProjectionToWorld _copy FromValue(mul(inverse(proj), inverse(view)));
			manager _load AllToGPU(pipeline->ProjectionToWorld);
		}

		if (+(elements & SceneElement::Lights))
		{
			pipeline->Lighting _copy FromValue(LightingCB{
					scene->getMainLight().Position, 0,
					scene->getMainLight().Intensity, 0,
					scene->getMainLight().Direction, 0
				});
			manager _load AllToGPU(pipeline->Lighting);
		}

		if (+(elements & SceneElement::GeometryTransforms))
		{
			GeometryTransforms _copy FromPtr(desc->getTransformsBuffer().Data);
			manager _load AllToGPU(GeometryTransforms);
		}

		if (+(elements & SceneElement::GeometryTransforms) ||
			+(elements & SceneElement::InstanceTransforms))
		{
			int transformIndex = 0;
			for (int i = 0; i < desc->Instances().Count; i++)
			{
				auto instance = desc->Instances().Data[i];
				for (int j = 0; j < instance.Count; j++) {
					auto geometry = desc->Geometries().Data[instance.GeometryIndices[j]];

					float4x4 geometryTransform = geometry.TransformIndex == -1 ?
						float4x4(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1) :
						Transforms::FromAffine(desc->getTransformsBuffer().Data[geometry.TransformIndex]);

					(pipeline->Transforms _create Slice(transformIndex, 1)) _copy FromValue(
						(float4x3)mul(geometryTransform, instance.Transform)
					);

					transformIndex++;
				}
			}

			manager _load AllToGPU(pipeline->Transforms);
		}
	}

	gObj<InstanceCollection> rtxScene;

	void CreateRTXScene(gObj<RaytracingManager> manager) {

		auto desc = scene->getScene();

		rtxScene = manager _create Intances();
		int geometryOffset = 0;
		for (int i = 0; i < desc->Instances().Count; i++)
		{
			auto instance = desc->Instances().Data[i];

			auto geometryCollection = manager _create TriangleGeometries();
			geometryCollection _set Transforms(GeometryTransforms);

			for (int j = 0; j < instance.Count; j++) // load every geometry
			{
				auto geometry = desc->Geometries().Data[instance.GeometryIndices[j]];

				geometryCollection _create Geometry(
					pipeline->VertexBuffer _create Slice(geometry.StartVertex, geometry.VertexCount),
					pipeline->IndexBuffer _create Slice(geometry.StartIndex, geometry.IndexCount),
					geometry.TransformIndex);
			}

			manager _load Geometry(geometryCollection);

			rtxScene _create Instance(geometryCollection,
				255U, geometryOffset, i, (float4x3)instance.Transform
			);

			geometryOffset += instance.Count;
		}

		manager _load Scene(rtxScene);

		pipeline->Scene = rtxScene;
	}

	void UpdateRTXScene(gObj<RaytracingManager> manager) {

		auto desc = scene->getScene();

		for (int i = 0; i < desc->Instances().Count; i++)
		{
			auto instance = desc->Instances().Data[i];

			rtxScene _load InstanceTransform(i, (float4x3)instance.Transform);
		}

		manager _load Scene(rtxScene);
	}

	void DrawScene(gObj<RaytracingManager> manager) {

		manager _set Pipeline(pipeline);
		manager _set Program(pipeline->MainProgram);

		static bool firstTime = true;
		if (firstTime) {
			manager _set RayGeneration(pipeline->Generating);
			manager _set Miss(pipeline->Missing, 0);

			auto desc = scene->getScene();

			int geometryOffset = 0;

			for (int i = 0; i < desc->Instances().Count; i++)
			{
				auto instance = desc->Instances().Data[i];
				for (int j = 0; j < instance.Count; j++) {
					auto geometry = desc->Geometries().Data[instance.GeometryIndices[j]];
					pipeline->PerGeometry.StartTriangle = geometry.StartIndex / 3;
					pipeline->PerGeometry.GeometryVertexOffset = geometry.StartVertex;
					pipeline->PerGeometry.MaterialIndex = geometry.MaterialIndex;
					pipeline->PerGeometry.TransformIndex = geometryOffset + j;

					manager _set HitGroup(pipeline->Hitting, j, 0, 1, geometryOffset);

					geometryOffset += instance.Count;
				}
			}

			firstTime = false;
		}

		if (pipeline->AccumulativeInfo.x == 0) // first frame after scene dirty
			manager _clear UAV(pipeline->Accumulation, uint4(0));

		pipeline->AccumulativeInfo.x++;

		manager _dispatch Rays(render_target->Width, render_target->Height);

		manager _copy Resource(render_target, pipeline->OutputImage);
	}

};