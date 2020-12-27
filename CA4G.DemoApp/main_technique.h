#pragma once

#include "ca4g.h"

using namespace CA4G;

//typedef class AsyncSample main_technique;
typedef class TriangleSample main_technique;

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
			__set VertexShader(ShaderLoader::FromFile("./Demo_VS.cso"));
			__set PixelShader(ShaderLoader::FromFile("./Demo_PS.cso"));
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