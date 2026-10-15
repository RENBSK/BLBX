#pragma once
#include "Object/ModelObject.h"

#include"Data/PlayerConfig.h"

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
	//向きを進行方向へ
	void FaceToMoveDirection(const DirectX::XMFLOAT3& move);

	//アニメーション番号切り替え
	void ChangeAnimation(PlayerAnimationState state, bool restart);

	void SetConfig(const PlayerConfig config);

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

private:

	//選手の能力値
	PlayerParameters m_parameters{};
	//選手の設定
	PlayerConfig m_config{};

	//アニメーション
	PlayerAnimationState m_animationState = PlayerAnimationState::Idle;
	float m_animationTime = 0.0f;
	std::vector<GltfModel::node> m_animatedNodes;

};