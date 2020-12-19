#include "private_ca4g_descriptors.h"
#include "ca4g_definitions.h"
#include "private_ca4g_definitions.h"

namespace CA4G {

	CPUDescriptorHeapManager::CPUDescriptorHeapManager(DX_Device device, D3D12_DESCRIPTOR_HEAP_TYPE type, int capacity)
	{
		size = device->GetDescriptorHandleIncrementSize(type);
		D3D12_DESCRIPTOR_HEAP_DESC desc = {};
		desc.NodeMask = 0;
		desc.NumDescriptors = capacity;
		desc.Type = type;
		desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&heap));
		startCPU = heap->GetCPUDescriptorHandleForHeapStart().ptr;

		allocator = new Allocator();
	}

	GPUDescriptorHeapManager::GPUDescriptorHeapManager(DX_Device device, D3D12_DESCRIPTOR_HEAP_TYPE type, int capacity, int persistentCapacity, int buffers):capacity(capacity) {
		size = device->GetDescriptorHandleIncrementSize(type);
		D3D12_DESCRIPTOR_HEAP_DESC desc = {};
		desc.NodeMask = 0;
		desc.NumDescriptors = capacity;
		desc.Type = type;
		desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		auto hr = device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&heap));
		if (FAILED(hr))
			throw CA4GException::FromError(CA4G_Errors_RunOutOfMemory, "Creating descriptor heaps.");
		startCPU = heap->GetCPUDescriptorHandleForHeapStart().ptr;
		startGPU = heap->GetGPUDescriptorHandleForHeapStart().ptr;

		frameCapacity = (capacity - persistentCapacity) / buffers;

		mallocOffset = 0;
		lastAvailableInBlock = frameCapacity;
		
		persistentAllocator = new Allocator();
	}
}