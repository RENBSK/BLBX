#pragma once

#include "Scene.h"
#include <memory>

#include "Resource/Model/GltfModel.h"
#include "Graphics/Graphics.h"
#include "Core/Config.h"
#include "Core/Transform.h"
#include "Graphics/Camera/FreeCameraController.h"

#include "Soccer/Stage/Stadium.h"
#include "Soccer/Ball/SoccerBall.h"
#include "Soccer/Player/SoccerPlayer.h"
#include "Soccer/Player/Controller/PlayerController.h"
#include "Soccer/Player/Movement/PlayerMovement.h"


//タイトルシーン
class SceneGame :public Scene
{
public:
	SceneGame() {}
	~SceneGame() override {}

	//初期化　
	void Initialize() override;

	//終了化
	virtual void Finalize() override;

	//更新処理
	virtual void Update(float elapsedTime) override;

	//描画処理
	virtual void Render()  override;

	//GUI描画
	virtual void DrawGUI() override;

private:
	//カメラ
	FreeCameraController m_freeCamera;

	//スタジアム
	Stadium m_stadium;

	//ボール
	SoccerBall m_soccerBall;

	//プレイヤー
	SoccerPlayer m_player;

	//プレイヤーコントローラー
	PlayerController m_playerController;

	PlayerMovement m_playerMovement;

private:
	//Imgui
	bool m_showDebugUI = true;
	bool m_showCameraDebug = true;
	bool m_showStadiumDebug = true;
	bool m_showSoccerBall = true;
	bool m_showSoccerPlayer = true;
};