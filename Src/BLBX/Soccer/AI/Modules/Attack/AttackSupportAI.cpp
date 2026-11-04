#include "AttackSupportAI.h"

void AttackAupportAI::SubmitRequests(
	int TeamNumber, 
	const BlackBoard& blackboard, 
	std::vector<std::unique_ptr<SoccerPlayer>>& players, 
	AIMoveResolver& resolver, 
	float elapsedTime)
{
	int SupportIndex = blackboard.Get<int>(PlayerKeys::AttackSupportPlayerIndex).value_or(-1);

	int OwnerIndex = blackboard.Get<int>(BallKeys::OwnerIndex).value_or(-1);

	if (SupportIndex < 0)
	{
		return;
	}

	DirectX::XMFLOAT3 OwnerPos = players[OwnerIndex]->GetPosition();

	DirectX::XMFLOAT3 GoalPos =
		blackboard.Get<DirectX::XMFLOAT3>(TeamKeys::GoalPosition).value_or(DirectX::XMFLOAT3{});

	DirectX::XMFLOAT3 AttackDirection =
	{
		GoalPos.x - OwnerPos.x,
		0.0f,
		GoalPos.z - OwnerPos.z
	};

	DirectX::XMFLOAT3 targetPos;


}
