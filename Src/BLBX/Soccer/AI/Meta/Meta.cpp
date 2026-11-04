#include "Meta.h"

#include "Soccer/AI/Core/BlackBoard.h"
#include "Soccer/AI/Keys/BlackBoardKeys.h"

#include "Soccer/Player/SoccerPlayer.h"

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

	allyTeamBB.Set(TeamKeys::IsAttacking, owmerTeam == 0);
	allyTeamBB.Set(TeamKeys::IsDefending, owmerTeam == 1);

	enemyTeamBB.Set(TeamKeys::IsAttacking, owmerTeam == 1);
	enemyTeamBB.Set(TeamKeys::IsDefending, owmerTeam == 0);

	//プレスプレイヤーを決める
	FindPressPlayerIndex(sharedBB, allyTeamBB, enemyTeamBB, players, elapsedTime);

	FindDefenderSupportPlayerIndex(sharedBB, allyTeamBB, enemyTeamBB, players, elapsedTime);
}

void Meta::FindPressPlayerIndex(
	const BlackBoard& sharedBB, 
	BlackBoard& allyTeamBB, 
	BlackBoard& enemyTeamBB, 
	std::vector<std::unique_ptr<SoccerPlayer>>& players,
	float elapsedTime)
{
	const int OwnerIndex = sharedBB.Get<int>(BallKeys::OwnerIndex).value_or(-1);

	if (OwnerIndex < 0 ||OwnerIndex >= static_cast<int>(players.size()) ||!players[OwnerIndex])
	{
		allyTeamBB.Set(PlayerKeys::PressPlayerIndex, -1);
		allyTeamBB.Set(PlayerKeys::PressTargetIndex, -1);

		enemyTeamBB.Set(PlayerKeys::PressPlayerIndex, -1);
		enemyTeamBB.Set(PlayerKeys::PressTargetIndex, -1);

		return;
	}

	DirectX::XMFLOAT3 OwnerPos = players[OwnerIndex]->GetPosition();

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

			float dx = OwnerPos.x - playerPos.x;
			float dz = OwnerPos.z - playerPos.z;

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
		teamBB.Set(PlayerKeys::PressTargetIndex, pressIndex >= 0 ? OwnerIndex : -1);
	}
}


void Meta::FindDefenderSupportPlayerIndex(
	const BlackBoard& sharedBB, 
	BlackBoard& allyTeamBB, 
	BlackBoard& enemyTeamBB, 
	std::vector<std::unique_ptr<SoccerPlayer>>& players, 
	float elapsedTime)
{
	int OwnerIndex = sharedBB.Get<int>(BallKeys::OwnerIndex).value_or(-1);
	
	if (OwnerIndex < 0)
	{
		return;
	}

	for (int teamNumber = 0; teamNumber < 2; ++teamNumber)
	{
		BlackBoard& teamBB = teamNumber == 0 ? allyTeamBB : enemyTeamBB;

		DirectX::XMFLOAT3 OwnerPos = players[OwnerIndex]->GetPosition();

		PressAssignmentState& state = m_pressStates[teamNumber];

		const int currentIndex = teamBB.Get<int>(PlayerKeys::PressPlayerIndex).value_or(-1);

		const bool isDefending = teamBB.Get<bool>(TeamKeys::IsDefending).value_or(false);

		if (!isDefending)
		{
			teamBB.Set(PlayerKeys::DefenceCoverIndex, -1);
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

			float dx = OwnerPos.x - playerPos.x;
			float dz = OwnerPos.z - playerPos.z;

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
	int OwnerIndex = sharedBB.Get<int>(BallKeys::OwnerIndex).value_or(-1);

	if (OwnerIndex < 0)
	{
		return;
	}

	for (int teamNumber = 0; teamNumber < 2; ++teamNumber)
	{
		BlackBoard& teamBB = teamNumber == 0 ? allyTeamBB : enemyTeamBB;

		DirectX::XMFLOAT3 OwnerPos = players[OwnerIndex]->GetPosition();

		PressAssignmentState& state = m_pressStates[teamNumber];

		const int currentIndex = teamBB.Get<int>(PlayerKeys::PressPlayerIndex).value_or(-1);

		const bool isAttacking = teamBB.Get<bool>(TeamKeys::IsAttacking).value_or(false);

		if (!isAttacking)
		{
			return;
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

		teamBB.Set(PlayerKeys::AttackSupportPlayerIndex, bestIndex);
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

		if (!ImGui::TreeNodeEx(
			teamLabel, ImGuiTreeNodeFlags_DefaultOpen))
		{
			continue;
		}

		const BlackBoard& teamBB =
			teamNumber == 0 ? allyTeamBB : enemyTeamBB;

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

	ImGui::PopID();
#endif
}


