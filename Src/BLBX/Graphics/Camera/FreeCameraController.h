#pragma once

#include <DirectXMath.h>

class FreeCameraController
{
public:
	void Initialize(
		const DirectX::XMFLOAT3& eye,
		const DirectX::XMFLOAT3& focus);

	void Update(float elapsedTime);

	void DrawImgui();


private:
	void ApplyCamera();

	void MoveCamera(float elapsedTime);

	void RotateCameraByMouse();

	void MoveByMouseWheel();

	void SyncAnglesFromEyeFocus();
	void SetPreset(const DirectX::XMFLOAT3& eye, const DirectX::XMFLOAT3& focus, float fov);

private:
	DirectX::XMFLOAT3 m_eye = { 0.0f,0.0f,0.0f };
	DirectX::XMFLOAT3 m_focus = { 0.0f,0.0f,0.0f };


	float m_fov = 20.0f;
	float m_moveSpeed = 10.0f;

	float m_yaw = 0.0f;
	float m_pitch = 0.0f;
	float m_focusDistance = 30.0f;
	float m_mouseSensitivity = 0.005f;

	bool m_mouseRotating = false;
	int m_prevMouseX = 0;
	int m_prevMouseY = 0;

	float m_wheelSpeed = 1.5f;
};