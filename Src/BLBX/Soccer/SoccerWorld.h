#pragma once
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
#include "Soccer/Player/Dribble/DribbleController.h"
#include "Soccer/Player/Manager/PlayerManager.h"
#include "Resource/Sprite/Sprite.h"

#include "Soccer/AI/Core/SoccerAISystem.h"

#define PLAYER_COUNT 3

class SoccerWorld
{
public:
	//初期化
	void Initialize(const PlayerConfig& config);

	//終了化
	void Finalize();

	//更新
	void Update(float elapsedTime);

	//描画
	void Render();

	//ImGui表示
	void DrawImGui(bool showBall, bool showPlayer);

	//操作プレイヤーの更新処理
	void UpdateBallOwner();

	//パス更新処理
	void UpdatePass(float elapsedTime);

	//パス相手を探す
	int FinPassTarget(SoccerPlayer& player);
	//ゴール
	bool IsGoal() const;

	void UpdateKickoff();

	
	void PlacePlayers();

	DirectX::XMFLOAT3 GetBallPosition() const
	{
		return m_ball.GetPosition();
	}


private:
	enum class MatchPhase
	{
		KickOff,
		Playing
	};

	MatchPhase m_matchPhase = MatchPhase::KickOff;

	//カメラ
	FreeCameraController m_freeCamera;

	//スタジアム
	Stadium m_stadium;

	//ボール
	SoccerBall m_ball;

	//プレイヤー
	SoccerPlayer m_player;

	//プレイヤーコントローラー
	PlayerController m_playerController;

	//ドリブル
	DribbleController m_dribbleController;

	SoccerAISystem m_soccerAISystem;

	std::unique_ptr<Sprite> sprites;

private:
	//Imgui
	bool m_showDebugUI = true;
	bool m_showCameraDebug = true;
	bool m_showStadiumDebug = true;
	bool m_showSoccerBall = true;
	bool m_showSoccerPlayer = true;

	float m_passElapsed = 0.0f;

	int m_kickoffTargetIndex = -1;

	bool m_isGoal = false;

	float m_spriteX = 0.0f;
	float m_spriteY = 0.0f;

	float m_spriteW = 0.0f;
	float m_spriteH = 0.0f;

	float m_goalElapsed = 0.0f;
	// 拡大にかける時間
	float m_goalDuration = 0.35f; 

	bool m_drawImGui = false;


};


