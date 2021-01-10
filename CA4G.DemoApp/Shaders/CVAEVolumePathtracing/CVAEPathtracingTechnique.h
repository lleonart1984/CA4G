#pragma once


#include "ca4g.h"
#include "../../GUITraits.h"

using namespace CA4G;

class CVAEPathtracingTechnique : public Technique, 
	public IManageScene,
	public IShowComplexity {

public:
	~CVAEPathtracingTechnique() {}

#pragma region Grid Construction Compute Shaders

	struct TriangleGrid : public ComputePipelineBindings {
		void Setup() {
			__set ComputeShader(ShaderLoader::FromFile(".\\Shaders\\CVAEVolumePathtracing\\TriangleGrid_CS.cso"));
		}

		gObj<Buffer> VertexBuffer;
		gObj<Buffer> IndexBuffer;
		
		gObj<Buffer> Triangles;
		gObj<Texture3D> Head;
		gObj<Buffer> NextBuffer;
		gObj<Texture1D> Malloc;

		float4x4 GridTransform;

		virtual void Bindings(gObj<ComputeBinder> binder) override {
			binder _set SRV(0, VertexBuffer);
			binder _set SRV(1, IndexBuffer);

			binder _set UAV(0, Triangles);
			binder _set UAV(1, Head);
			binder _set UAV(2, NextBuffer);
			binder _set UAV(3, Malloc);

			binder _set CBV(0, GridTransform);
		}
	};

	struct DistanceFieldInitial : public ComputePipelineBindings {
		void Setup() {
			__set ComputeShader(ShaderLoader::FromFile(".\\Shaders\\CVAEVolumePathtracing\\DistanceFieldInitial_CS.cso"));
		}

		gObj<Buffer> VertexBuffer;
		gObj<Buffer> IndexBuffer;
		gObj<Buffer> Triangles;
		gObj<Texture3D> Head;
		gObj<Buffer> NextBuffer;

		gObj<Texture3D> DistanceField;

		float4x4 GridTransform;

		virtual void Bindings(gObj<ComputeBinder> binder) override {
			binder _set SRV(0, VertexBuffer);
			binder _set SRV(1, IndexBuffer);
			binder _set SRV(2, Triangles);
			binder _set SRV(3, Head);
			binder _set SRV(4, NextBuffer);
			
			binder _set UAV(0, DistanceField);

			binder _set CBV(0, GridTransform);
		}
	};

	struct DistanceFieldSpread : public ComputePipelineBindings {
		void Setup() {
			__set ComputeShader(ShaderLoader::FromFile(".\\Shaders\\CVAEVolumePathtracing\\DistanceFieldSpread_CS.cso"));
		}

		gObj<Texture3D> GridSrc;
		gObj<Texture3D> GridDst;
		int LevelInfo;

		virtual void Bindings(gObj<ComputeBinder> binder) override {
			binder _set UAV(0, GridDst);
			binder _set SRV(0, GridSrc);
			binder _set CBV(0, LevelInfo);
		}
	};

	struct DebugingDistanceField : public ComputePipelineBindings {

		void Setup() {
			__set ComputeShader(ShaderLoader::FromFile(".\\Shaders\\CVAEVolumePathtracing\\DebugDF_CS.cso"));
		}

		gObj<Texture3D> Grid;
		gObj<Texture2D> Slice;

		virtual void Bindings(gObj<ComputeBinder> binder) override {
			binder _set UAV(0, Slice);
			binder _set SRV(0, Grid);
		}
	};

#pragma endregion

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
				binder _set SRV(1, Context()->GridInfos);
				binder _set SRV_Array(2, Context()->DistanceFields, Context()->NumberOfDFs);

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
			__load Code(ShaderLoader::FromFile("./Shaders/CVAEVolumePathtracing/CVAEPathtracing_RT.cso"));
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
		gObj<Buffer> GridInfos;
		gObj<Texture3D>* DistanceFields;
		int NumberOfDFs;
		gObj<Buffer> Lighting;
		gObj<Buffer> ProjectionToWorld;
	};
	gObj<RTXPathtracing> pipeline;

	gObj<TriangleGrid> creatingGrid;
	gObj<DistanceFieldInitial> computingInitialDistances;
	gObj<DistanceFieldSpread> spreadingDistances;

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
	
	// Transform from geometry space to world space for each
	// Instanced_geometry.
	float4x4* G2WTransforms;

	// Array with a Grid for every geometry.
	gObj<Texture3D>* perGeometryDF;
	gObj<Texture3D> tempGrid;
	int GridSize = 512;

	struct GridInfo {
		// Index of the base geometry (grid).
		int GridIndex;
		// Transform from the world space to the grid.
		// Considers Instance Transform, Geometry Transform and Grid transform.
		float4x4 FromWorldToGrid;
		// Scaling factor to convert distances from grid (cell is unit) to world.
		float FromGridToWorldScaling;
	};
	// Grid information for each Instanced_Geometry.
	gObj<Buffer> GridInfos;
	GridInfo* gridInfosData;
	int globalGeometryCount;
	// Grid transform for each geometry.
	float4x4* gridTransforms;

	gObj<DebugingDistanceField> debuging;
	gObj<ShowComplexityPipeline> showingComplexity;
	gObj<Buffer> screenVertices;

	// Inherited via Technique
	virtual void OnLoad() override {

		auto desc = scene->getScene();

		pipeline = __create Pipeline<RTXPathtracing>();
		creatingGrid = __create Pipeline<TriangleGrid>();
		computingInitialDistances = __create Pipeline<DistanceFieldInitial>();
		spreadingDistances = __create Pipeline<DistanceFieldSpread>();
		
		debuging = __create Pipeline<DebugingDistanceField>();
		debuging->Slice = __create Texture2D_UAV<int>(256, 256);

		showingComplexity = __create Pipeline<ShowComplexityPipeline>();
		showingComplexity->Complexity = debuging->Slice;
		screenVertices = __create Buffer_VB<float2>(6);
		screenVertices _copy FromList({
			float2(-1, -1), float2(1, -1), float2(1, 1),
			float2(-1, -1), float2(1, 1), float2(-1, 1)
			});

		GeometryTransforms = __create Buffer_SRV<float4x3>(desc->getTransformsBuffer().Count);

		globalGeometryCount = 0;
		for (int i = 0; i < desc->Instances().Count; i++)
			globalGeometryCount += desc->Instances().Data[i].Count;

		G2WTransforms = new float4x4[globalGeometryCount];

		// Allocate Memory for scene elements
		pipeline->VertexBuffer = __create Buffer_SRV<SceneVertex>(desc->Vertices().Count);
		pipeline->VertexBuffer->SetDebugName(L"Vertex Buffer");
		pipeline->IndexBuffer = __create Buffer_SRV<int>(desc->Indices().Count);
		pipeline->IndexBuffer->SetDebugName(L"Index Buffer");
		pipeline->Transforms = __create Buffer_SRV<float4x3>(globalGeometryCount);
		pipeline->Materials = __create Buffer_SRV<SceneMaterial>(desc->Materials().Count);
		pipeline->VolMaterials = __create Buffer_SRV<VolumeMaterial>(desc->Materials().Count);
		pipeline->TextureCount = desc->getTextures().Count;
		pipeline->Textures = new gObj<Texture2D>[desc->getTextures().Count];
		for (int i = 0; i < pipeline->TextureCount; i++)
			pipeline->Textures[i] = __create Texture2D_SRV(desc->getTextures().Data[i]);

		pipeline->OutputImage = __create Texture2D_UAV<RGBA>(render_target->Width, render_target->Height, 1, 1);
		pipeline->Accumulation = __create Texture2D_UAV<float4>(render_target->Width, render_target->Height, 1, 1);
		pipeline->Complexity = __create Texture2D_UAV<uint>(render_target->Width, render_target->Height);

		pipeline->Lighting = __create Buffer_CB<LightingCB>();
		pipeline->ProjectionToWorld = __create Buffer_CB<float4x4>();

		perGeometryDF = new gObj<Texture3D>[desc->Geometries().Count];
		for (int i = 0; i < desc->Geometries().Count; i++)
		{
			perGeometryDF[i] = __create Texture3D_UAV<float>(GridSize, GridSize, GridSize, 1);
			perGeometryDF[i]->SetDebugName(L"Distance Field");
		}
		tempGrid = __create Texture3D_UAV<float>(GridSize, GridSize, GridSize, 1);
		tempGrid->SetDebugName(L"Temporal Grid for DF");
		GridInfos = __create Buffer_SRV<GridInfo>(globalGeometryCount);
		GridInfos->SetDebugName(L"Grid Infos");
		gridInfosData = new GridInfo[globalGeometryCount];

		pipeline->GridInfos = GridInfos;
		pipeline->DistanceFields = perGeometryDF;
		pipeline->NumberOfDFs = desc->Geometries().Count;

		// The grid for triangle hashing in space and build initial distances.
		creatingGrid->Head = __create Texture3D_UAV<int>(GridSize, GridSize, GridSize, 1);
		creatingGrid->Head->SetDebugName(L"Head Grid");
		creatingGrid->Triangles = __create Buffer_UAV<int>(10000000);
		creatingGrid->Triangles->SetDebugName(L"Triangles Buffer");
		creatingGrid->NextBuffer = __create Buffer_UAV<int>(10000000);
		creatingGrid->NextBuffer->SetDebugName(L"Next Buffer");
		creatingGrid->Malloc = __create Texture1D_UAV<int>(1);
		creatingGrid->Malloc->SetDebugName(L"Malloc Buffer");

		computingInitialDistances->Head = creatingGrid->Head;
		computingInitialDistances->Triangles = creatingGrid->Triangles;
		computingInitialDistances->NextBuffer = creatingGrid->NextBuffer;

#pragma region Computing Per-Geometry Grid Dimensions and Transforms

		gridTransforms = new float4x4[desc->Geometries().Count];

		for (int i = 0; i < desc->Geometries().Count; i++)
		{
			auto geom = desc->Geometries().Data[i];

#pragma region Compute AABB of geometry and Transform
			float3 minim = float3(10000, 10000, 10000), maxim = float3(-10000, -10000, -10000);
			for (int j = 0; j < geom.IndexCount; j++)
			{
				float3 vPos = desc->Vertices().Data[
					geom.StartVertex + desc->Indices().Data[geom.StartIndex + j]
				].Position;
				minim = minf(minim, vPos);
				maxim = maxf(maxim, vPos);
			}
			float3 dimensions = maxim - minim;
			maxim = minim + dimensions + float3(0.001, 0.001, 0.001);
			minim = minim - float3(0.001, 0.001, 0.001);

			gridTransforms[i] = mul(Transforms::Translate(-minim), Transforms::Scale(GridSize / (maxim - minim)));
#pragma endregion
		}

#pragma endregion

		__dispatch member_collector(LoadAssets);

		__dispatch member_collector(BuildGrids);

		__dispatch member_collector(CreateRTXScene);
	}

	void LoadAssets(gObj<GraphicsManager> manager) {

		manager _load AllToGPU(screenVertices);

		SceneElement elements = scene->Updated(sceneVersion);
		UpdateBuffers(manager, elements);
	}

	void BuildGrids(gObj<GraphicsManager> gmanager) {

		// We need a compute inteface but working over a direct engine.
		// That prevent us the need to sync gpu flush of different engines.
		auto manager = gmanager.Dynamic_Cast<ComputeManager>();

		auto desc = scene->getScene();

		for (int i = 0; i < desc->Geometries().Count; i++)
		{
			auto geom = desc->Geometries().Data[i];

#pragma region creating Grid for Geometry i
			// Create grid with triangles linked lists
			creatingGrid->GridTransform = gridTransforms[i];
			creatingGrid->VertexBuffer = pipeline->VertexBuffer _create Slice(geom.StartVertex, geom.VertexCount);
			creatingGrid->IndexBuffer = pipeline->IndexBuffer _create Slice(geom.StartIndex, geom.IndexCount);
			
			manager _set Pipeline(creatingGrid);
			manager _clear UAV(creatingGrid->Head, uint4(-1));
			manager _clear UAV(creatingGrid->Malloc, uint4(0));
			manager _dispatch Threads((int)ceil(geom.IndexCount / 3.0 / 1024));

			// Compute initial distances
			computingInitialDistances->VertexBuffer = pipeline->VertexBuffer _create Slice(geom.StartVertex, geom.VertexCount);
			computingInitialDistances->IndexBuffer = pipeline->IndexBuffer _create Slice(geom.StartIndex, geom.IndexCount);
			computingInitialDistances->DistanceField = perGeometryDF[i];
			computingInitialDistances->GridTransform = gridTransforms[i];
			manager _set Pipeline(computingInitialDistances);
			manager _dispatch Threads(GridSize * GridSize * GridSize / 1024);

			// Spread distance for each possible level
			for (int level = 0; level < ceil(log(GridSize) / log(3)); level++)
			{
				spreadingDistances->GridSrc = perGeometryDF[i];
				spreadingDistances->GridDst = tempGrid;
				spreadingDistances->LevelInfo = level;
				manager _set Pipeline(spreadingDistances);

				manager _dispatch Threads(GridSize * GridSize * GridSize / 1024);

				perGeometryDF[i] = spreadingDistances->GridDst;
				tempGrid = spreadingDistances->GridSrc;
			}
#pragma endregion
		}
	}

	virtual void OnDispatch() override {
		// Update dirty elements
		__dispatch member_collector(UpdateAssets);

		// Draw current Frame
		__dispatch member_collector(DrawScene);

		//__dispatch member_collector(DebugDistanceField);
	}

	void DebugDistanceField(gObj<GraphicsManager> gmanager) {
		auto manager = gmanager.Dynamic_Cast<ComputeManager>();

		debuging->Grid = creatingGrid->Head;// perGeometryDF[0];
		//debuging->Grid = perGeometryDF[0];
		manager _set Pipeline(debuging);
		manager _dispatch Threads(256 * 256 / 1024);
		
		showingComplexity->RenderTarget = render_target;
		gmanager _set Pipeline(showingComplexity);
		gmanager _set VertexBuffer(screenVertices);
		gmanager _set Viewport(render_target->Width, render_target->Height);
		gmanager _dispatch Triangles(6);
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
		{ // Update Geometry2World transforms and Grid2World transforms in GridInfos
			int transformIndex = 0;
			for (int i = 0; i < desc->Instances().Count; i++)
			{
				auto instance = desc->Instances().Data[i];
				for (int j = 0; j < instance.Count; j++) {
					auto geometry = desc->Geometries().Data[instance.GeometryIndices[j]];

					float4x4 geometryTransform = geometry.TransformIndex == -1 ?
						float4x4(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1) :
						Transforms::FromAffine(desc->getTransformsBuffer().Data[geometry.TransformIndex]);

					G2WTransforms[transformIndex] = mul(geometryTransform, instance.Transform);

					(pipeline->Transforms _create Slice(transformIndex, 1)) _copy FromValue(
						(float4x3)G2WTransforms[transformIndex]
					);

					int gridIndex = instance.GeometryIndices[j];
					gridInfosData[transformIndex].GridIndex = gridIndex;
					gridInfosData[transformIndex].FromWorldToGrid
						= mul(
							inverse(G2WTransforms[transformIndex]),
							gridTransforms[gridIndex]
						);
					float scale = length(gridInfosData[transformIndex].FromWorldToGrid[0].get_xyz());
					scale = maxf(scale, length(gridInfosData[transformIndex].FromWorldToGrid[1].get_xyz()));
					scale = maxf(scale, length(gridInfosData[transformIndex].FromWorldToGrid[2].get_xyz()));
					gridInfosData[transformIndex].FromGridToWorldScaling = 1 / scale;

					transformIndex++;
				}
			}
			GridInfos _copy FromPtr(gridInfosData);
			manager _load AllToGPU(GridInfos);
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
		{
			manager _clear UAV(pipeline->Accumulation, uint4(0));
			manager _clear UAV(pipeline->Complexity, uint4(0));
		}

		pipeline->AccumulativeInfo.x++;
		pipeline->AccumulativeInfo.y = ShowComplexity ? 1 : 0;

		manager _dispatch Rays(render_target->Width, render_target->Height);

		manager _copy Resource(render_target, pipeline->OutputImage);
	}

};