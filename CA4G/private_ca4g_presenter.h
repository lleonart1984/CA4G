#ifndef PRIVATE_PRESENTER_H
#define PRIVATE_PRESENTER_H

#include "private_ca4g_definitions.h"
#include "private_ca4g_sync.h"
#include "ca4g_presenter.h"
#include "private_ca4g_descriptors.h"
#include "ca4g_collections.h"

namespace CA4G {

	struct DX_ResourceWrapper;
	struct DX_ViewWrapper;

	struct GPUScheduler;

	struct DX_Wrapper {
		DX_Device device;
		ID3D12Debug* debugController;

		HWND hWnd;

		bool UseFallbackDevice;
		bool UseWarpDevice;
		bool UseFrameBuffering;
		DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
		DXGI_SWAP_CHAIN_FULLSCREEN_DESC fullScreenDesc = {};

		CComPtr<IDXGISwapChain3> swapChain;

		gObj<Texture2D>* RenderTargets;
		D3D12_CPU_DESCRIPTOR_HANDLE* RenderTargetsRTV;

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

		static DX_ResourceWrapper* InternalResource(gObj<ResourceView> view)
		{
			return (DX_ResourceWrapper*)view->__InternalDXWrapper;
		}

		static DX_ViewWrapper* InternalView(gObj<ResourceView> view)
		{
			return (DX_ViewWrapper*)view->__InternalViewWrapper;
		}

		static gObj<ResourceView> ResolveNullView(DX_Wrapper* wrapper, D3D12_RESOURCE_DIMENSION dimension) {
			static gObj<ResourceView> nullBuffer = nullptr;
			static gObj<ResourceView> nullTexture1D = nullptr;
			static gObj<ResourceView> nullTexture2D = nullptr;
			static gObj<ResourceView> nullTexture3D = nullptr;

			switch (dimension)
			{
			case D3D12_RESOURCE_DIMENSION_BUFFER:
				return nullBuffer ? nullBuffer : nullBuffer = ResourceView::CreateNullView(wrapper, dimension);
			case D3D12_RESOURCE_DIMENSION_TEXTURE1D:
				return nullTexture1D ? nullTexture1D : nullTexture1D = ResourceView::CreateNullView(wrapper, dimension);
			case D3D12_RESOURCE_DIMENSION_TEXTURE2D:
				return nullTexture2D ? nullTexture2D : nullTexture2D = ResourceView::CreateNullView(wrapper, dimension);
			case D3D12_RESOURCE_DIMENSION_TEXTURE3D:
				return nullTexture3D ? nullTexture3D : nullTexture3D = ResourceView::CreateNullView(wrapper, dimension);
			default:
				return nullptr;
			}
		}

		D3D12_CPU_DESCRIPTOR_HANDLE ResolveNullRTVDescriptor();
	};

	struct DX_ResourceWrapper {

		static int SizeOfFormatElement(DXGI_FORMAT format) {
			switch (format) {
			case DXGI_FORMAT_R32G32B32A32_TYPELESS:
			case DXGI_FORMAT_R32G32B32A32_FLOAT:
			case DXGI_FORMAT_R32G32B32A32_UINT:
			case DXGI_FORMAT_R32G32B32A32_SINT:
				return 128 / 8;
			case DXGI_FORMAT_R32G32B32_TYPELESS:
			case DXGI_FORMAT_R32G32B32_FLOAT:
			case DXGI_FORMAT_R32G32B32_UINT:
			case DXGI_FORMAT_R32G32B32_SINT:
				return 96 / 8;
			case DXGI_FORMAT_R16G16B16A16_TYPELESS:
			case DXGI_FORMAT_R16G16B16A16_FLOAT:
			case DXGI_FORMAT_R16G16B16A16_UNORM:
			case DXGI_FORMAT_R16G16B16A16_UINT:
			case DXGI_FORMAT_R16G16B16A16_SNORM:
			case DXGI_FORMAT_R16G16B16A16_SINT:
			case DXGI_FORMAT_R32G32_TYPELESS:
			case DXGI_FORMAT_R32G32_FLOAT:
			case DXGI_FORMAT_R32G32_UINT:
			case DXGI_FORMAT_R32G32_SINT:
			case DXGI_FORMAT_R32G8X24_TYPELESS:
			case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
			case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
			case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
				return 64 / 8;
			case DXGI_FORMAT_R10G10B10A2_TYPELESS:
			case DXGI_FORMAT_R10G10B10A2_UNORM:
			case DXGI_FORMAT_R10G10B10A2_UINT:
			case DXGI_FORMAT_R11G11B10_FLOAT:
			case DXGI_FORMAT_R8G8B8A8_TYPELESS:
			case DXGI_FORMAT_R8G8B8A8_UNORM:
			case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
			case DXGI_FORMAT_R8G8B8A8_UINT:
			case DXGI_FORMAT_R8G8B8A8_SNORM:
			case DXGI_FORMAT_R8G8B8A8_SINT:
			case DXGI_FORMAT_R16G16_TYPELESS:
			case DXGI_FORMAT_R16G16_FLOAT:
			case DXGI_FORMAT_R16G16_UNORM:
			case DXGI_FORMAT_R16G16_UINT:
			case DXGI_FORMAT_R16G16_SNORM:
			case DXGI_FORMAT_R16G16_SINT:
			case DXGI_FORMAT_R32_TYPELESS:
			case DXGI_FORMAT_D32_FLOAT:
			case DXGI_FORMAT_R32_FLOAT:
			case DXGI_FORMAT_R32_UINT:
			case DXGI_FORMAT_R32_SINT:
			case DXGI_FORMAT_R24G8_TYPELESS:
			case DXGI_FORMAT_D24_UNORM_S8_UINT:
			case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
			case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
			case DXGI_FORMAT_B8G8R8A8_UNORM:
			case DXGI_FORMAT_B8G8R8X8_UNORM:
				return 32 / 8;
			case DXGI_FORMAT_R8G8_TYPELESS:
			case DXGI_FORMAT_R8G8_UNORM:
			case DXGI_FORMAT_R8G8_UINT:
			case DXGI_FORMAT_R8G8_SNORM:
			case DXGI_FORMAT_R8G8_SINT:
			case DXGI_FORMAT_R16_TYPELESS:
			case DXGI_FORMAT_R16_FLOAT:
			case DXGI_FORMAT_D16_UNORM:
			case DXGI_FORMAT_R16_UNORM:
			case DXGI_FORMAT_R16_UINT:
			case DXGI_FORMAT_R16_SNORM:
			case DXGI_FORMAT_R16_SINT:
			case DXGI_FORMAT_B5G6R5_UNORM:
			case DXGI_FORMAT_B5G5R5A1_UNORM:
				return 16 / 8;
			case DXGI_FORMAT_R8_TYPELESS:
			case DXGI_FORMAT_R8_UNORM:
			case DXGI_FORMAT_R8_UINT:
			case DXGI_FORMAT_R8_SNORM:
			case DXGI_FORMAT_R8_SINT:
			case DXGI_FORMAT_A8_UNORM:
				return 8 / 8;
			}
		}

		DX_ResourceWrapper(DX_Wrapper* wrapper, DX_Resource resource, const D3D12_RESOURCE_DESC& desc, D3D12_RESOURCE_STATES initialState, CPUAccessibility cpuAccessibility)
			: device(wrapper->device), dxWrapper(wrapper), resource(resource), desc(desc)
		{
			if (resource == nullptr) // null resource for nullview
				return;

			cpuaccess = cpuAccessibility;

			LastUsageState = initialState; // state at creation

			subresources = desc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE3D ? desc.MipLevels : desc.MipLevels * desc.DepthOrArraySize;

			pLayouts = new D3D12_PLACED_SUBRESOURCE_FOOTPRINT[subresources];
			pNumRows = new unsigned int[subresources];
			pRowSizesInBytes = new UINT64[subresources];
			device->GetCopyableFootprints(&desc, 0, subresources, 0, pLayouts, pNumRows, pRowSizesInBytes, &pTotalSizes);
		}

		~DX_ResourceWrapper() {
			if (resource == nullptr)
				return;

			delete[] pLayouts;
			delete[] pNumRows;
			delete[] pRowSizesInBytes;
		}

		DX_Device device;

		DX_Wrapper* dxWrapper;

		// Resource being wrapped
		DX_Resource resource;
		// Resourve version for uploading purposes (in case the CPU can not write directly).
		DX_Resource uploading;
		// Resource version for downloading purposes (in case the CPU can not read directly).
		DX_Resource downloading;

		// Resource description at creation
		D3D12_RESOURCE_DESC desc;
		// For buffer resources, get the original element stride width of the resource.
		int elementStride = 0;
		// Last used state of this resource on the GPU
		D3D12_RESOURCE_STATES LastUsageState;

		// For uploading/downloading data, a permanent CPU mapped version is cache to avoid several map overheads...
		byte* permanentUploadingMap = nullptr;
		byte* permanentDownloadingMap = nullptr;

		inline D3D12_GPU_VIRTUAL_ADDRESS GetGPUVirtualAddress() {
			return resource->GetGPUVirtualAddress();
		}

		int subresources;

		D3D12_PLACED_SUBRESOURCE_FOOTPRINT* pLayouts;
		unsigned int* pNumRows;
		UINT64* pRowSizesInBytes;
		UINT64 pTotalSizes;
		CPUAccessibility cpuaccess;

		// Used to synchronize access to the resource will mapping
		Mutex mutex;

		// How many resource views are referencing this resource. This resource is automatically released by the last view using it
		// when no view is referencing it.
		int references = 0;

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

		// Resolves an uploading CPU-Writable version for this resource
		void __ResolveUploading() {
			if (!uploading) {
				mutex.Acquire();

				if (!uploading) {

					if (cpuaccess == CPUAccessibility::Write)
						uploading = resource; // use the resource for uploading directly
					else {
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
					}

					// Automatically map the data to CPU to fill in next instructions
					// Uploading version is only required if some CPU data is gonna be copied to the GPU resource.
					D3D12_RANGE range{ };
					uploading->Map(0, &range, (void**)&permanentUploadingMap);
				}

				mutex.Release();
			}
		}

		// Resolves a downloading CPU-Readable version for this resource
		void __ResolveDownloading() {
			if (!this->downloading) {
				mutex.Acquire();
				if (!downloading) {

					if (cpuaccess == CPUAccessibility::Read)
						downloading = resource; // Use directly the resource for downloading
					else {

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

					D3D12_RANGE range{ 0, pTotalSizes };

					auto hr = downloading->Map(0, &range, (void**)&permanentDownloadingMap);
				}

				mutex.Release();
			}
		}

		// Writes data in specific subresources at mapped uploading memory
		void WriteToMapped(byte* data, DX_ViewWrapper* view, bool flipRows = false);

		// copies de specific range of subresources from uploading to the gpu resource
		void UpdateResourceFromMapped(DX_CommandList cmdList, DX_ViewWrapper* view);

		// Reads data from specific subresources at mapped downloading memory
		void ReadFromMapped(byte* data, DX_ViewWrapper* view, bool flipRows = false);

		// copies de specific range of subresources from gpu resource to the downloading version
		void UpdateMappedFromResource(DX_CommandList cmdList, DX_ViewWrapper* view);

		// Writes data in the first resource at specific region of the mapped data...
		void WriteRegionToMappedSubresource(byte* data, DX_ViewWrapper* view, const D3D12_BOX &region, bool flipRows = false);

		// copies de specific region of the single-subresource from uploading to the gpu resource
		void UpdateRegionFromUploading(DX_CommandList cmdList, DX_ViewWrapper* view, const D3D12_BOX &region);

		// Reads data from the first resource from specific region...
		void ReadRegionFromMappedSubresource(byte* data, DX_ViewWrapper* view, const D3D12_BOX &region, bool flipRows = false);

		void UpdateRegionToDownloading(DX_CommandList cmdList, DX_ViewWrapper* view, const D3D12_BOX &region);
		
		// Creates a barrier in a cmd list to trasition usage states for this resource.
		void AddBarrier(DX_CommandList cmdList, D3D12_RESOURCE_STATES dst) {
			// If the resource is used as UAV
			// Put a barrier to finish any pending read/write op
			if (this->LastUsageState & D3D12_RESOURCE_STATE_UNORDERED_ACCESS)
			{
				D3D12_RESOURCE_BARRIER barrier = { };
				barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
				barrier.UAV.pResource = this->resource;
				cmdList->ResourceBarrier(1, &barrier);
			}

			if (this->LastUsageState == dst)
				return;

			D3D12_RESOURCE_BARRIER barrier = { };
			barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
			barrier.Transition.pResource = this->resource;
			barrier.Transition.StateAfter = dst;
			barrier.Transition.StateBefore = this->LastUsageState;
			barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
			cmdList->ResourceBarrier(1, &barrier);
			this->LastUsageState = dst;
		}

		// Creates a barrier in a cmd list to trasition usage states for this resource.
		void AddUAVBarrier(DX_CommandList cmdList) {
			// If the resource is used as UAV
			// Put a barrier to finish any pending read/write op
			D3D12_RESOURCE_BARRIER barrier = { };
			barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
			barrier.UAV.pResource = this->resource;
			cmdList->ResourceBarrier(1, &barrier);
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

		D3D12_CPU_DESCRIPTOR_HANDLE GetCPUHandleFor(D3D12_DESCRIPTOR_RANGE_TYPE type) {
			switch (type) {
			case D3D12_DESCRIPTOR_RANGE_TYPE_SRV:
				return getSRVHandle();
			case D3D12_DESCRIPTOR_RANGE_TYPE_UAV:
				return getUAVHandle();
			case D3D12_DESCRIPTOR_RANGE_TYPE_CBV:
				return getCBVHandle();
			}
			return D3D12_CPU_DESCRIPTOR_HANDLE();
		}

#pragma region Creating view handles and caching

		void CreateRTVDesc(D3D12_RENDER_TARGET_VIEW_DESC& d)
		{
			d.Texture2DArray.ArraySize = arrayCount;
			d.Texture2DArray.FirstArraySlice = arrayStart;
			d.Texture2DArray.MipSlice = mipStart;
			d.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
			d.Format = !resource ? DXGI_FORMAT_UNKNOWN : resource->desc.Format;
		}

		void CreateSRVDesc(D3D12_SHADER_RESOURCE_VIEW_DESC& d)
		{
			switch (this->ViewDimension) {
			case D3D12_RESOURCE_DIMENSION_BUFFER:
				d.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
				d.Buffer.FirstElement = arrayStart;
				d.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
				d.Buffer.NumElements = arrayCount;
				d.Buffer.StructureByteStride = elementStride;
				d.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
				d.Format = !resource->resource ? DXGI_FORMAT_UNKNOWN : resource->desc.Format;
				break;
			case D3D12_RESOURCE_DIMENSION_TEXTURE1D:
				d.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
				d.Texture1DArray.ArraySize = arrayCount;
				d.Texture1DArray.FirstArraySlice = arrayStart;
				d.Texture1DArray.MipLevels = mipCount;
				d.Texture1DArray.MostDetailedMip = mipStart;
				d.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1DARRAY;
				d.Format = !resource->resource ? DXGI_FORMAT_R8G8B8A8_UNORM : resource->desc.Format;
				break;
			case D3D12_RESOURCE_DIMENSION_TEXTURE2D:
				d.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
				d.Texture2DArray.ArraySize = arrayCount;
				d.Texture2DArray.FirstArraySlice = arrayStart;
				d.Texture2DArray.MipLevels = mipCount;
				d.Texture2DArray.MostDetailedMip = mipStart;
				d.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
				d.Format = !resource->resource ? DXGI_FORMAT_R8G8B8A8_UNORM : resource->desc.Format;
				break;
			case D3D12_RESOURCE_DIMENSION_TEXTURE3D:
				d.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
				d.Texture3D.MipLevels = mipCount;
				d.Texture3D.MostDetailedMip = mipStart;
				d.Texture3D.ResourceMinLODClamp = 0;
				d.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
				d.Format = !resource->resource ? DXGI_FORMAT_R8G8B8A8_UNORM : resource->desc.Format;
				break;
			}
		}

		void CreateUAVDesc(D3D12_UNORDERED_ACCESS_VIEW_DESC& d)
		{
			switch (this->ViewDimension) {
			case D3D12_RESOURCE_DIMENSION_BUFFER:
				d.Buffer.CounterOffsetInBytes = 0;
				d.Buffer.FirstElement = arrayStart;
				d.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;
				d.Buffer.NumElements = arrayCount;
				d.Buffer.StructureByteStride = elementStride;
				d.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
				d.Format = DXGI_FORMAT_UNKNOWN;// !resource ? DXGI_FORMAT_UNKNOWN : resource->desc.Format;
				break;
			case D3D12_RESOURCE_DIMENSION_TEXTURE1D:
				d.Texture1DArray.ArraySize = arrayCount;
				d.Texture1DArray.FirstArraySlice = arrayStart;
				d.Texture1DArray.MipSlice = mipStart;
				d.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE1DARRAY;
				d.Format = !resource->resource ? DXGI_FORMAT_R8G8B8A8_UNORM : resource->desc.Format;
				break;
			case D3D12_RESOURCE_DIMENSION_TEXTURE2D:
				d.Texture2DArray.ArraySize = arrayCount;
				d.Texture2DArray.FirstArraySlice = arrayStart;
				d.Texture2DArray.MipSlice = mipStart;
				d.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
				d.Format = !resource->resource ? DXGI_FORMAT_R8G8B8A8_UNORM : resource->desc.Format;
				break;
			case D3D12_RESOURCE_DIMENSION_TEXTURE3D:
				d.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE3D;
				d.Texture3D.WSize = arrayCount;
				d.Texture3D.FirstWSlice = arrayStart;
				d.Texture3D.MipSlice = mipStart;
				d.Format = !resource->resource ? DXGI_FORMAT_R8G8B8A8_UNORM : resource->desc.Format;
				break;
			}
		}

		void CreateVBV(D3D12_VERTEX_BUFFER_VIEW& desc) {
			desc = { };
			desc.BufferLocation = !resource->resource ? 0 : resource->resource->GetGPUVirtualAddress();
			desc.StrideInBytes = elementStride;
			desc.SizeInBytes = (arrayStart + arrayCount) * elementStride;
		}

		void CreateIBV(D3D12_INDEX_BUFFER_VIEW& desc) {
			desc = { };
			desc.BufferLocation = !resource->resource ? 0 : resource->resource->GetGPUVirtualAddress();
			desc.Format = !resource ? DXGI_FORMAT_UNKNOWN : elementStride == 2 ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;
			desc.SizeInBytes = (arrayStart + arrayCount) * elementStride;
		}

		void CreateDSVDesc(D3D12_DEPTH_STENCIL_VIEW_DESC& d)
		{
			d.Texture2DArray.ArraySize = arrayCount;
			d.Texture2DArray.FirstArraySlice = arrayStart;
			d.Texture2DArray.MipSlice = mipStart;
			d.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
			d.Format = !resource->resource ? DXGI_FORMAT_UNKNOWN : resource->desc.Format;
		}

		void CreateCBVDesc(D3D12_CONSTANT_BUFFER_VIEW_DESC& d) {
			d.BufferLocation = !resource ? 0 : resource->resource->GetGPUVirtualAddress();
			d.SizeInBytes = (this->elementStride * this->arrayCount + 255) & ~255;
		}

		int getSRV() {
			if ((handle_mask & 1) != 0)
				return srv_cached_handle;

			mutex.Acquire();
			if ((handle_mask & 1) == 0)
			{
				D3D12_SHADER_RESOURCE_VIEW_DESC d;
				ZeroMemory(&d, sizeof(D3D12_SHADER_RESOURCE_VIEW_DESC));
				CreateSRVDesc(d);
				srv_cached_handle = resource->dxWrapper->cpu_csu->AllocateNewHandle();
				resource->dxWrapper->device->CreateShaderResourceView(!resource ? nullptr : resource->resource, &d, resource->dxWrapper->cpu_csu->getCPUVersion(srv_cached_handle));
				handle_mask |= 1;
			}
			mutex.Release();
			return srv_cached_handle;
		}

		int getUAV() {
			if ((handle_mask & 2) != 0)
				return uav_cached_handle;

			mutex.Acquire();
			if ((handle_mask & 2) == 0)
			{
				D3D12_UNORDERED_ACCESS_VIEW_DESC d;
				ZeroMemory(&d, sizeof(D3D12_UNORDERED_ACCESS_VIEW_DESC));
				CreateUAVDesc(d);
				uav_cached_handle = resource->dxWrapper->cpu_csu->AllocateNewHandle();
				resource->dxWrapper->device->CreateUnorderedAccessView(!resource ? nullptr : resource->resource, nullptr, &d, resource->dxWrapper->cpu_csu->getCPUVersion(uav_cached_handle));
				handle_mask |= 2;
			}
			mutex.Release();
			return uav_cached_handle;
		}


		int getCBV() {
			if ((handle_mask & 4) != 0)
				return cbv_cached_handle;

			mutex.Acquire();
			if ((handle_mask & 4) == 0)
			{
				D3D12_CONSTANT_BUFFER_VIEW_DESC d;
				ZeroMemory(&d, sizeof(D3D12_CONSTANT_BUFFER_VIEW_DESC));
				CreateCBVDesc(d);
				cbv_cached_handle = resource->dxWrapper->cpu_csu->AllocateNewHandle();
				resource->dxWrapper->device->CreateConstantBufferView(&d, resource->dxWrapper->cpu_csu->getCPUVersion(cbv_cached_handle));
				handle_mask |= 4;
			}
			mutex.Release();
			return cbv_cached_handle;
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

		int getDSV() {

			if ((handle_mask & 16) != 0)
				return dsv_cached_handle;

			mutex.Acquire();

			if ((handle_mask & 16) == 0) {
				D3D12_DEPTH_STENCIL_VIEW_DESC d;
				ZeroMemory(&d, sizeof(D3D12_DEPTH_STENCIL_VIEW_DESC));
				CreateDSVDesc(d);
				dsv_cached_handle = resource->dxWrapper->cpu_ds->AllocateNewHandle();
				resource->dxWrapper->device->CreateDepthStencilView(!resource ? nullptr : resource->resource, &d, resource->dxWrapper->cpu_ds->getCPUVersion(dsv_cached_handle));
				handle_mask |= 16;
			}

			mutex.Release();
			return dsv_cached_handle;
		}

		D3D12_CPU_DESCRIPTOR_HANDLE getDSVHandle() {
			return resource->dxWrapper->cpu_ds->getCPUVersion(getDSV());
		}

		D3D12_CPU_DESCRIPTOR_HANDLE getRTVHandle() {
			return resource->dxWrapper->cpu_rt->getCPUVersion(getRTV());
		}

		D3D12_CPU_DESCRIPTOR_HANDLE getSRVHandle() {
			return resource->dxWrapper->cpu_csu->getCPUVersion(getSRV());
		}

		D3D12_CPU_DESCRIPTOR_HANDLE getCBVHandle() {
			return resource->dxWrapper->cpu_csu->getCPUVersion(getCBV());
		}

		D3D12_CPU_DESCRIPTOR_HANDLE getUAVHandle() {
			return resource->dxWrapper->cpu_csu->getCPUVersion(getUAV());
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
				result->arrayCount = this->arrayCount;
			}

			return result;
		}
	};
	
	struct DX_CmdWrapper {
		DX_Wrapper* dxWrapper;
		DX_CommandList cmdList = nullptr;
		gObj<IPipelineBindings> currentPipeline = nullptr;
		gObj<RTProgramBase> activeProgram = nullptr;
		list<D3D12_CPU_DESCRIPTOR_HANDLE> srcDescriptors = {};
		list<D3D12_CPU_DESCRIPTOR_HANDLE> dstDescriptors = {};
		list<unsigned int> dstDescriptorRangeLengths = {};
		int vertexBufferSliceOffset = 0;
		int indexBufferSliceOffset = 0;
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
		Tagging Tag;
	};

	// This class is intended to manage all command queues, threads, allocators and command lists
	struct GPUScheduler {
		DX_Wrapper* dxWrapper;

		PerEngineInfo Engines[3] = { }; // 0- direct, 1- compute, 2 - copy

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
		Tagging Tag;
		
		// Gets whenever there is an async cmd list pendent.
		// This is used to know when to force a flush before changing render state at cmd list 0 to Present
		bool AsyncWorkPending = false;

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

			this->ActiveCmdLists = new DX_CommandList[threads * 3];

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

			for (int i = 0; i < 3; i++)
			{
				PerEngineInfo& info = Engines[i];

				D3D12_COMMAND_LIST_TYPE type;
				switch (i) {
				case 0: // Graphics
					type = D3D12_COMMAND_LIST_TYPE_DIRECT;
					break;
				case 1: // Compute
					type = D3D12_COMMAND_LIST_TYPE_COMPUTE;
					break;
				case 2:
					type = D3D12_COMMAND_LIST_TYPE_COPY;
					break;
				}

				// Create queue (REUSE queue of graphics for raytracing)
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
						info.threadInfos[j].manager = new RaytracingManager();
						break;
					case Engine::Compute:
						info.threadInfos[j].manager = new ComputeManager();
						break;
					case Engine::Copy:
						info.threadInfos[j].manager = new CopyManager();
						break;
					}

					DX_CmdWrapper* cmdWrapper = new DX_CmdWrapper();
					cmdWrapper->dxWrapper = this->dxWrapper;
					cmdWrapper->cmdList = info.threadInfos[j].cmdList;
					info.threadInfos[j].manager->__InternalDXCmdWrapper = cmdWrapper;
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

			for (int e = 0; e < 3; e++)
				Engines[e].frames[frame].ResetUsedAllocators();

			dxWrapper->gpu_csu->RestartAllocatorForFrame(frame);
			dxWrapper->gpu_smp->RestartAllocatorForFrame(frame);

			PrepareRenderTarget(D3D12_RESOURCE_STATE_RENDER_TARGET);
		}

		void PrepareRenderTarget(D3D12_RESOURCE_STATES state) {
			if (!Engines[0].threadInfos[0].isActive)
			{ // activate main cmd.
				Engines[0].threadInfos[0].Activate(
					Engines[0].frames[CurrentFrameIndex].allocatorSet[0]
				);
			}
			// Get current RT
			auto currentRT = this->dxWrapper->RenderTargets[this->CurrentFrameIndex];
			// Place a barrier at thread 0 cmdList to Present
			DX_ResourceWrapper* rtWrapper = (DX_ResourceWrapper*)currentRT->__InternalDXWrapper;
			rtWrapper->AddBarrier(this->Engines[0].threadInfos[0].cmdList, state);
		}

		void FinishFrame() {
			if (this->AsyncWorkPending)
				// Ensure all async work was done before.
				FlushAndSignal(EngineMask::All).WaitFor();

			PrepareRenderTarget(D3D12_RESOURCE_STATE_PRESENT);

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