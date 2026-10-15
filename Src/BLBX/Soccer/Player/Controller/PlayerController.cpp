#include "PlayerController.h"

#include "Graphics/Camera/Camera.h"

#include <Windows.h>

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

	// ƒJƒƒ‰‚©‚çŒ©‚½‰E•ûŒü
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
