#include "SceneGame.h"
#include "SceneManager.h"
#include "Scene/SceneTitle.h"
#include "Graphics/Camera/Camera.h"

#include "Soccer/Player/Data/PlayerConfig.h"

#include <ImGuiCtrl.h>
#include <imgui.h>

void SceneGame::Initialize()
{
	//カメラ
	m_freeCamera.Initialize(
		DirectX::XMFLOAT3(-10.0, 15, -0.18),
		DirectX::XMFLOAT3(9.1, -12.1, -0.12));

	//ステージ
	m_stadium.Initialize();

    //ボール
    m_soccerBall.Initialize();

    //プレイヤー
    PlayerConfig config;

    //モデルパス
    config.modelPath = ("./Data/Model/Character/sin/Untitled.glb");

    m_player.SetConfig(config);
    m_player.Initialize();
}

void SceneGame::Finalize()
{
	//ステージ
	m_stadium.Finalize();
    //ボール
    m_soccerBall.Finalize();
    //プレイヤー
    m_player.Finalize();
}

void SceneGame::Update(float elapsedTime)
{
	//カメラ
	m_freeCamera.Update(elapsedTime);

	//ステージ
	m_stadium.Update(elapsedTime);

    //ボール
    m_soccerBall.Update(elapsedTime);

    //プレイヤー移動
    m_playerMovement.Update(
        m_player,
        m_playerController.GetMoveDirection(),
        elapsedTime);

    //プレイヤー
    m_player.Update(elapsedTime);
}

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

    //ボール
    m_soccerBall.Render();

    //プレイヤー
    m_player.Render();
}

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

        if (m_showSoccerBall)
        {
            m_soccerBall.DrawImGui();
        }

        if (m_showSoccerPlayer)
        {
            m_player.DrawImGui();
        }
    }

#endif
}
