#ifndef PRIVATE_PRESENTER_H
#define PRIVATE_PRESENTER_H

#include "private_ca4g_definitions.h"
#include "private_ca4g_sync.h"
#include "ca4g_presenter.h"
#include "private_ca4g_descriptors.h"

namespace CA4G {

	struct GPUScheduler;

	struct DX_Wrapper {
		DX_Device device;
		DX_FallbackDevice fallbackDevice;
		ID3D12Debug* debugController;

		HWND hWnd;

		bool UseFallbackDevice;
		bool UseWarpDevice;
		bool UseFrameBuffering;
		DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
		DXGI_SWAP_CHAIN_FULLSCREEN_DESC fullScreenDesc = {};

		CComPtr<IDXGISwapChain3> swapChain;

		gObj<Texture2D>* RenderTargets;

		GPUScheduler* scheduler;

		#pragma region Descriptor Heaps

		// -- Shader visible heaps --

		// Descriptor heap for gui windows and fonts
		gObj<GPUDescriptorHeapManager> gui_csu;

		// -- GPU heaps --

		// Descriptor heap for CBV, SRV and UAV
		gObj<GPUDescriptorHeapManager> gpu_csu;
		// Descriptor heap for sampler objects
		gObj<GPUDescriptorHeapManager> gpu_smp;

		// -- CPU heaps --

		// Descriptor heap for render targets views
		gObj<CPUDescriptorHeapManager> cpu_rt;
		// Descriptor heap for depth stencil views
		gObj<CPUDescriptorHeapManager> cpu_ds;
		// Descriptor heap for cbs, srvs and uavs in CPU memory
		gObj<CPUDescriptorHeapManager> cpu_csu;
		// Descriptor heap for sampler objects in CPU memory
		gObj<CPUDescriptorHeapManager> cpu_smp;

		#pragma endregion

		DX_Wrapper() {
			UseFallbackDevice = false;
			UseWarpDevice = false;
			UseFrameBuffering = false;
			device = nullptr;
			fallbackDevice = nullptr;
			debugController = nullptr;
			fullScreenDesc.Windowed = true;
			swapChainDesc.BufferCount = 2;
			swapChainDesc.Width = 512;
			swapChainDesc.Height = 512;
			swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
			swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;
			swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
			swapChainDesc.SampleDesc.Count = 1;
		}

		void Initialize();
	};

	struct DX_ResourceWrapper {

		DX_ResourceWrapper(DX_Wrapper* wrapper, DX_Resource resource, const D3D12_RESOURCE_DESC& desc, D3D12_RESOURCE_STATES initialState)
			: device(wrapper->device), dxWrapper(wrapper), resource(resource), desc(desc)
		{
			LastUsageState = initialState; // state at creation

			subresources = desc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE3D ? desc.MipLevels : desc.MipLevels * desc.DepthOrArraySize;

			pLayouts = new D3D12_PLACED_SUBRESOURCE_FOOTPRINT[subresources];
			pNumRows = new unsigned int[subresources];
			pRowSizesInBytes = new UINT64[subresources];
			device->GetCopyableFootprints(&desc, 0, subresources, 0, pLayouts, pNumRows, pRowSizesInBytes, &pTotalSizes);
		}

		~DX_ResourceWrapper() {
			delete[] pLayouts;
			delete[] pNumRows;
			delete[] pRowSizesInBytes;
		}

		DX_Device device;

		DX_Wrapper* dxWrapper;

		// Resource being wrapped
		DX_Resource resource;
		// Resourve version for uploading purposes
		DX_Resource uploading;
		// Resource version for downloading purposes
		DX_Resource downloading;

		// Resource description at creation
		D3D12_RESOURCE_DESC desc;
		// For buffer resources, get the original element stride width of the resource.
		int elementStride = 0;
		// Last used state of this resource on the GPU
		D3D12_RESOURCE_STATES LastUsageState;

		// For uploading data, a permanent CPU mapped version is cache to avoid several map overheads...
		byte* permanentUploadingMap = nullptr;

		inline D3D12_GPU_VIRTUAL_ADDRESS GetGPUVirtualAddress() {
			return resource->GetGPUVirtualAddress();
		}

		int subresources;

		D3D12_PLACED_SUBRESOURCE_FOOTPRINT* pLayouts;
		unsigned int* pNumRows;
		UINT64* pRowSizesInBytes;
		UINT64 pTotalSizes;

		// Used to synchronize access to the resource will mapping
		Mutex mutex;

		// How many resource views are referencing this resource. This resource is automatically released by the last view using it
		// when no view is referencing it.
		int references = 0;

		// Resolves an uploading CPU-Writable version for this resource
		void ResolveUploading() {
			if (!uploading) {
				mutex.Acquire();

				if (!uploading) {
					//auto size = GetRequiredIntermediateSize(device, desc);

					D3D12_RESOURCE_DESC finalDesc = { };
					finalDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
					finalDesc.Format = DXGI_FORMAT_UNKNOWN;
					finalDesc.Width = pTotalSizes;
					finalDesc.Height = 1;
					finalDesc.DepthOrArraySize = 1;
					finalDesc.MipLevels = 1;
					finalDesc.SampleDesc.Count = 1;
					finalDesc.SampleDesc.Quality = 0;
					finalDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
					finalDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

					D3D12_HEAP_PROPERTIES uploadProp;
					uploadProp.Type = D3D12_HEAP_TYPE_UPLOAD;
					uploadProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
					uploadProp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
					uploadProp.VisibleNodeMask = 1;
					uploadProp.CreationNodeMask = 1;

					device->CreateCommittedResource(&uploadProp, D3D12_HEAP_FLAG_NONE, &finalDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
						IID_PPV_ARGS(&uploading));

					// Automatically map the data to CPU to fill in next instructions
					// Uploading version is only required if some CPU data is gonna be copied to the GPU resource.
					D3D12_RANGE range{ };
					uploading->Map(0, &range, (void**)&permanentUploadingMap);
				}

				mutex.Release();
			}
		}

		// Resolves a downloading CPU-Readable version for this resource
		void ResolveDownloading() {
			if (!this->downloading) {
				mutex.Acquire();

				if (!downloading) {
					//auto size = GetRequiredIntermediateSize(device, desc);

					D3D12_RESOURCE_DESC finalDesc = { };
					finalDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
					finalDesc.Format = DXGI_FORMAT_UNKNOWN;
					finalDesc.Width = pTotalSizes;
					finalDesc.Height = 1;
					finalDesc.DepthOrArraySize = 1;
					finalDesc.MipLevels = 1;
					finalDesc.SampleDesc.Count = 1;
					finalDesc.SampleDesc.Quality = 0;
					finalDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
					finalDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

					D3D12_HEAP_PROPERTIES downloadProp;
					downloadProp.Type = D3D12_HEAP_TYPE_READBACK;
					downloadProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
					downloadProp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
					downloadProp.VisibleNodeMask = 1;
					downloadProp.CreationNodeMask = 1;
					device->CreateCommittedResource(&downloadProp, D3D12_HEAP_FLAG_NONE, &finalDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
						IID_PPV_ARGS(&downloading));
				}

				mutex.Release();
			}
		}

		void UploadToSubresource0(long dstOffset, byte* data, long size) {
			memcpy(permanentUploadingMap + dstOffset, (UINT8*)data, size);
		}

		void DownloadFromSubresource0(long srcOffset, long size, byte* dstData) {
			mutex.Acquire(); // sync data access to map downloaded version

			D3D12_RANGE range{ srcOffset, size };

			byte* mappedData;
			auto hr = downloading->Map(0, &range, (void**)&mappedData);

			memcpy(dstData, mappedData + srcOffset, size);

			D3D12_RANGE emptyRange{ 0, 0 };
			downloading->Unmap(0, &emptyRange);

			mutex.Release();
		}

		//---Copied from d3d12x.h----------------------------------------------------------------------------
		// Row-by-row memcpy
		inline void MemcpySubresource(
			_In_ const D3D12_MEMCPY_DEST* pDest,
			_In_ const D3D12_SUBRESOURCE_DATA* pSrc,
			SIZE_T RowSizeInBytes,
			UINT NumRows,
			UINT NumSlices,
			bool flipRows = false
		)
		{
			for (UINT z = 0; z < NumSlices; ++z)
			{
				BYTE* pDestSlice = reinterpret_cast<BYTE*>(pDest->pData) + pDest->SlicePitch * z;
				const BYTE* pSrcSlice = reinterpret_cast<const BYTE*>(pSrc->pData) + pSrc->SlicePitch * z;
				for (UINT y = 0; y < NumRows; ++y)
				{
					memcpy(pDestSlice + pDest->RowPitch * y,
						pSrcSlice + pSrc->RowPitch * (flipRows ? NumRows - y - 1 : y),
						RowSizeInBytes);
				}
			}
		}

		void UploadToAllSubresource(byte* data, long size, bool flipRows) {
			int srcOffset = 0;
			for (UINT i = 0; i < subresources; ++i)
			{
				D3D12_MEMCPY_DEST DestData = { (byte*)permanentUploadingMap + pLayouts[i].Offset, pLayouts[i].Footprint.RowPitch, SIZE_T(pLayouts[i].Footprint.RowPitch) * SIZE_T(pNumRows[i]) };
				D3D12_SUBRESOURCE_DATA subData;
				subData.pData = data + srcOffset;
				subData.RowPitch = pRowSizesInBytes[i];
				subData.SlicePitch = pRowSizesInBytes[i] * pNumRows[i];
				MemcpySubresource(&DestData, &subData, static_cast<SIZE_T>(pRowSizesInBytes[i]), pNumRows[i], pLayouts[i].Footprint.Depth, flipRows);
				srcOffset += pRowSizesInBytes[i] * pNumRows[i];
			}
		}

		void DownloadFromAllSubresources(byte* data, long size, bool flipRows) {
			D3D12_RANGE range{ 0, size };

			byte* mappedData;

			mutex.Acquire();
			auto hr = downloading->Map(0, &range, (void**)&mappedData);

			int srcOffset = 0;
			for (UINT i = 0; i < 1; ++i)
			{
				D3D12_SUBRESOURCE_DATA DestData = {
					(byte*)mappedData + pLayouts[i].Offset,
					pLayouts[i].Footprint.RowPitch,
					SIZE_T(pLayouts[i].Footprint.RowPitch) * SIZE_T(pNumRows[i])
				};
				D3D12_MEMCPY_DEST subData;
				subData.pData = data + srcOffset;
				subData.RowPitch = pRowSizesInBytes[i];
				subData.SlicePitch = pRowSizesInBytes[i] * pNumRows[i];

				MemcpySubresource(&subData, &DestData, static_cast<SIZE_T>(pRowSizesInBytes[i]), pNumRows[i], pLayouts[i].Footprint.Depth, flipRows);
				srcOffset += pRowSizesInBytes[i] * pNumRows[i];
			}
			D3D12_RANGE emptyRange{ 0, 0 };
			downloading->Unmap(0, &emptyRange);
			mutex.Release();
		}

		void UploadToGPU(DX_CommandList cmd) {
			cmd->CopyResource(resource, uploading);
		}

		void DownloadFromGPU(DX_CommandList cmd) {
			cmd->CopyResource(downloading, resource);
		}
	};

	struct DX_ViewWrapper {

		DX_ResourceWrapper* resource;

		DX_ViewWrapper(DX_ResourceWrapper* resource) :resource(resource) {}

		// Mip start of this view slice
		int mipStart = 0;
		// Mip count of this view slice
		int mipCount = 1;
		// Array start of this view slice.
		// If current view is a Buffer, then this variable represents the first element
		int arrayStart = 0;
		// Array count of this view slice
		// If current view is a Buffer, then this variable represents the number of elements.
		int arrayCount = 1;

		// Represents the element stride in bytes of the underlaying resource in case of a Buffer.
		int elementStride = 0;

		// Cached cpu handle created for Shader resource view
		int srv_cached_handle = 0;
		// Cached cpu handle created for Unordered Access view
		int uav_cached_handle = 0;
		// Cached cpu hanlde created for Constant Buffer view
		int cbv_cached_handle = 0;
		// Cached cpu handle created for a Render Target View
		int rtv_cached_handle = 0;
		// Cached cpu handle created for DepthStencil buffer view
		int dsv_cached_handle = 0;
		// Mask of valid cached handles 1-srv, 2-uav, 4-cbv, 8-rtv, 16-dsv
		unsigned int handle_mask = 0;

		// Mutex object used to synchronize handle creation.
		Mutex mutex;

		#pragma region Creating view handles and caching

		void CreateRTVDesc(D3D12_RENDER_TARGET_VIEW_DESC& d)
		{
			d.Texture2DArray.ArraySize = arrayCount;
			d.Texture2DArray.FirstArraySlice = arrayStart;
			d.Texture2DArray.MipSlice = mipStart;
			d.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
			d.Format = !resource ? DXGI_FORMAT_UNKNOWN : resource->desc.Format;
		}

		int getRTV() {
			if ((handle_mask & 8) != 0)
				return rtv_cached_handle;

			mutex.Acquire();

			if ((handle_mask & 8) == 0) {
				D3D12_RENDER_TARGET_VIEW_DESC d;
				ZeroMemory(&d, sizeof(D3D12_RENDER_TARGET_VIEW_DESC));
				CreateRTVDesc(d);
				rtv_cached_handle = resource->dxWrapper->cpu_rt->AllocateNewHandle();
				resource->dxWrapper->device->CreateRenderTargetView(!resource ? nullptr : resource->resource, &d, resource->dxWrapper->cpu_rt->getCPUVersion(rtv_cached_handle));
				handle_mask |= 8;
			}

			mutex.Release();
			return rtv_cached_handle;
		}

		D3D12_CPU_DESCRIPTOR_HANDLE getRTVHandle() {
			return resource->dxWrapper->cpu_rt->getCPUVersion(getRTV());
		}

		#pragma endregion

		// Gets the current view dimension of the resource.
		D3D12_RESOURCE_DIMENSION ViewDimension = D3D12_RESOURCE_DIMENSION_UNKNOWN;

		DX_ViewWrapper* createSlicedClone(
			int mipOffset, int mipNewCount,
			int arrayOffset, int arrayNewCount) {
			DX_ViewWrapper* result = new DX_ViewWrapper(this->resource);

			result->elementStride = this->elementStride;
			result->ViewDimension = this->ViewDimension;

			if (mipNewCount > 0)
			{
				result->mipStart = this->mipStart + mipOffset;
				result->mipCount = mipNewCount;
			}
			else {
				result->mipStart = this->mipStart;
				result->mipCount = this->mipCount;
			}

			if (arrayNewCount > 0) {
				result->arrayStart = this->arrayStart + arrayOffset;
				result->arrayCount = arrayNewCount;
			}
			else {
				result->arrayStart = this->arrayStart;
				result->arrayCount = arrayNewCount;
			}

			return result;
		}
	};

	#pragma region Command Queue Manager
	struct CommandQueueManager {
		DX_CommandQueue dxQueue;
		long fenceValue;
		DX_Fence fence;
		HANDLE fenceEvent;
		D3D12_COMMAND_LIST_TYPE type;

		CommandQueueManager(DX_Device device, D3D12_COMMAND_LIST_TYPE type) {
			D3D12_COMMAND_QUEUE_DESC d = {};
			d.Type = type;
			device->CreateCommandQueue(&d, IID_PPV_ARGS(&dxQueue));

			fenceValue = 1;
			device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
			fenceEvent = CreateEventW(nullptr, false, false, nullptr);
		}

		// Executes a command list set in this engine
		inline void Commit(DX_CommandList* cmdLists, int count) {
			dxQueue->ExecuteCommandLists(count, (ID3D12CommandList**)cmdLists);
		}

		// Send a signals through this queue
		inline long Signal() {
			dxQueue->Signal(fence, fenceValue);
			return fenceValue++;
		}

		// Triggers the event of reaching some fence value.
		inline HANDLE TriggerEvent(long rallyPoint) {
			fence->SetEventOnCompletion(rallyPoint, fenceEvent);
			return fenceEvent;
		}
	};
	#pragma endregion

	// This object allows the scheduler to know what cmd list is used by thread, and if it has some commands to commit or not.
	struct PerThreadInfo {
		DX_CommandList cmdList;
		gObj<CommandListManager> manager;
		bool isActive;
		// Prepares this command list for recording. This method resets the command list to use the desiredAllocator.
		void Activate(DX_CommandAllocator desiredAllocator)
		{
			if (isActive)
				return;
			cmdList->Reset(desiredAllocator, nullptr);
			isActive = true;
		}

		// Prepares this cmd list for flushing to the GPU
		void Close() {
			if (!isActive)
				return;
			isActive = false;
			cmdList->Close();
		}
	};

	// Information needed by the scheduler for each frame on each engine
	struct PerFrameInfo {
		// Allocators for each thread.
		DX_CommandAllocator* allocatorSet;
		int allocatorsUsed;

		inline DX_CommandAllocator RequireAllocator(int index) {
			allocatorsUsed = max(allocatorsUsed, index+1);
			return allocatorSet[index];
		}

		void ResetUsedAllocators()
		{
			for (int i = 0; i < allocatorsUsed; i++)
				allocatorSet[i]->Reset();
			allocatorsUsed = 0;
		}
	};

	// Information needed by the scheduler for each engine.
	struct PerEngineInfo {
		// The queue representing this engine.
		CommandQueueManager* queue;
		
		// An array of command lists used per thread
		PerThreadInfo* threadInfos;
		int threadCount;

		// An array of allocators used per frame
		PerFrameInfo* frames;
		int framesCount;
	};

	// Struct pairing a process with the tag in the moment of enqueue
	// Used for defered command list population in async mode.
	struct TagProcess {
		gObj<IGPUProcess> process;
		gObj<TagData> Tag;
	};

	// This class is intended to manage all command queues, threads, allocators and command lists
	struct GPUScheduler {
		DX_Wrapper* dxWrapper;

		PerEngineInfo Engines[4] = { }; // 0- direct, 1- compute, 2 - copy, 3- raytracing

		// Struct for threading parameters
		struct GPUWorkerInfo {
			int Index;
			GPUScheduler* Scheduler;
		} *workers;

		ProducerConsumerQueue<TagProcess> *processQueue;

		// Threads on CPU to collect commands from GPU Processes
		HANDLE* threads;
		int threadsCount;

		// Counting sync object to wait for flushing all processes in processQueue.
		CountEvent* counting;
		
		// Gets or sets when this scheduler is closed.
		bool IsClosed = false;

		// Gets the index of the current frame
		int CurrentFrameIndex = 0;

		// Tag data will be associated to any command list manager for a process in the time they are enqueue
		gObj<TagData> Tag = nullptr;

		// Array to collect active cmd lists during flushing.
		// This array needs capacity to store a cmd list x thread x engine type.
		DX_CommandList* ActiveCmdLists;

		Signal* perFrameFinishedSignal;

		static DWORD WINAPI __WORKER_TODO(LPVOID param);

		GPUScheduler(DX_Wrapper* dxWrapper, int buffers, int threads) {
			this->dxWrapper = dxWrapper;

			this->perFrameFinishedSignal = new Signal[buffers];

			DX_Device device = dxWrapper->device;

			this->processQueue = new ProducerConsumerQueue<TagProcess>(threads);

			this->ActiveCmdLists = new DX_CommandList[threads * 4];

			this->threads = new HANDLE[threads];
			this->workers = new GPUWorkerInfo[threads];
			this->threadsCount = threads;
			this->counting = new CountEvent();

			for (int i = 0; i < threads; i++) {
				workers[i] = { i, this };

				DWORD threadId;
				if (i > 0) // only create threads for workerIndex > 0. Worker 0 will execute on main thread
					this->threads[i] = CreateThread(nullptr, 0, __WORKER_TODO, &workers[i], 0, &threadId);
			}

			for (int i = 0; i < 4; i++)
			{
				PerEngineInfo& info = Engines[i];

				D3D12_COMMAND_LIST_TYPE type;
				switch (i) {
				case 0: // Graphics
				case 3: // Raytracing
					type = D3D12_COMMAND_LIST_TYPE_DIRECT;
					break;
				case 1: // Compute
					type = D3D12_COMMAND_LIST_TYPE_COMPUTE;
					break;
				case 2:
					type = D3D12_COMMAND_LIST_TYPE_COPY;
					break;
				}

				// Create queue
				info.queue = new CommandQueueManager(device, type);

				info.threadInfos = new PerThreadInfo[threads];
				info.threadCount = threads;
				info.frames = new PerFrameInfo[buffers];
				info.framesCount = buffers;

				// Create allocators first because they are needed for command lists
				for (int j = 0; j < buffers; j++)
				{
					info.frames[j].allocatorSet = new DX_CommandAllocator[threads];
					info.frames[j].allocatorsUsed = 0;

					for (int k = 0; k < threads; k++)
						device->CreateCommandAllocator(type, IID_PPV_ARGS(&info.frames[j].allocatorSet[k]));
				}

				// Create command lists
				for (int j = 0; j < threads; j++)
				{
					device->CreateCommandList(0, type, info.frames[0].allocatorSet[0], nullptr, IID_PPV_ARGS(&info.threadInfos[j].cmdList));
					info.threadInfos[j].cmdList->Close(); // start cmdList state closed.
					info.threadInfos[j].isActive = false;

					switch ((Engine)i) {
					case Engine::Direct:
						info.threadInfos[j].manager = new GraphicsManager();
						break;
					case Engine::Compute:
						info.threadInfos[j].manager = new ComputeManager();
						break;
					case Engine::Copy:
						info.threadInfos[j].manager = new CopyManager();
						break;
					case Engine::Raytracing:
						info.threadInfos[j].manager = new RaytracingManager();
						break;
					}

					info.threadInfos[j].manager->__InternalDXCmd = info.threadInfos[j].cmdList;
				}
			}
		}

		void Enqueue(gObj<IGPUProcess> process);
		
		void EnqueueAsync(gObj<IGPUProcess> process);

		void PopulateCmdListWithProcess(TagProcess tagProcess, int threadIndex) {
			gObj<IGPUProcess> nextProcess = tagProcess.process;

			int engineIndex = (int)nextProcess->RequiredEngine();

			auto cmdListManager = this->Engines[engineIndex].threadInfos[threadIndex].manager;

			this->Engines[engineIndex].threadInfos[threadIndex].
				Activate(this->Engines[engineIndex].frames[this->CurrentFrameIndex].RequireAllocator(threadIndex));

			cmdListManager->__Tag = tagProcess.Tag;
			nextProcess->OnCollect(cmdListManager);

			if (threadIndex > 0)
				this->counting->Signal();
		}

		Signal FlushAndSignal(EngineMask mask);

		void WaitFor(const Signal &signal);

		void SetupFrame(int frame) {
			if (dxWrapper->UseFrameBuffering)
				WaitFor(perFrameFinishedSignal[frame]); // Grants the GPU finished working this frame in a previous stage

			CurrentFrameIndex = frame;

			for (int e = 0; e < 4; e++)
				Engines[e].frames[frame].ResetUsedAllocators();
		}

		void FinishFrame() {
			perFrameFinishedSignal[CurrentFrameIndex] = FlushAndSignal(EngineMask::All);

			if (!dxWrapper->UseFrameBuffering)
				// Grants the GPU finished working this frame before finishing this frame
				WaitFor(perFrameFinishedSignal[CurrentFrameIndex]);
		}
	};

#define __InternalState ((DX_Wrapper*)this->__InternalDXWrapper)
#define __PInternalState ((DX_Wrapper*)presenter->__InternalDXWrapper)

#define __InternalResourceState ((DX_ResourceWrapper*)this->__InternalDXWrapper)

}

#endif