#include "DribbleController.h"

#include "Soccer/Player/SoccerPlayer.h"
#include "Soccer/Ball/SoccerBall.h"

#include <algorithm>
#include <cmath>


void DribbleController::UpdateDribble(
	SoccerPlayer& player, SoccerBall& ball, 
	const DirectX::XMFLOAT3& playerVelocity,
	DirectX::XMFLOAT3 direction,
	float elapsedTime)
{

	//今だけ
	{
		DirectX::XMFLOAT3 playerPos = player.GetPosition();
		DirectX::XMFLOAT3 ballPos = ball.GetPosition();
		DirectX::XMFLOAT3 forward = player.GetForward();

		DirectX::XMFLOAT3 BallDistance =
		{
			playerPos.x + forward.x * 0.2f,
			ballPos.y,
			playerPos.z + forward.z * 0.2f
		};

		ball.Stop();
		ball.SetPosition(BallDistance);
		player.SetHasBall(true);
		return;
	}

	if (player.GetAnimationState() != PlayerAnimationState::Dribble)
	{
		m_hasPreviousPhase = false;
		return;
	}

	float currentPhase = player.GetAnimationPhase();
	
	if (!m_hasPreviousPhase)
	{
		m_dribbleDirection = direction;
		m_previousPhase = currentPhase;
		m_hasPreviousPhase = true;
		return;
	}

	if (direction.x * direction.x +
		direction.z * direction.z <= 0.000001f)
	{
		ball.Stop();
		Reset();
		return;
	}

	//前回から今回の間にタッチのタイミングを通過したか確認
	auto crossed = [&](float touchPhase)
		{
			if (currentPhase >= m_previousPhase)
			{
				return m_previousPhase < touchPhase &&
					touchPhase <= currentPhase;
			}

			return m_previousPhase < touchPhase ||
				touchPhase <= currentPhase;
		};

	//左右どちらかの足が通過したならtureに
	bool touch = crossed(m_leftTouchPhase[0]) || crossed(m_rightTouchPhase[0]) ||
		crossed(m_rightTouchPhase[1]) || crossed(m_leftTouchPhase[1]);

	//次の判定に使うため、今回の再生位置を保存
	m_previousPhase = currentPhase;


	DirectX::XMFLOAT3 playerPos = player.GetPosition();
	DirectX::XMFLOAT3 forward = player.GetForward();
	DirectX::XMFLOAT3 ballPos = ball.GetPosition();

	float Playerspeed = sqrtf(playerVelocity.x * playerVelocity.x + playerVelocity.z * playerVelocity.z);

	float speed = Playerspeed * 0.3f;

	const float kickSpeed = Playerspeed + speed;

	DirectX::XMFLOAT3 PlayerToBall =
	{
		ballPos.x - playerPos.x,
		0.0,
		ballPos.z - playerPos.z
	};

	float dist = DirectX::XMVectorGetX(DirectX::XMVector3LengthSq(DirectX::XMLoadFloat3(&PlayerToBall)));

	if (dist > m_dribbleDistance)
	{
		return;
	}

	//現在の入力
	const DirectX::XMFLOAT3 currentDirection = direction;
	//現在入力があるか
	const bool hasInput = sqrtf(currentDirection.x * currentDirection.x + currentDirection.z * currentDirection.z)>0.00001f;

	//ドリブル処理　入力あり
	if (touch && hasInput)
	{
		//この後にtouchするまでに入力方向とドリブル方向が違うかったら、
		//ドリブル方向に反映し、蹴ってから、プレイヤーは向くような仕組みにしたい。

		//ドリブル方向を今の入力方向に
		m_dribbleDirection = currentDirection;

		const float rightX = m_dribbleDirection.z;
		const float rightZ = -m_dribbleDirection.x;

		const float targetX = playerPos.x + m_dribbleDirection.z * m_footDistance;
		const float targetZ = playerPos.z + m_dribbleDirection.x * m_footDistance;

		const float errorX = targetX - ballPos.x;
		const float errorZ = targetZ - ballPos.z;

		const float forwardCorrection = (errorX + m_dribbleDirection.x + errorZ * m_dribbleDirection.z) * m_footDistance;

		//横方向だけ補正を弱くする
		constexpr float maxSideCorrection = 0.5f;

		const float sideCorrection = std::clamp((errorX * rightX + errorZ * rightZ) * m_footDistance, -maxSideCorrection, maxSideCorrection);

		DirectX::XMFLOAT3 velocity =
		{
			m_dribbleDirection.x * (kickSpeed + forwardCorrection) + rightX * sideCorrection,
			0.0f,
			m_dribbleDirection.z * (kickSpeed + forwardCorrection) + rightZ + sideCorrection
		};

		DirectX::XMFLOAT3 velocityW =
		{
			(playerVelocity.x * 0.5f) + m_dribbleDirection.x * kickSpeed,
			0.0f,
			(playerVelocity.z * 0.5f) + m_dribbleDirection.z * kickSpeed
		};

		ball.SetDribbleVelocity(velocityW);

	}
	else if(!hasInput)//入力なし
	{
		//ボールを収める処理
		ball.Stop();
	}
}

DirectX::XMFLOAT3 DribbleController::GetPlayerMoveDirection(const DirectX::XMFLOAT3& inputDirection) const
{
	if (inputDirection.x * inputDirection.x + inputDirection.z * inputDirection.z <= 0.00001f)
	{
		return { 0.0f,0.0f,0.0f };
	}

	return m_hasPreviousPhase ? m_dribbleDirection : inputDirection;
}

