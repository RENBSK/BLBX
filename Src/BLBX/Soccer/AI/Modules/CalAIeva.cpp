#include "CalAIeva.h"

#include "Soccer/AI/Core/BlackBoard.h"
#include "Soccer/AI/Keys/BlackBoardKeys.h"

#include "Soccer/Player/SoccerPlayer.h"


#include <algorithm>
#include <cmath>
#include<limits>

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

// 保持者→候補位置の直線から、一番近い敵までの横距離を返す
// 大きいほど、直線から敵が離れている
// 現段階では、敵がパス区間内にいるかの判定は含めていない
float CalAIeva::CalcPassSpace(
	const DirectX::XMFLOAT3& OwnerPos,
	const DirectX::XMFLOAT3& targetPos, 
	const std::vector<std::unique_ptr<SoccerPlayer>>& players, 
	int TeamNumber)
{
	// 最小値を探すため、最初は正の無限大にする
	// 調べる相手がいなければ、無限大のまま返る
	float minSideSlip = std::numeric_limits<float>::infinity();

	for (const auto& player : players)
	{
		// 選手が存在しない、または自チームなら対象外
		if (!player || player->GetTeamNumber() == TeamNumber)
		{
			continue;
		}

		DirectX::XMFLOAT3 enemyPos = player->GetPosition();

		// 保持者から敵への位置差
		// 横距離を求めたいので、こちらは正規化しない
		DirectX::XMFLOAT3 OwnerToEnemy =
		{
			enemyPos.x - OwnerPos.x,
			0.0f,
			enemyPos.z - OwnerPos.z
		};

		// 保持者から、調べる候補位置への位置差
		DirectX::XMFLOAT3 OwnerToTarget =
		{
			targetPos.x - OwnerPos.x,
			0.0f,
			targetPos.z - OwnerPos.z
		};

		// 保持者から候補位置までの距離
		float length = std::sqrt(OwnerToTarget.x * OwnerToTarget.x + OwnerToTarget.z * OwnerToTarget.z);

		// 保持者と候補位置が同じなら、方向を計算できない
		if (length <= 0.00001f)
		{
			continue;
		}

		// パス方向を長さ1にする
		// これにより、下の計算結果を横距離として扱える
		OwnerToTarget.x /= length;
		OwnerToTarget.z /= length;

		// パスの直線から、この敵までの横距離
		float sideSlip = std::abs(OwnerToTarget.x * OwnerToEnemy.z - OwnerToTarget.z * OwnerToEnemy.x);

		// 全相手の中で、一番小さい横距離を残す
		// 括弧はWindowsのminマクロとの衝突を避けるため
		minSideSlip = (std::min)(minSideSlip, sideSlip);
	}

	return minSideSlip;
}

float CalAIeva::CalcMarkThreat(
	const DirectX::XMFLOAT3& opponentPos, 
	const DirectX::XMFLOAT3& goalPos, 
	bool hasBall)
{
	if (hasBall)
	{
		return 1.0f;
	}

	const float dx = goalPos.x - opponentPos.x;
	const float dz = goalPos.z - opponentPos.z;
	const float distance = std::sqrt(dx * dx + dz * dz);

	const float nearGoal = std::clamp(1.0f - distance / m_MaxDangerDistance, 0.0f, 1.0f);

	//遠い選手でも最低限の警戒を残す
	return 0.15f + nearGoal * 0.7f;
}

float CalAIeva::CalcMarkCost(
	const DirectX::XMFLOAT3& defenderPos, 
	const DirectX::XMFLOAT3& opponentPos, 
	const DirectX::XMFLOAT3& goalPos, 
	float threat)
{
	constexpr float markDistance = 1.0f;

	float dirX = goalPos.x - opponentPos.x;
	float dirZ = goalPos.z - opponentPos.z;

	const float goalDistance =
		std::sqrt(dirX * dirX + dirZ * dirZ);

	if (goalDistance <= 0.00001f)
	{
		// ゴール方向を作れない位置は、評価上の大きな負担とする
		return 10.0f;
	}

	dirX /= goalDistance;
	dirZ /= goalDistance;

	// 相手からゴール側へ間合いを取った、対応の目安位置
	const float targetX = opponentPos.x + dirX * markDistance;
	const float targetZ = opponentPos.z + dirZ * markDistance;

	const float dx = targetX - defenderPos.x;
	const float dz = targetZ - defenderPos.z;

	const float moveDistance = std::sqrt(dx * dx + dz * dz);

	const float moveCost = std::clamp(
		moveDistance / m_MaxReach,
		0.0f,
		1.0f);

	// 守備選手が、相手よりどれだけゴール側にいるか
	const float toDefenderX = defenderPos.x - opponentPos.x;
	const float toDefenderZ = defenderPos.z - opponentPos.z;

	const float depth = toDefenderX * dirX + toDefenderZ * dirZ;

	// 相手と同じ位置や後ろなら負担が高い
	// ゴール側へmarkDistance以上入れていれば0
	const float positionCost = std::clamp(
		1.0f - depth / markDistance,
		0.0f,
		1.0f);

	return threat * (moveCost + positionCost * 0.5f);
}

std::vector<std::vector<float>> CalAIeva::CalcAttackCost(
	const DirectX::XMFLOAT3& Apos, 
	const DirectX::XMFLOAT3& Bpos, 
	const DirectX::XMFLOAT3& OwnerPos, 
	const DirectX::XMFLOAT3& GoalPos)
{
	return std::vector<std::vector<float>>();
}

std::vector<std::vector<float>> CalAIeva::CalcAttackCost(
	const DirectX::XMFLOAT3& Apos,
	const DirectX::XMFLOAT3& Bpos,
	const DirectX::XMFLOAT3& OwnerPos,
	const DirectX::XMFLOAT3& GoalPos,
	const BlackBoard& blackboard,
	const std::vector<std::unique_ptr<SoccerPlayer>>& players)
{
	//保持者から攻撃ゴールへの方向
	DirectX::XMFLOAT3 attackDir =
	{
		GoalPos.x - OwnerPos.x,
		0.0f,
		GoalPos.z - OwnerPos.z
	};

	const float length = std::sqrt(attackDir.x * attackDir.x + attackDir.z * attackDir.z);

	if (length < 0.000001f)
	{
		return{};
	}

	attackDir.x /= length;
	attackDir.z /= length;


	//仮位置からではなくて、現在地から負担などを足していく？
	{
		DirectX::XMFLOAT3 AposDir =
		{
			Apos.x - OwnerPos.x,
			0.0f,
			Apos.z - OwnerPos.z
		};

		const float length = std::sqrt(AposDir.x * AposDir.x + AposDir.z * AposDir.z);

		if (length < 0.00001f)
		{
			return {};
		}

		AposDir.x /= length;
		AposDir.z /= length;

		const int OwnerTeam = blackboard.Get<int>(BallKeys::OwnerTeam).value_or(-1);

		float PassDanger = CalcPassSpace(OwnerPos, Apos, players, OwnerTeam);

		//仮でコースから敵が2m以上離れていれば十分とする
		const float safeWidth = 2.0f;

		const float passScore = std::clamp(PassDanger / safeWidth, 0.0f, 1.0f);
		
		const float supportCost = 1.0 - passScore;

	}



	//仮の位置設定
	const float supportForward = 2.0f;
	const float supportSide = -2.0f;

	const float wideForward = 6.0f;
	const float wideSide = 3.0f;

	const DirectX::XMFLOAT3 supportPos =
	{
		OwnerPos.x + attackDir.x * supportForward + attackDir.z * supportSide,
		0.0f,
		OwnerPos.z + attackDir.z * supportForward - attackDir.x * supportSide,
	};

	const DirectX::XMFLOAT3 widePos =
	{
		OwnerPos.x + attackDir.x * wideForward + attackDir.z * wideSide,
		0.0f,
		OwnerPos.z + attackDir.z * wideForward - attackDir.x * wideSide,
	};

	//選手の並び。
	const DirectX::XMFLOAT3 playerPositions[2] =
	{
		Apos,
		Bpos
	};

	//役割の並び。
	const DirectX::XMFLOAT3 rolePositions[2] =
	{
		supportPos,
		widePos
	};


	std::vector<std::vector<float>> costs(2, std::vector<float>(2, 0.0f));

	for (int i = 0; i < 2; ++i)
	{
		for (int j = 0; j < 2; ++j)
		{
			const float dx = rolePositions[j].x - playerPositions[i].x;

			const float dz = rolePositions[j].z - playerPositions[i].z;

			costs[i][j] = std::sqrt(dx * dx + dz * dz);
		}
	}

	return costs;
}

DirectX::XMFLOAT3 CalAIeva::ClampToPitch(DirectX::XMFLOAT3 pos)
{
		constexpr float minX = -5.4f;
		constexpr float maxX = 5.6f;
		constexpr float minZ = 0.0f;
		constexpr float maxZ = 10.45f;
		constexpr float margin = 0.2f;

		pos.x = std::clamp(pos.x, minX + margin, maxX - margin);
		pos.z = std::clamp(pos.z, minZ + margin, maxZ - margin);
		return pos;
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



