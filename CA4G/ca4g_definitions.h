#pragma once

#include <Windows.h>
#include <atlbase.h>
#include <d3d12.h>
#include <d3d12sdklayers.h>
#include <d3d12shader.h>
#include <dxgi1_4.h>
#ifdef _DEBUG
#include <dxgidebug.h>
#endif
#include "D3D12RaytracingFallback.h"

class Presenter;
class DeviceManager;
class Technique;

#pragma region DX COM OBJECTS

typedef CComPtr<ID3D12DescriptorHeap> DX_DescriptorHeap;
typedef CComPtr<ID3D12Device5> DX_Device;
typedef CComPtr<ID3D12RaytracingFallbackDevice> DX_FallbackDevice;
typedef CComPtr<ID3D12RaytracingFallbackCommandList> DX_FallbackCommandList;
typedef CComPtr<ID3D12RaytracingFallbackStateObject> DX_FallbackStateObject;

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