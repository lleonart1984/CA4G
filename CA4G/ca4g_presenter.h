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
#include <atlcomcli.h>


namespace CA4G {

	class Tagging {
		void* __Tag;
		int* references;
	public:
		template<typename T>
		Tagging(T data) {
			__Tag = new T(data);
			references = new int(1);
		}
		
		Tagging(const Tagging& other) {
			this->__Tag = other.__Tag;
			this->references = other.references;
			if (other.__Tag != nullptr)
				(*this->references)++;
		}

		Tagging() {
			__Tag = nullptr;
			references = nullptr;
		}

		~Tagging() {
			if (__Tag != nullptr) {
				(*references)--;
				if (*references == 0)
					delete __Tag;
			}
		}

		Tagging& operator = (const Tagging& other) {
			if (__Tag != nullptr)
			{
				(*references)--;
				if (*references == 0)
					delete __Tag;
			}
			this->references = other.references;
			this->__Tag = other.__Tag;
			if (__Tag != nullptr)
			{
				(*references)++;
			}
			return *this;
		}

		template<typename T>
		T getData() {
			return *((T*)__Tag);
		}
	};

	/// Base type for all types of engines
	class CommandListManager : public ICmdManager {
		friend GPUScheduler;
		Tagging __Tag;

	public:
		virtual ~CommandListManager() { }
		// Tag data when the process was enqueue
		template<typename T>
		inline T Tag() { return __Tag.getData<T>(); }
	};

	class CopyManager : public CommandListManager {
		friend GPUScheduler;
	protected:
		CopyManager() :
			clear(new Clearing(this)),
			load(new Loading(this)),
			copy(new Copying(this)) {
		}
	public:
		~CopyManager() {
			delete clear;
			delete load;
		}

		// Allows access to all clearing functions for Resources
		class Clearing {
			friend CopyManager;

		protected:
			ICmdManager* wrapper;
			Clearing(ICmdManager* wrapper) :wrapper(wrapper) {}
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
		class Loading {
			friend CopyManager;

			ICmdManager* wrapper;

			Loading(ICmdManager* wrapper) : wrapper(wrapper) {}

		public:
			// Updates all subresources from mapped memory to the gpu
			void AllToGPU(gObj<ResourceView> resource);
			// Updates all subresources from the gpu to mapped memory
			void AllFromGPU(gObj<ResourceView> resource);

			// Updates a region of a single subresource from mapped memory to the gpu
			void RegionToGPU(gObj<ResourceView> singleSubresource, const D3D12_BOX &region);
			
			// Updates a region of a single subresource from the gpu to mapped memory
			void RegionFromGPU(gObj<ResourceView> singleSubresource, const D3D12_BOX &region);
		}
		* const load;


		class Copying {
			friend CopyManager;
			CopyManager* manager;
			Copying(CopyManager* manager) :manager(manager) {}
		public:
			void Resource(gObj<Texture2D> dst, gObj<Texture2D> src);
		}* const copy;
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
			ICmdManager* wrapper;
		public:
			Setter(ICmdManager* wrapper) :wrapper(wrapper) {
			}
			// Sets a graphics pipeline
			void Pipeline(gObj<IPipelineBindings> pipeline);
		}* const set;

		class Dispatcher {
			ICmdManager* wrapper;
		public:
			Dispatcher(ICmdManager* wrapper) :wrapper(wrapper) {
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
			ICmdManager* wrapper;
		public:
			Setter(ICmdManager* wrapper) : wrapper(wrapper) {
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

			Clearing(ICmdManager* wrapper) : CopyManager::Clearing(wrapper) {}
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
			ICmdManager* wrapper;
		public:
			Dispatcher(ICmdManager* wrapper) :wrapper(wrapper) {
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

	struct DX_BakedGeometry;
	struct DX_BakedScene;
	struct DX_GeometryCollectionWrapper;
	struct DX_InstanceCollectionWrapper;

	enum class CollectionState {
		// The collection has not been built yet, there is not any GPU version of it.
		NotBuilt,
		// The collection has been built and doesnt need any update
		UpToDate,
		// The collection has a GPU version but some elements might be updated.
		NeedsUpdate,
		// The collection has a GPU version with a different structure.
		// Needs to be rebuilt to update dirty elements.
		NeedsRebuilt
	};

	class InstanceCollection;
	class GeometryCollection {
		friend RaytracingManager;
		friend InstanceCollection;
	protected:
		DX_GeometryCollectionWrapper* wrapper = nullptr;
		GeometryCollection() {}
	public:
		virtual ~GeometryCollection() {
			delete wrapper;
		}

		CollectionState State();

		void ForceState(CollectionState state);

		int Count();

		// Clears this collection
		void Clear();
	};

	class TriangleGeometryCollection : public GeometryCollection {
		friend RaytracingManager;
		TriangleGeometryCollection() :
			load(new Loading(this)), 
			set(new Setting(this)),
		create(new Creating(this)){}
	public:
		class Loading {
			friend TriangleGeometryCollection;
			TriangleGeometryCollection* manager;
			Loading(TriangleGeometryCollection* manager) :manager(manager) {}
		public:
			void Vertices(int geometryID, gObj<Buffer> newVertices);

			void Transform(int geometryID, int transformIndex);
		}* const load;

		class Creating {
			friend TriangleGeometryCollection;
			TriangleGeometryCollection* manager;
			Creating(TriangleGeometryCollection* manager):manager(manager){}
		public:
			int Geometry(gObj<Buffer> vertices, int transformIndex = -1);
			
			int Geometry(gObj<Buffer> vertices, gObj<Buffer> indices, int transformIndex = -1);
		}* const create;

		class Setting {
			friend TriangleGeometryCollection;
			TriangleGeometryCollection* manager;
			Setting(TriangleGeometryCollection* manager) :manager(manager) {}
			void __SetInputLayout(VertexElement* elements, int count);
		public:
			template<int count>
			void VertexLayout(VertexElement(&layout)[count]) {
				__SetInputLayout((VertexElement*)&layout, count);
			}
			void VertexLayout(std::initializer_list<VertexElement> layout) {
				__SetInputLayout((VertexElement*)layout.begin(), layout.size());
			}

			void Transforms(gObj<Buffer> transforms);

		}* const set;
	};

	class ProceduralGeometryCollection : public GeometryCollection {
		friend RaytracingManager;
		ProceduralGeometryCollection() :GeometryCollection(),
		create(new Creating(this)),
		load(new Loading(this)){}
	public:
		class Loading {
			friend ProceduralGeometryCollection;
			ProceduralGeometryCollection* manager;
			Loading(ProceduralGeometryCollection* manager) :manager(manager) {}
		public:
			void Boxes(int start, gObj<Buffer> newBoxes);
		}* const load;

		class Creating {
			friend ProceduralGeometryCollection;
			ProceduralGeometryCollection* manager;
			Creating(ProceduralGeometryCollection* manager) :manager(manager) {}
		public:
			int Geometry(gObj<Buffer> boxes);
		} * const create;
	};

	class InstanceCollection {
		friend RaytracingManager;
		friend class InternalBindings;

		DX_InstanceCollectionWrapper* wrapper = nullptr;
		InstanceCollection() :load(new Loading(this)),
		create(new Creating(this)){}
	public:
		class Loading {
			friend InstanceCollection;
			InstanceCollection* manager;
			Loading(InstanceCollection* manager) :manager(manager) {}
		public:
			// Updates the geometry attribute for a specific instance
			void InstanceGeometry(int instance, gObj<GeometryCollection> geometries);
			// Updates the mask attribute for a specific instance
			void InstanceMask(int instance, UINT mask);
			// Updates the contribution attribute for a specific instance
			void InstanceContribution(int instance, int instanceContribution);
			// Updates the instance ID attribute for a specific instance
			void InstanceID(int instance, UINT instanceID);
			// Updates the transform attribute for a specific instance
			void InstanceTransform(int instance, float4x3 transform);
		}* const load;

		class Creating {
			friend InstanceCollection;
			InstanceCollection* manager;
			Creating(InstanceCollection* manager) :manager(manager) {}
		public:
			// Adds a new instance to the collection.
			// Returns the number of instances.
			int Instance(gObj<GeometryCollection> geometry,
				UINT mask = 255U,
				int contribution = 0,
				UINT instanceID = INTSAFE_UINT_MAX,
				float4x3 transform = float4x3(1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0));
		}* const create;

		int Count();

		CollectionState State();

		void ForceState(CollectionState state);

		// Clear this collection
		void Clear();
	};

	class RaytracingManager : public GraphicsManager {
		friend GPUScheduler;
	protected:
		RaytracingManager() : GraphicsManager(), 
			set(new Setter(this)) ,
			dispatch(new Dispatcher(this)),
			create(new Creating(this)),
			load (new Loading(this))
		{
		}
	public:
		class Creating {
			friend RaytracingManager;
			RaytracingManager* manager;
			Creating(RaytracingManager* manager) :manager(manager) {}
		public:
			gObj<TriangleGeometryCollection> TriangleGeometries();
			gObj<ProceduralGeometryCollection> ProceduralGeometries();
			gObj<InstanceCollection> Intances();

			gObj<GeometryCollection> Attach(gObj<GeometryCollection> collection);
		}* const create;

		class Loading {
			friend RaytracingManager;
			RaytracingManager* manager;
			Loading(RaytracingManager* manager) :manager(manager) {}
		public:
			// Depending on state, builds, updates or rebuilds current collection into GPU buffers.
			void Geometry(gObj<GeometryCollection> geometries,
				bool allowUpdate = false, bool preferFastRaycasting = true);
			// Depending on state, builds, updates or rebuilds current collection into GPU buffers.
			void Scene(gObj<InstanceCollection> instances,
				bool allowUpdate = false, bool preferFastRaycasting = true);
		}* const load;

		class Setter {
			ICmdManager* wrapper;
		public:
			Setter(ICmdManager* wrapper) :wrapper(wrapper) {
			}
			// Sets the pipeline bindings object on the GPU.
			void Pipeline(gObj<RaytracingPipelineBindings> bindings);

			// Activate this program to be used when dispatching rays.
			void Program(gObj<RTProgramBase> program);

			// Commit all local bindings for this ray generation shader
			void RayGeneration(gObj<RayGenerationHandle> shader);

			// Commit all local bindings for this miss shader
			void Miss(gObj<MissHandle> shader, int index);

			// Commit all local bindings for this shader group
			void HitGroup(gObj<HitGroupHandle> group, int geometryIndex,
				int rayContribution = 0, int multiplier = 1, int instanceContribution = 0);
		}* const set;

		class Dispatcher {
			friend RaytracingManager;
			RaytracingManager* manager;
			Dispatcher(RaytracingManager* manager) :manager(manager) {}
		public:
			void Rays(int width, int height, int depth = 1);
		}* const dispatch;
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
		static const Engine Type = Engine::Direct;
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

		IDXDeviceManager* wrapper;
		Dispatcher(IDXDeviceManager* wrapper) :wrapper(wrapper) {}
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

		// Finish a frame in scheduler flushing all pending work.
		// Prepares the render target to be presented.
		// Present a back buffer in the swap chain.
		void BackBuffer();

		// Prepares the render target.
		void RenderTarget();
	};

	struct InternalDXInfo {
		HWND hWnd;
		CComPtr<ID3D12Device5> device = nullptr;
		CComPtr<IDXGISwapChain3> swapChain = nullptr;
		CComPtr<ID3D12GraphicsCommandList4> mainCmdList = nullptr;
		int Buffers;
		DXGI_FORMAT RenderTargetFormat;
		D3D12_CPU_DESCRIPTOR_HANDLE* RenderTargets;
	};

	class Presenter : public IDXDeviceManager {
		gObj<InternalDXInfo> internalInfo = nullptr;
	public:
		// Creates a presenter object that creates de DX device attached to a specific window (hWnd).
		Presenter(HWND hWnd, gObj<InternalDXInfo> &internalInfo);
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