#include "CalAIeva.h"

#include "Soccer/AI/Core/BlackBoard.h"
#include "Soccer/AI/Keys/BlackBoardKeys.h"

#include "Soccer/Player/SoccerPlayer.h"

#include <algorithm>

#ifdef USE_IMGUI
#include <imgui.h>
#endif

//距離、危険性

Evaluation CalAIeva::Calculation(
	const BlackBoard& blackboard, 
	std::vector<std::unique_ptr<SoccerPlayer>>& players, 
	int playerIndex,
	float elapsedTime)
{
	const float Maxreach = m_MaxReach;
	const float Maxdanger = m_MaxDangerDistance;

	//選ばれたプレイヤーの位置
	DirectX::XMFLOAT3 PlayerPos = players[playerIndex]->GetPosition();

	//保持者の位置
	int OwnerIndex = blackboard.Get<int>(BallKeys::OwnerIndex).value_or(-1);

	if (OwnerIndex < 0)
	{
		return Evaluation{};
	}

	DirectX::XMFLOAT3 OwnerPos = players[OwnerIndex]->GetPosition();

	DirectX::XMFLOAT3 targetDirection =
	{
		OwnerPos.x - PlayerPos.x,
		0.0f,
		OwnerPos.z - PlayerPos.z
	};

	float distance = DirectX::XMVectorGetX(DirectX::XMVector3Length(DirectX::XMLoadFloat3(&targetDirection)));

	//最大距離からみて、今の距離の割合を計算し、近ければ高く
	//選ばれたプレイヤーから保持者までの距離
	float nearScore = std::clamp(1.0f - distance / Maxreach, 0.0f, 1.0f);

	//ゴールから保持者までの距離
	const auto GoalPos = blackboard.Get<DirectX::XMFLOAT3>(TeamKeys::DefendGoalPosition);

	DirectX::XMFLOAT3 GoalDirection =
	{
		GoalPos->x - OwnerPos.x,
		0.0f,
		GoalPos->z - OwnerPos.z
	};

	float Goaldistance = DirectX::XMVectorGetX(DirectX::XMVector3Length(DirectX::XMLoadFloat3(&GoalDirection)));



	//ゴールまでの距離が近ければ危険度を上げる
	//次はある範囲に入れば、より危険度を増すように
	float nearGoal = std::clamp(1.0f - Goaldistance / Maxdanger, 0.0f, 1.0f);



	//仮制作
	const float halfWidth = m_DangerHalfWidth;
	const float depth = m_DangerDepth;

		//ゴールからピッチ内へ向かうz方向
	const float frontSign = GoalPos->z < 0.0 ? 1.0f : -1.0f;

	const float sideDistance = OwnerPos.x - GoalPos->x;
	const float frontDistance = (OwnerPos.z - GoalPos->z) * frontSign;

	const float xRate = sideDistance / halfWidth;
	const float zRate = frontDistance / depth;

	const float ellipseValue = xRate * xRate + zRate * zRate;

	const bool inside = frontDistance >= 0.0f && ellipseValue <= 1.0f;

	const float areaDanger = inside ? 1.0f - std::sqrt(ellipseValue) : 0.0f;

	const float baseDanger = std::clamp(1.0f - Goaldistance / Maxreach, 0.0f, 1.0f);

	const float danger = std::clamp(nearGoal + areaDanger * m_AreaDangerBoost, 0.0f, 1.0f);

	Evaluation evaluation;

	evaluation.distance = distance;
	evaluation.goalDistance = Goaldistance;

	evaluation.nearScore = nearScore;
	evaluation.nearGoal = nearGoal;
	evaluation.areaDanger = areaDanger;
	evaluation.danger = danger;

	evaluation.insideDangerArea = inside;

	evaluation.utility = danger * nearScore;
	evaluation.urgency = danger;

	return evaluation;
}

float CalAIeva::CalculationCover(
	const DirectX::XMFLOAT3& ownerPos,
	const DirectX::XMFLOAT3& playerPos,
	const DirectX::XMFLOAT3& goalPos)
{
	DirectX::XMFLOAT3 OwnerToPlayer =
	{
		playerPos.x - ownerPos.x,
		0.0f,
		playerPos.z - ownerPos.z
	};

	DirectX::XMFLOAT3 OwnerToGoal =
	{
		goalPos.x - ownerPos.x,
		0.0f,
		goalPos.z - ownerPos.z
	};

	
	const float length = std::sqrt(
		OwnerToGoal.x * OwnerToGoal.x +
		OwnerToGoal.z * OwnerToGoal.z);

	if (length <= 0.000001f)
	{
		return 0.0f;
	}

	OwnerToGoal.x /= length;
	OwnerToGoal.z /= length;

	//前方の距離の計算
	float distanceTowardGoal = OwnerToPlayer.x * OwnerToGoal.x + OwnerToPlayer.z * OwnerToGoal.z;

	//保持者より後ろなら守れてない。
	if (distanceTowardGoal <= 0.0f)
	{
		return 0.0f;
	}

	//ゴールへの直線からの横ずれ
	float sideSlip = std::abs(OwnerToPlayer.x * OwnerToGoal.z - OwnerToPlayer.z * OwnerToGoal.x);

	const float score = std::clamp(1.0f - sideSlip / m_MaxSideSlip, 0.0f, 1.0f);

	return score;
}

void CalAIeva::DrawImGui()
{
#ifdef USE_IMGUI
	if (!ImGui::TreeNode("Calculation settings"))
	{
		return;
	}

	if (ImGui::TreeNode("Distance evaluation"))
	{
		ImGui::SliderFloat(
			"Reach distance",
			&m_MaxReach,
			0.5f, 20.0f, "%.2f");

		ImGui::TextDisabled(
			"Near-owner score reaches 0 at this distance.");

		ImGui::SliderFloat(
			"Goal danger distance",
			&m_MaxDangerDistance,
			0.5f, 20.0f, "%.2f");

		ImGui::TextDisabled(
			"Goal-distance danger reaches 0 here.");

		ImGui::TreePop();
	}

	if (ImGui::TreeNode("Danger area"))
	{
		ImGui::SliderFloat(
			"Half width",
			&m_DangerHalfWidth,
			0.1f, 8.0f, "%.2f");

		ImGui::Text(
			"Total width: %.2f",
			m_DangerHalfWidth * 2.0f);

		ImGui::SliderFloat(
			"Depth",
			&m_DangerDepth,
			0.1f, 10.0f, "%.2f");

		ImGui::SliderFloat(
			"Additional danger",
			&m_AreaDangerBoost,
			0.0f, 1.0f, "%.2f");

		ImGui::TextDisabled(
			"Maximum danger added inside the area.");

		ImGui::TreePop();
	}

	if (ImGui::TreeNode("Coverage evaluation"))
	{
		ImGui::SliderFloat(
			"Max side slip",
			&m_MaxSideSlip,
			0.1f, 3.0f, "%.2f");

		ImGui::TextDisabled(
			"Side score reaches 0 at this distance.");

		ImGui::TreePop();
	}

	ImGui::TreePop();
#endif
}



