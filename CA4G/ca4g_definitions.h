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

namespace CA4G {

	class Presenter;
	class DeviceManager;
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
	class ICmdWrapper;
	class CommandListManager;
	class CopyManager;
	class ComputeManager;
	class GraphicsManager;
	class RaytracingManager;

	// Internal Management
	struct GPUScheduler;
	struct DX_Wrapper;
}

#endif