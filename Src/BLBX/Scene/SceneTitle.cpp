#include "../Graphics/Graphics.h"
#include "SceneTitle.h"
#include "SceneManager.h"
#include "../Renderer/ShapeRenderer2D.h"

#include "Graphics/Camera/Camera.h"

#include "Soccer/Player/Data/PlayerConfig.h"
#include "Soccer/Player/Manager/PlayerManager.h"

#ifdef USE_IMGUI
#include <imgui.h>
#endif

//初期化
void SceneTitle::Initialize()
{
	m_freeCamera.Initialize(
		DirectX::XMFLOAT3(-1.3f, 2.0, 9.1),
		DirectX::XMFLOAT3(18.0f, -8.6, -12.0f));

	sprites[0] = std::make_unique<Sprite>(Graphics::Instance().GetDevice(), L"./Data/Sprite/Title.png");

	//サッカーワールド
	PlayerConfig config;
	config.modelPath = ("./Data/Model/Character/sin/test.glb");

	PlayerManager::Instance().CreateTeam(0, 1, config);
	
	SoccerPlayer* player = PlayerManager::Instance().GetPlayer(0);

	player->SetPosition({ 0.0f, 0.64f, 5.5f });

	m_stadium.Initialize();

	m_ShootCount = 0;
	m_shootTimer = 0.0f;
	
	m_spriteX = 7.0f;
	m_spriteY = 50.0f;

	m_spriteW = 640;
	m_spriteH = 360;
}

//終了化
void SceneTitle::Finalize()
{
	PlayerManager::Instance().Finalize();

	m_stadium.Finalize();
}

//更新処理
void SceneTitle::Update(float elapsedTime)
{
	auto& manager = PlayerManager::Instance();

	SoccerPlayer* player = manager.GetPlayer(0);

	if (player->GetPosition().z < 7.0f)
	{
		player->Move({ 0.0f, 0.0f, 1.3f*elapsedTime });
		player->ChangeAnimation(PlayerAnimationState::Run, false);
	}
	else if (m_ShootCount == 0)
	{
		// シュートを最初から再生し、時間計測を開始
		m_ShootCount = 1;
		m_shootTimer = 0.0f;

		player->SetAnimationPaused(false);
		player->ChangeAnimation(PlayerAnimationState::Shoot, true);
	}

	// シュート開始からの経過時間
	if (m_ShootCount == 1)
	{
		m_shootTimer += elapsedTime;

		if (m_shootTimer >= 0.6f)
		{
			// 現在のポーズのまま停止
			player->SetAnimationPaused(true);
			m_ShootCount = 2;
		}
	}

	manager.Update(elapsedTime);

	m_stadium.Update(elapsedTime);
	//カメラ
	m_freeCamera.Update(elapsedTime);
}

//描画処理
void SceneTitle::Render()
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

	m_stadium.Render();
	PlayerManager::Instance().Render();

	// 3Dを描いた後に、透過画像を重ねる
	if (m_ShootCount == 2)
	{
		auto& graphics = Graphics::Instance();

		graphics.SetDepthState(1); // 深度判定・書き込みなし
		graphics.SetBlendState(0); // アルファブレンド

		sprites[0]->render(
			graphics.GetContext(),
			m_spriteX, m_spriteY,
			m_spriteW, m_spriteH,
			1, 1, 1, 1,
			0);

		graphics.SetDepthState(0);
	}
}

//GUI描画
void SceneTitle::DrawGUI()
{
#ifdef USE_IMGUI

	m_freeCamera.DrawImgui();

	m_stadium.DrawImGui();

	PlayerManager::Instance().DrawImGui(m_ball);

	ImGui::DragFloat(
		"SpriteX",
		&m_spriteX,
		0.1f);

	ImGui::DragFloat(
		"SpriteY",
		&m_spriteY,
		0.1f);

	ImGui::DragFloat(
		"SpriteW",
		&m_spriteW,
		0.1f);

	ImGui::DragFloat(
		"SpriteH",
		&m_spriteH,
		0.1f);
#endif
}