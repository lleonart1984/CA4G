#pragma once

#include "ca4g.h"
#include "GUITraits.h"

using namespace CA4G;

//typedef class AsyncSample main_technique;
//typedef class TriangleSample main_technique;
//typedef class SceneSample main_technique;
//typedef class UAVSample main_technique;
typedef class BasicRaycastSample main_technique;


#pragma region Async Sample

class AsyncSample : public Technique {
public:

	~AsyncSample() {}

	// Inherited via Technique
	void OnLoad() override {
	}

	struct ClearingTagData  {
		float4 clearColor;
	};

	void OnDispatch() override {
		__set TagValue(ClearingTagData{ float4(1, 0, 0, 1) });
		__dispatch member_collector_async(ClearRT);

		__set TagValue(ClearingTagData{ float4(1, 1, 0, 1) });
		__dispatch member_collector_async(ClearRT);
	}

	void ClearRT(gObj<GraphicsManager> manager) {
		auto clearColor = manager->Tag<ClearingTagData>().clearColor;
		manager _clear RT(render_target, clearColor);
	}

};

#pragma endregion

#pragma region DrawTriangle

class TriangleSample : public Technique {
public:

	~TriangleSample() {}

	gObj<Buffer> vertexBuffer;
	gObj<Buffer> indexBuffer;

	struct Vertex {
		float3 P;
	};

	struct Pipeline : public GraphicsPipelineBindings {
		
		gObj<Texture2D> RenderTarget;
		gObj<Texture2D> Texture;

		// Inherited via GraphicsPipelineBindings
		void Setup() override {
			__set VertexShader(ShaderLoader::FromFile("./Shaders/Samples/Demo_VS.cso"));
			__set PixelShader(ShaderLoader::FromFile("./Shaders/Samples/Demo_PS.cso"));
			__set InputLayout({
					VertexElement { VertexElementType::Float, 3, "POSITION" }
				});
			//__set NoDepthTest();
			//__set CullMode(D3D12_CULL_MODE_FRONT);
			//__set FillMode();
		}

		void Bindings(gObj<GraphicsBinder> binder) {
			
			binder _set BindingsOnSet();
			
			binder _set PixelShaderBindings();
			binder _set RTV(0, RenderTarget);
			binder _set SRV(0, Texture);
			binder _set SMP_Static(0, Sampler::Linear());
		}
	};
	gObj<Pipeline> pipeline;
	gObj<Texture2D> texture;

	// Inherited via Technique
	void OnLoad() override {
		vertexBuffer = __create Buffer_VB<Vertex>(4);
		vertexBuffer _copy FromList({
			Vertex { float3(-1, -1, 0.5) },
			Vertex { float3(1, -1, 0.5) },
			Vertex { float3(1, 1, 0.5) },
			Vertex { float3(-1, 1, 0.5) }
			});

		indexBuffer = __create Buffer_IB<int>(6);
		indexBuffer _copy FromList({
			 0, 1, 2, 0, 2, 3
			});

		texture = __create Texture2D_SRV<float4>(2, 2, 2, 1);
		float4 pixels[] = {
				float4(1,0,0,1), float4(1,1,0,1),
				float4(0,1,0,1), float4(0,0,1,1),
				float4(1,0,1,1)
		};
		texture _copy FromPtr((byte*)pixels);

		float4 pixel = float4(1, 0, 1, 1);
		texture _copy RegionFromPtr((byte*)&pixel, D3D12_BOX{ 0,0,0,1,1,1 });

		pipeline = __create Pipeline<Pipeline>();
		
		__dispatch member_collector(LoadAssets);
	}

	void LoadAssets(gObj<CopyManager> manager) {
		manager _load AllToGPU(vertexBuffer);
		manager _load AllToGPU(indexBuffer);
		manager _load AllToGPU(texture);
	}

	void OnDispatch() override {
		__dispatch member_collector(DrawTriangle);
	}

	void DrawTriangle(gObj<GraphicsManager> manager) {
		static int frame = 0;
		frame++;
		pipeline->RenderTarget = render_target;
		pipeline->Texture = texture;
		manager _set Pipeline(pipeline);
		manager _set VertexBuffer(vertexBuffer);
		manager _set IndexBuffer(indexBuffer);
		manager _set Viewport(render_target->Width, render_target->Height);
		
		manager _clear RT(render_target, float3(0.2f, sin(frame*0.001), 0.5f));

		manager _dispatch IndexedTriangles(6);
	}

};

#pragma endregion

#pragma region Draw simple scene

class SceneSample : public Technique, public IManageScene {
public:

	~SceneSample() {}

	gObj<Buffer> VertexBuffer;
	gObj<Buffer> IndexBuffer;
	gObj<Buffer> Camera;
	gObj<Buffer> Lighting;
	gObj<Buffer> GeometryTransforms;
	gObj<Buffer> InstanceTransforms;
	gObj<Texture2D>* Textures;
	int TextureCount;

	struct CameraCB {
		float4x4 Projection;
		float4x4 View;
	};

	struct LightingCB {
		float3 Position;
		float3 Intensity;
	};

	struct Pipeline : public GraphicsPipelineBindings {

		gObj<Texture2D> RenderTarget;
		gObj<Texture2D> DepthBuffer;
		gObj<Buffer> InstanceTransforms;
		gObj<Buffer> GeometryTransforms;
		gObj<Buffer> Camera;
		gObj<Texture2D> Texture;
		
		struct ObjectInfoCB {
			int InstanceIndex;
			int TransformIndex;
			int MaterialIndex;
		} ObjectInfo;

		// Inherited via GraphicsPipelineBindings
		void Setup() override {
			__set VertexShader(ShaderLoader::FromFile("./Shaders/Samples/Basic_VS.cso"));
			__set PixelShader(ShaderLoader::FromFile("./Shaders/Samples/Basic_PS.cso"));
			__set InputLayout(SceneVertex::Layout());
			__set DepthTest();
		}

		void Bindings(gObj<GraphicsBinder> binder) {
			binder _set BindingsOnSet();
			{
				binder _set PixelShaderBindings();
				binder _set SMP_Static(0, Sampler::Linear());
				binder _set RTV(0, RenderTarget);
				binder _set DSV(DepthBuffer);

				binder _set VertexShaderBindings();
				binder _set SRV(0, InstanceTransforms);
				binder _set SRV(1, GeometryTransforms);
				binder _set CBV(0, Camera);
			}

			binder _set BindingsOnDispatch();
			{
				binder _set VertexShaderBindings();
				binder _set CBV(1, ObjectInfo);

				binder _set PixelShaderBindings();
				binder _set SRV(0, Texture);
			}
		}
	};
	gObj<Pipeline> pipeline;

	// Inherited via Technique
	virtual void OnLoad() override {

		auto desc = scene->getScene();
		
		// Allocate Memory for scene elements
		VertexBuffer = __create Buffer_VB<SceneVertex>(desc->Vertices().Count);
		IndexBuffer = __create Buffer_IB<int>(desc->Indices().Count);
		Camera = __create Buffer_CB<CameraCB>();
		Lighting = __create Buffer_CB<LightingCB>();
		GeometryTransforms = __create Buffer_SRV<float4x3>(desc->getTransformsBuffer().Count);
		InstanceTransforms = __create Buffer_SRV<float4x4>(desc->Instances().Count);
		Textures = new gObj<Texture2D>[desc->getTextures().Count];
		TextureCount = desc->getTextures().Count;
		for (int i = 0; i < TextureCount; i++)
			Textures[i] = __create Texture2D_SRV(desc->getTextures().Data[i]);
		pipeline = __create Pipeline<Pipeline>();
		pipeline->Camera = Camera;
		pipeline->GeometryTransforms = GeometryTransforms;
		pipeline->InstanceTransforms = InstanceTransforms;
		pipeline->DepthBuffer = __create Texture2D_DSV(render_target->Width, render_target->Height);

		__dispatch member_collector(UpdateDirtyElements);
	}

	void UpdateDirtyElements(gObj<GraphicsManager> manager) {

		auto elements = scene->Updated(sceneVersion);
		auto desc = scene->getScene();

		if (+(elements & SceneElement::Vertices))
		{
			VertexBuffer _copy FromPtr(desc->Vertices().Data);
			manager _load AllToGPU(VertexBuffer);
		}
		
		if (+(elements & SceneElement::Indices))
		{
			IndexBuffer _copy FromPtr(desc->Indices().Data);
			manager _load AllToGPU(IndexBuffer);
		}

		if (+(elements & SceneElement::Camera))
		{
			float4x4 proj, view;
			scene->getCamera().GetMatrices(render_target->Width, render_target->Height, view, proj);
			Camera _copy FromValue(CameraCB{
					proj,
					view
				});
			manager _load AllToGPU(Camera);
		}

		if (+(elements & SceneElement::Lights))
		{
			Lighting _copy FromValue(LightingCB{
					scene->getMainLight().Position,
					scene->getMainLight().Intensity
				});
			manager _load AllToGPU(Lighting);
		}

		if (+(elements & SceneElement::GeometryTransforms))
		{
			GeometryTransforms _copy FromPtr(desc->getTransformsBuffer().Data);
			manager _load AllToGPU(GeometryTransforms);
		}

		if (+(elements & SceneElement::Textures)) {
			for (int i = 0; i < TextureCount; i++)
				manager _load AllToGPU(Textures[i]);
		}

		if (+(elements & SceneElement::InstanceTransforms))
		{
			float4x4* transforms = new float4x4[desc->Instances().Count];
			for (int i = 0; i < desc->Instances().Count; i++)
				transforms[i] = desc->Instances().Data[i].Transform;

			InstanceTransforms _copy FromPtr(transforms);
			manager _load AllToGPU(InstanceTransforms);
			delete[] transforms;
		}
	}

	virtual void OnDispatch() override {
		// Update dirty elements
		__dispatch member_collector(UpdateDirtyElements);

		// Draw current Frame
		__dispatch member_collector(DrawScene);
	}

	void DrawScene(gObj<GraphicsManager> manager) {
		pipeline->RenderTarget = render_target;
		manager _set Pipeline(pipeline);
		manager _set VertexBuffer(VertexBuffer);
		manager _set IndexBuffer(IndexBuffer);
		manager _set Viewport(render_target->Width, render_target->Height);

		manager _clear RT(render_target, float3(0.2f, 0.2f, 0.5f));
		manager _clear Depth(pipeline->DepthBuffer);

		auto desc = scene->getScene();

		for (int i = 0; i < desc->Instances().Count; i++) {
			pipeline->ObjectInfo.InstanceIndex = i;
			InstanceDescription instance = desc->Instances().Data[i];
			for (int j = 0; j < instance.Count; j++) {
				GeometryDescription geometry = desc->Geometries().Data[instance.GeometryIndices[j]];
				pipeline->ObjectInfo.TransformIndex = geometry.TransformIndex;
				pipeline->ObjectInfo.MaterialIndex = geometry.MaterialIndex;

				pipeline->Texture = nullptr;
				if (geometry.MaterialIndex != -1)
				{
					int textureIndex = desc->Materials().Data[geometry.MaterialIndex].DiffuseMap;
					if (textureIndex != -1)
						pipeline->Texture = Textures[textureIndex];
				}
				manager _dispatch IndexedTriangles(geometry.IndexCount, geometry.StartIndex);
			}
		}
	}
};

#pragma endregion

#pragma region UAV Sample

class UAVSample : public Technique, public IManageScene {
public:

	~UAVSample() {}

	gObj<Buffer> VertexBuffer;
	gObj<Buffer> IndexBuffer;
	gObj<Buffer> Camera;
	gObj<Buffer> GeometryTransforms;
	gObj<Buffer> InstanceTransforms;
	gObj<Texture2D> Complexity;

	struct CameraCB {
		float4x4 Projection;
		float4x4 View;
	};

	struct Pipeline : public GraphicsPipelineBindings {

		gObj<Texture2D> RenderTarget;
		gObj<Texture2D> CountingBuffer;
		gObj<Buffer> InstanceTransforms;
		gObj<Buffer> GeometryTransforms;
		gObj<Buffer> Camera;

		struct ObjectInfoCB {
			int InstanceIndex;
			int TransformIndex;
			int MaterialIndex;
		} ObjectInfo;

		// Inherited via GraphicsPipelineBindings
		void Setup() override {
			__set VertexShader(ShaderLoader::FromFile("./Shaders/Samples/Basic_VS.cso"));
			__set PixelShader(ShaderLoader::FromFile("./Shaders/Samples/Counting_PS.cso"));
			__set InputLayout(SceneVertex::Layout());
		}

		void Bindings(gObj<GraphicsBinder> binder) {
			binder _set BindingsOnSet(); {
				binder _set PixelShaderBindings();
				binder _set UAV(0, CountingBuffer);

				binder _set VertexShaderBindings();
				binder _set SRV(0, InstanceTransforms);
				binder _set SRV(1, GeometryTransforms);
				binder _set CBV(0, Camera);
			}

			binder _set BindingsOnDispatch(); {
				binder _set VertexShaderBindings();
				binder _set CBV(1, ObjectInfo);
			}
		}
	};
	gObj<Pipeline> pipeline;
	gObj<ShowComplexityPipeline> showComplexity;
	gObj<Buffer> screenVertices;

	// Inherited via Technique
	virtual void OnLoad() override {

		auto desc = scene->getScene();

		// Allocate Memory for scene elements
		VertexBuffer = __create Buffer_VB<SceneVertex>(desc->Vertices().Count);
		IndexBuffer = __create Buffer_IB<int>(desc->Indices().Count);
		Camera = __create Buffer_CB<CameraCB>();
		GeometryTransforms = __create Buffer_SRV<float4x3>(desc->getTransformsBuffer().Count);
		InstanceTransforms = __create Buffer_SRV<float4x4>(desc->Instances().Count);
		Complexity = __create Texture2D_UAV<uint>(render_target->Width, render_target->Height, 1, 1);

		pipeline = __create Pipeline<Pipeline>();
		pipeline->Camera = Camera;
		pipeline->GeometryTransforms = GeometryTransforms;
		pipeline->InstanceTransforms = InstanceTransforms;
		pipeline->CountingBuffer = __create Texture2D_DSV(render_target->Width, render_target->Height);

		showComplexity = __create Pipeline<ShowComplexityPipeline>();
		screenVertices = __create Buffer_VB<float2>(6);
		screenVertices _copy FromList({
			float2(-1, -1), float2(1, -1), float2(1, 1),
			float2(-1, -1), float2(1, 1), float2(-1, 1)
			});

		__dispatch member_collector(UpdateDirtyElements);
	
		__dispatch member_collector(LoadScreenVertices);
	}

	void LoadScreenVertices(gObj<GraphicsManager> manager) {
		manager _load AllToGPU(screenVertices);
	}

	void UpdateDirtyElements(gObj<GraphicsManager> manager) {

		auto elements = scene->Updated(sceneVersion);
		auto desc = scene->getScene();

		if (+(elements & SceneElement::Vertices))
		{
			VertexBuffer _copy FromPtr(desc->Vertices().Data);
			manager _load AllToGPU(VertexBuffer);
		}

		if (+(elements & SceneElement::Indices))
		{
			IndexBuffer _copy FromPtr(desc->Indices().Data);
			manager _load AllToGPU(IndexBuffer);
		}

		if (+(elements & SceneElement::Camera))
		{
			float4x4 proj, view;
			scene->getCamera().GetMatrices(render_target->Width, render_target->Height, view, proj);
			Camera _copy FromValue(CameraCB{
					proj,
					view
				});
			manager _load AllToGPU(Camera);
		}

		if (+(elements & SceneElement::GeometryTransforms))
		{
			GeometryTransforms _copy FromPtr(desc->getTransformsBuffer().Data);
			manager _load AllToGPU(GeometryTransforms);
		}

		if (+(elements & SceneElement::InstanceTransforms))
		{
			float4x4* transforms = new float4x4[desc->Instances().Count];
			for (int i = 0; i < desc->Instances().Count; i++)
				transforms[i] = desc->Instances().Data[i].Transform;

			InstanceTransforms _copy FromPtr(transforms);
			manager _load AllToGPU(InstanceTransforms);
			delete[] transforms;
		}
	}

	virtual void OnDispatch() override {
		// Update dirty elements
		__dispatch member_collector(UpdateDirtyElements);

		// Draw current Frame
		__dispatch member_collector(DrawScene);

		__dispatch member_collector(DrawComplexity);
	}

	void DrawScene(gObj<GraphicsManager> manager) {
		pipeline->RenderTarget = render_target;
		pipeline->CountingBuffer = Complexity;

		manager _set Pipeline(pipeline);
		manager _set VertexBuffer(VertexBuffer);
		manager _set IndexBuffer(IndexBuffer);
		manager _set Viewport(render_target->Width, render_target->Height);

		manager _clear RT(render_target, float3(0.2f, 0.2f, 0.5f));
		manager _clear UAV(Complexity, uint4(0));

		auto desc = scene->getScene();

		for (int i = 0; i < desc->Instances().Count; i++) {
			pipeline->ObjectInfo.InstanceIndex = i;
			InstanceDescription instance = desc->Instances().Data[i];
			for (int j = 0; j < instance.Count; j++) {
				GeometryDescription geometry = desc->Geometries().Data[instance.GeometryIndices[j]];
				pipeline->ObjectInfo.TransformIndex = geometry.TransformIndex;
				pipeline->ObjectInfo.MaterialIndex = geometry.MaterialIndex;
				manager _dispatch IndexedTriangles(geometry.IndexCount, geometry.StartIndex);
			}
		}
	}

	void DrawComplexity(gObj<GraphicsManager> manager) {
		showComplexity->RenderTarget = render_target;
		showComplexity->Complexity = Complexity;

		manager _set Pipeline(showComplexity);
		manager _set VertexBuffer(screenVertices);
		manager _set Viewport(render_target->Width, render_target->Height);
		manager _dispatch Triangles(6);
	}
};

#pragma endregion

#pragma region Basic Raycasting

class BasicRaycastSample : public Technique, public IManageScene {
public:

	~BasicRaycastSample() {}

	struct TransformsCB {
		float4x4 FromProjectionToWorld;
	};

	gObj<Buffer> VertexBuffer;
	gObj<Buffer> IndexBuffer;
	gObj<Buffer> Transforms;
	gObj<Buffer> GeometryTransforms;
	gObj<Buffer> InstanceTransforms;
	gObj<Texture2D> OutputImage;

	struct RTXSample: public RaytracingPipelineBindings {
		struct Program : public RTProgram<RTXSample> {

			void Setup() {
				__load Shader(Context()->Generating);
				__load Shader(Context()->Missing);
				__load Shader(Context()->Hitting);
			}
			
			void Bindings(gObj<RaytracingBinder> binder) {
				binder _set BindingsOnSet();
				binder _set ADS(0, Context()->Scene);
				binder _set UAV(0, Context()->Output);
				binder _set CBV(0, Context()->Transforms);
			}
		};
		gObj<Program> MainProgram;

		void Setup() {
			__load Code(ShaderLoader::FromFile("./Shaders/Samples/RTXSample_RT.cso"));
			Generating			= __create Shader<RayGenerationHandle>(L"RayGen");
			Missing				= __create Shader<MissHandle>(L"OnMiss");
			auto closestHit		= __create Shader<ClosestHitHandle>(L"OnClosestHit");
			Hitting = __create HitGroup(closestHit, nullptr, nullptr);

			__load Program(MainProgram);
		}

		gObj<RayGenerationHandle> Generating;
		gObj<HitGroupHandle> Hitting;
		gObj<MissHandle> Missing;

		gObj<InstanceCollection> Scene;
		gObj<Texture2D> Output;
		gObj<Buffer> Transforms;
	};
	gObj<RTXSample> pipeline;

	// Inherited via Technique
	virtual void OnLoad() override {

		auto desc = scene->getScene();

		// Allocate Memory for scene elements
		VertexBuffer = __create Buffer_SRV<SceneVertex>(desc->Vertices().Count);
		IndexBuffer = __create Buffer_SRV<int>(desc->Indices().Count);
		Transforms = __create Buffer_CB<TransformsCB>();
		GeometryTransforms = __create Buffer_SRV<float4x3>(desc->getTransformsBuffer().Count);
		InstanceTransforms = __create Buffer_SRV<float4x4>(desc->Instances().Count);
		OutputImage = __create Texture2D_UAV<RGBA>(render_target->Width, render_target->Height, 1, 1);

		pipeline = __create Pipeline<RTXSample>();
		pipeline->Transforms = Transforms;
		pipeline->Output = OutputImage;
		
		__dispatch member_collector(UpdateDirtyElements);

		__dispatch member_collector(CreateSceneOnGPU);
	}

	void UpdateDirtyElements(gObj<GraphicsManager> manager) {

		auto elements = scene->Updated(sceneVersion);
		auto desc = scene->getScene();

		if (+(elements & SceneElement::Vertices))
		{
			VertexBuffer _copy FromPtr(desc->Vertices().Data);
			manager _load AllToGPU(VertexBuffer);
		}

		if (+(elements & SceneElement::Indices))
		{
			IndexBuffer _copy FromPtr(desc->Indices().Data);
			manager _load AllToGPU(IndexBuffer);
		}

		if (+(elements & SceneElement::Camera))
		{
			float4x4 proj, view;
			scene->getCamera().GetMatrices(render_target->Width, render_target->Height, view, proj);
			Transforms _copy FromValue(TransformsCB{
					mul(inverse(proj), inverse(view))
			});
			manager _load AllToGPU(Transforms);
		}

		if (+(elements & SceneElement::GeometryTransforms))
		{
			GeometryTransforms _copy FromPtr(desc->getTransformsBuffer().Data);
			manager _load AllToGPU(GeometryTransforms);
		}

		if (+(elements & SceneElement::InstanceTransforms))
		{
			float4x4* transforms = new float4x4[desc->Instances().Count];
			for (int i = 0; i < desc->Instances().Count; i++)
				transforms[i] = desc->Instances().Data[i].Transform;

			InstanceTransforms _copy FromPtr(transforms);
			manager _load AllToGPU(InstanceTransforms);
			delete[] transforms;
		}
	}

	gObj<InstanceCollection> rtxScene;

	void CreateSceneOnGPU(gObj<RaytracingManager> manager) {
		
		auto desc = scene->getScene();
		
		rtxScene = manager _create Intances();
		for (int i = 0; i < desc->Instances().Count; i++)
		{
			auto instance = desc->Instances().Data[i];

			auto geometryCollection = manager _create TriangleGeometries();
			geometryCollection _set Transforms(GeometryTransforms);
		
			for (int j = 0; i < instance.Count; i++) // load every geometry
			{
				auto geometry = desc->Geometries().Data[instance.GeometryIndices[j]];

				if (IndexBuffer)
					geometryCollection _create Geometry(
						VertexBuffer, IndexBuffer _create Slice(geometry.StartIndex, geometry.IndexCount), 
						geometry.TransformIndex);
				else
					geometryCollection _create Geometry(
						VertexBuffer _create Slice(geometry.StartVertex, geometry.VertexCount), 
						geometry.TransformIndex);
			}

			manager _load Geometry(geometryCollection);

			rtxScene _create Instance(geometryCollection,
				255U, 0, i, (float4x3)instance.Transform
			);
		}

		manager _load Scene(rtxScene, true, true);

		pipeline->Scene = rtxScene;
	}

	virtual void OnDispatch() override {
		// Update dirty elements
		__dispatch member_collector(UpdateDirtyElements);

		__dispatch member_collector(UpdateSceneOnGPU);

		// Draw current Frame
		__dispatch member_collector(DrawScene);
	}

	void UpdateSceneOnGPU(gObj<RaytracingManager> manager) {
		auto desc = scene->getScene();

		for (int i = 0; i < desc->Instances().Count; i++)
		{
			auto instance = desc->Instances().Data[i];

			rtxScene _load InstanceTransform(i, (float4x3)instance.Transform);
		}

		manager _load Scene(rtxScene, true, true);
	}

	void DrawScene(gObj<RaytracingManager> manager) {
		
		manager _set Pipeline(pipeline);
		manager _set Program(pipeline->MainProgram);

		static bool firstTime = true;
		if (firstTime) {
			manager _set RayGeneration(pipeline->Generating);
			manager _set Miss(pipeline->Missing, 0);
			firstTime = false;
		}

		manager _dispatch Rays(render_target->Width, render_target->Height);

		manager _copy Resource(render_target, OutputImage);
	}
};

#pragma endregion

