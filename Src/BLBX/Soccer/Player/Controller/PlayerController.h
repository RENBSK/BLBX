#pragma once

#include <DirectXMath.h>

class SoccerPlayer;
class SoccerBall;
class DribbleController;

class PlayerController
{
public:
	//XV
	bool Update(
		SoccerPlayer& player,
		SoccerBall& ball,
		const SoccerPlayer* passTarget,
		DribbleController& dribble,
		const DirectX::XMFLOAT3 GoalPos,
		float elapsedTime);


	//“ü—Í•ûŒü‚ğæ‚é
	DirectX::XMFLOAT3 GetMoveDirection() const;
};