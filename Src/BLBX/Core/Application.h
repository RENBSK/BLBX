#pragma once

#include <Windows.h>
#include <memory>
#include<sstream>
#include <tchar.h>
#include <wrl.h>

#include <d3d11.h>

#include"Misc.h"
#include "HighResolutionTimer.h"
#include "Graphics/Graphics.h"

#include"Graphics/SceneConstants.h"
#include "Config.h"

#ifdef USE_IMGUI
#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_impl_dx11.h"
#include "imgui_impl_win32.h"
extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
#endif

CONST BOOL FULLSCREEN{ FALSE };
CONST LPCWSTR APPLICATION_NAME{ L"FFBD" };

class Framework
{
public:
	Framework(HWND hWnd);
	~Framework();


	Framework(const Framework&) = delete;
	Framework& operator=(const Framework&) = delete;
	Framework(Framework&&) noexcept = delete;
	Framework& operator=(Framework&&) noexcept = delete;

private:
	bool Initialize();
	void Update(float elapsedTime);
	void Render(float elapsedTime);
	bool UnInitialize();
	void RenderImGui();

private:
	CONST HWND m_hwnd;

	High_resolution_timer tictoc;
	uint32_t frames{ 0 };
	float elapsed_time{ 0.0f };
	void Calculate_frame_stats()
	{
		static int frames = 0;
		static float time_tlapsed = 0.0f;

		frames++;

		// Compute averages over one second period.
		if ((tictoc.time_stamp() - time_tlapsed) >= 1.0f)
		{
			float fps = static_cast<float>(frames); // fps = frameCnt / 1
			float mspf = 1000.0f / fps;
			std::ostringstream outs;
			outs.precision(6);
			outs << "FPS : " << fps << " / " << "Frame Time : " << mspf << " (ms)";
			SetWindowTextA(m_hwnd, outs.str().c_str());

			// Reset for next average.
			frames = 0;
			time_tlapsed += 1.0f;
		}

		/*	if (++frames, (tictoc.time_stamp() - elapsed_time) >= 1.0f)
			{
				float fps = static_cast<float>(frames);
				std::wostringstream outs;
				outs.precision(6);
				outs << APPLICATION_NAME << L" : FPS : " << fps << L" / " << L"Frame Time : " << 1000.0f / fps << L" (ms)";
				SetWindowTextW(hwnd, outs.str().c_str());

				frames = 0;
				elapsed_time += 1.0f;
			}*/
	}

public:
	int Run();
	LRESULT CALLBACK Handle_Message(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

private:

	DirectX::XMFLOAT3 model_rotation = { 0.0f,0.0f,0.0f };
	float model_scale = 0.03f;

	SAMPLER_STATE samplerState = SAMPLER_STATE::LINEAR;
	int depthState = 0;
	int blendState = 0;
	int rasterState = 0;

	HDC m_hDC;
};