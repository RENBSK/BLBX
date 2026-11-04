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
	AIMoveResolver& resolver, 
	float elapsedTime)
{

	if (TeamNumber < 0 || TeamNumber >= 2)
	{
		return;
	}

	EvaluationDebug& debug = m_evaluationDebug[TeamNumber];
	debug = EvaluationDebug{};

	const int OwnerIndex = blackboard.Get<int>(BallKeys::OwnerIndex).value_or(-1);
	const int OwnerTeam = blackboard.Get<int>(BallKeys::OwnerTeam).value_or(-1);
	const bool hasOwner = OwnerIndex >= 0 && OwnerIndex < static_cast<int>(players.size()) && players[OwnerIndex] != nullptr;

	//保持者がいない、または自分のチームが保持している
	if (!hasOwner || TeamNumber == OwnerTeam)
	{
		for (int i = 0; i < static_cast<int>(players.size()); ++i)
		{
			if (!players[i])
			{
				continue;
			}

			if (players[i]->GetTeamNumber() == TeamNumber)
			{
				//データを削除
				m_states.erase(i);
			}
		}

		return;
	}

	int bestIndex = blackboard.Get<int>(PlayerKeys::PressPlayerIndex).value_or(-1);


	//良いプレイヤーがいなければ
	if (bestIndex < 0)
	{
		return;
	}


	//反応速度を無理やり付ける
	DefenderState& state = m_states[bestIndex];
	state.timer += elapsedTime;

	if (!state.hasTarget || state.timer >= m_TargetUpdateInterval)
	{
		const DirectX::XMFLOAT3 GoalPos =
			blackboard.Get<DirectX::XMFLOAT3>(TeamKeys::DefendGoalPosition).value_or(DirectX::XMFLOAT3{});

		DirectX::XMFLOAT3 OwnerPos = players[OwnerIndex]->GetPosition();

		//ボール保持者から守るゴール方向への方向
		DirectX::XMFLOAT3 targetDirection =
		{
			GoalPos.x - OwnerPos.x,
			0.0f,
			GoalPos.z - OwnerPos.z
		};

		

		//ボール保持者が後ろを向いているなら少し詰める。
		float goalLength = std::sqrt(targetDirection.x * targetDirection.x + targetDirection.z * targetDirection.z);
		targetDirection.x /= goalLength;
		targetDirection.z /= goalLength;

		DirectX::XMFLOAT3 forward = players[OwnerIndex]->GetForward();
		float forwardLength = std::sqrt(forward.x * forward.x + forward.z * forward.z);
		forward.x /= forwardLength;
		forward.z /= forwardLength;

		float dot = forward.x * targetDirection.x + forward.z * targetDirection.z;

		dot = std::clamp(dot, -1.0f, 1.0f);

		const float threshold = std::cos(DirectX::XMConvertToRadians(m_PressAngle));

		const float markDistance = dot < threshold ? m_PressDistance : m_MarkDistance;

		const DirectX::XMVECTOR targetVec = DirectX::XMVector3Normalize(DirectX::XMLoadFloat3(&targetDirection));
		DirectX::XMFLOAT3 markOffset;
		DirectX::XMStoreFloat3(&markOffset, DirectX::XMVectorScale(targetVec, markDistance));

		state.targetPosition =
		{
			OwnerPos.x + markOffset.x,
			players[bestIndex]->GetPosition().y,
			OwnerPos.z + markOffset.z
		};

		state.timer = 0.0f;
		state.hasTarget = true;
	}



	Evaluation evaluation = m_calAIeva.Calculation(blackboard, players, bestIndex, elapsedTime);

	evaluation.Movespeed = m_MoveSpeed * (0.1f + 0.3f * evaluation.urgency);
	
	debug.active = true;
	debug.playerIndex = bestIndex;
	debug.evaluation = evaluation;

	//リクエスト
	AIMoveRequest request{};
	request.active = true;

	request.type = AIMoveRequestType::Action;
	request.reason = AIMoveReason::Mark;
	request.mode = AIMoveMode::Arrive;


	request.priority = 30;
	request.utility = evaluation.utility;
	request.urgency = evaluation.urgency;

	request.confirmationTime = 0.2f;

	request.targetPosition = state.targetPosition;

	request.moveSpeed = evaluation.Movespeed;

	request.stopDistance = 0.05f;

	resolver.Submit(bestIndex, request);
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
