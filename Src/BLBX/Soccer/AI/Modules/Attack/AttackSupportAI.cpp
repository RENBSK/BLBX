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
	//一度リの基準となる保持者を取得
	int referenceIndex = blackboard.Get<int>(BallKeys::OwnerIndex).value_or(-1);
	//パス中は、受け手を基準に攻撃の配置を作る。
	if (referenceIndex < 0 )
	{
		referenceIndex = blackboard.Get<int>(BallKeys::PassTarget).value_or(-1);
	}
	//有効な基準選手がいなければ要求しない。
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

	//現在の位置へのパスコースを評価
	//値が大きいほど、直線から敵が横に離れている
	float minSideSlip = m_calAIeva.CalcPassSpace(referencePos, Position, players, TeamNumber);
	// ImGui表示用に結果を保存
	m_debugMinSideSlip[TeamNumber] = minSideSlip;
	//基準位置から、攻めるゴールへの方向
	DirectX::XMFLOAT3 AttackDirection =
	{
		GoalPos.x - referencePos.x,
		0.0f,
		GoalPos.z - referencePos.z
	};
	//攻撃方向を長さ1に正規化し、距離を掛けて使える方向にする
	DirectX::XMVECTOR AttackVec = DirectX::XMVector3Normalize(DirectX::XMLoadFloat3(&AttackDirection));
	DirectX::XMFLOAT3 attackDir;
	DirectX::XMStoreFloat3(&attackDir, AttackVec);

	// 右候補：攻撃方向へ進め、さらに右へずらす
	// 右方向は { attackDir.z, 0, -attackDir.x }
	DirectX::XMFLOAT3 rightCandidatePos =
	{
		referencePos.x + attackDir.x * m_forwardDistance + attackDir.z * m_sideDistance,
		Position.y,
		referencePos.z + attackDir.z * m_forwardDistance - attackDir.x * m_sideDistance
	};
	// 左候補：横方向の距離を反転させる
	DirectX::XMFLOAT3 leftCandidatePos =
	{
		referencePos.x + attackDir.x * m_forwardDistance + attackDir.z * -m_sideDistance,
		Position.y,
		referencePos.z + attackDir.z * m_forwardDistance - attackDir.x * -m_sideDistance
	};
	//候補位置がコートの外に出ないように補正
	rightCandidatePos = m_calAIeva.ClampToPitch(rightCandidatePos);
	leftCandidatePos = m_calAIeva.ClampToPitch(leftCandidatePos);
	//実際に移動できる候補位置で、左右のパスコースを評価
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

	// 選ばれた候補を今回の移動先にする
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
