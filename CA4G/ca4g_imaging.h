#ifndef CA4G_IMAGING_H
#define CA4G_IMAGING_H

#include "ca4g_memory.h"
#include "DirectXTex.h"
#include <Windows.h>
#include <atlbase.h>
#include <d3d12.h>
#include <d3d12sdklayers.h>
#include <d3d12shader.h>
#include <dxgi1_4.h>
#ifdef _DEBUG
#include <dxgidebug.h>
#endif

namespace CA4G {

	struct TextureData {
		int const Width;
		int const Height;
		int const MipMaps;
		union {
			int const ArraySlices;
			int const Depth;
		};
		DXGI_FORMAT const Format;
		byte* const Data;

		D3D12_PLACED_SUBRESOURCE_FOOTPRINT* footprints;

		long long const DataSize;
		bool const FlipY;
		D3D12_RESOURCE_DIMENSION Dimension;

		TextureData(int width, int height, int mipMaps, int slices, DXGI_FORMAT format, byte* data, long long dataSize, bool flipY = false);

		TextureData(int width, int height, int depth, DXGI_FORMAT format, byte* data, long long dataSize, bool flipY = false);

		~TextureData();

		static gObj<TextureData> CreateEmpty(int width, int height, int mipMaps, int slices, DXGI_FORMAT format);

		static gObj<TextureData> CreateEmpty(int width, int height, int depth, DXGI_FORMAT format);

		static gObj<TextureData> LoadFromFile(const char* filename);

		static void SaveToFile(gObj<TextureData> data, const char* filename);

	private:
		int pixelStride;

	};
}

#endif