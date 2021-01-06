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

	enum class CPUAccessibility {
		// The resource can not be read or write directly from the CPU.
		// A stagging internal version is used for so.
		// This grant an optimized use of the GPU access. It is the most common usage for resources.
		None,
		// The resource can be written fast by the CPU and the GPU can read moderatly.
		// Can be used for dynamic constant buffers that requires update every frame.
		Write,
		// The resource can be read fast by the CPU and the GPU can write moderately.
		// Only use in case of a every-frame data required to be drawback from the GPU
		Read
	};

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
		friend ICmdManager;
		friend Technique;
		friend Creating;
		friend CommandListManager;
		friend CopyManager;
		friend GraphicsManager;
		friend RaytracingManager;
		friend DX_Wrapper;
		friend GPUScheduler;
		friend TriangleGeometryCollection;
		friend ProceduralGeometryCollection;

	protected:
		// Internal object used to wrap a DX resource
		DX_ResourceWrapper* __InternalDXWrapper = nullptr;

		DX_ViewWrapper* __InternalViewWrapper = nullptr;

		ResourceView(DX_ResourceWrapper* internalDXWrapper, DX_ViewWrapper* internalViewWrapper);

	public:
		static gObj<ResourceView> CreateNullView(DX_Wrapper* deviceManager, D3D12_RESOURCE_DIMENSION dimension);

		void SetDebugName(LPCWSTR name);

		CPUAccessibility getCPUAccessibility();

		// Gets the total size in bytes this resource (or subresource) requires
		long getSizeInBytes();

		virtual ~ResourceView();
		
		class Copying {
			friend ResourceView;
		private:
			ResourceView* resource;
			Copying(ResourceView* resource);

		public:
			void FromPtr(byte* data, bool flipRows = false);
			template<typename T>
			void FromPtr(T* data) {
				FromPtr((byte*)data);
			}
			void ToPtr(byte* data, bool flipRows = false);
			void RegionFromPtr(byte* data, const D3D12_BOX &region, bool flipRows = false);
			void RegionToPtr(byte* data, const D3D12_BOX& region, bool flipRows = false);
			template<typename T>
			void FromList(std::initializer_list<T> data) {
				FromPtr((byte*)data.begin());
			}
			template<typename T>
			void FromValue(const T& elementValue) {
				FromPtr((byte*)&elementValue);
			}
		} * const copy;
	};

	class Buffer : public ResourceView {
		friend Technique;
		friend ResourceView;
		friend Creating;
		friend DX_Wrapper;

	protected:
		Buffer(
			DX_ResourceWrapper* internalDXWrapper, 
			DX_ViewWrapper* internalViewWrapper, 
			int stride, 
			int elementCount) : 
			ResourceView(internalDXWrapper, internalViewWrapper),
			ElementCount(elementCount), 
			Stride(stride),
			create(new Creating(this)){
		}
	public:
		// Number of elements of this buffer
		unsigned int const
			ElementCount;
		// Number of bytes of each element in this buffer
		unsigned int const
			Stride;

		D3D12_GPU_VIRTUAL_ADDRESS GPUVirtualAddress(int element = 0);

		class Creating {
			friend Buffer;
			Buffer* buffer;
			Creating(Buffer* buffer) :buffer(buffer) {}
		public:
			gObj<Buffer> Slice(int startElement, int count);
		} * const create;

	};

	class Texture1D : public ResourceView {
		friend Technique;
		friend ResourceView;
		friend Creating;
		friend DX_Wrapper;

	protected:
		Texture1D(
			DX_ResourceWrapper* internalDXWrapper,
			DX_ViewWrapper* internalViewWrapper, 
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
			DX_ResourceWrapper* internalDXWrapper,
			DX_ViewWrapper* internalViewWrapper, 
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
		friend DX_Wrapper;

	protected:
		Texture3D(
			DX_ResourceWrapper* internalDXWrapper,
			DX_ViewWrapper* internalViewWrapper, 
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