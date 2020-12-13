#pragma once

#include "ca4g_presenter.h"

namespace CA4G {

	struct GPUScheduler;

	struct DX_Wrapper {
		DX_Device device;
		DX_FallbackDevice fallbackDevice;
		ID3D12Debug* debugController;

		HWND hWnd;

		bool UseFallbackDevice;
		bool UseWarpDevice;
		bool UseFrameBuffering;
		DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
		DXGI_SWAP_CHAIN_FULLSCREEN_DESC fullScreenDesc = {};

		CComPtr<IDXGISwapChain3> swapChain;

		GPUScheduler* scheduler;

		DX_Wrapper() {
			UseFallbackDevice = false;
			UseWarpDevice = false;
			UseFrameBuffering = false;
			device = nullptr;
			fallbackDevice = nullptr;
			debugController = nullptr;
			fullScreenDesc.Windowed = true;
			swapChainDesc.BufferCount = 2;
			swapChainDesc.Width = 512;
			swapChainDesc.Height = 512;
			swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
			swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;
			swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
			swapChainDesc.SampleDesc.Count = 1;
		}
	};

	#pragma region Command Queue Manager
	struct CommandQueueManager {
		DX_CommandQueue dxQueue;
		long fenceValue;
		DX_Fence fence;
		HANDLE fenceEvent;
		D3D12_COMMAND_LIST_TYPE type;

		CommandQueueManager(DX_Device device, D3D12_COMMAND_LIST_TYPE type) {
			D3D12_COMMAND_QUEUE_DESC d = {};
			d.Type = type;
			device->CreateCommandQueue(&d, IID_PPV_ARGS(&dxQueue));

			fenceValue = 1;
			device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
			fenceEvent = CreateEventW(nullptr, false, false, nullptr);
		}

		// Executes a command list set in this engine
		inline void Commit(DX_CommandList* cmdLists, int count) {
			dxQueue->ExecuteCommandLists(count, (ID3D12CommandList**)cmdLists);
		}

		// Send a signals through this queue
		inline long Signal() {
			dxQueue->Signal(fence, fenceValue);
			return fenceValue++;
		}

		// Triggers the event of reaching some fence value.
		inline HANDLE TriggerEvent(long rallyPoint) {
			fence->SetEventOnCompletion(rallyPoint, fenceEvent);
			return fenceEvent;
		}
	};
	#pragma endregion

	// This object allows the scheduler to know what cmd list is used by thread, and if it has some commands to commit or not.
	struct PerThreadInfo {
		DX_CommandList cmdList;
		bool isActive;
		// Prepares this command list for recording. This method resets the command list to use the desiredAllocator.
		void Activate(DX_CommandAllocator desiredAllocator)
		{
			if (isActive)
				return;
			cmdList->Reset(desiredAllocator, nullptr);
			isActive = true;
		}

		// Prepares this cmd list for flushing to the GPU
		void Close() {
			if (!isActive)
				return;
			isActive = false;
			cmdList->Close();
		}
	};

	// Information needed by the scheduler for each frame on each engine
	struct PerFrameInfo {
		// Allocators for each thread.
		DX_CommandAllocator* allocatorSet;
		int allocatorsUsed;

		inline DX_CommandAllocator RequireAllocator(int index) {
			allocatorsUsed = max(allocatorsUsed, index+1);
			return allocatorSet[index];
		}

		void ResetUsedAllocators()
		{
			for (int i = 0; i < allocatorsUsed; i++)
				allocatorSet[i]->Reset();
			allocatorsUsed = 0;
		}
	};

	// Information needed by the scheduler for each engine.
	struct PerEngineInfo {
		// The queue representing this engine.
		CommandQueueManager* queue;
		
		// An array of command lists used per thread
		PerThreadInfo* threadInfos;
		int threadCount;

		// An array of allocators used per frame
		PerFrameInfo* frames;
		int framesCount;
	};

	// This class is intended to manage all command queues, threads, allocators and command lists
	struct GPUScheduler {
		PerEngineInfo Engines[4]; // 0- direct, 1- bundle, 2 - compute, 3- copy

		GPUScheduler(DX_Device device, int buffers, int threads) {
			for (int i = 0; i < 4; i++)
			{
				PerEngineInfo& info = Engines[i];

				D3D12_COMMAND_LIST_TYPE type = (D3D12_COMMAND_LIST_TYPE)i;
				// Create queue
				info.queue = new CommandQueueManager(device, type);
				info.threadInfos = new PerThreadInfo[threads];
				info.threadCount = threads;
				info.frames = new PerFrameInfo[buffers];
				info.framesCount = buffers;

				// Create allocators first because they are needed for command lists
				for (int j = 0; j < buffers; j++)
				{
					info.frames[j].allocatorSet = new DX_CommandAllocator[threads];
					info.frames[j].allocatorsUsed = 0;

					for (int k = 0; k < threads; k++)
						device->CreateCommandAllocator(type, IID_PPV_ARGS(&info.frames[j].allocatorSet[k]));
				}

				// Create command lists
				for (int j = 0; j < threads; j++)
				{
					device->CreateCommandList(0, type, info.frames[0].allocatorSet[0], nullptr, IID_PPV_ARGS(&info.threadInfos[j].cmdList));
					info.threadInfos[j].cmdList->Close(); // start cmdList state closed.
					info.threadInfos[j].isActive = false;
				}
			}
		}
	};

#define __InternalState ((DX_Wrapper*)this->__internalDXWrapper)
#define __PInternalState ((DX_Wrapper*)presenter->__internalDXWrapper)


}