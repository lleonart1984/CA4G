#pragma once

#include "ca4g.h"
#include "GUITraits.h"

using namespace CA4G;

//typedef class AsyncSample main_technique;
//typedef class TriangleSample main_technique;
typedef class SceneSample main_technique;

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

		void Globals(gObj<GraphicsBinder> binder) {
			binder _set RTV(0, RenderTarget);

			binder _set PixelShaderBindings();
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

		void Globals(gObj<GraphicsBinder> binder) {
			binder _set RTV(0, RenderTarget);
			binder _set DSV(DepthBuffer);

			binder _set VertexShaderBindings();

			binder _set SRV(0, InstanceTransforms);
			binder _set SRV(1, GeometryTransforms);
			binder _set CBV(0, Camera);
		}

		void Locals(gObj<GraphicsBinder> binder) {
			binder _set VertexShaderBindings();
			
			binder _set CBV(1, ObjectInfo);
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

				manager _dispatch IndexedTriangles(geometry.IndexCount, geometry.StartIndex);
			}
		}
	}
};


#pragma endregion