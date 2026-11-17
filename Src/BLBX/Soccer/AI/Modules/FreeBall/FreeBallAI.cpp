#include "FreeBallAI.h"

void FreeBallAI::Initialize()
{

}

void FreeBallAI::SubmitRequests(
	int TeamNumber, 
	const BlackBoard& blackboard, 
	std::vector<std::unique_ptr<SoccerPlayer>>& players, 
	const std::vector<int>& markTargets, 
	AIMoveResolver& resolver, 
	float elapsedTime)
{
	const int OwnerIndex = blackboard.Get<int>(BallKeys::OwnerIndex).value_or(-1);

	const int passTarget = blackboard.Get<int>(BallKeys::PassTarget).value_or(-1);

	if (OwnerIndex >= 0 || passTarget >= 0)
	{
		return;
	}

	const auto ballPos = blackboard.Get<DirectX::XMFLOAT3>(BallKeys::Position);

	if (!ballPos)
	{
		return;
	}

	const int playerIndex = blackboard.Get<int>(PlayerKeys::FreeBallPlayerIndex).value_or(-1);

	if (playerIndex < 0 ||
		playerIndex >= static_cast<int>(players.size()) ||
		!players[playerIndex])
	{
		return;
	}

	SoccerPlayer* player = players[playerIndex].get();

	if (player->GetTeamNumber() != TeamNumber)
	{
		return;
	}

	AIMoveRequest request{};
	request.active = true;

	request.type = AIMoveRequestType::Hard;
	request.reason = AIMoveReason::FreeBallChase;
	request.mode = AIMoveMode::Arrive;

	request.priority = 60;
	request.utility = 1.0f;
	request.urgency = 1.0f;

	request.confirmationTime = 0.0f;
	request.minCommitmentTime = 0.1f;

	request.targetPosition = *ballPos;
	request.moveSpeed = 3.0f;
	request.stopDistance = 0.1f;

	resolver.Submit(playerIndex, request);

}

void FreeBallAI::DrawImGui()
{

}
