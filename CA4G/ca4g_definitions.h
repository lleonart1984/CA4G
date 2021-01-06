#ifndef CA4G_DEFINITIONS_H
#define CA4G_DEFINITIONS_H

#include <Windows.h>
#include <d3d12.h>
#include <d3d12sdklayers.h>
#include <d3d12shader.h>
#include <dxgi1_4.h>
#ifdef _DEBUG
#include <dxgidebug.h>
#endif
#include <atlbase.h>

namespace CA4G {

#pragma region DX OBJECTS

	typedef CComPtr<ID3D12DescriptorHeap> DX_DescriptorHeap;
	typedef CComPtr<ID3D12Device5> DX_Device;

	typedef CComPtr<ID3D12GraphicsCommandList4> DX_CommandList;
	typedef CComPtr<ID3D12CommandAllocator> DX_CommandAllocator;
	typedef CComPtr<ID3D12CommandQueue> DX_CommandQueue;
	typedef CComPtr<ID3D12Fence1> DX_Fence;
	typedef CComPtr<ID3D12Heap1> DX_Heap;
	typedef CComPtr<ID3D12PipelineState> DX_PipelineState;
	typedef CComPtr<ID3D12Resource1> DX_Resource;
	typedef CComPtr<ID3D12RootSignature> DX_RootSignature;
	typedef CComPtr<ID3D12RootSignatureDeserializer> DX_RootSignatureDeserializer;
	typedef CComPtr<ID3D12StateObject> DX_StateObject;
	typedef CComPtr<ID3D12StateObjectProperties> DX_StateObjectProperties;

#pragma endregion

	class Presenter;
	class Creating;
	class Loading;
	class Dispatcher;
	class Technique;
	class Creating;
	class ResourceView;
	class Buffer;
	class Texture1D;
	class Texture2D;
	class Texture3D;
	struct Signal;
	class Tagging;
	class ICmdManager;
	class CommandListManager;
	class CopyManager;
	class ComputeManager;
	class GraphicsManager;
	class RaytracingManager;
	class StaticPipelineBindingsBase;
	template<typename ...PSS> class StaticPipelineBindings;
	class ComputePipelineBindings;
	class GraphicsPipelineBindings;
	class RaytracingPipelineBindings;
	class RTProgramBase;
	class ComputeBinder;
	class GraphicsBinder;
	class RaytracingBinder;
	class ProgramHandle;
	class RayGenerationHandle;
	class MissHandle;
	class AnyHitHandle;
	class ClosestHitHandle;
	class IntersectionHandle;
	class HitGroupHandle;
	class TriangleGeometryCollection;
	class ProceduralGeometryCollection;
	class VertexElement;
	class InstanceCollection;

	template<typename L> class RTProgram;

	// Internal Management
	struct GPUScheduler;
	struct DX_Wrapper;
	struct DX_ResourceWrapper;
	struct DX_ViewWrapper;
	struct DX_CmdWrapper;
}

#endif