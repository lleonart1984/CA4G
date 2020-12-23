#include "private_ca4g_presenter.h"

namespace CA4G {

	ResourceView::ResourceView(DX_ResourceWrapper* internalDXWrapper, DX_ViewWrapper* internalViewWrapper)
		: __InternalDXWrapper(internalDXWrapper),
		__InternalViewWrapper(internalViewWrapper) {

		DX_ViewWrapper* view = internalViewWrapper ? internalViewWrapper : new DX_ViewWrapper(internalDXWrapper);
		__InternalViewWrapper = view;
		(__InternalDXWrapper)->references++;
		if (internalViewWrapper == nullptr) { // Get view slice info from original resource description
			// Get default view from resource description
			auto desc = __InternalDXWrapper->desc;
			view->elementStride = __InternalDXWrapper->elementStride;
			view->arrayStart = 0;
			view->arrayCount = desc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER ?
				this->getSizeInBytes() / view->elementStride :
				desc.DepthOrArraySize;
			view->mipStart = 0;
			view->mipCount = desc.MipLevels;
			view->ViewDimension = desc.Dimension;
		}
	}

	gObj<ResourceView> CA4G::ResourceView::CreateNullView(DX_Wrapper* deviceManager, D3D12_RESOURCE_DIMENSION dimension)
	{
		D3D12_RESOURCE_DESC nullDesc = {
		};
		nullDesc.Dimension = dimension;
		DX_ResourceWrapper* nullresource = new DX_ResourceWrapper(deviceManager, nullptr, nullDesc, D3D12_RESOURCE_STATE_GENERIC_READ, CPUAccessibility::None);
		DX_ViewWrapper* nullview = new DX_ViewWrapper(nullresource);
		nullview->arrayCount = 1;
		nullview->mipCount = 1;
		nullview->elementStride = 1;
		nullview->ViewDimension = dimension;
		return new ResourceView(nullresource, nullview);
	}

	ResourceView::~ResourceView() {
		((DX_ResourceWrapper*)this->__InternalDXWrapper)->references--;
		if (((DX_ResourceWrapper*)this->__InternalDXWrapper)->references <= 0)
			delete (DX_ResourceWrapper*)this->__InternalDXWrapper;
		delete (DX_ViewWrapper*)this->__InternalViewWrapper;
	}

	void ResourceView::SetDebugName(LPCWSTR name) {
		__InternalResourceState->resource->SetName(name);
	}

	long ResourceView::getSizeInBytes() {
		return __InternalResourceState->pTotalSizes;
	}

	CPUAccessibility ResourceView::getCPUAccessibility() {
		return __InternalResourceState->cpuaccess;
	}

	gObj<Buffer> Buffer::Creating::Slice(int startElement, int count) {
		return new Buffer(
			buffer->__InternalDXWrapper,
			((DX_ViewWrapper*)buffer->__InternalViewWrapper)->
				createSlicedClone(0, 0, startElement, count),
			buffer->Stride, count);
	}

	gObj<Texture1D> Texture1D::SliceMips(int start, int count) {
		return new Texture1D(
			this->__InternalDXWrapper,
			((DX_ViewWrapper*)this->__InternalViewWrapper)->
				createSlicedClone(start, count, 0, 0),
			this->Format, this->Width, count, this->ArrayLength);
	}

	gObj<Texture1D> Texture1D::SliceArray(int start, int count) {
		return new Texture1D(
			this->__InternalDXWrapper,
			((DX_ViewWrapper*)this->__InternalViewWrapper)->
				createSlicedClone(0, 0, start, count),
			this->Format, this->Width, this->Mips, count);
	}

	gObj<Texture2D> Texture2D::SliceMips(int start, int count) {
		return new Texture2D(
			this->__InternalDXWrapper,
			((DX_ViewWrapper*)this->__InternalViewWrapper)->
				createSlicedClone(start, count, 0, 0),
			this->Format, this->Width, this->Height, count, this->ArrayLength);
	}

	gObj<Texture2D> Texture2D::SliceArray(int start, int count) {
		return new Texture2D(
			this->__InternalDXWrapper,
			((DX_ViewWrapper*)this->__InternalViewWrapper)->
				createSlicedClone(0, 0, start, count),
			this->Format, this->Width, this->Height, this->Mips, count);
	}

	gObj<Texture3D> Texture3D::SliceMips(int start, int count) {
		return new Texture3D(
			this->__InternalDXWrapper,
			((DX_ViewWrapper*)this->__InternalViewWrapper)->
				createSlicedClone(start, count, 0, 0),
			this->Format, this->Width, this->Height, this->Depth, count);
	}
}