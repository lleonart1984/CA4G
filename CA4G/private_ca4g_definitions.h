#ifndef CA4G_PRIVATE_DEFINITIONS
#define CA4G_PRIVATE_DEFINITIONS

#include "D3D12RaytracingFallback.h"
#include <atlbase.h>


#pragma region FALLBACK DX COM OBJECTS

typedef CComPtr<ID3D12RaytracingFallbackDevice> DX_FallbackDevice;
typedef CComPtr<ID3D12RaytracingFallbackCommandList> DX_FallbackCommandList;
typedef CComPtr<ID3D12RaytracingFallbackStateObject> DX_FallbackStateObject;

#pragma endregion

#endif
