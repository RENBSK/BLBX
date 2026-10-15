#include "PlayerMovement.h"

#include "Soccer/Player/SoccerPlayer.h"

#include <cmath>

void PlayerMovement::Update(SoccerPlayer& player, const DirectX::XMFLOAT3& direction, float elapsedTime)
{
	if (elapsedTime <= 0.0f)
	{
		return;
	}

	//選手の能力値を取得
	const PlayerParameters& parameters = player.GetParameters();

	//目標速度
	DirectX::XMFLOAT3 targetVelocity =
	{
		direction.x * parameters.maxSpeed,
		0.0f,
		direction.z * parameters.maxSpeed
	};

	float diffX = targetVelocity.x - m_velocity.x;
	float diffZ = targetVelocity.z - m_velocity.z;
	
	float diffLength = sqrtf(diffX * diffX + diffZ * diffZ);

	//移動入力があるか
	bool hasInput = direction.x * direction.x + direction.z * direction.z > 0.000001f;

	//加速と減速を決定
	float rate = hasInput ? parameters.acceleration : parameters.deceleration;

	float maxChange = rate * elapsedTime;

	// 現在速度を目標速度に近づける
	if (diffLength > 0.000001f)
	{
		if (diffLength <= maxChange)
		{
			m_velocity = targetVelocity;
		}
		else if (maxChange > 0.0f)
		{
			m_velocity.x +=
				diffX / diffLength * maxChange;

			m_velocity.z +=
				diffZ / diffLength * maxChange;
		}
	}

	//1フレームの移動量
	DirectX::XMFLOAT3 move =
	{
		m_velocity.x * elapsedTime,
		0.0f,
		m_velocity.z * elapsedTime
	};

	player.Move(move);
}
