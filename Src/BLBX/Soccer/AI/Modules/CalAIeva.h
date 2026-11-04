#pragma once

class SoccerPlayer;
class BlackBoard;

#include <vector>
#include <memory>

#include <DirectXMath.h>

struct Evaluation
{
	// 計算の内訳
	float distance = 0.0f;
	float goalDistance = 0.0f;

	float nearScore = 0.0f;
	float nearGoal = 0.0f;
	float areaDanger = 0.0f;
	float danger = 0.0f;

	bool insideDangerArea = false;

	// リクエストに使う結果
	float utility = 0.0f;
	float urgency = 0.0f;
	float Movespeed = 0.0f;
};

class CalAIeva
{
public:
	void Initialize();

	Evaluation Calculation(
		const BlackBoard& blackboard,
		std::vector<std::unique_ptr<SoccerPlayer>>& players,
		int playerIndex,
		float elapsedTime);

	float CalculationCover(
		const DirectX::XMFLOAT3& ownerPos,
		const DirectX::XMFLOAT3& playerPos,
		const DirectX::XMFLOAT3& goalPos);

	void DrawImGui();

private:
	// 距離による評価
	float m_MaxReach = 10.0f;
	float m_MaxDangerDistance = 5.0f;

	// ゴール前の危険範囲
	float m_DangerHalfWidth = 3.7f;
	float m_DangerDepth = 3.0f;
	float m_AreaDangerBoost = 0.3f;

	// 横ずれによる評価
	float m_MaxSideSlip = 0.8f;

};