#include "SoccerBall.h"

#include "Graphics/Graphics.h"

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

    minX = -5.4f;
    maxX = 5.6f;
    minZ = 0.0f;
    maxZ = 10.45f;
    margin = 0.2f;

    insideGoalMinX = -0.6f;
    insideGoalMaxX = 0.8f;

    m_previousPosition = m_transform.position;

    Stop();
}

void SoccerBall::Finalize()
{
	ModelObject::Finalize();
}

void SoccerBall::Update(float elapsedTime)
{
    // 水平移動
    m_transform.position.x += m_velocity.x * elapsedTime;
    m_transform.position.z += m_velocity.z * elapsedTime;

    // 重力
    if (m_useGravity)
    {
        m_velocity.y += m_gravity * elapsedTime;
        m_transform.position.y += m_velocity.y * elapsedTime;

        // 床より下へ行かないようにする
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

    // 水平方向の減速
    float damping = std::max(0.0f, 1.0f - m_friction * elapsedTime);

    m_velocity.x *= damping;
    m_velocity.z *= damping;

    // 水平方向の停止
    if (m_velocity.x * m_velocity.x +
        m_velocity.z * m_velocity.z < 0.000001f)
    {
        m_velocity.x = 0.0f;
        m_velocity.z = 0.0f;
    }

    const float dx = m_transform.position.x - m_previousPosition.x;
    const float dz = m_transform.position.z - m_previousPosition.z;

    m_transform.rotation.x += dz / m_rollRadius;
    m_transform.rotation.z -= dx / m_rollRadius;

    m_previousPosition = m_transform.position;

    BounceOffWalls();
}

void SoccerBall::Render()
{
    if (!m_model)
    {
        return;
    }

    // このモデルに入っている、球の中心のずれ
    constexpr float modelCenterY = 65.8606567f;

    // 中心を原点へ戻してから回転し、元の高さへ戻す
    const auto matrix =
        DirectX::XMMatrixTranslation(0.0f, -modelCenterY, 0.0f) *
        m_transform.GetMatrix() *
        DirectX::XMMatrixTranslation(
            0.0f, modelCenterY * m_transform.scale.y, 0.0f);

    DirectX::XMFLOAT4X4 world;
    DirectX::XMStoreFloat4x4(&world, matrix);

    m_model->render(
        Graphics::Instance().GetContext(),
        world);
}

void SoccerBall::DrawImGui()
{
#ifdef USE_IMGUI

    if (ImGui::CollapsingHeader("Pitch bounds"))
    {
        // 左側・右側を一行で調整
        ImGui::DragFloatRange2(
            "X range",
            &minX, &maxX,
            0.05f, -20.0f, 20.0f,
            "Min: %.2f", "Max: %.2f");

        // 手前側・奥側を一行で調整
        ImGui::DragFloatRange2(
            "Z range",
            &minZ, &maxZ,
            0.05f, -20.0f, 20.0f,
            "Min: %.2f", "Max: %.2f");

        ImGui::DragFloat(
            "Edge margin",
            &margin, 0.01f, 0.0f, 1.0f, "%.2f");

        ImGui::Separator();

        // 結果の大きさも表示
        ImGui::Text("Width : %.2f", maxX - minX);
        ImGui::Text("Length: %.2f", maxZ - minZ);

        ImGui::Text(
            "Usable X: %.2f ~ %.2f",
            minX + margin, maxX - margin);

        ImGui::Text(
            "Usable Z: %.2f ~ %.2f",
            minZ + margin, maxZ - margin);
    }

    {
        //位置
        ImGui::DragFloat3(
            "Position",
            &m_transform.position.x,
            0.01f);

        // 拡大縮小
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
    // ゴールの横幅・高さに収まっているか
    const bool insideGoal =
        m_transform.position.x >= -0.55f &&
        m_transform.position.x <= 0.76f &&
        m_transform.position.y >= 0.0f &&
        m_transform.position.y <= 1.44f;

    //左右の壁
    if ((m_transform.position.x > maxX && m_velocity.x > minZ) ||
        (m_transform.position.x < minX && m_velocity.x < minZ))
    {
        m_velocity.x = -m_velocity.x * m_restitution;
    }

    m_transform.position.x = std::clamp(m_transform.position.x, minX, maxX);


    // ゴールの外なら、通常の奥の壁で反射
    if (m_transform.position.z > maxZ && !insideGoal)
    {
        m_transform.position.z = maxZ;

        if (m_velocity.z > 0.0f)
        {
            m_velocity.z = -m_velocity.z * m_restitution;
        }
    }
    // ゴール内なら奥へ通し、ネット奥の壁で止める
    else if (m_transform.position.z > 10.8f)
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
