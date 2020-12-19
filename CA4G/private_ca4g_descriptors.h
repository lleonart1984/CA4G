#ifndef PRIVATE_DESCRIPTORS_H
#define PRIVATE_DESCRIPTORS_H

#include "private_ca4g_sync.h"
#include "ca4g_errors.h"
#include "ca4g_definitions.h"
#include "private_ca4g_definitions.h"

namespace CA4G {
	struct Allocator {
		struct Node {
			int index;
			Node* next;
		};

		int allocated = 0;
		Node* free = nullptr;

		Mutex mutex;

		~Allocator() {
			while (free != nullptr) {
				Node* toDelete = free;
				free = free->next;
				delete toDelete;
			}
		}

		int Allocate() {
			int index;
			mutex.Acquire();

			if (free != nullptr) {
				Node* toDelete = free;
				free = free->next;
				index = toDelete->index;
				delete toDelete;
			}
			else
				index = allocated++;

			mutex.Release();

			return index;
		}

		void Free(int index) {
			mutex.Acquire();

			free = new Node{
				index,
				free
			};

			mutex.Release();
		}
	};

	class CPUDescriptorHeapManager {
		DX_DescriptorHeap heap;
		unsigned int size;
		Allocator* allocator;
		SIZE_T startCPU;
	public:
		CPUDescriptorHeapManager(DX_Device device, D3D12_DESCRIPTOR_HEAP_TYPE type, int capacity);
		~CPUDescriptorHeapManager() {
			delete allocator;
		}
		// Allocates a new index for a resource descriptor
		inline int AllocateNewHandle() {
			return allocator->Allocate();
		}
		// Releases a specific index of a resource is being released.
		inline void Free(int index) {
			allocator->Free(index);
		}
		inline D3D12_CPU_DESCRIPTOR_HANDLE getCPUVersion(int index) { return D3D12_CPU_DESCRIPTOR_HANDLE{ startCPU + index * size }; }
		inline DX_DescriptorHeap getInnerHeap() { return heap; }
	};

	class GPUDescriptorHeapManager {
		DX_DescriptorHeap heap;
		unsigned int size;
		SIZE_T startCPU;
		UINT64 startGPU;

		// Current Avail
		volatile long mallocOffset = 0;

		// Total Capacity of the heap.
		int capacity;
		// Capacity of the heap for each frame.
		int frameCapacity;

		// Marks the capacity of the heap for a specific frame.
		int lastAvailableInBlock;

		// Allocator used for handles persistent during all execution.
		Allocator* persistentAllocator;

	public:

		// Resets the per-frame allocator to the initial values of a specific frame
		inline void RestartAllocatorForFrame(int frameIndex) {
			mallocOffset = frameIndex * frameCapacity;
			lastAvailableInBlock = (frameIndex + 1) * frameCapacity;
		}

		// Allocate some descriptors in current frame.
		inline long AllocateInFrame(int descriptors) {
			auto result = InterlockedExchangeAdd(&mallocOffset, descriptors);
			if (mallocOffset >= lastAvailableInBlock)
				throw CA4GException::FromError(CA4G_Errors_RunOutOfMemory, "Descriptor heap");
			return result;
		}

		inline int AllocatePersistent() {
			return capacity - 1 - persistentAllocator->Allocate();
		}

		inline void FreePersistent(int index) {
			persistentAllocator->Free(capacity - 1 - index);
		}

		GPUDescriptorHeapManager(
			DX_Device device, 
			D3D12_DESCRIPTOR_HEAP_TYPE type, 
			int capacity,
			int persistentCapacity,
			int buffers);
	
		~GPUDescriptorHeapManager() {
			delete persistentAllocator;
		}
		// Gets the cpu descriptor handle to access to a specific index slot of this descriptor heap
		inline D3D12_GPU_DESCRIPTOR_HANDLE getGPUVersion(int index) { return D3D12_GPU_DESCRIPTOR_HANDLE{ startGPU + index * size }; }
		// Gets the gpu descriptor handle to access to a specific index slot of this descriptor heap
		inline D3D12_CPU_DESCRIPTOR_HANDLE getCPUVersion(int index) { return D3D12_CPU_DESCRIPTOR_HANDLE{ startCPU + index * size }; }

		inline DX_DescriptorHeap getInnerHeap() { return heap; }
	};
}


#endif