#include "gui.h"

#include "../imgui/imgui.h"
#include "../imgui/imgui_impl_dx9.h"
#include "../imgui/imgui_impl_win32.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(
	HWND window,
	UINT message,
	WPARAM wideParameter,
	LPARAM longParameter
);

long __stdcall WindowProcess(
	HWND window,
	UINT message,
	WPARAM wideParameter,
	LPARAM longParameter)
{
	if (ImGui_ImplWin32_WndProcHandler(window, message, wideParameter, longParameter))
		return true;

	switch (message)
	{
	case WM_SIZE: {
		if (gui::device && wideParameter != SIZE_MINIMIZED)
		{
			gui::presentParameters.BackBufferWidth = LOWORD(longParameter);
			gui::presentParameters.BackBufferHeight = HIWORD(longParameter);
			gui::ResetDevice();
		}
	}return 0;

	case WM_SYSCOMMAND: {
		if ((wideParameter & 0xfff0) == SC_KEYMENU) 
			return 0;
	}break;

	case WM_DESTROY: {
		PostQuitMessage(0);
	}return 0;

	case WM_LBUTTONDOWN: {
		gui::position = MAKEPOINTS(longParameter); // set click points
	}return 0;

	case WM_MOUSEMOVE: {
		if (wideParameter == MK_LBUTTON)
		{
			const auto points = MAKEPOINTS(longParameter);
			auto rect = ::RECT{ };

			GetWindowRect(gui::window, &rect);

			rect.left += points.x - gui::position.x;
			rect.top += points.y - gui::position.y;

			if (gui::position.x >= 0 &&
				gui::position.x <= gui::WIDTH &&
				gui::position.y >= 0 && gui::position.y <= 19)
				SetWindowPos(
					gui::window,
					HWND_TOPMOST,
					rect.left,
					rect.top,
					0, 0,
					SWP_SHOWWINDOW | SWP_NOSIZE | SWP_NOZORDER
				);
		}

	}return 0;

	}

	return DefWindowProc(window, message, wideParameter, longParameter);
}

void gui::CreateHWindow(const char* windowName) noexcept
{
	windowClass.cbSize = sizeof(WNDCLASSEX);
	windowClass.style = CS_CLASSDC;
	windowClass.lpfnWndProc = WindowProcess;
	windowClass.cbClsExtra = 0;
	windowClass.cbWndExtra = 0;
	windowClass.hInstance = GetModuleHandleA(0);
	windowClass.hIcon = 0;
	windowClass.hCursor = 0;
	windowClass.hbrBackground = 0;
	windowClass.lpszMenuName = 0;
	windowClass.lpszClassName = "class001";
	windowClass.hIconSm = 0;

	RegisterClassEx(&windowClass);

	window = CreateWindowEx(
		0,
		"class001",
		windowName,
		WS_POPUP,
		100,
		100,
		WIDTH,
		HEIGHT,
		0,
		0,
		windowClass.hInstance,
		0
	);

	ShowWindow(window, SW_SHOWDEFAULT);
	UpdateWindow(window);
}

void gui::DestroyHWindow() noexcept
{
	DestroyWindow(window);
	UnregisterClass(windowClass.lpszClassName, windowClass.hInstance);
}

bool gui::CreateDevice() noexcept
{
	d3d = Direct3DCreate9(D3D_SDK_VERSION);

	if (!d3d)
		return false;

	ZeroMemory(&presentParameters, sizeof(presentParameters));

	presentParameters.Windowed = TRUE;
	presentParameters.SwapEffect = D3DSWAPEFFECT_DISCARD;
	presentParameters.BackBufferFormat = D3DFMT_UNKNOWN;
	presentParameters.EnableAutoDepthStencil = TRUE;
	presentParameters.AutoDepthStencilFormat = D3DFMT_D16;
	presentParameters.PresentationInterval = D3DPRESENT_INTERVAL_ONE;

	if (d3d->CreateDevice(
		D3DADAPTER_DEFAULT,
		D3DDEVTYPE_HAL,
		window,
		D3DCREATE_HARDWARE_VERTEXPROCESSING,
		&presentParameters,
		&device) < 0)
		return false;

	return true;
}

void gui::ResetDevice() noexcept
{
	ImGui_ImplDX9_InvalidateDeviceObjects();

	const auto result = device->Reset(&presentParameters);

	if (result == D3DERR_INVALIDCALL)
		IM_ASSERT(0);

	ImGui_ImplDX9_CreateDeviceObjects();
}

void gui::DestroyDevice() noexcept
{
	if (device)
	{
		device->Release();
		device = nullptr;
	}

	if (d3d)
	{
		d3d->Release();
		d3d = nullptr;
	}
}

void gui::CreateImGui() noexcept
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ::ImGui::GetIO();

	io.IniFilename = NULL;

	ImGui::StyleColorsDark();

	ImGui_ImplWin32_Init(window);
	ImGui_ImplDX9_Init(device);
}

void gui::DestroyImGui() noexcept
{
	ImGui_ImplDX9_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}

void gui::BeginRender() noexcept
{
	MSG message;
	while (PeekMessage(&message, 0, 0, 0, PM_REMOVE))
	{
		TranslateMessage(&message);
		DispatchMessage(&message);

		if (message.message == WM_QUIT)
		{
			isRunning = !isRunning;
			return;
		}
	}

	ImGui_ImplDX9_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
}

void gui::EndRender() noexcept
{
	ImGui::EndFrame();

	device->SetRenderState(D3DRS_ZENABLE, FALSE);
	device->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
	device->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);

	device->Clear(0, 0, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_RGBA(0, 0, 0, 255), 1.0f, 0);

	if (device->BeginScene() >= 0)
	{
		ImGui::Render();
		ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
		device->EndScene();
	}

	const auto result = device->Present(0, 0, 0, 0);

	if (result == D3DERR_DEVICELOST && device->TestCooperativeLevel() == D3DERR_DEVICENOTRESET)
		ResetDevice();
}

#include "color.h"
#include "config.h"

namespace ImGuiEx
{
	inline bool ColorEdit4(const char* label, Color* v, bool show_alpha = true)
	{
		auto clr = ImVec4{
			v->r() / 255.0f,
			v->g() / 255.0f,
			v->b() / 255.0f,
			v->a() / 255.0f
		};

		if (ImGui::ColorEdit4(label, &clr.x, show_alpha)) {
			v->SetColor(clr.x, clr.y, clr.z, clr.w);
			return true;
		}
		return false;
	}
	inline bool ColorEdit3(const char* label, Color* v)
	{
		return ColorEdit4(label, v, false);
	}
}

void gui::Render() noexcept
{
	ImGui::SetNextWindowPos({ 0, 0 });
	ImGui::SetNextWindowSize({ WIDTH, HEIGHT });
	ImGui::Begin(
		"CSGO External Hack", 
		&isRunning,
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoSavedSettings |
		ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoMove
	);

	if (ImGui::BeginTabBar("CSGO External Hack")) { 
		if (ImGui::BeginTabItem("Aim")) { 
			ImGui::Checkbox("Aim", g_Options.aim); 
			ImGui::SliderInt("Aim Smooth", g_Options.aim_smooth, 1, 100); 
			ImGui::SliderInt("Aim Smooth After Shot", g_Options.aim_smooth_after_shot, 1, 100);
			ImGui::SliderInt("Aim FOV", g_Options.aim_fov, 1, 100);
			ImGui::Checkbox("Recoil", g_Options.recoil);
			ImGui::SliderInt("Recoil Smooth X", g_Options.recoil_smooth_x, 1, 100);
			ImGui::SliderInt("Recoil Smooth Y", g_Options.recoil_smooth_y, 1, 100);
			ImGui::Checkbox("Trigger", g_Options.trigger);
			ImGui::SliderInt("Trigger delay", g_Options.trigger_delay, 1, 50);
			ImGui::EndTabItem();
		}

		if (ImGui::BeginTabItem("Visual")) {
			ImGui::Checkbox("Glow", g_Options.glow);
			ImGuiEx::ColorEdit3("Glow color", g_Options.glow_color);
			ImGui::EndTabItem();
		}

		if (ImGui::BeginTabItem("Misc")) {
			ImGui::Checkbox("Bhop", g_Options.bhop);
			ImGui::SliderInt("Bhop Chance", g_Options.bhop_chance, 1, 100);
			ImGui::Checkbox("Radar", g_Options.radar);
			ImGui::EndTabItem();
		}

		if (ImGui::BeginTabItem("Skins")) {
			const char* ak_skins[] = { "None", "Neon Rider", "Bloodsport", "Wasteland Rebel", "Fuel Injector", "Vulcan", "Frontside Misty", "Point Disarray", "Redline", "Uncharted", "Elite Build"};
			ImGui::Combo("AK47", g_Options.ak_skin, ak_skins, IM_ARRAYSIZE(ak_skins), 0);

			const char* m4_skins[] = { "None", "Howl", "The Emperor", "Neo-Noir", "Buzz Kill", "Cyber Security", "Desolate Space", "Dragon King", "In Living Color", "Asiimov", "Spider Lily"};
			ImGui::Combo("M4A4", g_Options.m4_skin, m4_skins, IM_ARRAYSIZE(m4_skins), 0);

			const char* m4_silencer_skins[] = { "None", "Printsreen", "Player Two", "Chantico's Fire", "Golden Coil", "Hyber Beast", "Cyrex", "Nightmare", "Leaded Glass", "Decimator", "Icarus Fell"};
			ImGui::Combo("M4A1-S", g_Options.m4_silencer_skin, m4_silencer_skins, IM_ARRAYSIZE(m4_silencer_skins), 0);

			const char* awp_skins[] = { "None", "Containment Breach", "Wild Fire", "Neo-Noir", "Hyper Beast", "Asiimov", "Lightning Strike", "Fade", "The Prince", "Gungnir", "Medusa", "Dragon Lore", "Fever Dream", "Elite Build", "Redline", "Electric Hive", "BOOM", "Atheris", "Wrom God", "Ping DDPAT", };
			ImGui::Combo("AWP", g_Options.awp_skin, awp_skins, IM_ARRAYSIZE(awp_skins), 0);

			ImGui::EndTabItem();
		}

		if (ImGui::BeginTabItem("Cfg")) {
			if (ImGui::Button("Save cfg"))
				save();

			if (ImGui::Button("Load cfg"))
				load();

			ImGui::EndTabItem();
		}

		ImGui::EndTabBar();
	}

	ImGui::End();
}