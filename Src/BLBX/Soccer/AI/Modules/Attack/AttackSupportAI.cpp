#include "AttackSupportAI.h"

void AttackAupportAI::Initialize()
{
	m_forwardDistance = 5.0f;
	m_sideDistance = 3.0f;
	m_MoveSpeed = 5.0f;

	for (int i = 0; i < 2; ++i)
	{
		m_debugMinSideSlip[i] = {};
		m_debugRightMinSideSlip[i] = {};
		m_debugLeftMinSideSlip[i] = {};
	};
}

void AttackAupportAI::SubmitRequests(
	int TeamNumber, 
	const BlackBoard& blackboard,
	std::vector<std::unique_ptr<SoccerPlayer>>& players,
	const std::vector<int>& markTargets,
	AIMoveResolver& resolver, 
	float elapsedTime)
{
	m_debugMinSideSlip[TeamNumber].reset();

	int SupportIndex = blackboard.Get<int>(PlayerKeys::AttackSupportPlayerIndex).value_or(-1);

	if (SupportIndex < 0)
	{
		return;
	}

	int referenceIndex = blackboard.Get<int>(BallKeys::OwnerIndex).value_or(-1);

	if (referenceIndex < 0 )
	{
		referenceIndex = blackboard.Get<int>(BallKeys::PassTarget).value_or(-1);
	}

	if (referenceIndex < 0 ||
		referenceIndex >= static_cast<int>(players.size()) ||
		!players[referenceIndex])
	{
		return;
	}

	DirectX::XMFLOAT3 Position = players[SupportIndex]->GetPosition();

	DirectX::XMFLOAT3 referencePos = players[referenceIndex]->GetPosition();

	DirectX::XMFLOAT3 GoalPos =
		blackboard.Get<DirectX::XMFLOAT3>(TeamKeys::GoalPosition).value_or(DirectX::XMFLOAT3{});

	float minSideSlip = m_calAIeva.CalcPassSpace(referencePos, Position, players, TeamNumber);

	m_debugMinSideSlip[TeamNumber] = minSideSlip;
	
	DirectX::XMFLOAT3 AttackDirection =
	{
		GoalPos.x - referencePos.x,
		0.0f,
		GoalPos.z - referencePos.z
	};

	//攻撃方向
	DirectX::XMVECTOR AttackVec = DirectX::XMVector3Normalize(DirectX::XMLoadFloat3(&AttackDirection));

	DirectX::XMFLOAT3 attackDir;
	DirectX::XMStoreFloat3(&attackDir, AttackVec);


	//今は仮の候補を作っているが、今後は位置もどこに動くか計算してだしたい。
	//右の仮位置
	DirectX::XMFLOAT3 rightCandidatePos =
	{
		referencePos.x + attackDir.x * m_forwardDistance + attackDir.z * m_sideDistance,
		Position.y,
		referencePos.z + attackDir.z * m_forwardDistance - attackDir.x * m_sideDistance
	};
	//左の仮位置
	DirectX::XMFLOAT3 leftCandidatePos =
	{
		referencePos.x + attackDir.x * m_forwardDistance + attackDir.z * -m_sideDistance,
		Position.y,
		referencePos.z + attackDir.z * m_forwardDistance - attackDir.x * -m_sideDistance
	};

	rightCandidatePos = m_calAIeva.ClampToPitch(rightCandidatePos);
	leftCandidatePos = m_calAIeva.ClampToPitch(leftCandidatePos);

	float rightMinSideSlip = m_calAIeva.CalcPassSpace(referencePos, rightCandidatePos, players, TeamNumber);

	m_debugRightMinSideSlip[TeamNumber] = rightMinSideSlip;

	float LeftMinSideSlip = m_calAIeva.CalcPassSpace(referencePos, leftCandidatePos, players, TeamNumber);
	
	m_debugLeftMinSideSlip[TeamNumber] = LeftMinSideSlip;
	
	bool& useLeft = m_useLeft[TeamNumber];

	if (useLeft)
	{
		if (rightMinSideSlip > LeftMinSideSlip + 0.5f)
		{
			useLeft = false;
		}
	}
	else
	{
		if (LeftMinSideSlip > rightMinSideSlip + 0.5f)
		{
			useLeft = true;
		}
	}

	DirectX::XMFLOAT3 targetPos = rightCandidatePos;

	if (useLeft)
	{
		targetPos = leftCandidatePos;
	}

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

	request.targetPosition = targetPos;

	request.moveSpeed = m_MoveSpeed;

	request.stopDistance = 0.2f;

	resolver.Submit(SupportIndex, request);
}

void AttackAupportAI::DrawImGui()
{
#ifdef USE_IMGUI
		ImGui::SliderFloat("Forward distance", &m_forwardDistance, -8.0f,8.0f, "%.2f");

		ImGui::SliderFloat("Side distance", &m_sideDistance, -6.0f, 6.0f, "%.2f");

		ImGui::SliderFloat("Move Speed", &m_MoveSpeed, 0.1f, 12.0f, "%.2f");

		auto DrawSideSlip = [](
			const char* label,
			const std::optional<float>& value)
			{
				if (!value)
				{
					ImGui::TextDisabled("%s: not calculated", label);
				}
				else if (std::isinf(*value))
				{
					ImGui::Text("%s: clear", label);
				}
				else
				{
					ImGui::Text("%s: %.2f", label, *value);
				}
			};

		for (int team = 0; team < 2; ++team)
		{
			ImGui::Text("Team %d", team);

			DrawSideSlip("Current", m_debugMinSideSlip[team]);
			DrawSideSlip("Right", m_debugRightMinSideSlip[team]);
			DrawSideSlip("Left", m_debugLeftMinSideSlip[team]);

			ImGui::Separator();
		}
#endif
}
