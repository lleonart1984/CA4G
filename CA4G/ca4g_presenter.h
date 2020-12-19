#ifndef CA4G_PRESENTER_H
#define CA4G_PRESENTER_H

#include "ca4g_definitions.h"
#include "ca4g_errors.h"
#include "ca4g_memory.h"
#include "ca4g_techniques.h"
#include "ca4g_dxr_support.h"

#ifndef CA4G_NUMBER_OF_WORKERS
#define CA4G_NUMBER_OF_WORKERS 8
#endif // !CA4G_NUMBER_OF_WORKERS


namespace CA4G {

	/// Base type for all types of engines
	class CommandListManager : public ICmdWrapper {
		friend GPUScheduler;
		gObj<TagData> __Tag;

		
	public:
		virtual ~CommandListManager() { }
		// Tag data when the process was enqueue
		inline gObj<TagData> Tag() { return __Tag; }
	};

	class CopyManager : public CommandListManager {
		friend GPUScheduler;
	protected:
		CopyManager():clear(new Clearing(this)), copy(new Copying(this)){}
	public:
		~CopyManager() {
			delete clear;
			delete copy;
		}

		// Allows access to all clearing functions for Resources
		class Clearing {
			friend CopyManager;

		protected:
			ICmdWrapper* wrapper;
			Clearing(ICmdWrapper* wrapper) :wrapper(wrapper) {}
		public:

			void UAV(gObj<ResourceView> uav, const FLOAT values[4]);
			void UAV(gObj<ResourceView> uav, const unsigned int values[4]);
			void UAV(gObj<ResourceView> uav, const float4& value) {
				float v[4]{ value.x, value.y, value.z, value.w };
				UAV(uav, v);
			}
			inline void UAV(gObj<ResourceView> uav, const unsigned int& value) {
				unsigned int v[4]{ value, value, value, value };
				UAV(uav, v);
			}
			inline void UAV(gObj<ResourceView> uav, const uint4& value) {
				unsigned int v[4]{ value.x, value.y, value.z, value.w };
				UAV(uav, v);
			}
		}
		* const clear;

		// Allows access to all copying functions from and to Resources
		class Copying {
			friend CopyManager;

			ICmdWrapper* wrapper;

			// Data is given by a reference using a pointer
			void FullCopyToSubresource(gObj<ResourceView> dst, void* data, const D3D12_BOX* box = nullptr);

			void FullCopyFromSubresource(gObj<ResourceView> dst, void* data, const D3D12_BOX* box = nullptr);
			
			Copying(ICmdWrapper* wrapper) : wrapper(wrapper) {}

		public:

			// Copies the data from a ptr to the buffer at specific range
			template<typename T>
			void Ptr(gObj<Buffer> dst, T* data, int offset = 0, int size = -1) {
				if (size == -1)
					size = dst->getSizeInBytes() - offset;

				FullCopyToSubresource(dst, data, D3D12_BOX{
						offset, 0, 0,
						offset + size, 1, 1
					});
			}

			// Copies the data from a ptr to a specific subresource at specific region
			template<typename T>
			void Ptr(gObj<Texture1D> dst, int mip, int slice, T* data, const D3D12_BOX* box = nullptr) {
				gObj<Texture1D> subresource = dst->SliceArray(slice, 1)->SliceMips(mip, 1);
				FullCopyToSubresource(subresource, data, box);
			}

			// Copies the data from a ptr to a specific subresource at specific region
			template<typename T>
			void Ptr(gObj<Texture2D> dst, int mip, int slice, T* data, const D3D12_BOX* box = nullptr) {
				gObj<Texture2D> subresource = dst->SliceArray(slice, 1)->SliceMips(mip, 1);
				FullCopyToSubresource(subresource, data, box);
			}

			// Copies the data from a ptr to a specific subresource at specific region
			template<typename T>
			void Ptr(gObj<Texture3D> dst, int mip, T* data, const D3D12_BOX* box = nullptr) {
				gObj<Texture2D> subresource = dst->SliceMips(mip, 1);
				FullCopyToSubresource(subresource, data, box);
			}

			// Copies the data from a ptr to a full resource 
			template<typename T>
			void Ptr(gObj<Texture1D> dst, T* data) {
				FullCopyToSubresource(dst, data, nullptr);
			}

			// Copies the data from a ptr to a full resource
			template<typename T>
			void Ptr(gObj<Texture2D> dst, T* data) {
				FullCopyToSubresource(dst, data, nullptr);
			}

			// Copies the data from a ptr to a full resource
			template<typename T>
			void Ptr(gObj<Texture3D> dst, T* data) {
				FullCopyToSubresource(dst, data, nullptr);
			}

			// Data is given by an initializer list
			template<typename T>
			void List(gObj<Buffer> dst, std::initializer_list<T> data) {
				FullCopyToSubresource(dst, data.begin());
			}

			// Data is given expanded in a value object
			template<typename T>
			void Value(gObj<Buffer> dst, const T& data) {
				FullCopyToSubresource(dst, &data);
			}

			void Resource(gObj<ResourceView> dst, gObj<ResourceView> src);
		}
		* const copy;
	};

	class ComputeManager : public CopyManager
	{
		friend GPUScheduler;
	protected:
		ComputeManager() : CopyManager(),
			set(new Setter(this)),
			dispatch(new Dispatcher(this))
		{
		}
	public:
		~ComputeManager() {
			delete set;
			delete dispatch;
		}
		class Setter {
			ICmdWrapper* wrapper;
		public:
			Setter(ICmdWrapper* wrapper) :wrapper(wrapper) {
			}
			// Sets a graphics pipeline
			void Pipeline(gObj<IPipelineBindings> pipeline);
		}* const set;

		class Dispatcher {
			ICmdWrapper* wrapper;
		public:
			Dispatcher(ICmdWrapper* wrapper) :wrapper(wrapper) {
			}

			// Dispatches a specific number of threads to execute current compute shader set.
			void Threads(int dimx, int dimy = 1, int dimz = 1);
		}* const dispatch;
	};

	class GraphicsManager : public ComputeManager {
		friend GPUScheduler;
	protected:
		GraphicsManager() :ComputeManager(),
			set(new Setter(this)),
			dispatch(new Dispatcher(this)),
			clear(new Clearing(this))
		{
		}
	public:
		class Setter {
			ICmdWrapper* wrapper;
		public:
			Setter(ICmdWrapper* wrapper) : wrapper(wrapper) {
			}

			// Sets a graphics pipeline
			void Pipeline(gObj<IPipelineBindings> pipeline);

			// Changes the viewport
			void Viewport(float width, float height, float maxDepth = 1.0f, float x = 0, float y = 0, float minDepth = 0.0f);

			// Sets a vertex buffer in a specific slot
			void VertexBuffer(gObj<Buffer> buffer, int slot = 0);

			// Sets an index buffer
			void IndexBuffer(gObj<Buffer> buffer);
		}* const set;

		class Clearing : public CopyManager::Clearing {
			friend GraphicsManager;

			Clearing(ICmdWrapper* wrapper) : CopyManager::Clearing(wrapper) {}
		public:
			void RT(gObj<Texture2D> rt, const FLOAT values[4]);
			inline void RT(gObj<Texture2D> rt, const float4& value) {
				float v[4]{ value.x, value.y, value.z, value.w };
				RT(rt, v);
			}
			// Clears the render target accessed by the Texture2D View with a specific float3 value
			inline void RT(gObj<Texture2D> rt, const float3& value) {
				float v[4]{ value.x, value.y, value.z, 1.0f };
				RT(rt, v);
			}
			void DepthStencil(gObj<Texture2D> depthStencil, float depth, UINT8 stencil, D3D12_CLEAR_FLAGS flags = D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL);
			inline void Depth(gObj<Texture2D> depthStencil, float depth = 1.0f) {
				DepthStencil(depthStencil, depth, 0, D3D12_CLEAR_FLAG_DEPTH);
			}
			inline void Stencil(gObj<Texture2D> depthStencil, UINT8 stencil) {
				DepthStencil(depthStencil, 1, stencil, D3D12_CLEAR_FLAG_STENCIL);
			}
		}
		* const clear;

		class Dispatcher {
			ICmdWrapper* wrapper;
		public:
			Dispatcher(ICmdWrapper* wrapper) :wrapper(wrapper) {
			}

			// Draws a primitive using a specific number of vertices
			void Primitive(D3D_PRIMITIVE_TOPOLOGY topology, int count, int start = 0);

			// Draws a primitive using a specific number of vertices
			void IndexedPrimitive(D3D_PRIMITIVE_TOPOLOGY topology, int count, int start = 0);

			// Draws triangles using a specific number of vertices
			void Triangles(int count, int start = 0) {
				Primitive(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST, count, start);
			}

			void IndexedTriangles(int count, int start = 0) {
				IndexedPrimitive(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST, count, start);
			}
		}* const dispatch;

	};

	class RaytracingManager : public GraphicsManager {
		friend GPUScheduler;
	protected:
		RaytracingManager() : GraphicsManager(), set(new Setter(this)) 
		{
		}
	public:
		class Setter {
			ICmdWrapper* wrapper;
		public:
			Setter(ICmdWrapper* wrapper) :wrapper(wrapper) {
			}
			void Pipeline(gObj<IPipelineBindings> bindings);

			void Program(gObj<IRTProgram> program);

			// Commit all local bindings for this ray generation shader
			void RayGeneration(gObj<RayGenerationHandle> program);

			void Miss(gObj<MissHandle> program, int index);

			void HitGroup(gObj<HitGroupHandle> group, int geometryIndex,
				int rayContribution = 0, int multiplier = 1, int instanceContribution = 0);
		}* const set;

	};

	/// Base type for any graphic process that populates a command list.
	class IGPUProcess {
		friend GPUScheduler;
	protected:
		// When implemented, populate the specific command list with the instructions to draw.
		virtual void OnCollect(gObj<CommandListManager> manager) = 0;
		// Gets the type of engine required by this process
		virtual Engine RequiredEngine() = 0;
	};

	template<typename A>
	struct EngineType {
		static const Engine Type;
	};
	template<>
	struct EngineType<GraphicsManager> {
		static const Engine Type = Engine::Direct;
	};
	template<>
	struct EngineType<RaytracingManager> {
		static const Engine Type = Engine::Raytracing;
	};
	template<>
	struct EngineType<ComputeManager> {
		static const Engine Type = Engine::Compute;
	};
	template<>
	struct EngineType<CopyManager> {
		static const Engine Type = Engine::Copy;
	};

	template<typename T, typename A>
	struct CallableMember : public IGPUProcess {
		T* instance;
		typedef void(T::* Member)(gObj<A>);
		Member function;
		CallableMember(T* instance, Member function) : instance(instance), function(function) {
		}

		void OnCollect(gObj<CommandListManager> manager) {
			(instance->*function)(manager.Dynamic_Cast<A>());
		}

		Engine RequiredEngine() {
			return EngineType<A>::Type;
		}
	};

	class Dispatcher {
		friend Presenter;
		friend Technique;

		IDXWrapper* wrapper;
		Dispatcher(IDXWrapper* wrapper) :wrapper(wrapper) {}
	public:
		void TechniqueObj(gObj<Technique> technique) {
			technique->OnDispatch();
		}

		void GPUProcess(gObj<IGPUProcess> process);

		void GPUProcess_Async(gObj<IGPUProcess> process);

		template<typename T>
		void Collector(T* instance, typename CallableMember<T, CopyManager>::Member member) {
			GPUProcess(new CallableMember<T, CopyManager>(instance, member));
		}

		template<typename T>
		void Collector(T* instance, typename CallableMember<T, ComputeManager>::Member member) {
			GPUProcess(new CallableMember<T, ComputeManager>(instance, member));
		}

		template<typename T>
		void Collector(T* instance, typename CallableMember<T, GraphicsManager>::Member member) {
			GPUProcess(new CallableMember<T, GraphicsManager>(instance, member));
		}

		template<typename T>
		void Collector(T* instance, typename CallableMember<T, RaytracingManager>::Member member) {
			GPUProcess(new CallableMember<T, RaytracingManager>(instance, member));
		}

		template<typename T>
		void Collector_Async(T* instance, typename CallableMember<T, CopyManager>::Member member) {
			GPUProcess_Async(new CallableMember<T, CopyManager>(instance, member));
		}

		template<typename T>
		void Collector_Async(T* instance, typename CallableMember<T, ComputeManager>::Member member) {
			GPUProcess_Async(new CallableMember<T, ComputeManager>(instance, member));
		}

		template<typename T>
		void Collector_Async(T* instance, typename CallableMember<T, GraphicsManager>::Member member) {
			GPUProcess_Async(new CallableMember<T, GraphicsManager>(instance, member));
		}

		template<typename T>
		void Collector_Async(T* instance, typename CallableMember<T, RaytracingManager>::Member member) {
			GPUProcess_Async(new CallableMember<T, RaytracingManager>(instance, member));
		}

		void BackBuffer();
	};

	class Presenter : public IDXWrapper {

	public:
		// Creates a presenter object that creates de DX device attached to a specific window (hWnd).
		Presenter(HWND hWnd);
		~Presenter();

		Creating * const create;
		Loading * const load;
		Dispatcher * const dispatch;

		class Settings {
			friend Presenter;
			Presenter* presenter;
			Settings(Presenter* presenter) :presenter(presenter) {}
		public:
			// Sets the number of buffers in the swap chain.
			// Default value is 2
			void Buffers(int buffers);

			// Determines if frames are buffered.
			// Frame buffering allows to continue allocating next frame in another queues without sync on the CPU.
			// Normal usage is useFrameBuffering = true and Buffers = 3.
			void UseFrameBuffering(bool useFrameBuffering = true);

			// Set the resolution for the back buffers.
			void Resolution(int width, int height);

			// Set the resolution of the host window.
			void WindowResolution() {
				Resolution(0, 0);
			}

			// Set full screen mode.
			void FullScreenMode(bool fullScreen = true);

			// Set if use a warp device.
			void WarpDevice(bool usewrap = true);

			// Set if use a Raytracing Fallback device to emulate missing RTX support.
			void DXRFallback(bool forceFallback = true);

			// Set the Render Target format to use.
			void RT_Format(DXGI_FORMAT rtFormat = DXGI_FORMAT_R8G8B8A8_UNORM);

			// Set the multisample mode.
			void Sampling(int count = 1, int quality = 0);

			// This method should be called last. This method will create the swapchain and the DirectX12 device.
			void SwapChain();
		}* const set;
	};
}

#endif