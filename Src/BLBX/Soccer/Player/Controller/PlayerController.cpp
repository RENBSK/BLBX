#include "PlayerController.h"

#include "Graphics/Camera/Camera.h"
#include "Soccer/Player/SoccerPlayer.h"
#include "Soccer/Ball/SoccerBall.h"
#include "Soccer/Player/Dribble/DribbleController.h"

#include <Windows.h>

bool PlayerController::Update(
	SoccerPlayer& player, 
	SoccerBall& ball, 
	const SoccerPlayer* passTarget, 
	DribbleController& dribble,
	const DirectX::XMFLOAT3 GoalPos,
	float elapsedTime)
{
	
	const auto direction = GetMoveDirection();

	bool hasBall = player.GetHasBall();

	//選手はタッチで確定した方向へ進む
	const auto moveDirection = hasBall 
		? dribble.GetPlayerMoveDirection(direction) : direction;

	//プレイヤーの移動・旋回
	player.UpdateMovement(moveDirection, player.GetParameters().maxSpeed, elapsedTime);

	if (!hasBall)
	{
		dribble.Reset();
		return false;
	}

	dribble.UpdateDribble(player, ball, player.GetVelocity(), direction, elapsedTime);

	const bool isPassing = player.GetAnimationState() == PlayerAnimationState::Pass_Right;
	const bool isShooting = player.GetAnimationState() == PlayerAnimationState::Shoot;

	const bool passInput = (GetAsyncKeyState('P') & 0x8000) != 0;
	const bool shootInput = (GetAsyncKeyState('C') & 0x8000) != 0;

	bool kicked = false;

	if (isShooting || (shootInput && !isPassing))
	{
		kicked = player.Shoot(GoalPos, ball, 20.0f);
	}
	else if ((passInput || isPassing) && passTarget && passTarget != &player)
	{
		kicked = player.Pass(ball, passTarget->GetPosition(), 10.0f);
	}

	if (kicked)
	{
		dribble.Reset();
	}

	return kicked;
}

DirectX::XMFLOAT3 PlayerController::GetMoveDirection() const
{
    using namespace DirectX;

    Camera& camera = Camera::Instance();

	XMFLOAT3 eyePos = camera.GetEye();
	XMFLOAT3 focusPos = camera.GetFocus();

	XMVECTOR eye = XMLoadFloat3(&eyePos);
	XMVECTOR focus = XMLoadFloat3(&focusPos);

    XMVECTOR cameraForward = focus - eye;

    cameraForward = XMVectorSetY(cameraForward, 0.0f);

	if (XMVectorGetX(XMVector3LengthSq(cameraForward)) < 0.000001f)
	{
		cameraForward = DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
	}
	else
	{
		cameraForward = XMVector3Normalize(cameraForward);
	}

	// カメラから見た右方向
	XMVECTOR up = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	XMVECTOR cameraRight = XMVector3Normalize(XMVector3Cross(up, cameraForward));

	XMVECTOR MoveVec = XMVectorZero();

	if (GetAsyncKeyState('W') & 0x8000)
	{
		MoveVec += cameraForward;
	}
	if (GetAsyncKeyState('S') & 0x8000)
	{
		MoveVec -= cameraForward;
	}
	if (GetAsyncKeyState('D') & 0x8000)
	{
		MoveVec += cameraRight;
	}
	if (GetAsyncKeyState('A') & 0x8000)
	{
		MoveVec -= cameraRight;
	}

	if (XMVectorGetX(XMVector3LengthSq(MoveVec)))
	{
		MoveVec = XMVector3Normalize(MoveVec);
	}

	XMFLOAT3 move;

	XMStoreFloat3(&move, MoveVec);

	return move;
}
