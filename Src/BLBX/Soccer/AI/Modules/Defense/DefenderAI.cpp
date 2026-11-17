#include "DefenderAI.h"

#include <algorithm>
#include <cmath>

void DefenderAI::Initialize()
{
	m_MarkDistance = 1.0f;
	m_PressDistance = 0.5f;
	m_PressAngle = 120.0f;
	m_MoveSpeed = 1.2f;
	m_TargetUpdateInterval = 0.1f;
}

void DefenderAI::SubmitRequests(
	int TeamNumber, 
	const BlackBoard& blackboard, 
	std::vector<std::unique_ptr<SoccerPlayer>>& players,
	const std::vector<int>& markTargets,
	AIMoveResolver& resolver, 
	float elapsedTime)
{

	if (TeamNumber < 0 || TeamNumber >= 2)
	{
		return;
	}

	EvaluationDebug& debug = m_evaluationDebug[TeamNumber];
	debug = EvaluationDebug{};

	const int playerCount = static_cast<int>(players.size());

	/*const int OwnerIndex = blackboard.Get<int>(BallKeys::OwnerIndex).value_or(-1);*/

	int referenceIndex = blackboard.Get<int>(BallKeys::OwnerIndex).value_or(-1);

	if (referenceIndex < 0)
	{
		referenceIndex = blackboard.Get<int>(BallKeys::PassTarget).value_or(-1);
	}


	const int OwnerTeam = blackboard.Get<int>(BallKeys::OwnerTeam).value_or(-1);
	const bool hasOwner = referenceIndex >= 0 && referenceIndex < static_cast<int>(players.size()) && players[referenceIndex] != nullptr;

	//守備する状況が終わったら、このチームの状態を解除
	if (!hasOwner || TeamNumber == OwnerTeam)
	{
		for (int i = 0; i < static_cast<int>(players.size()); ++i)
		{
			if (players[i] &&
				players[i]->GetTeamNumber() == TeamNumber)
			{
				m_states.erase(i);
			}
		}

		return;
	}



	const auto GoalPos = blackboard.Get<DirectX::XMFLOAT3>(TeamKeys::DefendGoalPosition);

	if (!GoalPos)
	{
		return;
	}

	//このチームの守備選手全員を調べる
	for (int playerIndex = 0; playerIndex < playerCount; ++playerIndex)
	{
		if (!players[playerIndex] || players[playerIndex]->GetTeamNumber() != TeamNumber)
		{
			continue;
		}

		if (playerIndex >= static_cast<int>(markTargets.size()))
		{
			m_states.erase(playerIndex);
			continue;
		}

		const int targetIndex = markTargets[playerIndex];

		//有効な相手選手を担当しているか確認
		if (targetIndex < 0 ||
			targetIndex >= playerCount ||
			!players[targetIndex] ||
			players[targetIndex]->GetTeamNumber() == TeamNumber)
		{
			m_states.erase(playerIndex);
			continue;
		}

		const bool targetHasBall = targetIndex == referenceIndex;

		DefenderState& state = m_states[playerIndex];

		// 担当や保持状態が変わったら、キャッシュを解除
		if (state.targetIndex != targetIndex ||
			state.targetHasBall != targetHasBall)
		{
			state = DefenderState{};
		}

		state.timer += elapsedTime;

		if (!state.hasTarget ||
			state.timer >= m_TargetUpdateInterval)
		{
			// 自分がマークする相手の位置
			const auto targetPos =
				players[targetIndex]->GetPosition();

			// マーク相手から、自分たちのゴールへの方向
			DirectX::XMFLOAT3 targetDirection =
			{
				GoalPos->x - targetPos.x,
				0.0f,
				GoalPos->z - targetPos.z
			};

			const float goalLength = std::sqrt(
				targetDirection.x * targetDirection.x +
				targetDirection.z * targetDirection.z);

			if (goalLength <= 0.00001f)
			{
				m_states.erase(playerIndex);
				continue;
			}

			targetDirection.x /= goalLength;
			targetDirection.z /= goalLength;

			// 全員共通の基本間合い
			float markDistance = m_MarkDistance;

			// 保持者を担当している場合だけ、向きで間合いを調整
			if (targetHasBall)
			{
				auto forward = players[targetIndex]->GetForward();

				const float forwardLength = std::sqrt(
					forward.x * forward.x +
					forward.z * forward.z);

				if (forwardLength > 0.00001f)
				{
					forward.x /= forwardLength;
					forward.z /= forwardLength;

					const float dot = std::clamp(
						forward.x * targetDirection.x +
						forward.z * targetDirection.z,
						-1.0f,
						1.0f);

					const float threshold = std::cos(
						DirectX::XMConvertToRadians(m_PressAngle));

					if (dot < threshold)
					{
						markDistance = m_PressDistance;
					}
				}
			}

			// 自分の相手のゴール側へ目標を作る
			state.targetPosition =
			{
				targetPos.x + targetDirection.x * markDistance,
				players[playerIndex]->GetPosition().y,
				targetPos.z + targetDirection.z * markDistance
			};

			state.targetIndex = targetIndex;
			state.targetHasBall = targetHasBall;
			state.timer = 0.0f;
			state.hasTarget = true;
		}

		float moveSpeed = m_MoveSpeed;
		Evaluation evaluation{};

		// 既存の保持者用評価は、保持者担当に適用
		if (targetHasBall)
		{
			evaluation = m_calAIeva.Calculation(
				blackboard,
				players,
				playerIndex,
				elapsedTime);

			moveSpeed =
				m_MoveSpeed * (0.7f + 0.3f * evaluation.urgency);

			evaluation.Movespeed = moveSpeed;

			debug.active = true;
			debug.playerIndex = playerIndex;
			debug.evaluation = evaluation;
		}

		AIMoveRequest request{};
		request.active = true;

		request.type = AIMoveRequestType::Base;
		request.reason = AIMoveReason::Mark;
		request.mode = AIMoveMode::Arrive;

		request.priority = 30;
		request.utility = evaluation.utility;
		request.urgency = evaluation.urgency;

		request.targetPosition = state.targetPosition;
		request.moveSpeed = moveSpeed;
		request.stopDistance = 0.05f;

		resolver.Submit(playerIndex, request);
	}
}

void DefenderAI::DrawImGui()
{
#ifdef USE_IMGUI
	if (ImGui::CollapsingHeader(
		"Defender AI"))
	{
		ImGui::SliderFloat(
			"Mark distance", &m_MarkDistance, 0.1f, 3.0f, "%.2f");

		ImGui::SliderFloat(
			"Press distance", &m_PressDistance, 0.05f, 2.0f, "%.2f");

		ImGui::SliderFloat(
			"Press angle (deg)", &m_PressAngle, 0.0f, 180.0f, "%.0f");

		ImGui::SliderFloat(
			"Move speed", &m_MoveSpeed, 0.0f, 3.0f, "%.2f");

		ImGui::SliderFloat(
			"Target update (s)",
			&m_TargetUpdateInterval, 0.0f, 0.5f, "%.2f");


		ImGui::Separator();


		m_calAIeva.DrawImGui();

		if (ImGui::TreeNode("Evaluation"))
		{
			// ラベルと棒グラフを横に並べる
			auto DrawScore = [](const char* label, float value)
				{
					ImGui::TableNextRow();

					ImGui::TableSetColumnIndex(0);
					ImGui::TextUnformatted(label);

					ImGui::TableSetColumnIndex(1);

					char text[32];
					std::snprintf(text, sizeof(text), "%.2f", value);

					ImGui::ProgressBar(
						std::clamp(value, 0.0f, 1.0f),
						ImVec2(-1.0f, 0.0f),
						text);
				};

			for (int team = 0; team < 2; ++team)
			{
				ImGui::PushID(team);

				const EvaluationDebug& debug = m_evaluationDebug[team];

				ImGui::Text("Team %d", team);

				if (!debug.active)
				{
					ImGui::TextDisabled("No mark request");
					ImGui::PopID();
					continue;
				}

				const Evaluation& e = debug.evaluation;

				ImGui::Text("Player index: %d", debug.playerIndex);
				ImGui::Text("Distance to owner: %.2f", e.distance);
				ImGui::Text("Distance to goal: %.2f", e.goalDistance);

				ImGui::TextColored(
					e.insideDangerArea
					? ImVec4(1.0f, 0.4f, 0.2f, 1.0f)
					: ImVec4(0.6f, 0.6f, 0.6f, 1.0f),
					"Danger area: %s",
					e.insideDangerArea ? "INSIDE" : "OUTSIDE");

				if (ImGui::BeginTable(
					"Scores",
					2,
					ImGuiTableFlags_BordersInnerV |
					ImGuiTableFlags_RowBg |
					ImGuiTableFlags_SizingStretchProp))
				{
					ImGui::TableSetupColumn(
						"Name", ImGuiTableColumnFlags_WidthFixed, 150.0f);
					ImGui::TableSetupColumn("Value");

					DrawScore("Near owner", e.nearScore);
					DrawScore("Near goal", e.nearGoal);
					DrawScore("Area danger", e.areaDanger);
					DrawScore("Final danger", e.danger);

					DrawScore("Request utility", e.utility);
					DrawScore("Request urgency", e.urgency);

					ImGui::EndTable();
				}

				ImGui::Text("Requested speed: %.2f", e.Movespeed);
				ImGui::Spacing();

				ImGui::PopID();
			}

			ImGui::TreePop();
		}


	}
#endif
}
