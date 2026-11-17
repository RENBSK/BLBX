#include "CoverAI.h"

void CoverAI::Initialize()
{
	m_MarkDistance = 0.5f;
	m_MoveSpeed = 2.5f;

	for (int i = 0; i < 2; ++i)
	{
		m_isSteppingIn[i] = false;
	}
}

void CoverAI::SubmitRequests(
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

	int SupportIndex = blackboard.Get<int>(PlayerKeys::DefenceCoverIndex).value_or(-1);

	/*int OwnerIndex = blackboard.Get<int>(BallKeys::OwnerIndex).value_or(-1);*/

	if (SupportIndex < 0)	
	{
		return;
	}

	DirectX::XMFLOAT3 GoalPosition =
		blackboard.Get<DirectX::XMFLOAT3>(TeamKeys::DefendGoalPosition).value_or(DirectX::XMFLOAT3{});

	int referenceIndex = blackboard.Get<int>(BallKeys::OwnerIndex).value_or(-1);

	if (referenceIndex < 0)
	{
		referenceIndex = blackboard.Get<int>(BallKeys::PassTarget).value_or(-1);
	}

	if (referenceIndex < 0 ||
		referenceIndex >= static_cast<int>(players.size()) ||
		!players[referenceIndex])
	{
		return;
	}

	DirectX::XMFLOAT3 referencePos = players[referenceIndex]->GetPosition();

	DirectX::XMFLOAT3 GoalToPlayerVec =
	{
		referencePos.x - GoalPosition.x,
		0.0,
		referencePos.z - GoalPosition.z
	};

	DirectX::XMVECTOR targetVec = DirectX::XMVector3Normalize(DirectX::XMLoadFloat3(&GoalToPlayerVec));

	DirectX::XMFLOAT3 offset;

	DirectX::XMStoreFloat3(&offset, DirectX::XMVectorScale(targetVec, m_MarkDistance));

	DirectX::XMFLOAT3 target =
	{
		GoalPosition.x + offset.x,
		0.0f,
		GoalPosition.z + offset.z
	};

	// 共通の状況評価
	Evaluation evaluation = m_calAIeva.Calculation(
		blackboard,
		players,
		SupportIndex,
		elapsedTime);

	const int markIndex = blackboard.Get<int>(PlayerKeys::PressPlayerIndex).value_or(-1);

	float markCoverage = 0.0f;

	if (markIndex >= 0 && players[markIndex])
	{
		// Cover自身が進路を塞げている度合い
		markCoverage = m_calAIeva.CalculationCover(referencePos,players[markIndex]->GetPosition(),GoalPosition);
	}

	// 位置を修正する必要度
	const float repositionNeed = 1.0f - markCoverage;

	// 先に、今回の行動に合わせて評価を更新
	evaluation.urgency = evaluation.danger * repositionNeed;

	evaluation.Movespeed = m_MoveSpeed * (0.7f + 0.3f * evaluation.urgency);

	bool& isSteppingIn = m_isSteppingIn[TeamNumber];

	if (!isSteppingIn)
	{
		// 待機中：守れていなくなったら出る
		if (markCoverage <= 0.3f)
		{
			isSteppingIn = true;
		}
	}
	else
	{
		// 対応中：十分に守れる状態へ戻ったら引く
		if (markCoverage >= 0.6f)
		{
			isSteppingIn = false;
		}
	}

	//マークしているプレイヤーが守れてなかったら前に出る。
	if (isSteppingIn)
	{
		DirectX::XMFLOAT3 targetDirection =
		{
			GoalPosition.x - referencePos.x,
			0.0f,
			GoalPosition.z - referencePos.z
		};

		const DirectX::XMVECTOR targetVec = DirectX::XMVector3Normalize(DirectX::XMLoadFloat3(&targetDirection));
		DirectX::XMFLOAT3 markOffset;
		DirectX::XMStoreFloat3(&markOffset, DirectX::XMVectorScale(targetVec, 1.0f));

		target =
		{
			referencePos.x + markOffset.x,
			players[SupportIndex]->GetPosition().y,
			referencePos.z + markOffset.z
		};
	}

	//Debug用
	{
		debug.active = true;
		debug.playerIndex = SupportIndex;
		debug.coverScore = markCoverage;
		debug.repositionNeed = repositionNeed;
		debug.evaluation = evaluation;
	}

	

	AIMoveRequest request;
	request.active = true;

	request.type = isSteppingIn ? AIMoveRequestType::Action : AIMoveRequestType::Soft;
	request.reason = AIMoveReason::Cover;
	request.mode = AIMoveMode::Arrive;

	request.weight = m_coverWeight;

	request.priority = 45;
	request.utility = evaluation.utility;
	request.urgency = evaluation.urgency;

	request.confirmationTime = 0.10f;
	request.minCommitmentTime = 0.30f;

	request.targetPosition = target;

	request.moveSpeed = evaluation.Movespeed;

	request.stopDistance = 0.2f;

	resolver.Submit(SupportIndex, request);

}

void CoverAI::DrawImGui()
{
#ifdef USE_IMGUI
	if (ImGui::CollapsingHeader(
		"DefenceSupport AI"))
	{
		ImGui::DragFloat("Mark distance", &m_MarkDistance, 0.1f, 0.1f,10.0f, "%.2f");
		ImGui::SliderFloat("Move speed", &m_MoveSpeed, 0.0f, 10.0f, "%.2f");
		ImGui::SliderFloat("Cover weight", &m_coverWeight, 0.0f, 5.0f, "%.2f");

		m_calAIeva.DrawImGui();

		if (ImGui::TreeNode("Cover evaluation"))
		{
			for (int team = 0; team < 2; ++team)
			{
				ImGui::PushID(team);

				const EvaluationDebug& debug =
					m_evaluationDebug[team];

				ImGui::Text("Team %d", team);

				if (!debug.active)
				{
					ImGui::TextDisabled("No cover request");
					ImGui::PopID();
					continue;
				}

				const Evaluation& e = debug.evaluation;

				ImGui::Text("Player index: %d", debug.playerIndex);

				ImGui::Text("Mark coverage: %.3f", debug.coverScore);
				ImGui::ProgressBar(
					debug.coverScore, ImVec2(-1.0f, 0.0f));

				ImGui::Text(
					"Reposition need: %.3f",
					debug.repositionNeed);

				ImGui::Text("Danger: %.3f", e.danger);
				ImGui::Text("Request urgency: %.3f", e.urgency);
				ImGui::Text("Requested speed: %.3f", e.Movespeed);

				ImGui::Separator();
				ImGui::PopID();
			}

			ImGui::TreePop();
		}
	}
#endif
}
