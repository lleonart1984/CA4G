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

		// Inherited via GraphicsPipelineBindings
		void Setup() override {
			__set VertexShader(ShaderLoader::FromFile("./Demo_VS.cso"));
			__set InputLayout({
					VertexElement { VertexElementType::Float, 3, "POSITION" }
				});
			__set PixelShader(ShaderLoader::FromFile("./Demo_PS.cso"));
			//__set NoDepthTest();
			//__set CullMode();
			//__set FillMode();
		}

		void Globals(gObj<GraphicsBinder> binder) {
			binder _set RTV(0, RenderTarget);
		}
	};
	gObj<Pipeline> pipeline;

	// Inherited via Technique
	void OnLoad() override {
		vertexBuffer = __create Buffer_VB<Vertex>(3);
		indexBuffer = __create Buffer_IB<int>(3);
		pipeline = __create Pipeline<Pipeline>();
		
		__dispatch member_collector(CreateAssets);
	}

	void CreateAssets(gObj<CopyManager> manager) {
		manager _copy List(vertexBuffer, {
			Vertex { float3(-1, 0, 0.5) },
			Vertex { float3(1, 0, 0.5) },
			Vertex { float3(0, 1, 0.5) }
			});
		manager _copy List(indexBuffer, {
			 0, 1, 2
			});
	}

	void OnDispatch() override {
		__dispatch member_collector(DrawTriangle);
	}

	void DrawTriangle(gObj<GraphicsManager> manager) {

		static int frame = 0;
		frame++;

		pipeline->RenderTarget = render_target;
		manager _set Pipeline(pipeline);
		manager _set VertexBuffer(vertexBuffer);
		manager _set IndexBuffer(indexBuffer);
		manager _set Viewport(render_target->Width, render_target->Height);
		
		manager _clear RT(render_target, float3(0.2f, sin(frame*0.001), 0.5f));

		manager _dispatch Triangles(3);
	}

};


#pragma endregion