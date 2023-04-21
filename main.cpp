// Dear ImGui: standalone example application for DirectX 11
// If you are new to Dear ImGui, read documentation from the docs/ folder + read the top of imgui.cpp.
// Read online: https://github.com/ocornut/imgui/tree/master/docs
#include "pch.h"
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include <ppl.h>
#include "resource.h"

#pragma comment(lib, "d3d11")

// Data
static ID3D11Device* g_pd3dDevice = nullptr;
static ID3D11DeviceContext* g_pd3dDeviceContext = nullptr;
static IDXGISwapChain* g_pSwapChain = nullptr;
static ID3D11RenderTargetView* g_mainRenderTargetView = nullptr;

// Forward declarations of helper functions
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

uint8_t Mandelbrot(std::complex<double> const& c) {
	std::complex<double> z{};
	int count = 256;
	while (count && abs(z) < 4) {
		z = z * z + c;
		count--;
	}

	return count;
}

int WINAPI wWinMain(HINSTANCE hInst, HINSTANCE, LPTSTR cmdLine, int cmdShow) {
	// Create application window
	ImGui_ImplWin32_EnableDpiAwareness();
	WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, hInst, nullptr, nullptr, nullptr, nullptr, L"ImGuiMandelbrot", nullptr };
	::RegisterClassExW(&wc);
	HWND hwnd = ::CreateWindowW(wc.lpszClassName, L"Dear ImGui Mandelbrot", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
		1280, 800, nullptr, nullptr, wc.hInstance, nullptr);
	::SendMessage(hwnd, WM_SETICON, 1, (LPARAM)::LoadIcon(hInst, MAKEINTRESOURCE(IDI_MANDELIMGUI)));
	::SendMessage(hwnd, WM_SETICON, 0, (LPARAM)::LoadIcon(hInst, MAKEINTRESOURCE(IDI_MANDELIMGUI)));

	// Initialize Direct3D
	if (!CreateDeviceD3D(hwnd)) {
		CleanupDeviceD3D();
		::UnregisterClassW(wc.lpszClassName, wc.hInstance);
		return 1;
	}

	// Show the window
	::ShowWindow(hwnd, SW_SHOWDEFAULT);
	::UpdateWindow(hwnd);


	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
	//io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // Enable Docking
	io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;       // Enable Multi-Viewport / Platform Windows
	//io.ConfigViewportsNoAutoMerge = true;
	//io.ConfigViewportsNoTaskBarIcon = true;
	//io.ConfigViewportsNoDefaultParent = true;
	//io.ConfigDockingAlwaysTabBar = true;
	//io.ConfigDockingTransparentPayload = true;
	//io.ConfigFlags |= ImGuiConfigFlags_DpiEnableScaleFonts;     // FIXME-DPI: Experimental. THIS CURRENTLY DOESN'T WORK AS EXPECTED. DON'T USE IN USER APP!
	//io.ConfigFlags |= ImGuiConfigFlags_DpiEnableScaleViewports; // FIXME-DPI: Experimental.

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();
	//ImGui::StyleColorsLight();

	// When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
	ImGuiStyle& style = ImGui::GetStyle();
	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
		style.WindowRounding = 0.0f;
		style.Colors[ImGuiCol_WindowBg].w = 1.0f;
	}

	// Setup Platform/Renderer backends
	ImGui_ImplWin32_Init(hwnd);
	ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

	// Load Fonts
	// - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
	// - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
	// - If the file cannot be loaded, the function will return a nullptr. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
	// - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
	// - Use '#define IMGUI_ENABLE_FREETYPE' in your imconfig file to use Freetype for higher quality font rendering.
	// - Read 'docs/FONTS.md' for more instructions and details.
	// - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
	//io.Fonts->AddFontDefault();
	//io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\segoeui.ttf", 16.0f);
	//io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\DroidSans.ttf", 16.0f);
	//io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
	//io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
	//ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, nullptr, io.Fonts->GetGlyphRangesJapanese());
	//IM_ASSERT(font != nullptr);

	ImVec4 clear_color = ImVec4(0, 0, 0, 1);
	bool done = false;
	int oldsx = 0, oldsy = 0;
	std::vector<ImU32> colors;
	ImVec2 ptTL(-1, -1), ptBR(-1, -1);
	bool mouseDown = false;
	std::complex<double> from(-1.9, -1), to(.6, 1);
	auto orgfrom(from), orgto(to);
	float components[] = { 1, 1, 1 };

	// Main loop
	while (!done) {
		// Poll and handle messages (inputs, window resize, etc.)
		// See the WndProc() function below for our to dispatch events to the Win32 backend.
		MSG msg;
		while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
			::TranslateMessage(&msg);
			::DispatchMessage(&msg);
			if (msg.message == WM_QUIT)
				done = true;
		}
		if (done)
			break;

		// Start the Dear ImGui frame
		ImGui_ImplDX11_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

		static bool use_work_area = true;
		const ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings;

		const ImGuiViewport* viewport = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(use_work_area ? viewport->WorkPos : viewport->Pos);
		ImGui::SetNextWindowSize(use_work_area ? viewport->WorkSize : viewport->Size);

		if (ImGui::Begin("Fullscreen window", nullptr, flags)) {
			static const struct {
				PCSTR Text;
				ImVec4 Color;
				float Values[3];
				bool Reset{ false };
			} buttons[] = {
				{ "Reset", ImVec4(.5f, .5f, .5f, 1), { 1, 1, 1 }, true },
				{ "Red", ImVec4(1, 0, 0, 1), { 1, .1f, 0 } },
				{ "Green", ImVec4(0, 0.5f, 0, 1), { 0, 1, 0 } },
				{ "Blue", ImVec4(0, 0, .8f, 1), { 0, 0, 1 } }
			};

			for (auto& b : buttons) {
				ImGui::PushStyleColor(ImGuiCol_Button, b.Color);
				if (ImGui::Button(b.Text)) {
					if (b.Reset) {
						from = orgfrom;
						to = orgto;
					}
					oldsx = 0;
					ptTL.x = ptBR.x = ptTL.y = ptBR.y = -1;
					mouseDown = false;
					memcpy(components, b.Values, sizeof(components));
				}
				ImGui::SameLine();
			}
			ImGui::PopStyleColor(_countof(buttons));

			ImGui::SameLine();
			static bool colChanged = false;
			if (ImGui::ColorEdit3("Tint", components, ImGuiColorEditFlags_NoInputs)) {
				colChanged = true;
				oldsx = 0;
				ptTL.x = ptBR.x = -1;
			}

			auto dl = ImGui::GetWindowDrawList();
			auto px = (int)ImGui::GetCursorScreenPos().x, py = (int)ImGui::GetCursorScreenPos().y;
			auto sx = (int)ImGui::GetWindowSize().x, sy = (int)ImGui::GetWindowSize().y;

			if (!ImGui::IsPopupOpen(nullptr, ImGuiPopupFlags_AnyPopupId)) {
				if (oldsx != sx || oldsy != sy) {
					colors.resize(sx * sy);
					::SetCursor(::LoadCursor(nullptr, IDC_WAIT));
					concurrency::parallel_for(py, sy + py - 1, [&](int y) {
						for (auto x = px; x < sx + px; x++) {
							std::complex<double> c((to.real() - from.real()) * (x - px) / sx + from.real(), (to.imag() - from.imag()) * (y - py) / sy + from.imag());
							auto gray = Mandelbrot(c);
							colors[(y - py) * sx + x - px] = IM_COL32(gray * components[0], gray * components[1], gray * components[2], 255);
						}
						});
					oldsx = sx; oldsy = sy;
					::SetCursor(::LoadCursor(nullptr, IDC_ARROW));
				}
			}
			int i = 0;
			for (auto y = py; y < sy + py; y++)
				for (auto x = px; x < sx + px; x++)
					dl->AddRectFilled(ImVec2(float(x), float(y)), ImVec2(float(x + 1), float(y + 1)), colors[i++]);

			if (!ImGui::IsPopupOpen(nullptr, ImGuiPopupFlags_AnyPopupId)) {
				if (!mouseDown && ImGui::IsMouseDown(ImGuiMouseButton_Left) && ImGui::GetMousePos().y > py) {
					ptTL = ImGui::GetMousePos();
					ImGui::SetNextFrameWantCaptureMouse(true);
					mouseDown = true;
				}
				if (ImGui::IsMouseDragging(ImGuiMouseButton_Left))
					ptBR = ImGui::GetMousePos();
				if (mouseDown && ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
					ImGui::CaptureMouseFromApp(false);
					if (ptTL.x >= 0 && ptBR.y >= 0) {
						if (ptTL.x > ptBR.x)
							std::swap(ptTL.x, ptBR.x);
						if (ptTL.y > ptBR.y)
							std::swap(ptTL.y, ptBR.y);

						auto width = ptBR.x - ptTL.x;
						auto height = ptBR.y - ptTL.y;
						if (width > 10 && height > 10) {
							auto newWidth = width * (to.real() - from.real()) / sx;
							auto newHeight = height * (to.imag() - from.imag()) / sy;
							auto deltax = (ptTL.x - px) * (to.real() - from.real()) / sx;
							auto deltay = (ptTL.y - py) * (to.imag() - from.imag()) / sy;
							from += std::complex<double>(deltax, deltay);
							to = from + std::complex<double>(newWidth, newHeight);
							oldsx = 0;
							ptTL.x = ptBR.x = ptTL.y = ptBR.y = -1;
						}
					}
					mouseDown = false;
				}

				if (ptTL.x >= 0 && ptBR.x >= 0) {
					dl->AddRect(ptTL, ptBR, IM_COL32(255 - components[0] * 255, 255 - components[1] * 255, 255 - components[2] * 255, 255), 0, 0, 1);
				}
			}
		}
		ImGui::End();

		// Rendering
		ImGui::Render();
		const float clear_color_with_alpha[4] = { clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w };
		g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
		g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color_with_alpha);
		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

		// Update and Render additional Platform Windows
		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
		}

		g_pSwapChain->Present(1, 0); // Present with vsync
		//g_pSwapChain->Present(0, 0); // Present without vsync
	}

	// Cleanup
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	CleanupDeviceD3D();
	::DestroyWindow(hwnd);
	::UnregisterClassW(wc.lpszClassName, wc.hInstance);

	return 0;
}

// Helper functions
bool CreateDeviceD3D(HWND hWnd) {
	// Setup swap chain
	DXGI_SWAP_CHAIN_DESC sd;
	ZeroMemory(&sd, sizeof(sd));
	sd.BufferCount = 2;
	sd.BufferDesc.Width = 0;
	sd.BufferDesc.Height = 0;
	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	sd.BufferDesc.RefreshRate.Numerator = 60;
	sd.BufferDesc.RefreshRate.Denominator = 1;
	sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.OutputWindow = hWnd;
	sd.SampleDesc.Count = 1;
	sd.SampleDesc.Quality = 0;
	sd.Windowed = TRUE;
	sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

	UINT createDeviceFlags = 0;
	//createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
	D3D_FEATURE_LEVEL featureLevel;
	const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
	HRESULT res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
	if (res == DXGI_ERROR_UNSUPPORTED) // Try high-performance WARP software driver if hardware is not available.
		res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
	if (res != S_OK)
		return false;

	CreateRenderTarget();
	return true;
}

void CleanupDeviceD3D() {
	CleanupRenderTarget();
	if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = nullptr; }
	if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = nullptr; }
	if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }
}

void CreateRenderTarget() {
	ID3D11Texture2D* pBackBuffer;
	g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
	g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
	pBackBuffer->Release();
}

void CleanupRenderTarget() {
	if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = nullptr; }
}

#ifndef WM_DPICHANGED
#define WM_DPICHANGED 0x02E0 // From Windows SDK 8.1+ headers
#endif

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Win32 message handler
// You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
// Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
		return true;

	switch (msg) {
		case WM_SIZE:
			if (g_pd3dDevice != nullptr && wParam != SIZE_MINIMIZED) {
				CleanupRenderTarget();
				g_pSwapChain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
				CreateRenderTarget();
			}
			return 0;
		case WM_SYSCOMMAND:
			if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
				return 0;
			break;
		case WM_DESTROY:
			::PostQuitMessage(0);
			return 0;
		case WM_DPICHANGED:
			if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_DpiEnableScaleViewports) {
				//const int dpi = HIWORD(wParam);
				//printf("WM_DPICHANGED to %d (%.0f%%)\n", dpi, (float)dpi / 96.0f * 100.0f);
				const RECT* suggested_rect = (RECT*)lParam;
				::SetWindowPos(hWnd, nullptr, suggested_rect->left, suggested_rect->top, suggested_rect->right - suggested_rect->left, suggested_rect->bottom - suggested_rect->top, SWP_NOZORDER | SWP_NOACTIVATE);
			}
			break;
	}
	return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}
