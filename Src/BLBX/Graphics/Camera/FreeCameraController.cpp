#include "FreeCameraController.h"

#include "Camera.h"
#include "Graphics/Graphics.h"

#include <Windows.h>

#include <imgui.h>

void FreeCameraController::Initialize(const DirectX::XMFLOAT3& eye, const DirectX::XMFLOAT3& focus)
{
	m_eye = eye;
	m_focus = focus;

	using namespace DirectX;

	XMVECTOR e = XMLoadFloat3(&m_eye);
	XMVECTOR f = XMLoadFloat3(&m_focus);
	XMVECTOR dir = f - e;

	m_focusDistance = XMVectorGetX(XMVector3Length(dir));

	if (m_focusDistance < 1.0f)
	{
		m_focusDistance = 70.0f;
	}

	dir = XMVector3Normalize(dir);

	XMFLOAT3 d;
	XMStoreFloat3(&d, dir);

	m_pitch = asinf(d.y);

	m_yaw = atan2f(d.x, d.z);

	ApplyCamera();
}

void FreeCameraController::Update(float elapsedTime)
{
#ifdef USE_IMGUI
	ImGuiIO& io = ImGui::GetIO();

	// ImGuiがマウス操作中なら、カメラ回転しない
	if (!io.WantCaptureMouse)
	{
		RotateCameraByMouse();
	}
	else
	{
		m_mouseRotating = false;
	}

	// ImGuiがキーボード操作中なら、IJKL移動しない
	if (!io.WantCaptureKeyboard)
	{
		MoveCamera(elapsedTime);
	}
#else
	RotateCameraByMouse();
	MoveCamera(elapsedTime);
#endif
}

void FreeCameraController::DrawImgui()
{
	// マウスホイールによるズーム
	MoveByMouseWheel();

	ImGui::SetNextWindowSize(
		ImVec2(380, 450),
		ImGuiCond_FirstUseEver
	);

	if (ImGui::Begin("Free Camera"))
	{
		// =====================================
		// Camera Presets
		// =====================================
		ImGui::SeparatorText("Camera Presets");

		if (ImGui::Button("Stadium View"))
		{
			SetPreset(
				{ -4.0f, 6.5f, -0.16f },
				{ 9.1f, -12.1f, -0.12f },
				45.0f
			);
		}

		ImGui::SameLine();

		// 少し引いた視点
		if (ImGui::Button("Wide"))
		{
			SetPreset(
				{ -8.0f, 12.0f, -0.17f },
				{ 9.1f, -12.1f, -0.12f },
				45.0f
			);
		}

		ImGui::SameLine();

		// 少し近づいた視点
		if (ImGui::Button("Close"))
		{
			SetPreset(
				{ -1.0f, 2.5f, -0.15f },
				{ 9.1f, -12.1f, -0.12f },
				45.0f
			);
		}

		// 高い位置から見下ろす
		if (ImGui::Button("High"))
		{
			SetPreset(
				{ -4.0f, 10.5f, -0.16f },
				{ 9.1f, -12.1f, -0.12f },
				45.0f
			);
		}

		ImGui::SameLine();

		// 低い位置から見る
		if (ImGui::Button("Low"))
		{
			SetPreset(
				{ -4.0f, 3.5f, -0.16f },
				{ 12.5f, -8.4f, -0.3f },
				45.0f
			);
		}

		// =====================================
		// Transform
		// =====================================
		ImGui::SeparatorText("Transform");

		bool transformChanged = false;

		transformChanged |= ImGui::DragFloat3(
			"Position",
			&m_eye.x,
			0.1f
		);

		transformChanged |= ImGui::DragFloat3(
			"Look At",
			&m_focus.x,
			0.1f
		);

		if (transformChanged)
		{
			SyncAnglesFromEyeFocus();
			ApplyCamera();
		}

		ImGui::TextDisabled(
			"Focus Distance : %.2f",
			m_focusDistance
		);

		// =====================================
		// Lens
		// =====================================
		ImGui::SeparatorText("Lens");

		if (ImGui::SliderFloat(
			"Field of View",
			&m_fov,
			10.0f,
			120.0f,
			"%.1f deg"
		))
		{
			ApplyCamera();
		}

		// =====================================
		// Movement Settings
		// =====================================
		if (ImGui::CollapsingHeader("Movement Settings"))
		{
			ImGui::DragFloat(
				"Move Speed",
				&m_moveSpeed,
				0.1f,
				1.0f,
				300.0f
			);

			ImGui::DragFloat(
				"Wheel Speed",
				&m_wheelSpeed,
				0.1f,
				1.0f,
				100.0f
			);

			ImGui::DragFloat(
				"Mouse Sensitivity",
				&m_mouseSensitivity,
				0.0001f,
				0.001f,
				0.05f,
				"%.4f"
			);
		}

		// =====================================
		// Controls
		// =====================================
		if (ImGui::CollapsingHeader("Controls"))
		{
			ImGui::BulletText("I / K : Forward / Back");
			ImGui::BulletText("J / L : Left / Right");
			ImGui::BulletText("U / O : Up / Down");
			ImGui::BulletText("Shift : Fast Move");
			ImGui::BulletText("Left Drag : Rotate");
			ImGui::BulletText("Mouse Wheel : Zoom");
		}

		ImGui::Spacing();
		ImGui::Separator();

		// =====================================
		// Reset
		// =====================================
		if (ImGui::Button("Reset Camera", ImVec2(-1, 0)))
		{
			SetPreset(
				{ -4.0f, 6.5f, -0.16f },
				{ 9.1f, -12.1f, -0.12f },
				45.0f
			);
		}
	}

	ImGui::End();
}

void FreeCameraController::ApplyCamera()
{
	//カメラ初期設定
	Graphics& graphics = Graphics::Instance();
	Camera& camera = Camera::Instance();
	camera.SetLookAt(
		m_eye,		//視点
		m_focus,		//注意点
		DirectX::XMFLOAT3(0, 1, 0)		//上方向
	);
	camera.SetPerspectiveFov(
		DirectX::XMConvertToRadians(m_fov),		//視野角
		static_cast<float>(graphics.GetScreenWidth()) /graphics.GetScreenHeight(),
		0.1f,	//クリップ距離(近)
		1000.0f		//クリップ距離(遠)
	);
}

void FreeCameraController::MoveCamera(float elapsedTime)
{
	using namespace DirectX;

	XMVECTOR eye = XMLoadFloat3(&m_eye);
	XMVECTOR focus = XMLoadFloat3(&m_focus);

	XMVECTOR forward = focus - eye;
	forward = XMVectorSetY(forward, 0.0f);

	if (XMVectorGetX(XMVector3LengthSq(forward)) < 0.001f)
	{
		forward = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
	}
	else
	{
		forward = XMVector3Normalize(forward);
	}

	XMVECTOR right = XMVector3Normalize(
		XMVector3Cross(
			XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f),
			forward
		)
	);

	float speed = m_moveSpeed;

	if (GetAsyncKeyState(VK_SHIFT) & 0x8000)
	{
		speed *= 3.0f;
	}

	float move = speed * elapsedTime;

	XMVECTOR moveVec = XMVectorZero();

	if (GetAsyncKeyState('I') & 0x8000) moveVec += forward * move;
	if (GetAsyncKeyState('K') & 0x8000) moveVec -= forward * move;
	if (GetAsyncKeyState('J') & 0x8000) moveVec -= right * move;
	if (GetAsyncKeyState('L') & 0x8000) moveVec += right * move;
	if (GetAsyncKeyState('U') & 0x8000) moveVec += XMVectorSet(0.0f, move, 0.0f, 0.0f);
	if (GetAsyncKeyState('O') & 0x8000) moveVec -= XMVectorSet(0.0f, move, 0.0f, 0.0f);

	if (XMVectorGetX(XMVector3LengthSq(moveVec)) <= 0.0001f)
	{
		return;
	}

	eye += moveVec;
	focus += moveVec;

	XMStoreFloat3(&m_eye, eye);
	XMStoreFloat3(&m_focus, focus);

	ApplyCamera();
}

void FreeCameraController::RotateCameraByMouse()
{
	using namespace DirectX;

	// 右クリック中だけ回転
	if (!(GetAsyncKeyState(VK_LBUTTON) & 0x8000))
	{
		m_mouseRotating = false;
		return;
	}

	POINT mousePos{};
	GetCursorPos(&mousePos);

	if (!m_mouseRotating)
	{
		m_prevMouseX = mousePos.x;
		m_prevMouseY = mousePos.y;
		m_mouseRotating = true;
		return;
	}

	int dx = mousePos.x - m_prevMouseX;
	int dy = mousePos.y - m_prevMouseY;

	m_prevMouseX = mousePos.x;
	m_prevMouseY = mousePos.y;

	m_yaw += dx * m_mouseSensitivity;
	m_pitch -= dy * m_mouseSensitivity;

	// 上下を向きすぎると反転するので制限
	const float pitchLimit = DirectX::XMConvertToRadians(85.0f);

	if (m_pitch > pitchLimit)
	{
		m_pitch = pitchLimit;
	}
	if (m_pitch < -pitchLimit)
	{
		m_pitch = -pitchLimit;
	}

	XMVECTOR eye = XMLoadFloat3(&m_eye);

	XMVECTOR forward = XMVectorSet(
		sinf(m_yaw) * cosf(m_pitch),
		sinf(m_pitch),
		cosf(m_yaw) * cosf(m_pitch),
		0.0f
	);

	forward = XMVector3Normalize(forward);

	XMVECTOR focus = eye + forward * m_focusDistance;

	XMStoreFloat3(&m_focus, focus);

	ApplyCamera();
}

void FreeCameraController::MoveByMouseWheel()
{
	using namespace DirectX;

#ifdef USE_IMGUI
	if (ImGui::GetIO().WantCaptureMouse)
	{
		return;
	}
#endif

	float wheel = ImGui::GetIO().MouseWheel;

	if (wheel == 0.0f)
	{
		return;
	}

	XMVECTOR eye = XMLoadFloat3(&m_eye);
	XMVECTOR focus = XMLoadFloat3(&m_focus);

	// eye から focus への方向
	XMVECTOR forward = focus - eye;

	float distance = XMVectorGetX(XMVector3Length(forward));

	if (distance < 0.001f)
	{
		return;
	}

	forward = XMVector3Normalize(forward);

	// ホイールで距離だけを変える
	distance -= wheel * m_wheelSpeed;

	// focusを通り越さないように制限
	const float minDistance = 5.0f;
	const float maxDistance = 2000.0f;

	if (distance < minDistance)
	{
		distance = minDistance;
	}

	if (distance > maxDistance)
	{
		distance = maxDistance;
	}

	// focusを中心にして、eyeを距離で再配置
	eye = focus - forward * distance;

	XMStoreFloat3(&m_eye, eye);

	m_focusDistance = distance;

	ApplyCamera();
}

void FreeCameraController::SyncAnglesFromEyeFocus()
{
	using namespace DirectX;

	XMVECTOR e = XMLoadFloat3(&m_eye);
	XMVECTOR f = XMLoadFloat3(&m_focus);
	XMVECTOR dir = f - e;

	m_focusDistance = XMVectorGetX(XMVector3Length(dir));

	if (m_focusDistance < 1.0f)
	{
		m_focusDistance = 1.0f;
		return;
	}

	dir = XMVector3Normalize(dir);

	XMFLOAT3 d;
	XMStoreFloat3(&d, dir);

	m_pitch = asinf(d.y);
	m_yaw = atan2f(d.x, d.z);
}

void FreeCameraController::SetPreset(
	const DirectX::XMFLOAT3& eye,
	const DirectX::XMFLOAT3& focus,
	float fov)
{
	m_eye = eye;
	m_focus = focus;
	m_fov = fov;

	SyncAnglesFromEyeFocus();
	ApplyCamera();
}