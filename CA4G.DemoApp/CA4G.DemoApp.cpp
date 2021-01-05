#include <Windows.h>

#include "ImGui/imgui.h"
#include "ImGui/imgui_impl_win32.h"
#include "ImGui/imgui_impl_dx12.h"

#include "main_technique.h"
#include "main_scene.h"

using namespace CA4G;

#define USE_GUI

#ifdef USE_GUI

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

#endif

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

#include "ca4g_ImGuiTraits.h"

// Main code
int main(int, char**)
{
	// Create application window
	//ImGui_ImplWin32_EnableDpiAwareness();
	WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, TEXT("CA4G Example"), NULL };
	::RegisterClassEx(&wc);
	HWND hwnd = ::CreateWindow(wc.lpszClassName, TEXT("DirectX12 Example with CA4G"), WS_OVERLAPPEDWINDOW, 100, 100, 1280, 800, NULL, NULL, wc.hInstance, NULL);

	gObj<InternalDXInfo> dxObjects;

	// Create the presenter object
	gObj<Presenter> presenter = new Presenter(hwnd, dxObjects);

	presenter _set WindowResolution(); // Defines the render target to fit the window resolution
	presenter _set UseFrameBuffering(); // Use frame buffering, that means that next frame will be commiting while previous frame is presented.
	presenter _set Buffers(3); // Number of in fly frames set to 3.
	presenter _set SwapChain(); // Final set of the presenter to create swap chain and device objects.

	// Show the window
	::ShowWindow(hwnd, SW_SHOWDEFAULT);
	::UpdateWindow(hwnd);

	#pragma region ImGui Initialization

#ifdef USE_GUI
	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();
	//ImGui::StyleColorsClassic();

	CComPtr<ID3D12DescriptorHeap> guiDescriptors;
	// Create GUI SRV Descriptor Heap
	{
		D3D12_DESCRIPTOR_HEAP_DESC desc = {};
		desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		desc.NumDescriptors = 1;
		desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		if (dxObjects->device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&guiDescriptors)) != S_OK)
			return false;
	}

	// Setup Platform/Renderer backends
	ImGui_ImplWin32_Init(hwnd);
	ImGui_ImplDX12_Init(dxObjects->device, dxObjects->Buffers,
		dxObjects->RenderTargetFormat, guiDescriptors,
		guiDescriptors->GetCPUDescriptorHandleForHeapStart(),
		guiDescriptors->GetGPUDescriptorHandleForHeapStart());

#endif

	#pragma endregion

	auto technique = presenter _create TechniqueObj<main_technique>();
	gObj<SceneManager> scene = new main_scene();
	scene->SetupScene();

	if (technique.Dynamic_Cast<IManageScene>())
		technique.Dynamic_Cast<IManageScene>()->SetSceneManager(scene);

	presenter _load TechniqueObj(technique);

	presenter _create FlushAndSignal().WaitFor();

	// Main loop
	MSG msg;
	ZeroMemory(&msg, sizeof(msg));
	while (msg.message != WM_QUIT)
	{
		if (::PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
		{
			::TranslateMessage(&msg);
			::DispatchMessage(&msg);
			continue;
		}

		#pragma region New Frame GUI

#ifdef USE_GUI
		// Start the Dear ImGui frame
		ImGui_ImplDX12_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();
#endif

		#pragma endregion

		#pragma region GUI Population
		
#ifdef USE_GUI
		if (true)
		{
			ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.
			ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

			RenderGUI<IManageScene>(technique);

			ImGui::End();
		}
#endif

		#pragma endregion

#ifdef USE_GUI
		scene->Animate(ImGui::GetTime(), ImGui::GetFrameCount());
#endif

		presenter _dispatch TechniqueObj(technique); // excutes the technique 
		
		#pragma region Present GUI
#ifdef USE_GUI

		// Prepares the render target to draw on it...
		// Just in case the technique leave it in other state
		presenter _dispatch RenderTarget();

		auto renderTargetHandle = dxObjects->RenderTargets[dxObjects->swapChain->GetCurrentBackBufferIndex()];
		dxObjects->mainCmdList->OMSetRenderTargets(1, &renderTargetHandle, false, nullptr);
		ID3D12DescriptorHeap* dh[1] = { guiDescriptors };
		dxObjects->mainCmdList->SetDescriptorHeaps(1, dh);

		ImGui::Render();
		ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), dxObjects->mainCmdList);
#endif
		#pragma endregion

		presenter _dispatch BackBuffer(); // present the current back buffer.
	}

	return 0;
}

// Win32 message handler
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
#ifdef USE_GUI
	if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
		return true;
#endif
	switch (msg)
	{
	case WM_SYSCOMMAND:
		if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
			return 0;
		break;
	case WM_DESTROY:
		::PostQuitMessage(0);
		return 0;
	}
	return ::DefWindowProc(hWnd, msg, wParam, lParam);
}
