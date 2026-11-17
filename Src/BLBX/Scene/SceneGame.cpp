#include "SceneGame.h"
#include "SceneManager.h"
#include "Scene/SceneTitle.h"
#include "Graphics/Camera/Camera.h"

#include "Soccer/Player/Data/PlayerConfig.h"

#include <ImGuiCtrl.h>
#include <imgui.h>

// ========================================
// 初期化
// ========================================
void SceneGame::Initialize()
{
	//カメラ初期設定
	m_freeCamera.Initialize(
		DirectX::XMFLOAT3(-10.0, 15, -0.18),
		DirectX::XMFLOAT3(9.1, -12.1, -0.12));

	//ステージ
	m_stadium.Initialize();

    //サッカーワールド
    PlayerConfig config;
    config.modelPath = ("./Data/Model/Character/sin/test.glb");

    m_soccerWorld.Initialize(config);
}

// ========================================
// 終了化
// ========================================
void SceneGame::Finalize()
{
	//ステージ
	m_stadium.Finalize();

    m_soccerWorld.Finalize();
}

// ========================================
// 更新
// ========================================
void SceneGame::Update(float elapsedTime)
{
    if (m_isPaused)
    {
        return;
    }

	//カメラ
	m_freeCamera.Update(elapsedTime);

	//ステージ
	m_stadium.Update(elapsedTime);

    //サッカーワールド
    m_soccerWorld.Update(elapsedTime);

    m_freeCamera.Follow(m_soccerWorld.GetBallPosition(), elapsedTime);
}

// ========================================
// 描画
// ========================================
void SceneGame::Render()
{
	Camera& camera = Camera::Instance();

	SceneConstants data{};

	DirectX::XMMATRIX V =
		DirectX::XMLoadFloat4x4(&camera.GetView());

	DirectX::XMMATRIX P =
		DirectX::XMLoadFloat4x4(&camera.GetProjection());

	DirectX::XMStoreFloat4x4(&data.view_projection, V * P);

	DirectX::XMFLOAT3 eye = camera.GetEye();
	data.camera_position = { eye.x, eye.y, eye.z, 1.0f };

	data.light_direction = { 0.0f, -1.0f, 0.0f, 0.0f };

	Graphics::Instance().UpdateSceneConstants(data);


	//ステージ
	m_stadium.Render();

    m_soccerWorld.Render();
}

// ========================================
// ImGui
// ========================================
void SceneGame::DrawGUI()
{
#ifdef USE_IMGUI

    // F1でデバッグパネルを一括切り替え
    if (ImGui::IsKeyPressed(ImGuiKey_F1, false))
    {
        m_showDebugUI = !m_showDebugUI;
    }

    // メニューバー（常に表示）
    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("View"))
        {
            ImGui::MenuItem(
                "Debug Panels", "F1", &m_showDebugUI
            );

            ImGui::Separator();

            ImGui::MenuItem(
                "Camera", nullptr, &m_showCameraDebug
            );

            ImGui::MenuItem(
                "Stadium", nullptr, &m_showStadiumDebug
            );

            ImGui::MenuItem(
                "Ball", nullptr, &m_showSoccerBall
            );

            ImGui::MenuItem(
                "Player", nullptr, &m_showSoccerPlayer
            );


            ImGui::EndMenu();
        }


        if (ImGui::Button(m_isPaused ? "Resume" : "Pause"))
        {
            m_isPaused = !m_isPaused;
        }

        ImGui::EndMainMenuBar();
    }

    // デバッグパネル
    if (m_showDebugUI)
    {
        if (m_showCameraDebug)
        {
            m_freeCamera.DrawImgui();
        }

        if (m_showStadiumDebug)
        {
            m_stadium.DrawImGui();
        }

        m_soccerWorld.DrawImGui(
            m_showSoccerBall,
            m_showSoccerPlayer);
    }

#endif
}
