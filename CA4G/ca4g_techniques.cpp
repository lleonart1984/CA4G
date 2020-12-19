#include "private_ca4g_presenter.h"
//#include "ca4g_techniques.h"

namespace CA4G {

	Technique::Technique() :set(new Setting(this)), create(new Creating(this)), dispatch(new Dispatcher(this)), load(new Loading(this)) {}

	gObj<ResourceView> Creating::CreateDXResource(
		int elementWidth, const D3D12_RESOURCE_DESC& desc, D3D12_RESOURCE_STATES initialState,
		D3D12_CLEAR_VALUE* clearing)
	{
		DX_Wrapper* w = (DX_Wrapper*)wrapper->__InternalDXWrapper;
		auto device = w->device;

		D3D12_HEAP_PROPERTIES defaultProp;
		defaultProp.Type = D3D12_HEAP_TYPE_DEFAULT;
		defaultProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		defaultProp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		defaultProp.VisibleNodeMask = 1;
		defaultProp.CreationNodeMask = 1;

		DX_Resource resource;
		auto hr = device->CreateCommittedResource(&defaultProp, D3D12_HEAP_FLAG_NONE, &desc, initialState, clearing,
			IID_PPV_ARGS(&resource));

		if (FAILED(hr))
		{
			auto _hr = device->GetDeviceRemovedReason();
			throw CA4GException::FromError(CA4G_Errors_RunOutOfMemory, nullptr, _hr);
		}

		DX_ResourceWrapper* rwrapper = new DX_ResourceWrapper((DX_Wrapper*) this->wrapper->__InternalDXWrapper, resource, desc, initialState);

		switch (desc.Dimension) {
		case D3D12_RESOURCE_DIMENSION_BUFFER:
			return new Buffer(wrapper, rwrapper, elementWidth, desc.Width / elementWidth);
		case D3D12_RESOURCE_DIMENSION_TEXTURE1D:
			return new Texture1D(wrapper, rwrapper, desc.Format, desc.Width, desc.MipLevels, desc.DepthOrArraySize);
		case D3D12_RESOURCE_DIMENSION_TEXTURE2D:
			return new Texture2D(wrapper, rwrapper, desc.Format, desc.Width, desc.Height, desc.MipLevels, desc.DepthOrArraySize);
		case D3D12_RESOURCE_DIMENSION_TEXTURE3D:
			return new Texture3D(wrapper, rwrapper, desc.Format, desc.Width, desc.Height, desc.DepthOrArraySize, desc.MipLevels);
		}
		return nullptr;
	}

	/*gObj<Texture2D> Creating::WrapBackBuffer(void* dxBackBuffer) {
		DX_Wrapper* w = (DX_Wrapper*)wrapper->__InternalDXWrapper;
		auto device = w->device;

		CComPtr<ID3D12Resource1> resource = (ID3D12Resource1*)dxBackBuffer;

		auto desc = resource->GetDesc();

		DX_ResourceWrapper* rw = new DX_ResourceWrapper(device, resource, desc, D3D12_RESOURCE_STATE_RENDER_TARGET);

		return new Texture2D(rw, nullptr, desc.Format, desc.Width, desc.Height, 1, 1);
	}*/

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

	gObj<Buffer> Creating::Buffer_CB(int elementStride) {
		D3D12_RESOURCE_DESC desc = { };
		FillBufferDescription(desc, (elementStride + 255) & (~255));
		return CreateDXResource(elementStride, desc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr).Dynamic_Cast<Buffer>();
	}

	gObj<Buffer> Creating::Buffer_ADS(int elementStride, int count) {
		D3D12_RESOURCE_DESC desc = { };
		FillBufferDescription(desc, elementStride * count, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
		return CreateDXResource(elementStride, desc, D3D12_RESOURCE_STATE_COMMON, nullptr).Dynamic_Cast<Buffer>();
	}

	gObj<Buffer> Creating::Buffer_SRV(int elementStride, int count) {
		D3D12_RESOURCE_DESC desc = { };
		FillBufferDescription(desc, elementStride * count);
		return CreateDXResource(elementStride, desc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr).Dynamic_Cast<Buffer>();
	}

	gObj<Buffer> Creating::Buffer_VB(int elementStride, int count) {
		D3D12_RESOURCE_DESC desc = { };
		FillBufferDescription(desc, elementStride * count);
		return CreateDXResource(elementStride, desc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr).Dynamic_Cast<Buffer>();
	}

	gObj<Buffer> Creating::Buffer_IB(int elementStride, int count) {
		D3D12_RESOURCE_DESC desc = { };
		FillBufferDescription(desc, elementStride * count);
		return CreateDXResource(elementStride, desc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr).Dynamic_Cast<Buffer>();
	}

	gObj<Buffer> Creating::Buffer_UAV(int elementStride, int count) {
		D3D12_RESOURCE_DESC desc = { };
		FillBufferDescription(desc, elementStride * count, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
		return CreateDXResource(elementStride, desc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nullptr).Dynamic_Cast<Buffer>();
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
		return CreateDXResource(0, desc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr).Dynamic_Cast<Texture1D>();
	}

	gObj<Texture1D> Creating::Texture1D_UAV(DXGI_FORMAT format, int width, int mips, int arrayLength) {
		D3D12_RESOURCE_DESC desc = { };
		FillTexture1DDescription(format, desc, width, mips, arrayLength, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
		return CreateDXResource(0, desc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nullptr).Dynamic_Cast<Texture1D>();
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
		desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		desc.SampleDesc = { 1, 0 };
	}


	gObj<Texture2D> Creating::Texture2D_SRV(DXGI_FORMAT format, int width, int height, int mips, int arrayLength) {
		D3D12_RESOURCE_DESC desc = { };
		FillTexture2DDescription(format, desc, width, height, mips, arrayLength);
		return CreateDXResource(0, desc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr).Dynamic_Cast<Texture2D>();
	}

	gObj<Texture2D> Creating::Texture2D_UAV(DXGI_FORMAT format, int width, int height, int mips, int arrayLength) {
		D3D12_RESOURCE_DESC desc = { };
		FillTexture2DDescription(format, desc, width, height, mips, arrayLength, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
		return CreateDXResource(0, desc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS | D3D12_RESOURCE_STATE_RENDER_TARGET, nullptr).Dynamic_Cast<Texture2D>();
	}

	gObj<Texture2D> Creating::Texture2D_RT(DXGI_FORMAT format, int width, int height, int mips, int arrayLength) {
		D3D12_RESOURCE_DESC desc = { };
		FillTexture2DDescription(format, desc, width, height, mips, arrayLength, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
		return CreateDXResource(0, desc, D3D12_RESOURCE_STATE_RENDER_TARGET, nullptr).Dynamic_Cast<Texture2D>();
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
		return CreateDXResource(0, desc, D3D12_RESOURCE_STATE_DEPTH_WRITE, &clearing).Dynamic_Cast<Texture2D>();
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
		desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		desc.SampleDesc = { 1, 0 };
	}

	gObj<Texture3D> Creating::Texture3D_SRV(DXGI_FORMAT format, int width, int height, int depth, int mips) {
		D3D12_RESOURCE_DESC desc = { };
		FillTexture3DDescription(format, desc, width, height, depth, mips);
		return CreateDXResource(0, desc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr).Dynamic_Cast<Texture3D>();
	}

	gObj<Texture3D> Creating::Texture3D_UAV(DXGI_FORMAT format, int width, int height, int depth, int mips) {
		D3D12_RESOURCE_DESC desc = { };
		FillTexture3DDescription(format, desc, width, height, depth, mips, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
		return CreateDXResource(0, desc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nullptr).Dynamic_Cast<Texture3D>();
	}

	gObj<Texture2D> IDXWrapper::GetRenderTarget() {
		return __InternalState->RenderTargets[__InternalState->swapChain->GetCurrentBackBufferIndex()];
	}

	Signal Creating::FlushAndSignal(EngineMask mask) {
		DX_Wrapper* w = (DX_Wrapper*)this->wrapper->__InternalDXWrapper;
		return w->scheduler->FlushAndSignal(mask);
	}

	void Signal::WaitFor() {
		((GPUScheduler*)this->scheduler)->WaitFor(*this);
	}

	void Technique::Setting::Tag(gObj<TagData> data)
	{
		((DX_Wrapper*)this->wrapper->__InternalDXWrapper)->scheduler->Tag = data;
	}
}