#include "private_ca4g_presenter.h"

namespace CA4G {

	ResourceView::ResourceView(void* internalDXWrapper, void* internalViewWrapper)
		: __InternalDXWrapper(internalDXWrapper),
		__InternalViewWrapper(internalViewWrapper) {
		((DX_ResourceWrapper*)this->__InternalDXWrapper)->references++;

		if (internalDXWrapper == nullptr) {
			// Resource View used for Null Descriptors
			return;
		}

		if (this->__InternalViewWrapper == nullptr) {
			// Get default view from resource description
			auto desc = ((DX_ResourceWrapper*)this->__InternalDXWrapper)->desc;
			DX_ViewWrapper* view = new DX_ViewWrapper((DX_ResourceWrapper*)internalDXWrapper);
			view->elementStride = ((DX_ResourceWrapper*)this->__InternalDXWrapper)->elementStride;
			view->arrayStart = 0;
			view->arrayCount = desc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER ?
				this->getSizeInBytes() / view->elementStride :
				desc.DepthOrArraySize;
			view->mipStart = 0;
			view->mipCount = desc.MipLevels;
			view->ViewDimension = desc.Dimension;

			this->__InternalViewWrapper = view;
		}
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

	gObj<Buffer> ResourceView::AsBuffer(int elementWidth) {
		DX_ViewWrapper* wrapper = new DX_ViewWrapper(((DX_ViewWrapper*) this->__InternalViewWrapper)->resource);
		wrapper->arrayCount = this->getSizeInBytes() / elementWidth;
		wrapper->arrayStart = 0;
		wrapper->mipStart = 0;
		wrapper->mipCount = 1;
		wrapper->elementStride = elementWidth;
		wrapper->ViewDimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		return new Buffer(
			this->__InternalDXWrapper,
			wrapper,
			elementWidth,
			this->getSizeInBytes() / elementWidth);
	}

	gObj<Texture1D> ResourceView::AsTexture1D(DXGI_FORMAT format, int width, int mips, int arrayLength)
	{
		DX_ViewWrapper* wrapper = new DX_ViewWrapper(((DX_ViewWrapper*)this->__InternalViewWrapper)->resource);
		wrapper->arrayCount = arrayLength;
		wrapper->arrayStart = 0;
		wrapper->mipStart = 0;
		wrapper->mipCount = mips;
		wrapper->ViewDimension = D3D12_RESOURCE_DIMENSION_TEXTURE1D;
		return new Texture1D(
			this->__InternalDXWrapper,
			wrapper,
			format, width, mips, arrayLength);
	}

	gObj<Texture2D> ResourceView::AsTexture2D(DXGI_FORMAT format, int width, int height, int mips, int arrayLength) {
		DX_ViewWrapper* wrapper = new DX_ViewWrapper(((DX_ViewWrapper*)this->__InternalViewWrapper)->resource);
		wrapper->arrayCount = arrayLength;
		wrapper->arrayStart = 0;
		wrapper->mipStart = 0;
		wrapper->mipCount = mips;
		wrapper->ViewDimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		return new Texture2D(
			this->__InternalDXWrapper,
			wrapper,
			format, width, height, mips, arrayLength);
	}

	gObj<Texture3D> ResourceView::AsTexture3D(DXGI_FORMAT format, int width, int height, int depth, int mips) {
		DX_ViewWrapper* wrapper = new DX_ViewWrapper(((DX_ViewWrapper*)this->__InternalViewWrapper)->resource);
		wrapper->arrayCount = depth;
		wrapper->arrayStart = 0;
		wrapper->mipStart = 0;
		wrapper->mipCount = mips;
		wrapper->ViewDimension = D3D12_RESOURCE_DIMENSION_TEXTURE3D;
		return new Texture3D(
			this->__InternalDXWrapper,
			wrapper,
			format, width, height, depth, mips);
	}

	gObj<Buffer> Buffer::Slice(int startElement, int count) {
		return new Buffer(
			this->__InternalDXWrapper,
			((DX_ViewWrapper*)this->__InternalViewWrapper)->
				createSlicedClone(0, 0, startElement, count),
			this->Stride, count);
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