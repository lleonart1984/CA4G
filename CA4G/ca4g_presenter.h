#pragma once
#include "ca4g_definitions.h"
#include "ca4g_errors.h"
#include "ca4g_memory.h"

#ifndef CA4G_NUMBER_OF_WORKERS
#define CA4G_NUMBER_OF_WORKERS 8
#endif // !CA4G_NUMBER_OF_WORKERS


namespace CA4G {
	class Presenter {

		// Internal object used for the implementation
		void* __internalDXWrapper;

	public:
		// Creates a presenter object that creates de DX device attached to a specific window (hWnd).
		Presenter(HWND hWnd);
		~Presenter();

		class Creating {
			friend Presenter;
			Presenter* presenter;
			Creating(Presenter* presenter) :presenter(presenter) {}
		public:
			// Creates a technique of specific type and passing some arguments to the constructor.
			template<typename T, typename... A>
			gObj<T> Technique(A... args) {
				return new T(args...);
			}
		} const* create;

		class Settings {
			friend Presenter;
			Presenter* presenter;
			Settings(Presenter* presenter) :presenter(presenter) {}
		public:
			// Sets the number of buffers in the swap chain.
			// Default value is 2
			void Buffers(int buffers);
		
			// Determines if frames are buffered.
			// Frame buffering allows to continue allocating next frame in another queues without sync on the CPU.
			// Normal usage is useFrameBuffering = true and Buffers = 3.
			void UseFrameBuffering(bool useFrameBuffering = true);

			// Set the resolution for the back buffers.
			void Resolution(int width, int height);

			// Set full screen mode.
			void FullScreenMode(bool fullScreen = true);

			// Set if use a warp device.
			void WarpDevice(bool usewrap = true);

			// Set if use a Raytracing Fallback device to emulate missing RTX support.
			void DXRFallback(bool forceFallback = true);

			// Set the Render Target format to use.
			void RT_Format(DXGI_FORMAT rtFormat = DXGI_FORMAT_R8G8B8A8_UNORM);

			// Set the multisample mode.
			void Sampling(int count = 1, int quality = 0);

			// This method should be called last. This method will create the swapchain and the DirectX12 device.
			void SwapChain();
		} const *set;

		class Loading {
			friend Presenter;
			Presenter* presenter;
			Loading(Presenter* presenter) :presenter(presenter) {}
		public:
			// Load a technique initializing on the GPU.
			void Technique(gObj<Technique> technique);
		} const *load;

		class Dispatcher {
			friend Presenter;
			Presenter* presenter;
			Dispatcher(Presenter* presenter) :presenter(presenter) {}
		public:
			// Execute a frame for a technique and presents current render target.
			void Technique(gObj<Technique> technique);
		} const* dispatch;

	};
}