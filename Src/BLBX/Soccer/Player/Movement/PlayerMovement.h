#pragma once

#include <DirectXMath.h>

class SoccerPlayer;

class PlayerMovement
{
public:
	void Update(
		SoccerPlayer& player,
		const DirectX::XMFLOAT3& direction,
		float elapsedTime);

private:
	//Œ»İ‚ÌˆÚ“®‘¬“x
	DirectX::XMFLOAT3 m_velocity = { 0.0f,0.0f,0.0f };
};
