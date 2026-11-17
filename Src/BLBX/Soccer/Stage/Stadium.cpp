#include "Stadium.h"

#include <imgui.h>

void Stadium::Initialize()
{
	LoadModel("./Data/Model/futsal_court/test.glb", true);
	//LoadModel("C:/Users/2240216/Desktop/external/estadio_unico_madre_de_ciudades/Soccer.RK.glb", true);

	m_transform.position = { 0.105f,0.64f,-0.11f };
	m_transform.rotation = { 0.0f,0.0f,0.0f };
	m_transform.scale = { 0.5f,0.5f,0.5f };
}

void Stadium::Update(float elapsedTime)
{

}

void Stadium::Render()
{
	//描画スイッチ
	if (!m_visible) return;
	//描画
	ModelObject::Render();
}

void Stadium::DrawImGui()
{
    ImGui::SetNextWindowSize(ImVec2(360, 300), ImGuiCond_FirstUseEver);

    if (ImGui::Begin("Stadium"))
    {
        // 表示設定
        ImGui::SeparatorText("Display");
        ImGui::Checkbox("Visible", &m_visible);

        ImGui::Spacing();

        // Transform
        if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen))
        {
            // 位置
            ImGui::DragFloat3(
                "Position",
                &m_transform.position.x,
                0.01f
            );

            // 回転 (ラジアン → 度)
            float rotation[3] =
            {
                DirectX::XMConvertToDegrees(m_transform.rotation.x),
                DirectX::XMConvertToDegrees(m_transform.rotation.y),
                DirectX::XMConvertToDegrees(m_transform.rotation.z)
            };

            if (ImGui::DragFloat3("Rotation", rotation, 1.0f, 0.0f, 0.0f, "%.1f deg"))
            {
                m_transform.rotation.x = DirectX::XMConvertToRadians(rotation[0]);
                m_transform.rotation.y = DirectX::XMConvertToRadians(rotation[1]);
                m_transform.rotation.z = DirectX::XMConvertToRadians(rotation[2]);
            }

            // 拡大縮小
            ImGui::DragFloat3(
                "Scale",
                &m_transform.scale.x,
                0.01f
            );

            ImGui::Spacing();

            // 初期位置に戻す
            if (ImGui::Button("Reset Transform"))
            {
                m_transform.position = { 0.105f, 0.64f, -0.11f };
                m_transform.rotation = { 0.0f, 0.0f, 0.0f };
                m_transform.scale = { 0.5f, 0.5f, 0.5f };
            }
        }
    }

    ImGui::End();
}

