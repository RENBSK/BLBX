#include "Meta.h"

#include "Soccer/AI/Core/BlackBoard.h"
#include "Soccer/AI/Keys/BlackBoardKeys.h"

#include "Soccer/Player/SoccerPlayer.h"

#include "Renderer/PrimitiveRenderer.h"

#include <cmath>
#include <algorithm>
#include <limits>

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
	// 保持者がいなければ、パスを出したチームを攻撃側にする
	const int owmerTeam = sharedBB.Get<int>(BallKeys::OwnerTeam).value_or(-1);
	int attackTeam = sharedBB.Get<int>(BallKeys::OwnerTeam).value_or(-1);
	if (attackTeam < 0)
	{
		attackTeam = sharedBB.Get<int>(BallKeys::PassTeam).value_or(-1);
	}

	// 両チームの攻守を更新
	allyTeamBB.Set(TeamKeys::IsAttacking, attackTeam == 0);
	allyTeamBB.Set(TeamKeys::IsDefending, attackTeam == 1);

	enemyTeamBB.Set(TeamKeys::IsAttacking, attackTeam == 1);
	enemyTeamBB.Set(TeamKeys::IsDefending, attackTeam == 0);

	//守備の担当
	// 両チームのPress担当を決める
	for (int team = 0; team < 2; ++team)
	{
		BlackBoard& teamBB = team == 0 ? allyTeamBB : enemyTeamBB;

		//Pressを先に決め、Coverと担当が重ならないようにする。
		AssignPress(team, teamBB, players, elapsedTime);
		AssignCover(team, teamBB, players);
		//攻撃の担当
		AssignAttack(team, teamBB, players);
		//マークする相手の割り当て
		AssignMarks(team, teamBB, players);
		//フリーボールを追う担当
		AssignChase(team, teamBB, players);
	}
	
}

void Meta::AssignPress(
	int teamNumber,
	BlackBoard& teamBB,
	std::vector < std::unique_ptr<SoccerPlayer>>& players,
	float elapsedTime)
{
	//守備の基準にする相手を取得(保持者、パス中なら受け手)
	int referenceIndex = GetReference(teamBB, players);
	//有効な相手がいなければ、今回の担当決めは行わない。
	if (referenceIndex < 0)
	{
		return;
	}
	//対象に一番近い、このチームの選手を探す
	const auto referencePos = players[referenceIndex]->GetPosition();
	//対象に一番近い、このチームの選手をPress候補にする
	const auto bestIndex = FindNearest(players, teamNumber, referencePos);
	//このチームの交代候補を確認タイマーを参照で取得
	auto& state = m_pressStates[teamNumber];
	//現在のPress担当を取得。担当なしなら-1
	const int currentIndex = teamBB.Get<int>(PlayerKeys::PressPlayerIndex).value_or(-1);
	//このチームが今守備中か確認
	const bool isDefending = teamBB.Get<bool>(TeamKeys::IsDefending).value_or(false);

	int pressIndex = -1;

	if (!isDefending || bestIndex < 0)
	{
		//守備しない、または候補がいないので解除
		state = PressAssignmentState{};
	}
	else if (currentIndex < 0 || currentIndex == bestIndex)
	{
		//担当なしなら決定。同じ担当なら維持
		pressIndex = bestIndex;
		state = PressAssignmentState{};
	}
	else
	{
		//確認中は今の担当を維持
		pressIndex = currentIndex;

		// 候補が変わったら計り直す
		if (state.candidateIndex != bestIndex)
		{
			state.candidateIndex = bestIndex;
			state.timer = 0.0f;
		}

		state.timer += elapsedTime;

		//同じ候補が一定時間続いたら交代
		//簡単には切り替えないように
		if (state.timer >= m_PressSwitchTimer)
		{
			pressIndex = bestIndex;
			state = PressAssignmentState{};
		}
	}

	// 担当とマーク対象を登録
	teamBB.Set(PlayerKeys::PressPlayerIndex, pressIndex);
	teamBB.Set(PlayerKeys::PressTargetIndex, pressIndex >= 0 ? referenceIndex : -1);
}


void Meta::AssignCover(
	int teamNumber,
	BlackBoard& teamBB,
	std::vector < std::unique_ptr<SoccerPlayer>>& players)
{
	//守備の基準にする相手を取得(保持者、パス中なら受け手)
	int referenceIndex = GetReference(teamBB, players);
	//有効な相手がいなければ、今回の担当決めは行わない。
	if (referenceIndex < 0)
	{
		return;
	}
	//守備中でなければ、Cover担当を解除
	if (!teamBB.Get<bool>(TeamKeys::IsDefending).value_or(false))
	{
		teamBB.Set(PlayerKeys::DefenceCoverIndex, -1);
		return;
	}

	const int pressIndex = teamBB.Get<int>(PlayerKeys::PressPlayerIndex).value_or(-1);
	const int coverIndex = teamBB.Get<int>(PlayerKeys::DefenceCoverIndex).value_or(-1);

	//一旦Coverがいたら後から変更しないで維持する
	const bool canKeepCover = coverIndex >= 0
		&& players[coverIndex]->GetTeamNumber() == teamNumber
		&& coverIndex != pressIndex;

	if (canKeepCover)
	{
		return;
	}

	//守備サポートを決める。
	//最も近いプレイヤーを探す。
	const auto referncePos = players[referenceIndex]->GetPosition();
	const int bestIndex = FindNearest(players, teamNumber, referncePos, pressIndex);

	teamBB.Set(PlayerKeys::DefenceCoverIndex, bestIndex);
}

void Meta::AssignAttack(
	int teamNumber,
	BlackBoard& teamBB,
	const std::vector<std::unique_ptr<SoccerPlayer>>& players)
{
	//攻撃中でなければ攻撃担当を解除
	if (!teamBB.Get<bool>(TeamKeys::IsAttacking).value_or(false))
	{
		teamBB.Set(PlayerKeys::AttackSupportPlayerIndex, -1);

		teamBB.Set(PlayerKeys::AttackWidePlayerIndex, -1);

		return;
	}

	//保持者、またはパスの受け手を取得
	const int referenceIndex = GetReference(teamBB, players);

	if (referenceIndex < 0)
	{
		return;
	}

	// 保持者・受け手以外の、有効な味方か確認する
	const auto isCandidate = [&](int index)
		{
			return index >= 0 &&
				index < static_cast<int>(players.size()) &&
				players[index] &&
				players[index]->GetTeamNumber() == teamNumber &&
				index != referenceIndex;
		};

	// 現在の担当を取得
	int supportIndex = teamBB.Get<int>(PlayerKeys::AttackSupportPlayerIndex).value_or(-1);
	int wideIndex = teamBB.Get<int>(PlayerKeys::AttackWidePlayerIndex).value_or(-1);

	// Supportが候補から外れたら、基準選手に近い味方を選ぶ
	if (!isCandidate(supportIndex))
	{
		const auto referencePos = players[referenceIndex]->GetPosition();

		supportIndex = FindNearest(players, teamNumber, referencePos, referenceIndex);
	}

	// Wideが無効、またはSupportと重複したら選び直す
	if (!isCandidate(wideIndex) || wideIndex == supportIndex)
	{
		wideIndex = -1;

		for (int i = 0; i < static_cast<int>(players.size()); ++i)
		{
			if (i != supportIndex && isCandidate(i))
			{
				wideIndex = i;
				break;
			}
		}
	}

	// 候補がいない役割は-1のまま登録
	teamBB.Set(PlayerKeys::AttackSupportPlayerIndex, supportIndex);
	teamBB.Set(PlayerKeys::AttackWidePlayerIndex, wideIndex);
}

//誰が誰をつくかを決める
//その後に担当によってつく相手にどう動くか決める。
void Meta::AssignMarks(
	int teamNumber,
	const BlackBoard& teamBB,
	const std::vector<std::unique_ptr<SoccerPlayer>>& players)
{
	//前回の担当は、交代コストの計算に使うので残す
	m_markTargets.resize(players.size(), -1);

	auto& defenders = m_markPlayers[teamNumber];
	defenders.clear();

	std::vector<int> opponents;

	for (int i = 0; i < static_cast<int>(players.size()); ++i)
	{
		if (!players[i])
		{
			m_markTargets[i] = -1;
			continue;
		}

		const int team = players[i]->GetTeamNumber();

		if (team == teamNumber)
		{
			defenders.push_back(i);
		}
		else if (team == 1 - teamNumber)
		{
			opponents.push_back(i);
		}
	}

	const auto goalPos = teamBB.Get<DirectX::XMFLOAT3>(TeamKeys::GoalPosition);

	// 今の組み合わせ計算は、両チームが同じ人数の場合に対応
	if (!goalPos ||
		defenders.empty() ||
		defenders.size() != opponents.size())
	{
		for (int index : defenders)
		{
			m_markTargets[index] = -1;
		}

		return;
	}

	const int ownerIndex =
		teamBB.Get<int>(BallKeys::OwnerIndex).value_or(-1);

	const int count = static_cast<int>(defenders.size());

	// 相手ごとの危険度を計算
	std::vector<float> threats(count);

	for (int j = 0; j < count; ++j)
	{
		const int opponentIndex = opponents[j];

		threats[j] = m_calAIeva.CalcMarkThreat(
			players[opponentIndex]->GetPosition(),
			*goalPos,
			opponentIndex == ownerIndex);
	}

	// 各選手が各相手を担当する場合のコスト表
	std::vector<std::vector<float>> costs(
		count, std::vector<float>(count));

	for (int i = 0; i < count; ++i)
	{
		const int defenderIndex = defenders[i];
		const auto defenderPos =
			players[defenderIndex]->GetPosition();

		for (int j = 0; j < count; ++j)
		{
			const int opponentIndex = opponents[j];

			float cost = m_calAIeva.CalcMarkCost(
				defenderPos,
				players[opponentIndex]->GetPosition(),
				*goalPos,
				threats[j]);

			// 担当変更には負担を加え、頻繁な交代を抑える
			const int currentTarget =
				m_markTargets[defenderIndex];

			if (currentTarget >= 0 &&
				currentTarget != opponentIndex)
			{
				cost += m_markSwitchCost;
			}

			costs[i][j] = cost;
		}
	}

	// 最も合計コストが小さい組み合わせを登録
	const auto bestOrder = FindOrder(costs);

	for (int i = 0; i < count; ++i)
	{
		m_markTargets[defenders[i]] =
			opponents[bestOrder[i]];
	}

}

void Meta::AssignChase(
	int teamNumber,
	BlackBoard& teamBB,
	const std::vector<std::unique_ptr<SoccerPlayer>>& players)
{
	// 今回の担当を決める前に、前回の担当を解除
	teamBB.Set(PlayerKeys::FreeBallPlayerIndex, -1);

	// 保持中・パス中ならフリーボールとして扱わない
	const int ownerIndex =teamBB.Get<int>(BallKeys::OwnerIndex).value_or(-1);

	const int passTarget =
		teamBB.Get<int>(BallKeys::PassTarget).value_or(-1);

	if (ownerIndex >= 0 || passTarget >= 0)
	{
		return;
	}

	// 共有BBから現在のボール位置を取得
	const auto ballPos =
		teamBB.Get<DirectX::XMFLOAT3>(BallKeys::Position);

	if (!ballPos)
	{
		return;
	}

	// ボールに一番近い、このチームの選手を担当にする
	const int bestIndex =
		FindNearest(players, teamNumber, *ballPos);

	// 候補がいなければ-1を登録
	teamBB.Set(PlayerKeys::FreeBallPlayerIndex, bestIndex);

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

int Meta::GetReference(
	const BlackBoard& blackboard, 
	const std::vector<std::unique_ptr<SoccerPlayer>>& players) const
{
	//まずは保持者を取得
	int index = blackboard.Get<int>(BallKeys::OwnerIndex).value_or(-1);

	//保持者がいなければ、パスの受け手を使う
	if (index < 0)
	{
		index = blackboard.Get<int>(BallKeys::PassTarget).value_or(-1);
	}

	//有効な選手か確認
	if (index < 0 || index >= static_cast<int>(players.size()) || !players[index])
	{
		return -1;
	}

	return index;
}

int Meta::FindNearest(
	const std::vector<std::unique_ptr<SoccerPlayer>>& players, 
	int TeamNumber, 
	const DirectX::XMFLOAT3& targetPosition, 
	int excludeIndex, 
	int secondExcludeIndex) const
{
	int bestIndex = -1;

	float nearestDistanceSq = std::numeric_limits<float>::infinity();

	for (int i = 0; i < static_cast<int>(players.size()); ++i)
	{
		SoccerPlayer* player = players[i].get();

		//選手なし、別チーム、除外対象なら飛ばす
		if (!player || player->GetTeamNumber() != TeamNumber || i == excludeIndex || i == secondExcludeIndex)
		{
			continue;
		}

		const auto position = player->GetPosition();

		const float dx = targetPosition.x - position.x;
		const float dz = targetPosition.z - position.z;

		const float distanceSq = dx * dx + dz * dz;

		if (distanceSq < nearestDistanceSq)
		{
			nearestDistanceSq = distanceSq;
			bestIndex = i;
		}
	}
		return bestIndex;
}

std::vector<int> Meta::FindOrder(const std::vector<std::vector<float>>& costs) const
{
	const int count = static_cast<int>(costs.size());

	if (count == 0)
	{
		return{};
	}

	//最初の組み合わせを0・1・２の順にする
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

	return bestOrder;
}


