#include "private_ca4g_presenter.h"

namespace CA4G {
	Presenter::Presenter(HWND hWnd):
		create(new Creating(this)),
		load(new Loading(this)),
		set(new Settings(this)),
		dispatch(new Dispatcher(this)) {

		__internalDXWrapper = new DX_Wrapper();
		__InternalState->hWnd = hWnd;
	}

	Presenter::~Presenter() {
		delete __internalDXWrapper;
	}

	void Presenter::Settings::Buffers(int count) {
		__PInternalState->swapChainDesc.BufferCount = count;
	}

	void Presenter::Settings::DXRFallback(bool useFallback) {
		__PInternalState->UseFallbackDevice = useFallback;
	}

	void Presenter::Settings::UseFrameBuffering(bool frameBuffering) {
		__PInternalState->UseFrameBuffering = frameBuffering;
	}

	void Presenter::Settings::Resolution(int width, int height) {
		__PInternalState->swapChainDesc.Width = width;
		__PInternalState->swapChainDesc.Height = height;
	}

	void Presenter::Settings::FullScreenMode(bool fullScreen) {
		__PInternalState->fullScreenDesc.Windowed = !fullScreen;
	}

	void Presenter::Settings::RT_Format(DXGI_FORMAT format) {
		__PInternalState->swapChainDesc.Format = format;
	}

	void Presenter::Settings::Sampling(int count, int quality) {
		__PInternalState->swapChainDesc.SampleDesc.Count = count;
		__PInternalState->swapChainDesc.SampleDesc.Quality = quality;
	}

	void Presenter::Settings::WarpDevice(bool useWarpDevice) {
		__PInternalState->UseWarpDevice = useWarpDevice;
	}

	// Helper function for acquiring the first available hardware adapter that supports Direct3D 12.
		// If no such adapter can be found, *ppAdapter will be set to nullptr.
	_Use_decl_annotations_
		void GetHardwareAdapter(CComPtr<IDXGIFactory4> pFactory, CComPtr<IDXGIAdapter1>& ppAdapter)
	{
		IDXGIAdapter1* adapter;
		ppAdapter = nullptr;

		for (UINT adapterIndex = 0; DXGI_ERROR_NOT_FOUND != pFactory->EnumAdapters1(adapterIndex, &adapter); ++adapterIndex)
		{
			DXGI_ADAPTER_DESC1 desc;
			adapter->GetDesc1(&desc);

			if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
			{
				// Don't select the Basic Render Driver adapter.
				// If you want a software adapter, pass in "/warp" on the command line.
				continue;
			}

			// Check to see if the adapter supports Direct3D 12, but don't create the
			// actual device yet.
			if (SUCCEEDED(D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr)))
			{
				break;
			}
		}

		ppAdapter = adapter;
	}

	// Enable experimental features required for compute-based raytracing fallback.
	// This will set active D3D12 devices to DEVICE_REMOVED state.
	// Returns bool whether the call succeeded and the device supports the feature.
	inline bool EnableComputeRaytracingFallback(CComPtr<IDXGIAdapter1> adapter)
	{
		ID3D12Device* testDevice;
		UUID experimentalFeatures[] = { D3D12ExperimentalShaderModels };

		return SUCCEEDED(D3D12EnableExperimentalFeatures(ARRAYSIZE(experimentalFeatures), experimentalFeatures, nullptr, nullptr))
			&& SUCCEEDED(D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&testDevice)));
	}

	// Returns bool whether the device supports DirectX Raytracing tier.
	inline bool IsDirectXRaytracingSupported(IDXGIAdapter1* adapter)
	{
		ID3D12Device* testDevice;
		D3D12_FEATURE_DATA_D3D12_OPTIONS5 featureSupportData = {};

		return SUCCEEDED(D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&testDevice)))
			&& SUCCEEDED(testDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &featureSupportData, sizeof(featureSupportData)))
			&& featureSupportData.RaytracingTier != D3D12_RAYTRACING_TIER_NOT_SUPPORTED;
	}


	void Presenter::Settings::SwapChain() {
		UINT dxgiFactoryFlags = 0;
		DX_Device device;

#if defined(_DEBUG)
		// Enable the debug layer (requires the Graphics Tools "optional feature").
		// NOTE: Enabling the debug layer after device creation will invalidate the active device.
		{
			if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&__PInternalState->debugController))))
			{
				__PInternalState->debugController->EnableDebugLayer();

				// Enable additional debug layers.
				dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
			}
		}
#endif

		CComPtr<IDXGIFactory4> factory;
#if _DEBUG
		CComPtr<IDXGIInfoQueue> dxgiInfoQueue;
		if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiInfoQueue))))
		{
			CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(&factory));
			dxgiInfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR, true);
			dxgiInfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION, true);
		}
		else
			CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&factory));
#else
		CreateDXGIFactory2(0, IID_PPV_ARGS(&factory));
#endif

		if (__PInternalState->UseWarpDevice)
		{
			CComPtr<IDXGIAdapter> warpAdapter;
			factory->EnumWarpAdapter(IID_PPV_ARGS(&warpAdapter));

			D3D12CreateDevice(
				warpAdapter,
				D3D_FEATURE_LEVEL_12_1,
				IID_PPV_ARGS(&device)
			);
		}
		else
		{
			CComPtr<IDXGIAdapter1> hardwareAdapter;
			GetHardwareAdapter(factory, hardwareAdapter);

			EnableComputeRaytracingFallback(hardwareAdapter);

			D3D12CreateDevice(
				hardwareAdapter,
				D3D_FEATURE_LEVEL_11_0,
				IID_PPV_ARGS(&device)
			);
		}

		D3D12_FEATURE_DATA_D3D12_OPTIONS ops;
		device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &ops, sizeof(ops));

		__PInternalState->device = device; // Device created.

		__PInternalState->scheduler = new GPUScheduler(device, __PInternalState->swapChainDesc.BufferCount, 8);

		IDXGISwapChain1* tmpSwapChain;

		factory->CreateSwapChainForHwnd(
			__PInternalState->scheduler->Engines[0].queue->dxQueue,        // Swap chain needs the queue so that it can force a flush on it.
			__PInternalState->hWnd,
			&__PInternalState->swapChainDesc,
			&__PInternalState->fullScreenDesc,
			nullptr,
			&tmpSwapChain
		);

		__PInternalState->swapChain = (IDXGISwapChain3*)tmpSwapChain;
		__PInternalState->swapChain->SetMaximumFrameLatency(__PInternalState->swapChainDesc.BufferCount);

		// This sample does not support fullscreen transitions.
		factory->MakeWindowAssociation(__PInternalState->hWnd, DXGI_MWA_NO_ALT_ENTER);

		//// Create rendertargets resources.
		//{
		//	renderTargetViews = new gObj<Texture2D>[buffers];
		//	// Create a RTV and a command allocator for each frame.
		//	for (UINT n = 0; n < buffers; n++)
		//	{
		//		DX_Resource rtResource;
		//		swapChain->GetBuffer(n, IID_PPV_ARGS(&rtResource));

		//		gObj<ResourceWrapper> rtResourceWrapper = new ResourceWrapper(manager, rtResource->GetDesc(), rtResource, D3D12_RESOURCE_STATE_COPY_DEST);
		//		renderTargetViews[n] = new Texture2D(rtResourceWrapper);
		//	}
		//}

		//CurrentBuffer = swapChain->GetCurrentBackBufferIndex();
		//manager->BackBuffer = renderTargetViews[CurrentBuffer];

		//renderTargetDescriptorSlot = manager->descriptors->gui_csu->MallocPersistent();

	}
}