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

	float CalcPassSpace(
		const DirectX::XMFLOAT3& OwnerPos, 
		const DirectX::XMFLOAT3& targetPos, 
		const std::vector<std::unique_ptr<SoccerPlayer>>& players, 
		int TeamNumber);

	float CalcMarkThreat(
		const DirectX::XMFLOAT3& opponentPos,
		const DirectX::XMFLOAT3& goalPos,
		bool hasBall);

	float CalcMarkCost(
		const DirectX::XMFLOAT3& defenderPos,
		const DirectX::XMFLOAT3& opponentPos,
		const DirectX::XMFLOAT3& goalPos,
		float threat);
	
	//味方のプレイヤーの役割の負担を計算
	std::vector<std::vector<float>> CalcAttackCost(
		const DirectX::XMFLOAT3& Apos,
		const DirectX::XMFLOAT3& Bpos,
		const DirectX::XMFLOAT3& OwnerPos,
		const DirectX::XMFLOAT3& GoalPos);

	std::vector<std::vector<float>> CalcAttackCost(
		const DirectX::XMFLOAT3& Apos,
		const DirectX::XMFLOAT3& Bpos,
		const DirectX::XMFLOAT3& OwnerPos,
		const DirectX::XMFLOAT3& GoalPos,
		const BlackBoard& blackboard,
		const std::vector<std::unique_ptr<SoccerPlayer>>& players);

	DirectX::XMFLOAT3 ClampToPitch(DirectX::XMFLOAT3 pos);

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