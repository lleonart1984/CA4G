#ifndef CA4G_TECHNIQUE_H
#define CA4G_TECHNIQUE_H

#include "ca4g_definitions.h"
#include "ca4g_resources.h"

namespace CA4G {
	
	// Represents the object manages a DX12 device internally for rendering purposes.
	class IDXDeviceManager {
		friend Presenter;
		friend Technique;
		friend Creating;
		friend Loading;
		friend Dispatcher;
		friend Signal;
		template<typename ...PSS> friend class StaticPipelineBindings;
	protected:
		// Gets the internal DX12 core object
		DX_Wrapper* __InternalDXWrapper;

	public:
		// Gets the current render target active in the swapchain
		gObj<Texture2D> GetRenderTarget();
	};

	// Represents the object manages a DX12 command list internally for populating purposes.
	class ICmdManager {
		friend GPUScheduler;
		friend CopyManager;
		friend GraphicsManager;
		friend StaticPipelineBindingsBase;
	protected:
		DX_CmdWrapper* __InternalDXCmdWrapper;
	};

	// Represents the different engines can be used to work with the GPU.
	enum class Engine : int {
		// Engine used to generate graphics using rasterization with vertex, hull, domain, geometry and pixel shaders.
		Direct = 0,
		// Engine used for parallel computation using compute shaders
		Compute = 1,
		// Engine used for copying on the GPU.
		Copy = 2,
		// Engine used for ray-tracing programs using libraries. (This engine uses the same queue that graphics).
		Raytracing = 3
	};

	class IPipelineBindings {
		friend Creating;
		friend Dispatcher;
		friend CopyManager;
		friend GraphicsManager;
		friend RaytracingManager;
	protected:
		virtual Engine GetEngine() = 0;
		virtual void OnCreate(DX_Wrapper* dxWrapper) = 0;
		virtual void OnSet(ICmdManager* cmdWrapper) = 0;
		virtual void OnDispatch(ICmdManager* cmdWrapper) = 0;
	};

	enum class EngineMask : int {
		Direct = 1,
		Compute = 2,
		Copy = 4,
		Raytracing = 8,
		All = 15
	};

	struct Signal {
		friend Dispatcher;
		friend GPUScheduler;
	private:
		Signal(GPUScheduler* scheduler, long rallyPoints[4]) {
			this->scheduler = scheduler;
			this->rallyPoints[0] = rallyPoints[0];
			this->rallyPoints[1] = rallyPoints[1];
			this->rallyPoints[2] = rallyPoints[2];
			this->rallyPoints[3] = rallyPoints[3];
		}
		long rallyPoints[4] = { };
		GPUScheduler* scheduler = nullptr;
	public:
		Signal() {}
		void WaitFor();
	};

	class Technique : public IDXDeviceManager {
		friend Presenter;
		friend ResourceView;
		friend Buffer;
		friend Texture1D;
		friend Texture2D;
		friend Texture3D;
		friend Creating;

		void OnCreate(DX_Wrapper* deviceWrapper) {
			this->__InternalDXWrapper = deviceWrapper;
		}

	public:
		Creating* const create;
		Loading* const load;
		Dispatcher* const dispatch;

		class Setting {
			friend Technique;
			IDXDeviceManager* manager;
			Setting(IDXDeviceManager* manager) :manager(manager) {}
			
			void Tag(Tagging data);

		public:
			template<typename T>
			void TagValue(const T& data) {
				Tag(Tagging(data));
			}
		}* const set;
	
		virtual void OnLoad() = 0;

		virtual void OnDispatch() = 0;

	public:
		Technique();
		virtual ~Technique() {}
	};

	class Creating {
		friend Presenter;
		friend Technique;

		IDXDeviceManager* manager;
		Creating(IDXDeviceManager* manager) :manager(manager) {}

	public:

		// Tool method generic to create DX12 resources
		gObj<ResourceView> CustomDXResource(int elementWidth, const D3D12_RESOURCE_DESC& desc, D3D12_RESOURCE_STATES initialState,
			D3D12_CLEAR_VALUE* clearing = nullptr, CPUAccessibility cpuAccessibility = CPUAccessibility::None);

		// Creates a technique of specific type and passing some arguments to the constructor.
		template<typename T, typename... A>
		gObj<T> TechniqueObj(A... args);

		template<typename T, typename... A>
		gObj<T> Pipeline(A... args);

		Signal FlushAndSignal(EngineMask mask = EngineMask::All);

#pragma region Creating resources

		gObj<Buffer> Buffer_CB(int elementStride, bool dynamic);
		template <typename T>
		gObj<Buffer> Buffer_CB(bool dynamic = false) {
			return Buffer_CB(sizeof(T));
		}

		// Creates a buffer to store modifiable acceleration datastructure geometry and instance buffers.
		gObj<Buffer> Buffer_ADS(int elementStride, int count = 1);
		// Creates a buffer to store modifiable acceleration datastructure geometry and instance buffers.
		template <typename T>
		gObj<Buffer> Buffer_ADS(int count = 1) {
			return Buffer_ADS(sizeof(T), count);
		}

		// Creates a buffer to be used as a StructuredBuffer in a shader.
		gObj<Buffer> Buffer_SRV(int elementStride, int count = 1);
		// Creates a buffer to be used as a StructuredBuffer in a shader.
		template <typename T>
		gObj<Buffer> Buffer_SRV(int count = 1) {
			return Buffer_SRV(sizeof(T), count);
		}

		// Creates a buffer to be binded as a vertex buffer.
		gObj<Buffer> Buffer_VB(int elementStride, int count);
		// Creates a buffer to be binded as a vertex buffer.
		template <typename T>
		gObj<Buffer> Buffer_VB(int count) {
			return Buffer_VB(sizeof(T), count);
		}

		// Creates a buffer to be binded as an index buffer.
		gObj<Buffer> Buffer_IB(int elementStride, int count);
		// Creates a buffer to be binded as an index buffer.
		template <typename T>
		gObj<Buffer> Buffer_IB(int count) {
			throw new CA4GException("Error in type");
		}

		// Creates a buffer to be binded as an index buffer.
		template <>
		gObj<Buffer> Buffer_IB<int>(int count) {
			return Buffer_IB(4, count);
		}
		// Creates a buffer to be binded as an index buffer.
		template <>
		gObj<Buffer> Buffer_IB<unsigned int>(int count) {
			return Buffer_IB(4, count);
		}
		// Creates a buffer to be binded as an index buffer.
		template <>
		gObj<Buffer> Buffer_IB<short>(int count) {
			return Buffer_IB(2, count);
		}
		// Creates a buffer to be binded as an index buffer.
		template <>
		gObj<Buffer> Buffer_IB<unsigned short>(int count) {
			return Buffer_IB(2, count);
		}

		// Creates a buffer to be used as a RWStructuredBuffer in a shader.
		gObj<Buffer> Buffer_UAV(int elementStride, int count = 1);
		// Creates a buffer to be used as a RWStructuredBuffer in a shader.
		template <typename T>
		gObj<Buffer> Buffer_UAV(int count = 1) {
			return Buffer_UAV(sizeof(T), count);
		}

		// Creates a onedimensional texture to be used as a Texture1D in a shader.
		gObj<Texture1D> Texture1D_SRV(DXGI_FORMAT format, int width, int mips = 1, int arrayLength = 1);
		// Creates a onedimensional texture to be used as a Texture1D in a shader.
		template<typename T>
		gObj<Texture1D> Texture1D_SRV(int width, int mips = 1, int arrayLength = 1) {
			return Texture1D_SRV(Formats<T>::format, width, mips, arrayLength);
		}

		// Creates a onedimensional texture to be used as a RWTexture1D in a shader.
		// if mips == 0 all possible mips are allocated.
		gObj<Texture1D> Texture1D_UAV(DXGI_FORMAT format, int width, int mips = 0, int arrayLength = 1);
		// Creates a onedimensional texture to be used as a RWTexture1D in a shader.
		// if mips == 0 all possible mips are allocated.
		template<typename T>
		gObj<Texture1D> Texture1D_UAV(int width, int mips = 0, int arrayLength = 1) {
			return Texture1D_UAV(Formats<T>::Value, width, mips, arrayLength);
		}

		// Creates a bidimensional texture to be used as a Texture2D in a shader.
		// if mips == 0 all possible mips are allocated.
		gObj<Texture2D> Texture2D_SRV(DXGI_FORMAT format, int width, int height, int mips = 0, int arrayLength = 1);
		// Creates a bidimensional texture to be used as a Texture2D in a shader.
		// if mips == 0 all possible mips are allocated.
		template<typename T>
		gObj<Texture2D> Texture2D_SRV(int width, int height, int mips = 0, int arrayLength = 1) {
			return Texture2D_SRV(Formats<T>::Value, width, height, mips, arrayLength);
		}

		// Creates a bidimensional texture to be used as a RWTexture2D in a shader or a render target.
		// if mips == 0 all possible mips are allocated.
		gObj<Texture2D> Texture2D_UAV(DXGI_FORMAT format, int width, int height, int mips = 0, int arrayLength = 1);
		// Creates a bidimensional texture to be used as a RWTexture2D in a shader.
		// if mips == 0 all possible mips are allocated.
		template<typename T>
		gObj<Texture2D> Texture2D_UAV(int width, int height, int mips = 0, int arrayLength = 1) {
			return Texture2D_UAV(Formats<T>::Value, width, height, mips, arrayLength);
		}

		// Creates a bidimensional texture to be used exclusively as a render target.
		// if mips == 0 all possible mips are allocated.
		gObj<Texture2D> Texture2D_RT(DXGI_FORMAT format, int width, int height, int mips = 0, int arrayLength = 1);
		// Creates a bidimensional texture to be used exclusively as a render target.
		// if mips == 0 all possible mips are allocated.
		template<typename T>
		gObj<Texture2D> Texture2D_RT(int width, int height, int mips = 0, int arrayLength = 1) {
			return Texture2D_RT(Formats<T>::Value, width, height, mips, arrayLength);
		}

		// Creates a bidimensional texture to be used as exclusively as DepthStencil Buffer.
		gObj<Texture2D> Texture2D_DSV(int width, int height, DXGI_FORMAT format = DXGI_FORMAT_D32_FLOAT);

		// Creates a threedimensional texture to be used as a Texture3D in a shader.
		// if mips == 0 all possible mips are allocated.
		gObj<Texture3D> Texture3D_SRV(DXGI_FORMAT format, int width, int height, int depth, int mips = 0);
		// Creates a threedimensional texture to be used as a Texture3D in a shader.
		// if mips == 0 all possible mips are allocated.
		template<typename T>
		gObj<Texture3D> Texture3D_SRV(int width, int height, int depth, int mips = 0) {
			return Texture3D_SRV(Formats<T>::Value, width, height, depth, mips);
		}

		// Creates a threedimensional texture to be used as a RWTexture3D in a shader.
		gObj<Texture3D> Texture3D_UAV(DXGI_FORMAT format, int width, int height, int depth, int mips = 0);
		// Creates a threedimensional texture to be used as a RWTexture3D in a shader.
		template<typename T>
		gObj<Texture3D> Texture3D_UAV(int width, int height, int depth, int mips = 0) {
			return Texture3D_UAV(Formats<T>::Value, width, height, depth, mips);
		}

#pragma endregion
	};

	class Loading {
		friend Presenter;
		friend Technique;

		IDXDeviceManager* manager;
		Loading(IDXDeviceManager* manager) : manager(manager) {}
	public:
		void TechniqueObj(gObj<Technique> technique) {
			technique->OnLoad();
		}
	};

	template<typename T, typename... A>
	gObj<T> Creating::TechniqueObj(A... args) {
		auto subTechnique = new T(args...);
		subTechnique->OnCreate(manager->__InternalDXWrapper);
		return subTechnique;
	}

	template<typename T, typename... A>
	gObj<T> Creating::Pipeline(A... args) {
		auto pipeline = new T(args...);
		pipeline->OnCreate(manager->__InternalDXWrapper);
		return pipeline; 
	}
}

#endif