#include "SoccerBall.h"

#include <algorithm>
#include <cmath>

#ifdef USE_IMGUI
#include "imgui.h"
#endif // USE_IMGUI

void SoccerBall::Initialize()
{
	LoadModel("./Data/Model/Ball/soccer_ball.glb", true);

	m_transform.position = { 0.0f,m_ground,0.0f };
	m_transform.rotation = { 0.0f,0.0f,0.0f };
	m_transform.scale = { 0.6f,0.6f,0.6f };

    Stop();
}

void SoccerBall::Finalize()
{
	ModelObject::Finalize();
}

void SoccerBall::Update(float elapsedTime)
{
    // êÖïΩà⁄ìÆ
    m_transform.position.x += m_velocity.x * elapsedTime;
    m_transform.position.z += m_velocity.z * elapsedTime;

    // èdóÕ
    if (m_useGravity)
    {
        m_velocity.y += m_gravity * elapsedTime;
        m_transform.position.y += m_velocity.y * elapsedTime;

        // è∞ÇÊÇËâ∫Ç÷çsÇ©Ç»Ç¢ÇÊÇ§Ç…Ç∑ÇÈ
        if (m_transform.position.y <= m_ground)
        {
            m_transform.position.y = m_ground;
            m_velocity.y = 0.0f;
        }
    }
    else
    {
        m_velocity.y = 0.0f;
    }

    // êÖïΩï˚å¸ÇÃå∏ë¨
    float damping = std::max(
        0.0f, 1.0f - m_friction * elapsedTime);

    m_velocity.x *= damping;
    m_velocity.z *= damping;

    // êÖïΩï˚å¸ÇÃí‚é~
    if (m_velocity.x * m_velocity.x +
        m_velocity.z * m_velocity.z < 0.000001f)
    {
        m_velocity.x = 0.0f;
        m_velocity.z = 0.0f;
    }
}

void SoccerBall::Render()
{
	ModelObject::Render();
}

void SoccerBall::DrawImGui()
{
#ifdef USE_IMGUI
    if (ImGui::Begin("Ball Inspector"))
    {
        //à íu
        ImGui::DragFloat3(
            "Position",
            &m_transform.position.x,
            0.01f);

        // ägëÂèkè¨
        ImGui::DragFloat3(
            "Scale",
            &m_transform.scale.x,
            0.001f,
            0.001f,
            1.0f);

    // Physics
    if (ImGui::CollapsingHeader(
        "Physics", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Checkbox("Use Gravity", &m_useGravity);

        ImGui::DragFloat(
            "Ground Y",
            &m_ground,
            0.001f);

        if (m_useGravity)
        {
            ImGui::DragFloat(
                "Gravity",
                &m_gravity,
                0.1f,
                -30.0f,
                0.0f);
        }

        ImGui::DragFloat(
            "Friction",
            &m_friction,
            0.05f,
            0.0f,
            10.0f);
    }

    // Debug
    if (ImGui::CollapsingHeader("Debug"))
    {
        ImGui::Text(
            "Velocity: %.3f, %.3f, %.3f",
            m_velocity.x,
            m_velocity.y,
            m_velocity.z);

        if (ImGui::Button("Stop Ball"))
        {
            Stop();
        }
    }
}

ImGui::End();
#endif
}

void SoccerBall::SetDribbleVelocity(const DirectX::XMFLOAT3& velocity)
{
    m_velocity.x = velocity.x;
    m_velocity.y = 0.0f;
    m_velocity.z = velocity.z;
}

void SoccerBall::Stop()
{
    m_velocity = { 0.0f,0.0f,0.0f };
}
