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

	void Follow(const DirectX::XMFLOAT3& target, float elapsedTime);

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

	// 追従開始時のカメラ位置とボール位置
	DirectX::XMFLOAT3 m_followStartEye{};
	DirectX::XMFLOAT3 m_followStartTarget{};

	bool m_hasFollowTarget = false;
	bool m_enableFollow = true;

	//ボールの移動量に対して追う速さ
	float m_followAmount = 0.6f;
	//追いつく速さ
	float m_followSpeed = 3.0f;

	//カメラが追いかける範囲
	float m_followMinX = -2.0f;
	float m_followMaxX = 2.0f;

	float m_followMinZ = -7.0f;
	float m_followMaxZ = 7.0f;

	bool m_snapFollow = false;
	bool m_hasInitialTarget = false;
};