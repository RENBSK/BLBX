#include "Application.h"
#include "Input/Input.h"

#include "../Scene/SceneManager.h"
#include "../Scene/SceneTitle.h"
#include "../Scene/SceneGame.h"
#include "Renderer/ImGuiRenderer.h"

#include <ImGuiCtrl.h>
#include <imgui.h>

#if defined(_DEBUG)
#include <dxgidebug.h>
#pragma comment(lib, "dxguid.lib")
#endif

static const int syncInterval = 1;

#if defined(_DEBUG)
static void ReportD3DLiveObjects()
{
	ID3D11Device* device = Graphics::Instance().GetDevice();
	if (device == nullptr) return;

	ID3D11Debug* debug = nullptr;

	if (SUCCEEDED(device->QueryInterface(__uuidof(ID3D11Debug), reinterpret_cast<void**>(&debug))))
	{
		debug->ReportLiveDeviceObjects(D3D11_RLDO_DETAIL);
		debug->Release();
		debug = nullptr;
	}
}
#endif

Framework::Framework(HWND hWnd)
	: m_hwnd(hWnd)
{
	m_hDC = GetDC(hWnd);

	Graphics::Instance().Initialize(hWnd);

	// IMGUI‰Šú‰»
	ImGuiRenderer::Initialize(hWnd, Graphics::Instance().GetDevice(), Graphics::Instance().GetContext());

	SceneManager::Instance().ChangeScene(new SceneTitle);
}

Framework::~Framework()
{
	SceneManager::Instance().Clear();

	ImGuiRenderer::Finalize();

	ID3D11DeviceContext* dc = Graphics::Instance().GetContext();
	if (dc)
	{
		dc->ClearState();
		dc->Flush();
	}

	Graphics::Instance().Finalize();

	ReleaseDC(m_hwnd, m_hDC);
}

bool Framework::Initialize()
{

	return true;
}

void Framework::Update(float elapsedTime)
{
	SceneManager::Instance().Update(elapsedTime);

	ImGuiRenderer::NewFrame();

	RenderImGui();
}

void Framework::Render(float elapsedTime)
{
	std::lock_guard<std::mutex> lock(Graphics::Instance().GetMutex());

	ID3D11DeviceContext* dc = Graphics::Instance().GetContext();

	Graphics::Instance().Clear(0, 0, 1, 1);

	Graphics::Instance().SetRenderTargets();

	SceneManager::Instance().Render();

#ifdef USE_IMGUI
	ImGui::Render();
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
#endif
	ImGuiRenderer::Render(dc);

	Graphics::Instance().Present(syncInterval);
}

bool Framework::UnInitialize()
{
	return true;
}

void Framework::RenderImGui()
{
#ifdef USE_IMGUI
	if (ImGui::BeginMainMenuBar())
	{
		if (ImGui::BeginMenu("Scene"))
		{
			if (ImGui::MenuItem("Title"))
			{
				SceneManager::Instance().ChangeScene(new SceneTitle);
				ImGui::EndMenu();
				ImGui::EndMainMenuBar();
				return;
			}

			if (ImGui::MenuItem("Game"))
			{
				SceneManager::Instance().ChangeScene(new SceneGame);
				ImGui::EndMenu();
				ImGui::EndMainMenuBar();
				return;
			}

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Render"))
		{
			ImGui::Combo("Sampler", (int*)&samplerState,
				"Point\0Linear\0Anisotropic\0Border Black\0Border White\0");
			ImGui::Combo("Depth", &depthState, "Enable\0Disable\0");
			ImGui::Combo("Blend", &blendState, "Alpha\0None\0");
			ImGui::Combo(
				"Raster",
				&rasterState,
				"Solid CullNone\0Wireframe CullNone\0Solid CullBack\0"
			);
			ImGui::EndMenu();
		}

		ImGui::EndMainMenuBar();
	}


	Graphics::Instance().SetSampler(samplerState);
	Graphics::Instance().SetSampler(samplerState, 2);
	Graphics::Instance().SetDepthState(depthState);
	Graphics::Instance().SetBlendState(blendState);
	Graphics::Instance().SetRasterizerState(rasterState);

	SceneManager::Instance().DrawGUI();

#endif
}

int Framework::Run()
{
	MSG msg{};

	while (WM_QUIT != msg.message)
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			tictoc.tick();
			Calculate_frame_stats();

			/*	float elapsedTime = syncInterval == 0
					? tictoc.time_interval()
					: syncInterval / static_cast<float>(GetDeviceCaps(hDC, VREFRESH))
					;*/


			float elapsedTime = tictoc.time_interval();

			// d‚·‚¬‚½‚Æ‚«‚É•¨—‚ª”ò‚Ñ‚·‚¬‚È‚¢‚æ‚¤‚É§ŒÀ
			if (elapsedTime > 0.05f)
			{
				elapsedTime = 0.05f;
			}

			Update(elapsedTime);
			Render(elapsedTime);

		}
	}
	return static_cast<int>(msg.wParam);
}

LRESULT CALLBACK Framework::Handle_Message(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
#ifdef USE_IMGUI
	if (ImGuiRenderer::HandleMessage(hWnd, msg, wParam, lParam))
		return true;
#endif 
	switch (msg)
	{
	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		HDC hdc;
		hdc = BeginPaint(hWnd, &ps);
		EndPaint(hWnd, &ps);
		break;
	}

	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	case WM_CREATE:
		break;
	case WM_KEYDOWN:
		if (wParam == VK_ESCAPE)
		{
			PostMessage(hWnd, WM_CLOSE, 0, 0);
		}
		break;
	case WM_ENTERSIZEMOVE:
		tictoc.stop();
		break;
	case WM_EXITSIZEMOVE:
		tictoc.start();
		break;
	default:
		return DefWindowProc(hWnd, msg, wParam, lParam);
	}
	return 0;
}
