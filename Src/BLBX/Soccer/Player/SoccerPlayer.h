#pragma once
#include "Object/ModelObject.h"

#include "Soccer/Player/Movement/PlayerMovement.h"

#include"Data/PlayerConfig.h"

struct SoccerBall;

class SoccerPlayer : public ModelObject
{
public:
	//初期化
	void Initialize() override;
	//終了化
	void Finalize();
	//更新
	void Update(float elapsedTime) override;
	//描画
	void Render() override;
	//ImGui描画
	void DrawImGui() override;
	//移動
	void Move(const DirectX::XMFLOAT3& move);
	//パス
	bool Pass(SoccerBall& ball, const DirectX::XMFLOAT3& targetPosition, float passSpeed);
	//シュート
	bool Shoot(DirectX::XMFLOAT3 GoalPos,SoccerBall& ball,float shootSpeed);
	//向きを進行方向へ
	void FaceToMoveDirection(const DirectX::XMFLOAT3& move);
	//移動関数
	void UpdateMovement(const DirectX::XMFLOAT3& direction, float moveSpeed, float elapsedTime);
	//アニメーションの再生
	void UpdateAnimation(float elapsedTime);
	//アニメーション番号切り替え
	void ChangeAnimation(PlayerAnimationState state, bool restart);
	//手動と自動を切り替え
	bool IsAutoAnimationEnabled() const
	{
		return m_autoAnimation;
	}
	//アニメーション再生終了判定
	bool IsControlLocked() const;

	void SetConfig(const PlayerConfig config);

	void SetTeamNumber(int teamnumber)
	{
		m_TeamNumber = teamnumber;
	}

	float GetAnimationPhase() const;

	//前方向を取得
	DirectX::XMFLOAT3 GetForward() const;

	const PlayerParameters& GetParameters() const
	{
		return m_parameters;
	}

	const PlayerConfig& GetConfig() const
	{
		return m_config;
	}

	void SetMovementSpeed(float speed)
	{
		m_movementSpeed = speed;
	}

	void SetHasBall(bool set)
	{
		m_hasBall = set;
	}
	
	void SetAnimationPaused(bool paused)
	{
		m_animationPaused = paused;
	}

	PlayerAnimationState GetAnimationState() const
	{
		return m_animationState;
	}

	int GetTeamNumber()
	{
		return m_TeamNumber;
	}

	float GetMovementSpeed()
	{
		return m_movementSpeed;
	}

	DirectX::XMFLOAT3 GetVelocity() const
	{
		return m_Playermovement.GetVelocity();
	}

	bool GetHasBall() const
	{
		return m_hasBall;
	}

private:
	//選手の能力値
	PlayerParameters m_parameters{};
	//選手の設定
	PlayerConfig m_config{};
	//選手の移動基盤
	PlayerMovement m_Playermovement{};

	//アニメーション
	PlayerAnimationState m_animationState = PlayerAnimationState::Idle;
	float m_animationTime = 0.0f;
	std::vector<GltfModel::node> m_animatedNodes;

	bool m_autoAnimation = true;

	bool m_animationFinished = false;

	//切り替え前の姿勢
	std::vector<GltfModel::node> m_blendFromNodes;

	//アニメーションブレンド
	float m_blendTime = 0.0f;
	float m_blendDuration = 0.0f;
	bool m_isBlending = false;

	bool m_animationPaused = false;

	//プレイヤー移動速度
	float m_movementSpeed = 0.0f;

	//チームナンバー
	int m_TeamNumber = 0;
	//ボール保持
	bool m_hasBall = false;

	bool m_passKicked = false;

};