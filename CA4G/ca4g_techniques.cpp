#include "private_ca4g_presenter.h"
//#include "ca4g_techniques.h"
#include "ca4g_imaging.h"

namespace CA4G {

	Technique::Technique() :set(new Setting(this)), create(new Creating(this)), dispatch(new Dispatcher(this)), load(new Loading(this)) {}

	gObj<ResourceView> Creating::CustomDXResource(
		DX_Wrapper* dxWrapper,
		int elementWidth, const D3D12_RESOURCE_DESC& desc, D3D12_RESOURCE_STATES initialState,
		D3D12_CLEAR_VALUE* clearing, CPUAccessibility cpuAccessibility, bool isConstantBuffer)
	{
		if (elementWidth == 0)
			elementWidth = DX_ResourceWrapper::SizeOfFormatElement(desc.Format);

		DX_Wrapper* w = dxWrapper;
		auto device = w->device;

		D3D12_HEAP_PROPERTIES heapProp;

		switch (cpuAccessibility) {
		case CPUAccessibility::None:
			heapProp.Type = D3D12_HEAP_TYPE_DEFAULT;
			heapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
			heapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
			heapProp.VisibleNodeMask = 1;
			heapProp.CreationNodeMask = 1;
			break;
		case CPUAccessibility::Write:
			heapProp.Type = D3D12_HEAP_TYPE_UPLOAD;
			heapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
			heapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
			heapProp.VisibleNodeMask = 1;
			heapProp.CreationNodeMask = 1;
			break;
		case CPUAccessibility::Read:
			heapProp.Type = D3D12_HEAP_TYPE_READBACK;
			heapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
			heapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
			heapProp.VisibleNodeMask = 1;
			heapProp.CreationNodeMask = 1;
			break;
		}

		DX_Resource resource;
		auto hr = device->CreateCommittedResource(&heapProp, D3D12_HEAP_FLAG_NONE, &desc, initialState, clearing,
			IID_PPV_ARGS(&resource));

		if (FAILED(hr))
		{
			auto _hr = device->GetDeviceRemovedReason();
			throw CA4GException::FromError(CA4G_Errors_RunOutOfMemory, nullptr, _hr);
		}

		DX_ResourceWrapper* rwrapper = new DX_ResourceWrapper(dxWrapper, resource, desc, initialState, cpuAccessibility);
		rwrapper->elementStride = rwrapper->desc.Format == DXGI_FORMAT_UNKNOWN ? elementWidth : rwrapper->SizeOfFormatElement(rwrapper->desc.Format);
		switch (desc.Dimension) {
		case D3D12_RESOURCE_DIMENSION_BUFFER:
			return new Buffer(rwrapper, nullptr, elementWidth, isConstantBuffer ? 1 : desc.Width / elementWidth);
		case D3D12_RESOURCE_DIMENSION_TEXTURE1D:
			return new Texture1D(rwrapper, nullptr, desc.Format, desc.Width, desc.MipLevels, desc.DepthOrArraySize);
		case D3D12_RESOURCE_DIMENSION_TEXTURE2D:
			return new Texture2D(rwrapper, nullptr, desc.Format, desc.Width, desc.Height, desc.MipLevels, desc.DepthOrArraySize);
		case D3D12_RESOURCE_DIMENSION_TEXTURE3D:
			return new Texture3D(rwrapper, nullptr, desc.Format, desc.Width, desc.Height, desc.DepthOrArraySize, desc.MipLevels);
		}
		return nullptr;
	}

	void FillBufferDescription(D3D12_RESOURCE_DESC& desc, long width, D3D12_RESOURCE_FLAGS flag = D3D12_RESOURCE_FLAG_NONE) {
		desc.Width = width;
		desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		desc.Flags = flag;
		desc.Format = DXGI_FORMAT_UNKNOWN;
		desc.Height = 1;
		desc.Alignment = 0;
		desc.DepthOrArraySize = 1;
		desc.MipLevels = 1;
		desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		desc.SampleDesc = { 1, 0 };
	}

	gObj<Buffer> Creating::Buffer_CB(int elementStride, bool dynamic) {
		D3D12_RESOURCE_DESC desc = { };
		FillBufferDescription(desc, (elementStride + 255) & (~255));
		return CustomDXResource(manager->__InternalDXWrapper, elementStride, desc, dynamic ? D3D12_RESOURCE_STATE_GENERIC_READ : D3D12_RESOURCE_STATE_COPY_DEST, nullptr, dynamic ? CPUAccessibility::Write : CPUAccessibility::None, true).Dynamic_Cast<Buffer>();
	}

	gObj<Buffer> Creating::Buffer_ADS(int elementStride, int count) {
		if (count == 0) return nullptr;
		D3D12_RESOURCE_DESC desc = { };
		FillBufferDescription(desc, elementStride * count, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
		return CustomDXResource(manager->__InternalDXWrapper, elementStride, desc, D3D12_RESOURCE_STATE_COMMON | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, nullptr).Dynamic_Cast<Buffer>();
	}

	gObj<Buffer> Creating::Buffer_SRV(int elementStride, int count) {
		if (count == 0) return nullptr;
		D3D12_RESOURCE_DESC desc = { };
		FillBufferDescription(desc, elementStride * count);
		return CustomDXResource(manager->__InternalDXWrapper, elementStride, desc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr).Dynamic_Cast<Buffer>();
	}

	gObj<Buffer> Creating::Buffer_VB(int elementStride, int count) {
		if (count == 0) return nullptr;
		D3D12_RESOURCE_DESC desc = { };
		FillBufferDescription(desc, elementStride * count);
		return CustomDXResource(manager->__InternalDXWrapper, elementStride, desc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr).Dynamic_Cast<Buffer>();
	}

	gObj<Buffer> Creating::Buffer_IB(int elementStride, int count)
	{
		if (count == 0) return nullptr;
		D3D12_RESOURCE_DESC desc = { };
		FillBufferDescription(desc, elementStride * count);
		return CustomDXResource(manager->__InternalDXWrapper, elementStride, desc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr).Dynamic_Cast<Buffer>();
	}

	gObj<Buffer> Creating::Buffer_UAV(int elementStride, int count) {
		if (count == 0) return nullptr;
		D3D12_RESOURCE_DESC desc = { };
		FillBufferDescription(desc, elementStride * count, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
		return CustomDXResource(manager->__InternalDXWrapper, elementStride, desc, D3D12_RESOURCE_STATE_COMMON, nullptr).Dynamic_Cast<Buffer>();
	}

	int MaxMipsFor(int dimension) {
		int mips = 0;
		while (dimension > 0)
		{
			mips++;
			dimension >>= 1;
		}
		return mips;
	}

	void FillTexture1DDescription(DXGI_FORMAT format, D3D12_RESOURCE_DESC& desc, long width, int mips, int arrayLength, D3D12_RESOURCE_FLAGS flag = D3D12_RESOURCE_FLAG_NONE) {
		if (mips == 0) // compute maximum possible value
			mips = 100000;
		desc.Width = width;
		desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE1D;
		desc.Flags = flag;
		desc.Format = format;
		desc.Height = 1;
		desc.Alignment = 0;
		desc.DepthOrArraySize = arrayLength;
		desc.MipLevels = min(mips, MaxMipsFor(width));
		desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		desc.SampleDesc = { 1, 0 };
	}

	gObj<Texture1D> Creating::Texture1D_SRV(DXGI_FORMAT format, int width, int mips, int arrayLength) {
		D3D12_RESOURCE_DESC desc = { };
		FillTexture1DDescription(format, desc, width, mips, arrayLength);
		return CustomDXResource(manager->__InternalDXWrapper, 0, desc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr).Dynamic_Cast<Texture1D>();
	}

	gObj<Texture1D> Creating::Texture1D_UAV(DXGI_FORMAT format, int width, int mips, int arrayLength) {
		D3D12_RESOURCE_DESC desc = { };
		FillTexture1DDescription(format, desc, width, mips, arrayLength, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
		return CustomDXResource(manager->__InternalDXWrapper, 0, desc, D3D12_RESOURCE_STATE_COMMON, nullptr).Dynamic_Cast<Texture1D>();
	}

	void FillTexture2DDescription(DXGI_FORMAT format, D3D12_RESOURCE_DESC& desc, long width, int height, int mips, int arrayLength, D3D12_RESOURCE_FLAGS flag = D3D12_RESOURCE_FLAG_NONE) {
		if (mips == 0) // compute maximum possible value
			mips = 100000;

		desc.Width = width;
		desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		desc.Flags = flag;
		desc.Format = format;
		desc.Height = height;
		desc.Alignment = 0;
		desc.DepthOrArraySize = arrayLength;
		desc.MipLevels = min(mips, MaxMipsFor(min(width, height)));
		desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		desc.SampleDesc = { 1, 0 };
	}


	gObj<Texture2D> Creating::Texture2D_SRV(DXGI_FORMAT format, int width, int height, int mips, int arrayLength) {
		D3D12_RESOURCE_DESC desc = { };
		FillTexture2DDescription(format, desc, width, height, mips, arrayLength);
		return CustomDXResource(manager->__InternalDXWrapper, 0, desc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr).Dynamic_Cast<Texture2D>();
	}

	gObj<Texture2D> Creating::Texture2D_SRV(string filePath)
	{
		auto data = TextureData::LoadFromFile(filePath.c_str());

		gObj<Texture2D> texture = Texture2D_SRV(data->Format, data->Width, data->Height, data->MipMaps, data->ArraySlices);

		texture->copy->FromPtr(data->Data, true);

		return texture;
	}

	gObj<Texture2D> Creating::Texture2D_UAV(DXGI_FORMAT format, int width, int height, int mips, int arrayLength) {
		D3D12_RESOURCE_DESC desc = { };
		FillTexture2DDescription(format, desc, width, height, mips, arrayLength, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS | D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);
		return CustomDXResource(manager->__InternalDXWrapper, 0, desc, D3D12_RESOURCE_STATE_COMMON, nullptr).Dynamic_Cast<Texture2D>();
	}

	gObj<Texture2D> Creating::Texture2D_RT(DXGI_FORMAT format, int width, int height, int mips, int arrayLength) {
		D3D12_RESOURCE_DESC desc = { };
		FillTexture2DDescription(format, desc, width, height, mips, arrayLength, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);
		return CustomDXResource(manager->__InternalDXWrapper, 0, desc, D3D12_RESOURCE_STATE_RENDER_TARGET, nullptr).Dynamic_Cast<Texture2D>();
	}

	gObj<Texture2D> Creating::Texture2D_DSV(int width, int height, DXGI_FORMAT format) {
		D3D12_RESOURCE_DESC desc = { };
		FillTexture2DDescription(format, desc, width, height, 1, 1, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);
		D3D12_CLEAR_VALUE clearing;
		clearing.DepthStencil = D3D12_DEPTH_STENCIL_VALUE{
			1.0f,
			0
		};
		clearing.Format = format;
		return CustomDXResource(manager->__InternalDXWrapper, 0, desc, D3D12_RESOURCE_STATE_DEPTH_WRITE, &clearing).Dynamic_Cast<Texture2D>();
	}

	void FillTexture3DDescription(DXGI_FORMAT format, D3D12_RESOURCE_DESC& desc, long width, int height, int slices, int mips, D3D12_RESOURCE_FLAGS flag = D3D12_RESOURCE_FLAG_NONE) {
		if (mips == 0) // compute maximum possible value
			mips = 100000;

		desc.Width = width;
		desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE3D;
		desc.Flags = flag;
		desc.Format = format;
		desc.Height = height;
		desc.Alignment = 0;
		desc.DepthOrArraySize = slices;
		desc.MipLevels = min(mips, MaxMipsFor(min(min(width, height), slices)));
		desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		desc.SampleDesc = { 1, 0 };
	}

	gObj<Texture3D> Creating::Texture3D_SRV(DXGI_FORMAT format, int width, int height, int depth, int mips) {
		D3D12_RESOURCE_DESC desc = { };
		FillTexture3DDescription(format, desc, width, height, depth, mips);
		return CustomDXResource(manager->__InternalDXWrapper, 0, desc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr).Dynamic_Cast<Texture3D>();
	}

	gObj<Texture3D> Creating::Texture3D_UAV(DXGI_FORMAT format, int width, int height, int depth, int mips) {
		D3D12_RESOURCE_DESC desc = { };
		FillTexture3DDescription(format, desc, width, height, depth, mips, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
		return CustomDXResource(manager->__InternalDXWrapper, 0, desc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nullptr).Dynamic_Cast<Texture3D>();
	}

	gObj<Texture2D> IDXDeviceManager::GetRenderTarget() {
		return __InternalState->RenderTargets[__InternalState->scheduler->CurrentFrameIndex];// swapChain->GetCurrentBackBufferIndex()];
	}

	Signal Creating::FlushAndSignal(EngineMask mask) {
		DX_Wrapper* w = manager->__InternalDXWrapper;
		return w->scheduler->FlushAndSignal(mask);
	}

	void Signal::WaitFor() {
		((GPUScheduler*)this->scheduler)->WaitFor(*this);
	}

	void Technique::Setting::Tag(Tagging data)
	{
		auto scheduler = this->manager->__InternalDXWrapper->scheduler;
		scheduler->Tag = data;
	}
}