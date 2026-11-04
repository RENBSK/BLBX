#pragma once

#include <DirectXMath.h>

class SoccerPlayer;
class SoccerBall;

class PlayerMovement
{
public:
	void Update(
		SoccerPlayer& player,
		const DirectX::XMFLOAT3& direction,
		float moveSpeed,
		float elapsedTime);

	DirectX::XMFLOAT3 GetVelocity() const
	{
		return m_velocity;
	}

private:
	//åªç›ÇÃà⁄ìÆë¨ìx
	DirectX::XMFLOAT3 m_velocity = { 0.0f,0.0f,0.0f };

	bool shouldStop = false;
};
