#ifndef CA4G_RESOURCES_H
#define CA4G_RESOURCES_H

#include "ca4g_definitions.h"
#include "ca4g_memory.h"
#include "ca4g_math.h"

namespace CA4G {

	class ResourceView;
	class Buffer;
	class Texture1D;
	class Texture2D;
	class Texture3D;

#pragma region Color Formats

	typedef struct ARGB
	{
		unsigned int value;

		int Alpha() { return value >> 24; }
		int Red() { return (value & 0xFF0000) >> 16; }
		int Green() { return (value & 0xFF00) >> 8; }
		int Blue() { return (value & 0xFF); }

		ARGB() { value = 0; }

		ARGB(int alpha, int red, int green, int blue) {
			value =
				((unsigned int)alpha) << 24 |
				((unsigned int)red) << 16 |
				((unsigned int)green) << 8 |
				((unsigned int)blue);
		}
	} ARGB;

	typedef struct RGBA
	{
		unsigned int value;

		int Alpha() { return value >> 24; }
		int Blue() { return (value & 0xFF0000) >> 16; }
		int Green() { return (value & 0xFF00) >> 8; }
		int Red() { return (value & 0xFF); }

		RGBA() { value = 0; }

		RGBA(int alpha, int red, int green, int blue) {
			value =
				((unsigned int)alpha) << 24 |
				((unsigned int)blue) << 16 |
				((unsigned int)green) << 8 |
				((unsigned int)red);
		}
	} RGBA;

#pragma endregion

#pragma region Formats from Generic type

	template<typename T>
	struct Formats {
		static const DXGI_FORMAT Value = DXGI_FORMAT_UNKNOWN;
		static const int Size = 1;
	};
	template<>
	struct Formats<char> {
		static const DXGI_FORMAT Value = DXGI_FORMAT_R8_SINT;
		static const int Size = 1;
	};
	template<>
	struct Formats<float> {
		static const DXGI_FORMAT Value = DXGI_FORMAT_R32_FLOAT;
		static const int Size = 4;
	};
	template<>
	struct Formats <int> {
		static const DXGI_FORMAT Value = DXGI_FORMAT_R32_SINT;
		static const int Size = 4;
	};
	template<>
	struct Formats <uint> {
		static const DXGI_FORMAT Value = DXGI_FORMAT_R32_UINT;
		static const int Size = 4;
	};

	template<>
	struct Formats<float1> {
		static const DXGI_FORMAT Value = DXGI_FORMAT_R32_FLOAT;
		static const int Size = 4;
	};
	template<>
	struct Formats <int1> {
		static const DXGI_FORMAT Value = DXGI_FORMAT_R32_SINT;
		static const int Size = 4;
	};
	template<>
	struct Formats <uint1> {
		static const DXGI_FORMAT Value = DXGI_FORMAT_R32_UINT;
		static const int Size = 4;
	};

	template<>
	struct Formats <ARGB> {
		static const DXGI_FORMAT Value = DXGI_FORMAT_B8G8R8A8_UNORM;
		static const int Size = 4;
	};

	template<>
	struct Formats <RGBA> {
		static const DXGI_FORMAT Value = DXGI_FORMAT_R8G8B8A8_UNORM;
		static const int Size = 4;
	};

	template<>
	struct Formats <float2> {
		static const DXGI_FORMAT Value = DXGI_FORMAT_R32G32_FLOAT;
		static const int Size = 8;
	};
	template<>
	struct Formats <float3> {
		static const DXGI_FORMAT Value = DXGI_FORMAT_R32G32B32_FLOAT;
		static const int Size = 12;
	};
	template<>
	struct Formats <float4> {
		static const DXGI_FORMAT Value = DXGI_FORMAT_R32G32B32A32_FLOAT;
		static const int Size = 16;
	};

	template<>
	struct Formats <int2> {
		static const DXGI_FORMAT Value = DXGI_FORMAT_R32G32_SINT;
		static const int Size = 8;
	};
	template<>
	struct Formats <int3> {
		static const DXGI_FORMAT Value = DXGI_FORMAT_R32G32B32_SINT;
		static const int Size = 12;
	};
	template<>
	struct Formats <int4> {
		static const DXGI_FORMAT Value = DXGI_FORMAT_R32G32B32A32_SINT;
		static const int Size = 16;
	};

	template<>
	struct Formats <uint2> {
		static const DXGI_FORMAT Value = DXGI_FORMAT_R32G32_UINT;
		static const int Size = 8;
	};
	template<>
	struct Formats <uint3> {
		static const DXGI_FORMAT Value = DXGI_FORMAT_R32G32B32_UINT;
		static const int Size = 12;
	};
	template<>
	struct Formats <uint4> {
		static const DXGI_FORMAT Value = DXGI_FORMAT_R32G32B32A32_UINT;
		static const int Size = 16;
	};

#pragma endregion

	// Represents a sampler object
	struct Sampler {
		D3D12_FILTER Filter;
		D3D12_TEXTURE_ADDRESS_MODE AddressU;
		D3D12_TEXTURE_ADDRESS_MODE AddressV;
		D3D12_TEXTURE_ADDRESS_MODE AddressW;
		FLOAT MipLODBias;
		UINT MaxAnisotropy;
		D3D12_COMPARISON_FUNC ComparisonFunc;
		float4 BorderColor;
		FLOAT MinLOD;
		FLOAT MaxLOD;

		// Creates a default point sampling object.
		static Sampler Point(
			D3D12_TEXTURE_ADDRESS_MODE AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP,
			D3D12_TEXTURE_ADDRESS_MODE AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP,
			D3D12_TEXTURE_ADDRESS_MODE AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP
		)
		{
			return Create(D3D12_FILTER_MIN_MAG_MIP_POINT,
				AddressU,
				AddressV,
				AddressW);
		}

		// Creates a default point sampling object.
		static Sampler PointWithoutMipMaps(
			D3D12_TEXTURE_ADDRESS_MODE AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP,
			D3D12_TEXTURE_ADDRESS_MODE AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP,
			D3D12_TEXTURE_ADDRESS_MODE AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP
		)
		{
			return Create(D3D12_FILTER_MIN_MAG_MIP_POINT,
				AddressU,
				AddressV,
				AddressW, 0, 0, D3D12_COMPARISON_FUNC_ALWAYS, float4(0, 0, 0, 0), 0, 0);
		}

		// Creates a default linear sampling object
		static Sampler Linear(
			D3D12_TEXTURE_ADDRESS_MODE AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP,
			D3D12_TEXTURE_ADDRESS_MODE AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP,
			D3D12_TEXTURE_ADDRESS_MODE AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP
		)
		{
			return Create(D3D12_FILTER_MIN_MAG_MIP_LINEAR,
				AddressU,
				AddressV,
				AddressW);
		}

		static Sampler LinearWithoutMipMaps(
			D3D12_TEXTURE_ADDRESS_MODE AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER,
			D3D12_TEXTURE_ADDRESS_MODE AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER,
			D3D12_TEXTURE_ADDRESS_MODE AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER
		)
		{
			return Create(D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT,
				AddressU,
				AddressV,
				AddressW, 0, 0, D3D12_COMPARISON_FUNC_ALWAYS, float4(0, 0, 0, 0), 0, 0);
		}

		// Creates a default anisotropic sampling object
		static Sampler Anisotropic(
			D3D12_TEXTURE_ADDRESS_MODE AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP,
			D3D12_TEXTURE_ADDRESS_MODE AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP,
			D3D12_TEXTURE_ADDRESS_MODE AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP
		)
		{
			return Create(D3D12_FILTER_ANISOTROPIC,
				AddressU,
				AddressV,
				AddressW);
		}
	private:
		static Sampler Create(
			D3D12_FILTER filter = D3D12_FILTER_MIN_MAG_MIP_POINT,
			D3D12_TEXTURE_ADDRESS_MODE AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP,
			D3D12_TEXTURE_ADDRESS_MODE AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP,
			D3D12_TEXTURE_ADDRESS_MODE AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP,
			float mipLODBias = 0.0f,
			UINT maxAnisotropy = 16,
			D3D12_COMPARISON_FUNC comparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS,
			float4 borderColor = float4(0, 0, 0, 0),
			float minLOD = 0,
			float maxLOD = D3D12_FLOAT32_MAX
		) {
			return Sampler{
				filter,
				AddressU,
				AddressV,
				AddressW,
				mipLODBias,
				maxAnisotropy,
				comparisonFunc,
				borderColor,
				minLOD,
				maxLOD
			};
		}
	};

	class ResourceView {
		friend Technique;
		friend Creating;
		friend CopyManager;

	protected:
		// Internal object used to wrap a DX resource
		void* __InternalDXWrapper = nullptr;

		void* __InternalViewWrapper = nullptr;

		ResourceView(void* internalDXWrapper, void* internalViewWrapper);

	public:
		void SetDebugName(LPCWSTR name);

		// Gets the total size in bytes this resource (or subresource) requires
		long getSizeInBytes();

		gObj<Buffer> AsBuffer(int elementWidth);

		gObj<Texture1D> AsTexture1D(DXGI_FORMAT format, int width, int mips, int arrayLength);

		gObj<Texture2D> AsTexture2D(DXGI_FORMAT format, int width, int height, int mips, int arrayLength);

		gObj<Texture3D> AsTexture3D(DXGI_FORMAT format, int width, int height, int depth, int mips);

		virtual ~ResourceView();
	};

	class Buffer : public ResourceView {
		friend Technique;
		friend ResourceView;
		friend Creating;

	protected:
		Buffer(
			void* internalDXWrapper, 
			void* internalViewWrapper, 
			int stride, 
			int elementCount) : 
			ResourceView(internalDXWrapper, internalViewWrapper),
			ElementCount(elementCount), 
			Stride(stride) {
		}
	public:
		// Number of elements of this buffer
		unsigned int const
			ElementCount;
		// Number of bytes of each element in this buffer
		unsigned int const
			Stride;

		gObj<Buffer> Slice(int startElement, int count);
	};

	class Texture1D : public ResourceView {
		friend Technique;
		friend ResourceView;
		friend Creating;

	protected:
		Texture1D(
			void* internalDXWrapper,
			void* internalViewWrapper, 
			DXGI_FORMAT format, 
			int width, 
			int mips, 
			int arrayLength) : 
			ResourceView(internalDXWrapper, internalViewWrapper),
			Format(format), 
			Width(width), 
			Mips(mips), 
			ArrayLength(arrayLength)
		{
		}
	public:
		// Gets the format of each element in this Texture1D
		DXGI_FORMAT const Format;
		// Gets the number of elements for this Texture1D
		unsigned int const Width;
		// Gets the length of this Texture1D array
		unsigned int const ArrayLength;
		// Gets the number of mips of this Texture1D
		unsigned int const Mips;

		gObj<Texture1D> SliceMips(int start, int count);

		gObj<Texture1D> SliceArray(int start, int count);

		gObj<Texture1D> Subresource(int mip, int arrayIndex) {
			return SliceArray(arrayIndex, 1)->SliceMips(mip, 1);
		}
	};

	class Texture2D : public ResourceView {
		friend Technique;
		friend ResourceView;
		friend Creating;
		friend DX_Wrapper;

	protected:
		Texture2D(
			void* internalDXWrapper,
			void* internalViewWrapper, 
			DXGI_FORMAT format, 
			int width, 
			int height, 
			int mips, 
			int arrayLength) : 
			ResourceView(internalDXWrapper, internalViewWrapper),
			Format(format),
			Width(width), 
			Height(height), 
			Mips(mips), 
			ArrayLength(arrayLength) {
		}
	public:
		// Gets the format of each element in this Texture2D
		DXGI_FORMAT const Format;
		// Gets the width for this Texture2D
		unsigned int const Width;
		// Gets the height for this Texture2D
		unsigned int const Height;
		// Gets the length of this Texture2D array
		unsigned int const ArrayLength;
		// Gets the number of mips of this Texture2D
		unsigned int const Mips;

		gObj<Texture2D> SliceMips(int start, int count);

		gObj<Texture2D> SliceArray(int start, int count);

		gObj<Texture2D> Subresource(int mip, int arrayIndex) {
			return SliceArray(arrayIndex, 1)->SliceMips(mip, 1);
		}
	};

	class Texture3D : public ResourceView {
		friend Technique;
		friend ResourceView;
		friend Creating;

	protected:
		Texture3D(
			void* internalDXWrapper,
			void* internalViewWrapper, 
			DXGI_FORMAT format, 
			int width, 
			int height, 
			int depth, 
			int mips) : 
			ResourceView(internalDXWrapper, internalViewWrapper),
			Format(format),
			Width(width), 
			Height(height), 
			Mips(mips), 
			Depth(depth) {
		}
	public:
		// Gets the format of each element in this Texture2D
		DXGI_FORMAT const Format;
		// Gets the width for this Texture3D
		unsigned int const Width;
		// Gets the height for this Texture3D
		unsigned int const Height;
		// Gets the depth (number of slices) of this Texture3D
		unsigned int const Depth;
		// Gets the number of mips of this Texture3D
		unsigned int const Mips;

		gObj<Texture3D> SliceMips(int start, int count);
	};

}

#endif