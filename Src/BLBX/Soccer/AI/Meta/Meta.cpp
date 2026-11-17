#include "Meta.h"

#include "Soccer/AI/Core/BlackBoard.h"
#include "Soccer/AI/Keys/BlackBoardKeys.h"

#include "Soccer/Player/SoccerPlayer.h"

#include "Renderer/PrimitiveRenderer.h"
#include <cmath>
#include <algorithm>

#ifdef USE_IMGUI
#include <imgui.h>
#endif

void Meta::Update(
	const BlackBoard& sharedBB,
	BlackBoard& allyTeamBB,
	BlackBoard& enemyTeamBB,
	std::vector<std::unique_ptr<SoccerPlayer>>& players,
	float elapsedTime)
{
	//味方チームを0,敵チームを1に登録しておく
	const int owmerTeam = sharedBB.Get<int>(BallKeys::OwnerTeam).value_or(-1);

	int attackTeam = sharedBB.Get<int>(BallKeys::OwnerTeam).value_or(-1);
	if (attackTeam < 0)
	{
		attackTeam = sharedBB.Get<int>(BallKeys::PassTeam).value_or(-1);
	}

	allyTeamBB.Set(TeamKeys::IsAttacking, attackTeam == 0);
	allyTeamBB.Set(TeamKeys::IsDefending, attackTeam == 1);

	enemyTeamBB.Set(TeamKeys::IsAttacking, attackTeam == 1);
	enemyTeamBB.Set(TeamKeys::IsDefending, attackTeam == 0);



	//プレスプレイヤーを決める
	FindPressPlayerIndex(sharedBB, allyTeamBB, enemyTeamBB, players, elapsedTime);

	FindDefenderSupportPlayerIndex(sharedBB, allyTeamBB, enemyTeamBB, players, elapsedTime);

	FindAttackSupportPlayerIndex(sharedBB, allyTeamBB, enemyTeamBB, players, elapsedTime);

	//AssignAttack(sharedBB, allyTeamBB, enemyTeamBB, players, elapsedTime);

	AssignMarks(sharedBB, allyTeamBB, enemyTeamBB, players, elapsedTime);

	AssainFreeBallPlayerIndex(sharedBB, allyTeamBB, enemyTeamBB, players, elapsedTime);
}

void Meta::FindPressPlayerIndex(
	const BlackBoard& sharedBB, 
	BlackBoard& allyTeamBB, 
	BlackBoard& enemyTeamBB, 
	std::vector<std::unique_ptr<SoccerPlayer>>& players,
	float elapsedTime)
{
	int referenceIndex = sharedBB.Get<int>(BallKeys::OwnerIndex).value_or(-1);

	if (referenceIndex < 0)
	{
		referenceIndex = sharedBB.Get<int>(BallKeys::PassTarget).value_or(-1);
	}

	if (referenceIndex < 0 ||
		referenceIndex >= static_cast<int>(players.size()) ||
		!players[referenceIndex])
	{
		return;
	}

	DirectX::XMFLOAT3 referencePos = players[referenceIndex]->GetPosition();

	for (int teamNumber = 0; teamNumber < 2; ++teamNumber)
	{
		BlackBoard& teamBB = teamNumber == 0 ? allyTeamBB : enemyTeamBB;

		//プレスプレイヤーを決める。
		//最も近いプレイヤーを探す。
		int bestIndex = -1;
		float nearestDistSq = FLT_MAX;
		for (int i = 0; i < static_cast<int>(players.size()); ++i)
		{
			SoccerPlayer* player = players[i].get();
			if (!player || player->GetTeamNumber() != teamNumber)
			{
				continue;
			}

			DirectX::XMFLOAT3 playerPos = player->GetPosition();

			float dx = referencePos.x - playerPos.x;
			float dz = referencePos.z - playerPos.z;

			float distSq = dx * dx + dz * dz;

			if (distSq < nearestDistSq)
			{
				nearestDistSq = distSq;
				bestIndex = i;
			}
		}

		//すぐにプレスプレイヤーを切り替えるのを防ぐ
		//将来的には候補型にして、さまざまの情報から候補にするかきめるようにしたい。
		PressAssignmentState& state = m_pressStates[teamNumber];

		const int currentIndex = teamBB.Get<int>(PlayerKeys::PressPlayerIndex).value_or(-1);

		const bool isDefending = teamBB.Get<bool>(TeamKeys::IsDefending).value_or(false);

		int pressIndex = -1;

		if (!isDefending || bestIndex < 0)
		{
			// 守備しない、または候補がいないので解除
			state = PressAssignmentState{};
		}
		else if (currentIndex < 0 || currentIndex == bestIndex)
		{
			// 初回はすぐ決定。同じ担当ならそのまま維持
			pressIndex = bestIndex;
			state = PressAssignmentState{};
		}
		else
		{
			// 切り替え待ちの間は、現在の担当を維持
			pressIndex = currentIndex;

			// 候補が変わったら計り直す
			if (state.candidateIndex != bestIndex)
			{
				state.candidateIndex = bestIndex;
				state.timer = 0.0f;
			}

			state.timer += elapsedTime;

			if (state.timer >= m_PressSwitchTimer)
			{
				pressIndex = bestIndex;
				state = PressAssignmentState{};
			}
		}

		// 決定した担当を登録
		teamBB.Set(PlayerKeys::PressPlayerIndex, pressIndex);
		teamBB.Set(PlayerKeys::PressTargetIndex, pressIndex >= 0 ? referenceIndex : -1);
	}
}


void Meta::FindDefenderSupportPlayerIndex(
	const BlackBoard& sharedBB, 
	BlackBoard& allyTeamBB, 
	BlackBoard& enemyTeamBB, 
	std::vector<std::unique_ptr<SoccerPlayer>>& players, 
	float elapsedTime)
{
	int referenceIndex = sharedBB.Get<int>(BallKeys::OwnerIndex).value_or(-1);

	if (referenceIndex < 0)
	{
		referenceIndex = sharedBB.Get<int>(BallKeys::PassTarget).value_or(-1);
	}

	if (referenceIndex < 0 ||
		referenceIndex >= static_cast<int>(players.size()) ||
		!players[referenceIndex])
	{
		return;
	}

	for (int teamNumber = 0; teamNumber < 2; ++teamNumber)
	{
		BlackBoard& teamBB = teamNumber == 0 ? allyTeamBB : enemyTeamBB;

		DirectX::XMFLOAT3 referencePos = players[referenceIndex]->GetPosition();

		PressAssignmentState& state = m_pressStates[teamNumber];

		const int currentIndex = teamBB.Get<int>(PlayerKeys::PressPlayerIndex).value_or(-1);

		const bool isDefending = teamBB.Get<bool>(TeamKeys::IsDefending).value_or(false);

		if (!isDefending)
		{
			teamBB.Set(PlayerKeys::DefenceCoverIndex, -1);
			continue;
		}

		const int currentCoverIndex = teamBB.Get<int>(PlayerKeys::DefenceCoverIndex).value_or(-1);

		//一旦Coverがいたら後から変更しないで維持する
		const bool canKeepCover = currentCoverIndex >= 0
			&& players[currentCoverIndex]->GetTeamNumber() == teamNumber
			&& currentCoverIndex != currentIndex;
		if (canKeepCover)
		{
			continue;
		}

		//守備サポートを決める。
		//最も近いプレイヤーを探す。
		int bestIndex = -1;
		float nearestDistSq = FLT_MAX;
		for (int i = 0; i < static_cast<int>(players.size()); ++i)
		{
			SoccerPlayer* player = players[i].get();
			if (!player || player->GetTeamNumber() != teamNumber || i == currentIndex)
			{
				continue;
			}

			DirectX::XMFLOAT3 playerPos = player->GetPosition();

			float dx = referencePos.x - playerPos.x;
			float dz = referencePos.z - playerPos.z;

			float distSq = dx * dx + dz * dz;

			if (distSq < nearestDistSq)
			{
				nearestDistSq = distSq;
				bestIndex = i;
			}
		}

		teamBB.Set(PlayerKeys::DefenceCoverIndex, bestIndex);
	}
}

void Meta::FindAttackSupportPlayerIndex(
	const BlackBoard& sharedBB, 
	BlackBoard& allyTeamBB,
	BlackBoard& enemyTeamBB, 
	std::vector<std::unique_ptr<SoccerPlayer>>& players, 
	float elapsedTime)
{
	//三人にも対応した役振りを作る。
	int referenceIndex = sharedBB.Get<int>(BallKeys::OwnerIndex).value_or(-1);

	if (referenceIndex < 0)
	{
		referenceIndex = sharedBB.Get<int>(BallKeys::PassTarget).value_or(-1);
	}

	if (referenceIndex < 0 ||
		referenceIndex >= static_cast<int>(players.size()) ||
		!players[referenceIndex])
	{
		return;
	}

	//保持者のチーム番号
	const int referenceTeam = players[referenceIndex]->GetTeamNumber();

	if (referenceTeam < 0 || referenceTeam>2)
	{
		return;
	}

	//保持者以外の味方の番号
	std::vector<int> candidates;

	for (int i = 0; i < static_cast<int>(players.size()); ++i)
	{
		if (i == referenceIndex || !players[i])
		{
			continue;
		}

		if (players[i]->GetTeamNumber()!= referenceTeam)
		{
			continue;
		}

		//味方の番号を保存
		candidates.push_back(i);
	}

	BlackBoard& teamBB = referenceTeam == 0 ? allyTeamBB : enemyTeamBB;

	// 保持者以外に味方がいない
	if (candidates.empty())
	{
		teamBB.Set(PlayerKeys::AttackSupportPlayerIndex, -1);
		teamBB.Set(PlayerKeys::AttackWidePlayerIndex, -1);
		return;
	}

	// 全体で２人：残る１人をSupportにする
	if (candidates.size() == 1)
	{
		teamBB.Set(PlayerKeys::AttackSupportPlayerIndex,candidates[0]);
		teamBB.Set(PlayerKeys::AttackWidePlayerIndex, -1);
		return;
	}


	const int a = candidates[0];
	const int b = candidates[1];


	int supportIndex = a;
	int wideIndex = b;

	DirectX::XMFLOAT3 OwnerPos = players[referenceIndex]->GetPosition();

	DirectX::XMFLOAT3 supportPos = players[supportIndex]->GetPosition();
	DirectX::XMFLOAT3 widePos = players[wideIndex]->GetPosition();

	const auto GoalPos = teamBB.Get<DirectX::XMFLOAT3>(TeamKeys::GoalPosition);

	std::vector<std::vector<float>> costs(candidates.size(), std::vector<float>(2, 0.0f));

	m_calAIeva.CalcAttackCost(supportPos, widePos, OwnerPos, *GoalPos);





	

	if (referenceIndex < 0)
	{
		return;
	}

	for (int teamNumber = 0; teamNumber < 2; ++teamNumber)
	{
		BlackBoard& teamBB = teamNumber == 0 ? allyTeamBB : enemyTeamBB;

		DirectX::XMFLOAT3 OwnerPos = players[referenceIndex]->GetPosition();

		PressAssignmentState& state = m_pressStates[teamNumber];

		const int currentIndex = teamBB.Get<int>(PlayerKeys::PressPlayerIndex).value_or(-1);

		const bool isAttacking = teamBB.Get<bool>(TeamKeys::IsAttacking).value_or(false);

		if (!isAttacking)
		{
			teamBB.Set(PlayerKeys::AttackSupportPlayerIndex, -1);
			continue;
		}

		//攻撃サポートを決める。
		//最も近いプレイヤーを探す。
		int bestIndex = -1;
		float nearestDistSq = FLT_MAX;
		for (int i = 0; i < static_cast<int>(players.size()); ++i)
		{
			SoccerPlayer* player = players[i].get();
			if (!player || player->GetTeamNumber() != teamNumber || i == currentIndex)
			{
				continue;
			}

			if (i == referenceIndex)
			{
				continue;
			}

			DirectX::XMFLOAT3 playerPos = player->GetPosition();

			float dx = OwnerPos.x - playerPos.x;
			float dz = OwnerPos.z - playerPos.z;

			float distSq = dx * dx + dz * dz;

			if (distSq < nearestDistSq)
			{
				nearestDistSq = distSq;
				bestIndex = i;
			}
		}

		int supportIndex =
			teamBB.Get<int>(PlayerKeys::AttackSupportPlayerIndex).value_or(-1);

		int wideIndex =
			teamBB.Get<int>(PlayerKeys::AttackWidePlayerIndex).value_or(-1);

		// 保持者になった選手など、候補から外れた担当を選び直す
		if (std::find(candidates.begin(), candidates.end(), supportIndex)
			== candidates.end())
		{
			supportIndex = bestIndex;
		}

		if (wideIndex == supportIndex ||
			std::find(candidates.begin(), candidates.end(), wideIndex)
			== candidates.end())
		{
			wideIndex = -1;

			for (int index : candidates)
			{
				if (index != supportIndex)
				{
					wideIndex = index;
					break;
				}
			}
		}

		teamBB.Set(PlayerKeys::AttackSupportPlayerIndex, supportIndex);
		teamBB.Set(PlayerKeys::AttackWidePlayerIndex, wideIndex);
	}
}

//誰が誰をつくかを決める
//その後に担当によってつく相手にどう動くか決める。
void Meta::AssignMarks(
	const BlackBoard& sharedBB, 
	BlackBoard& allyTeamBB, 
	BlackBoard& enemyTeamBB, 
	std::vector<std::unique_ptr<SoccerPlayer>>& players, 
	float elapsedTime)
{
	m_markTargets.resize(players.size(), -1);

	std::vector<int> nextTargets(players.size(), -1);

	for (int team = 0; team < 2; ++team)
	{
		m_markPlayers[team].clear();
	}


	//今いる選手をチーム別に集める。
	for (int i = 0; i < static_cast<int>(players.size()); ++i)
	{
		if (!players[i])
		{
			continue;
		}

		const int  teamNumber = players[i]->GetTeamNumber();

		if (teamNumber < 0 || teamNumber >= 2)
		{
			continue;
		}

		m_markPlayers[teamNumber].push_back(i);
	}

	const int ownerIndex = sharedBB.Get<int>(BallKeys::OwnerIndex).value_or(-1);

	for (int team = 0; team < 2; ++team)
	{
		const auto& defenders = m_markPlayers[team];
		const auto& opponents = m_markPlayers[1 - team];

		if (defenders.empty() || defenders.size() != opponents.size())
		{
			continue;
		}


		const BlackBoard& teamBB = team == 0 ? allyTeamBB : enemyTeamBB;

		const auto goalPos = teamBB.Get<DirectX::XMFLOAT3>(TeamKeys::DefendGoalPosition);

		if (!goalPos)
		{
			continue;
		}

		const int  count = static_cast<int>(defenders.size());

		std::vector<float> threats(count);

		for (int j = 0; j < count; ++j)
		{
			const int opponentIndex = opponents[j];

			threats[j] = m_calAIeva.CalcMarkThreat(
				players[opponentIndex]->GetPosition(), *goalPos, opponentIndex == ownerIndex);
		}

		std::vector<std::vector<float>> costs(count, std::vector<float>(count));

		for (int i = 0; i < count; ++i)
		{
			const int defenderIndex = defenders[i];

			for (int j = 0; j < count; ++j)
			{
				const int opponentIndex = opponents[j];

				float cost = m_calAIeva.CalcMarkCost(
					players[defenderIndex]->GetPosition(),
					players[opponentIndex]->GetPosition(),
					*goalPos,
					threats[j]);

				const int currentTarget = m_markTargets[defenderIndex];

				if (currentTarget >= 0 && currentTarget != opponentIndex)
				{
					cost += m_markSwitchCost;
				}

				costs[i][j] = cost;
			}
		}


		std::vector<int> order(count);

		for (int i = 0; i < count; ++i)
		{
			order[i] = i;
		}

		std::vector<int> bestOrder = order;

		float bestCost = std::numeric_limits<float>::infinity();

		do
		{
			float totalCost = 0.0f;

			for (int i = 0; i < count; ++i)
			{
				totalCost += costs[i][order[i]];
			}

			if (totalCost < bestCost)
			{
				bestCost = totalCost;
				bestOrder = order;
			}
		} while (std::next_permutation(order.begin(), order.end()));
		//全パターン分作ってくれてる
		

		for (int i = 0; i < count; ++i)
		{
			nextTargets[defenders[i]] = opponents[bestOrder[i]];
		}
	}

	m_markTargets = std::move(nextTargets);
}

//味方プレイヤーの役割を決める。
void Meta::AssignAttack(
	const BlackBoard& sharedBB, 
	BlackBoard& allyTeamBB, 
	BlackBoard& enemyTeamBB, 
	std::vector<std::unique_ptr<SoccerPlayer>>& players, 
	float elapsedTime)
{
	//三人にも対応した役振りを作る。
	const int OwnerIndex = sharedBB.Get<int>(BallKeys::OwnerIndex).value_or(-1);

	if (OwnerIndex < 0 || !players[OwnerIndex])
	{
		return;
	}

	//保持者のチーム番号
	const int OwnerTeam = players[OwnerIndex]->GetTeamNumber();

	if (OwnerTeam < 0 || OwnerTeam>2)
	{
		return;
	}

	//保持者以外の味方の番号
	std::vector<int> candidates;

	for (int i = 0; i < static_cast<int>(players.size()); ++i)
	{
		if (i == OwnerIndex || !players[i])
		{
			continue;
		}

		if (players[i]->GetTeamNumber() != OwnerTeam)
		{
			continue;
		}

		//味方の番号を保存
		candidates.push_back(i);
	}

	const int a = candidates[0];
	const int b = candidates[1];


	int supportIndex = a;
	int wideIndex = b;

	DirectX::XMFLOAT3 OwnerPos = players[OwnerIndex]->GetPosition();


	BlackBoard& teamBB = OwnerTeam == 0 ? allyTeamBB : enemyTeamBB;
	const auto GoalPos = teamBB.Get<DirectX::XMFLOAT3>(TeamKeys::GoalPosition);


	std::vector<std::vector<float>> costs =
		m_calAIeva.CalcAttackCost(players[a]->GetPosition(),
			players[b]->GetPosition(),
			OwnerPos,
			*GoalPos);
	

	if (costs.empty())
	{
		return;
	}

	// 現在その役割を担当している選手
	const int currentPlayers[2] =
	{
		teamBB.Get<int>(
			PlayerKeys::AttackSupportPlayerIndex).value_or(-1),

		teamBB.Get<int>(
			PlayerKeys::AttackWidePlayerIndex).value_or(-1)
	};

	// １人の担当を変更する負担。
	//const float switchCost = 1.0f;

	//for (int i = 0; i < 2; ++i)
	//{
	//	for (int role = 0; role < 2; ++role)
	//	{
	//		if (currentPlayers[role] >= 0 &&
	//			currentPlayers[role] != candidates[i])
	//		{
	//			costs[i][role] += switchCost;
	//		}
	//	}
	//}

	const float costAB = costs[0][0] + costs[1][1];

	const float costBA = costs[1][0] + costs[0][1];


	if (costBA < costAB)
	{
		supportIndex = b;
		wideIndex = a;
	}

	teamBB.Set(PlayerKeys::AttackSupportPlayerIndex, supportIndex);
	teamBB.Set(PlayerKeys::AttackWidePlayerIndex, wideIndex);
}

void Meta::AssainFreeBallPlayerIndex(
	const BlackBoard& sharedBB, 
	BlackBoard& allyTeamBB, 
	BlackBoard& enemyTeamBB, 
	std::vector<std::unique_ptr<SoccerPlayer>>& players, 
	float elapsedTime)
{
	allyTeamBB.Set(PlayerKeys::FreeBallPlayerIndex, -1);
	enemyTeamBB.Set(PlayerKeys::FreeBallPlayerIndex, -1);

	const int ownerIndex =
		sharedBB.Get<int>(BallKeys::OwnerIndex).value_or(-1);

	const int passTarget =
		sharedBB.Get<int>(BallKeys::PassTarget).value_or(-1);

	if (ownerIndex >= 0 || passTarget >= 0)
	{
		return;
	}

	const auto ballPos =
		sharedBB.Get<DirectX::XMFLOAT3>(BallKeys::Position);

	if (!ballPos)
	{
		return;
	}

	const int controlledIndex =
		sharedBB.Get<int>(PlayerKeys::ControlledIndex).value_or(-1);

	for (int teamNumber = 0; teamNumber < 2; ++teamNumber)
	{
		int bestIndex = -1;
		float nearestDistSq = FLT_MAX;

		for (int i = 0; i < static_cast<int>(players.size()); ++i)
		{
			SoccerPlayer* player = players[i].get();

			if (!player ||
				player->GetTeamNumber() != teamNumber)
			{
				continue;
			}

			const auto playerPos = player->GetPosition();

			const float dx = ballPos->x - playerPos.x;
			const float dz = ballPos->z - playerPos.z;
			const float distSq = dx * dx + dz * dz;

			if (distSq < nearestDistSq)
			{
				nearestDistSq = distSq;
				bestIndex = i;
			}
		}

		BlackBoard& teamBB =
			teamNumber == 0 ? allyTeamBB : enemyTeamBB;

		teamBB.Set(PlayerKeys::FreeBallPlayerIndex, bestIndex);
	}

}

void Meta::DrawImGui(
	const BlackBoard& allyTeamBB,
	const BlackBoard& enemyTeamBB)
{
#ifdef USE_IMGUI
	if (!ImGui::CollapsingHeader(
		"Meta AI"))
	{
		return;
	}

	ImGui::PushID("MetaAI");

	ImGui::SliderFloat(
		"Switch confirmation (s)",
		&m_PressSwitchTimer,
		0.0f,
		5.0f,
		"%.2f");



	ImGui::TextDisabled("-1 = None");

	for (int teamNumber = 0; teamNumber < 2; ++teamNumber)
	{
		const char* teamLabel =
			teamNumber == 0 ? "Team 0" : "Team 1";

		if (!ImGui::TreeNode(
			teamLabel))
		{
			continue;
		}

		const BlackBoard& teamBB =
			teamNumber == 0 ? allyTeamBB : enemyTeamBB;

		const int ownerIndex =
			teamBB.Get<int>(BallKeys::OwnerIndex).value_or(-1);

		const int supportIndex =
			teamBB.Get<int>(PlayerKeys::AttackSupportPlayerIndex).value_or(-1);

		const int wideIndex =
			teamBB.Get<int>(PlayerKeys::AttackWidePlayerIndex).value_or(-1);

		ImGui::Text("Ball owner: %d", ownerIndex);
		ImGui::Text("Attack support: %d", supportIndex);
		ImGui::Text("Attack wide: %d", wideIndex);
		ImGui::Separator();

		const PressAssignmentState& state =
			m_pressStates[teamNumber];

		const bool isAttacking =
			teamBB.Get<bool>(TeamKeys::IsAttacking).value_or(false);

		const bool isDefending =
			teamBB.Get<bool>(TeamKeys::IsDefending).value_or(false);

		const int pressIndex =
			teamBB.Get<int>(PlayerKeys::PressPlayerIndex).value_or(-1);

		const int targetIndex =
			teamBB.Get<int>(PlayerKeys::PressTargetIndex).value_or(-1);

		const bool hasAssignment =
			pressIndex >= 0 && targetIndex >= 0;

		const bool isChecking =
			isDefending &&
			hasAssignment &&
			state.candidateIndex >= 0;

		// チームの状態
		const char* teamState = "Neutral";

		if (isDefending)
		{
			teamState = "Defending";
		}
		else if (isAttacking)
		{
			teamState = "Attacking";
		}

		// 現在の判断状態
		const char* decision = "Keeping assignment";

		if (!isDefending)
		{
			decision = "Not defending";
		}
		else if (!hasAssignment)
		{
			decision = "No assignment";
		}
		else if (isChecking)
		{
			decision = "Checking candidate";
		}

		ImGui::Text("State: %s", teamState);
		ImGui::Text("Press player: %d", pressIndex);
		ImGui::Text("Target player: %d", targetIndex);

		ImGui::Text(
			"Candidate: %d",
			isChecking ? state.candidateIndex : -1);

		ImGui::Text(
			"Confirmation: %.2f / %.2f s",
			isChecking ? state.timer : 0.0f,
			m_PressSwitchTimer);

		// 候補の確認時間をバーで表示
		float progress = 0.0f;

		if (isChecking)
		{
			progress = m_PressSwitchTimer > 0.0f? std::clamp(state.timer / m_PressSwitchTimer, 0.0f, 1.0f): 1.0f;
		}

		ImGui::ProgressBar(progress, ImVec2(-1.0f, 0.0f));
		ImGui::Text("Decision: %s", decision);

		ImGui::TreePop();
	}

	if (ImGui::TreeNode("Mark assignments"))
	{
		for (int team = 0; team < 2; ++team)
		{
			ImGui::Text("Team %d", team);

			for (int playerIndex : m_markPlayers[team])
			{
				const int targetIndex = m_markTargets[playerIndex];

				ImGui::Text(
					"Player %d -> Player %d",
					playerIndex,
					targetIndex);
			}

			ImGui::Separator();
		}

		ImGui::TreePop();
	}

	ImGui::PopID();
#endif
}

void Meta::DrawDebug(
	PrimitiveRenderer& renderer,
	const std::vector<std::unique_ptr<SoccerPlayer>>& players,
	const BlackBoard& allyTeamBB,
	const BlackBoard& enemyTeamBB) const
{
	// ２頂点を追加すると、１本の線になる
	auto DrawLine = [&renderer](
		const DirectX::XMFLOAT3& from,
		const DirectX::XMFLOAT3& to,
		const DirectX::XMFLOAT4& color)
		{
			renderer.AddVertex(from, color);
			renderer.AddVertex(to, color);
		};

	// 選手の位置を中心に、水平な円を描く
	auto DrawCircle = [&DrawLine](
		const DirectX::XMFLOAT3& center,
		const DirectX::XMFLOAT4& color)
		{
			constexpr int segments = 24;
			constexpr float radius = 0.25f;

			for (int s = 0; s < segments; ++s)
			{
				const float angle1 =
					DirectX::XM_2PI * s / segments;

				const float angle2 =
					DirectX::XM_2PI * (s + 1) / segments;

				DrawLine(
					{
						center.x + std::cos(angle1) * radius,
						center.y + 0.03f,
						center.z + std::sin(angle1) * radius
					},
				{
					center.x + std::cos(angle2) * radius,
					center.y + 0.03f,
					center.z + std::sin(angle2) * radius
				},
					color);
			}
		};

	const int playerCount = static_cast<int>(players.size());

	for (int team = 0; team < 2; ++team)
	{
		const BlackBoard& teamBB =
			team == 0 ? allyTeamBB : enemyTeamBB;

		const bool attacking =
			teamBB.Get<bool>(TeamKeys::IsAttacking).value_or(false);

		const bool defending =
			teamBB.Get<bool>(TeamKeys::IsDefending).value_or(false);

		const int owner =
			teamBB.Get<int>(BallKeys::OwnerIndex).value_or(-1);

		const int support =
			teamBB.Get<int>(
				PlayerKeys::AttackSupportPlayerIndex).value_or(-1);

		const int wide =
			teamBB.Get<int>(
				PlayerKeys::AttackWidePlayerIndex).value_or(-1);

		const int press =
			teamBB.Get<int>(
				PlayerKeys::PressPlayerIndex).value_or(-1);

		const int cover =
			teamBB.Get<int>(
				PlayerKeys::DefenceCoverIndex).value_or(-1);

		for (int i = 0; i < playerCount; ++i)
		{
			if (!players[i] ||
				players[i]->GetTeamNumber() != team)
			{
				continue;
			}

			// 個別の役割が付いていない選手は灰色
			DirectX::XMFLOAT4 color{ 0.7f, 0.7f, 0.7f, 1.0f };

			if (attacking)
			{
				if (i == owner)
				{
					color = { 1.0f, 0.9f, 0.1f, 1.0f };
				}
				else if (i == support)
				{
					color = { 0.2f, 1.0f, 0.3f, 1.0f };
				}
				else if (i == wide)
				{
					color = { 0.2f, 0.5f, 1.0f, 1.0f };
				}
			}
			else if (defending)
			{
				if (i == press)
				{
					color = { 1.0f, 0.2f, 0.2f, 1.0f };
				}
				else if (i == cover)
				{
					color = { 0.1f, 1.0f, 1.0f, 1.0f };
				}
			}

			auto position = players[i]->GetPosition();
			DrawCircle(position, color);

			// 守備中のマーク関係を表示する
			if (!defending ||
				i >= static_cast<int>(m_markTargets.size()))
			{
				continue;
			}

			const int targetIndex = m_markTargets[i];

			if (targetIndex < 0 ||
				targetIndex >= playerCount ||
				!players[targetIndex] ||
				players[targetIndex]->GetTeamNumber() == team)
			{
				continue;
			}

			auto targetPos = players[targetIndex]->GetPosition();

			position.y += 0.08f;
			targetPos.y += 0.08f;

			DrawLine(
				position,
				targetPos,
				{ 1.0f, 1.0f, 1.0f, 1.0f });
		}
	}
}


