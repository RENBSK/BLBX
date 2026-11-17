#include "AttackWideAI.h"

void AttackWideAI::SubmitRequests(
	int TeamNumber, const BlackBoard& blackboard,
	std::vector<std::unique_ptr<SoccerPlayer>>& players,
	const std::vector<int>& markTargets,
	AIMoveResolver& resolver,
	float elapsedTime)
{
	const int WideIndex = blackboard.Get<int>(PlayerKeys::AttackWidePlayerIndex).value_or(-1);
	/*const int OwnerIndex = blackboard.Get<int>(BallKeys::OwnerIndex).value_or(-1);*/

	if (WideIndex < 0)
	{
		return;
	}

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
	const auto GoalPos = blackboard.Get<DirectX::XMFLOAT3>(TeamKeys::GoalPosition);

	DirectX::XMFLOAT3 AttackDir =
	{
		GoalPos->x - referencePos.x,
		0.0f,
		GoalPos->z - referencePos.z
	};

	const float length = std::sqrt(AttackDir.x * AttackDir.x + AttackDir.z * AttackDir.z);

	if (length < 0.000001f)
	{
		return;
	}

	AttackDir.x /= length;
	AttackDir.z /= length;

	DirectX::XMFLOAT3 targetPosition =
	{
		referencePos.x + m_WodeDistance,
		0.0f,
		referencePos.z,
	};

	targetPosition = m_calAI.ClampToPitch(targetPosition);


	AIMoveRequest request;
	request.active = true;

	request.type = AIMoveRequestType::Base;
	request.reason = AIMoveReason::Support;
	request.mode = AIMoveMode::Arrive;

	request.priority = 45;
	request.utility = 0.5;
	request.urgency = 0.3;

	request.confirmationTime = 0.10f;
	request.minCommitmentTime = 0.30f;

	request.targetPosition = targetPosition;

	request.moveSpeed = 3.0f;

	request.stopDistance = 0.2f;

	resolver.Submit(WideIndex, request);
}

void AttackWideAI::DrawImGui()
{
#ifdef USE_IMGUI
	if (ImGui::CollapsingHeader("Attack Wide AI"))
	{
		ImGui::SliderFloat("Forward distance", &m_WodeDistance, -15.0f, 15.0f, "%.2f");

	}
#endif
}
