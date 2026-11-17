#include "SoccerBall.h"

#include <algorithm>
#include <cmath>

#ifdef USE_IMGUI
#include "imgui.h"
#endif // USE_IMGUI

void SoccerBall::Initialize()
{
	LoadModel("./Data/Model/Ball/ball.gltf", true);

    m_transform.position = { 0.09f,m_ground,-0.1f };
	m_transform.rotation = { 0.0f,0.0f,0.0f };
	m_transform.scale = { 0.0015f,0.0015f,0.0015f };

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
    float damping = std::max(0.0f, 1.0f - m_friction * elapsedTime);

    m_velocity.x *= damping;
    m_velocity.z *= damping;

    // êÖïΩï˚å¸ÇÃí‚é~
    if (m_velocity.x * m_velocity.x +
        m_velocity.z * m_velocity.z < 0.000001f)
    {
        m_velocity.x = 0.0f;
        m_velocity.z = 0.0f;
    }

    BounceOffWalls();
}

void SoccerBall::Render()
{
    ModelObject::Render();
}

void SoccerBall::DrawImGui()
{
#ifdef USE_IMGUI
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

#endif
}

void SoccerBall::BounceOffWalls()
{
    //Ç±ÇÍÇ™ï«ÇÃç¿ïW
    {
        constexpr float minX = -5.4f;
        constexpr float maxX = 5.6f;
        constexpr float minZ = 0.0f;
        constexpr float maxZ = 10.45f;
        constexpr float margin = 0.2f;
    }


    //ç∂âEÇÃï«
    if ((m_transform.position.x > 5.6f && m_velocity.x > 0.0f) ||
        (m_transform.position.x < -5.4f && m_velocity.x < 0.0f))
    {
        m_velocity.x = -m_velocity.x * m_restitution;
    }

    m_transform.position.x = std::clamp(m_transform.position.x, -5.4f, 5.6f);

    const bool insideGoalWidth = m_transform.position.x >= -0.6f && m_transform.position.x <= 0.8f;

    //âúÇÃï«
    if ((m_transform.position.z > 10.45f)&&insideGoalWidth)
    {
        m_transform.position.z = 10.45f;

        if (m_velocity.z > 0.0f)
        {
            m_velocity.z = -m_velocity.z * m_restitution;
        }
    }
    else if (m_transform.position.z > 10.8)
    {
        m_transform.position.z = 10.8f;
        if (m_velocity.z > 0.0f)
        {
            m_velocity.z = -m_velocity.z * m_restitution;
        }

    }


}

void SoccerBall::SetDribbleVelocity(const DirectX::XMFLOAT3& velocity)
{
    m_velocity.x = velocity.x;
    m_velocity.y = 0.0f;
    m_velocity.z = velocity.z;
}

void SoccerBall::Kick(const DirectX::XMFLOAT3& velocity)
{
    m_velocity = velocity;
}

void SoccerBall::Stop()
{
    m_velocity = { 0.0f,0.0f,0.0f };
}
