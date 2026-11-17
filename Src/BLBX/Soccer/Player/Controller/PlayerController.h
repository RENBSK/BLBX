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

	void DrawImgui();

private:
	float m_passSpeed=10.0f;
	float m_shootSpeed=10.0f;
};