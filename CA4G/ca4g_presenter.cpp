#include "private_ca4g_presenter.h"

namespace CA4G {
	Presenter::Presenter(HWND hWnd):
		create(new Creating(this)),
		load(new Loading(this)),
		dispatch(new Dispatcher(this)), 
		set(new Settings(this)) {
		__InternalDXWrapper = new DX_Wrapper();
		__InternalState->hWnd = hWnd;
	}

	Presenter::~Presenter() {
		delete __InternalDXWrapper;
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
		__PInternalState->Initialize();
	}

	// uses the presenter to swap buffers and present.
	void Dispatcher::BackBuffer() {
		DX_Wrapper* wrapper = (DX_Wrapper*)this->wrapper->__InternalDXWrapper;

		// Wait for all 
		wrapper->scheduler->FinishFrame();
		
		auto hr = wrapper->swapChain->Present(0, 0);

		if (hr != S_OK) {
			HRESULT r = wrapper->device->GetDeviceRemovedReason();
			throw CA4GException::FromError(CA4G_Errors_Any, nullptr, r);
		}
		
		wrapper->scheduler->SetupFrame(wrapper->swapChain->GetCurrentBackBufferIndex());
	}

	void Dispatcher::GPUProcess(gObj<IGPUProcess> process) {
		DX_Wrapper* w = (DX_Wrapper*) wrapper->__InternalDXWrapper;
		w->scheduler->Enqueue(process);
	}

	void Dispatcher::GPUProcess_Async(gObj<IGPUProcess> process) {
		DX_Wrapper* w = (DX_Wrapper*) wrapper->__InternalDXWrapper;
		w->scheduler->EnqueueAsync(process);
	}

#pragma region DX_Wrapper

	D3D12_CPU_DESCRIPTOR_HANDLE DX_Wrapper::ResolveNullRTVDescriptor() {
		return ResolveNullView(this, D3D12_RESOURCE_DIMENSION_TEXTURE2D)->__InternalViewWrapper->getRTVHandle();
	}


#pragma endregion


#pragma region Clearing

	void GraphicsManager::Clearing::RT(gObj<Texture2D> rt, const FLOAT values[4]) {
		auto cmdList = ((DX_CmdWrapper*) this->wrapper->__InternalDXCmdWrapper)->cmdList;
		DX_ResourceWrapper* resourceWrapper = (DX_ResourceWrapper*)rt->__InternalDXWrapper;
		DX_ViewWrapper* view = (DX_ViewWrapper*)rt->__InternalViewWrapper;
		resourceWrapper->AddBarrier(cmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);
		cmdList->ClearRenderTargetView(view->getRTVHandle(), values, 0, nullptr);
	}

#pragma endregion

#pragma region Copy Manager

	void CopyManager::Copying::FastCopyToStart(gObj<ResourceView> dst, byte* data, long size) {
		auto resource = (DX_ResourceWrapper*)dst->__InternalDXWrapper;
		auto view = (DX_ViewWrapper*)dst->__InternalViewWrapper;
		int subresource = resource->desc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE3D ?
			view->mipStart :
			view->mipStart + (view->arrayStart * resource->desc.MipLevels);
		resource->UploadToSubresource0(resource->pLayouts[subresource].Offset, (byte*)data, size);
		resource->GrantGPUAccess(((DX_CmdWrapper*)this->wrapper->__InternalDXCmdWrapper)->cmdList);
	}

	void CopyManager::Copying::FullCopyToSubresource(gObj<ResourceView> dst, byte* data, const D3D12_BOX* box)
	{
		auto resource = (DX_ResourceWrapper*)dst->__InternalDXWrapper;
		
		if (box != nullptr) // region inside subresource
		{ // Only copy region
			
		}
		else { // copy all subresources of the view/slice

		}
	}

#pragma endregion

#pragma region Graphics Manager

	void GraphicsManager::Setter::Pipeline(gObj<IPipelineBindings> pipeline) {
		DX_CmdWrapper* cmdWrapper = this->wrapper->__InternalDXCmdWrapper;
		cmdWrapper->currentPipeline = pipeline;

		pipeline->OnSet(this->wrapper);
	}

	void GraphicsManager::Setter::Viewport(float width, float height, float maxDepth, float x, float y, float minDepth)
	{
		D3D12_VIEWPORT viewport;
		viewport.Width = width;
		viewport.Height = height;
		viewport.MinDepth = minDepth;
		viewport.MaxDepth = maxDepth;
		viewport.TopLeftX = x;
		viewport.TopLeftY = y;
		wrapper->__InternalDXCmdWrapper->cmdList->RSSetViewports(1, &viewport);

		D3D12_RECT rect;
		rect.left = (int)x;
		rect.top = (int)y;
		rect.right = (int)x + (int)width;
		rect.bottom = (int)y + (int)height;
		wrapper->__InternalDXCmdWrapper->cmdList->RSSetScissorRects(1, &rect);
	}

	void CA4G::GraphicsManager::Setter::VertexBuffer(gObj<Buffer> buffer, int slot)
	{
		buffer->__InternalDXWrapper->AddBarrier(wrapper->__InternalDXCmdWrapper->cmdList, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
		D3D12_VERTEX_BUFFER_VIEW view;
		buffer->__InternalViewWrapper->CreateVBV(view);
		wrapper->__InternalDXCmdWrapper->cmdList->IASetVertexBuffers(slot, 1, &view);
		if (slot == 0) {
			wrapper->__InternalDXCmdWrapper->vertexBufferSliceOffset = buffer->__InternalViewWrapper->arrayStart;
		}
	}

	void CA4G::GraphicsManager::Setter::IndexBuffer(gObj<Buffer> buffer)
	{
		buffer->__InternalDXWrapper->AddBarrier(wrapper->__InternalDXCmdWrapper->cmdList, D3D12_RESOURCE_STATE_INDEX_BUFFER);
		D3D12_INDEX_BUFFER_VIEW view;
		buffer->__InternalViewWrapper->CreateIBV(view);
		wrapper->__InternalDXCmdWrapper->cmdList->IASetIndexBuffer(&view);
		wrapper->__InternalDXCmdWrapper->indexBufferSliceOffset = buffer->__InternalViewWrapper->arrayStart;
	}

	void CA4G::GraphicsManager::Dispatcher::IndexedPrimitive(D3D_PRIMITIVE_TOPOLOGY topology, int count, int start)
	{
		DX_CmdWrapper* cmdWrapper = wrapper->__InternalDXCmdWrapper;

		if (!cmdWrapper->currentPipeline) {
			throw new CA4G::CA4GException("Invalid operation, Pipeline should be set first");
		}
		cmdWrapper->currentPipeline->OnDispatch(this->wrapper);
		cmdWrapper->cmdList->IASetPrimitiveTopology(topology);
		cmdWrapper->cmdList->DrawIndexedInstanced(count, 1, start + cmdWrapper->indexBufferSliceOffset, cmdWrapper->vertexBufferSliceOffset, 0);
	}

	void CA4G::GraphicsManager::Dispatcher::Primitive(D3D_PRIMITIVE_TOPOLOGY topology, int count, int start)
	{
		DX_CmdWrapper* cmdWrapper = wrapper->__InternalDXCmdWrapper;

		if (!cmdWrapper->currentPipeline) {
			throw new CA4G::CA4GException("Invalid operation, Pipeline should be set first");
		}
		cmdWrapper->currentPipeline->OnDispatch(this->wrapper);
		cmdWrapper->cmdList->IASetPrimitiveTopology(topology);
		cmdWrapper->cmdList->DrawInstanced(count, 1, start + cmdWrapper->vertexBufferSliceOffset, 0);
	}

#pragma endregion

	void DX_Wrapper::Initialize()
	{
		UINT dxgiFactoryFlags = 0;

#if defined(_DEBUG)
		// Enable the debug layer (requires the Graphics Tools "optional feature").
		// NOTE: Enabling the debug layer after device creation will invalidate the active device.
		{
			if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
			{
				debugController->EnableDebugLayer();

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

		if (UseWarpDevice)
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

		scheduler = new GPUScheduler(this, swapChainDesc.BufferCount, 8);

		IDXGISwapChain1* tmpSwapChain;

		factory->CreateSwapChainForHwnd(
			scheduler->Engines[0].queue->dxQueue,        // Swap chain needs the queue so that it can force a flush on it.
			hWnd,
			&swapChainDesc,
			&fullScreenDesc,
			nullptr,
			&tmpSwapChain
		);

		swapChain = (IDXGISwapChain3*)tmpSwapChain;
		swapChain->SetMaximumFrameLatency(swapChainDesc.BufferCount);

		// This sample does not support fullscreen transitions.
		factory->MakeWindowAssociation(hWnd, DXGI_MWA_NO_ALT_ENTER);

		// Create rendertargets resources.
		{
			RenderTargets = new gObj<Texture2D>[swapChainDesc.BufferCount];
			// Create a RTV and a command allocator for each frame.
			for (UINT n = 0; n < swapChainDesc.BufferCount; n++)
			{
				DX_Resource rtResource;
				swapChain->GetBuffer(n, IID_PPV_ARGS(&rtResource));

				auto desc = rtResource->GetDesc();

				DX_ResourceWrapper* rw = new DX_ResourceWrapper(this, rtResource, desc, D3D12_RESOURCE_STATE_COPY_DEST, CPUAccessibility::None);

				RenderTargets[n] = new Texture2D(rw, nullptr, desc.Format, desc.Width, desc.Height, 1, 1);
			}
		}

		// Initialize descriptor heaps

		gui_csu = new GPUDescriptorHeapManager(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 100, 100, 1);

		gpu_csu = new GPUDescriptorHeapManager(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 900000, 1000, swapChainDesc.BufferCount);
		gpu_smp = new GPUDescriptorHeapManager(device, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, 2000, 100, swapChainDesc.BufferCount);
		
		cpu_rt = new CPUDescriptorHeapManager(device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 1000);
		cpu_ds = new CPUDescriptorHeapManager(device, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 1000);
		cpu_csu = new CPUDescriptorHeapManager(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1000000);
		cpu_smp = new CPUDescriptorHeapManager(device, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, 2000);

		//CurrentBuffer = swapChain->GetCurrentBackBufferIndex();
		//manager->BackBuffer = renderTargetViews[CurrentBuffer];

		//renderTargetDescriptorSlot = manager->descriptors->gui_csu->MallocPersistent();
	}

	Signal GPUScheduler::FlushAndSignal(EngineMask mask) {
		int engines = (int)mask;
		long rally[4];
		// Barrier to wait for all pending workers to populate command lists
		// After this, next CPU processes can assume previous CPU collecting has finished
		counting->Wait();

		#pragma region Flush Pending Workers

		int resultMask = 0;

		for (int e = 0; e < 4; e++)
			if (engines & (1 << e))
			{
				int activeCmdLists = 0;
				for (int t = 0; t < threadsCount; t++) {
					if (this->Engines[e].threadInfos[t].isActive) // pending work here
					{
						this->Engines[e].threadInfos[t].Close();
						this->ActiveCmdLists[activeCmdLists++] = Engines[e].threadInfos[t].cmdList;
					}
					auto manager = Engines[e].threadInfos[t].manager;

					//// Copy all collected descriptors from non-visible to visible DHs.
					//if (manager->srcDescriptors.size() > 0)
					//{
					//	this->manager->device->CopyDescriptors(
					//		manager->dstDescriptors.size(), &manager->dstDescriptors.first(), &manager->dstDescriptorRangeLengths.first(),
					//		manager->srcDescriptors.size(), &manager->srcDescriptors.first(), nullptr, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
					//	);
					//	// Clears the lists for next usage
					//	manager->srcDescriptors.reset();
					//	manager->dstDescriptors.reset();
					//	manager->dstDescriptorRangeLengths.reset();
					//}
				}

				if (activeCmdLists > 0) // some cmdlist to execute
				{
					resultMask |= 1 << e;
					Engines[e].queue->Commit(ActiveCmdLists, activeCmdLists);

					rally[e] = Engines[e].queue->Signal();
				}
				else
					rally[e] = 0;
			}

		return Signal(this, rally);
	}

	void GPUScheduler::WaitFor(const Signal& signal) {
		int fencesForWaiting = 0;
		HANDLE FencesForWaiting[4];
		for (int e = 0; e < 4; e++)
			if (signal.rallyPoints[e] != 0)
				FencesForWaiting[fencesForWaiting++] = Engines[e].queue->TriggerEvent(signal.rallyPoints[e]);
		WaitForMultipleObjects(fencesForWaiting, FencesForWaiting, true, INFINITE);
		if (signal.rallyPoints[0] != 0 || signal.rallyPoints[3] != 0)
			this->AsyncWorkPending = false;
	}

	void GPUScheduler::Enqueue(gObj<IGPUProcess> process) {
		this->PopulateCmdListWithProcess(TagProcess{ process, this->Tag }, 0);
	}

	void GPUScheduler::EnqueueAsync(gObj<IGPUProcess> process) {
		this->AsyncWorkPending |= process->RequiredEngine() == Engine::Direct || process->RequiredEngine() == Engine::Raytracing;
		counting->Increment();
		processQueue->TryProduce(TagProcess{ process, this->Tag });
	}

	DWORD WINAPI GPUScheduler::__WORKER_TODO(LPVOID param) {
		GPUWorkerInfo* wi = (GPUWorkerInfo*)param;
		int index = wi->Index;
		GPUScheduler* scheduler = wi->Scheduler;

		while (!scheduler->IsClosed){
			TagProcess tagProcess;
			if (!scheduler->processQueue->TryConsume(tagProcess))
				break;

			scheduler->PopulateCmdListWithProcess(tagProcess, index);
		}
		return 0;
	}

}