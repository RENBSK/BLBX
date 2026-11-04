#include "SoccerWorld.h"
#include "Soccer/AI/Keys/BlackBoardKeys.h"

#include <ImGuiCtrl.h>
#include <imgui.h>

void SoccerWorld::Initialize(const PlayerConfig& config)
{
	//ボール
	m_ball.Initialize();
	//プレイヤー
    PlayerManager::Instance().CreateTeam(0, 2, config);
    PlayerManager::Instance().CreateTeam(1, 2, config);

    auto& manager = PlayerManager::Instance().GetPlayers();

    m_soccerAISystem.Initialize(static_cast<int>(manager.size()));
}

void SoccerWorld::Finalize()
{
	//ボール
	m_ball.Finalize();
	//プレイヤー
    PlayerManager::Instance().Finalize();
}

void SoccerWorld::Update(float elapsedTime)
{
    auto& manager = PlayerManager::Instance();
    auto& bb = manager.GetSharedBlackBoard();

    const int ControledIndex = bb.Get<int>(PlayerKeys::ControlledIndex).value_or(-1);
    
    SoccerPlayer* player = manager.GetPlayer(ControledIndex);

    if (player)
    {
        DirectX::XMFLOAT3 ballPos = m_ball.GetPosition();
        DirectX::XMFLOAT3 playerPos = player->GetPosition();

        DirectX::XMFLOAT3 PlayerToBall =
        {
            ballPos.x - playerPos.x,
            0.0f,
            ballPos.z - playerPos.z
        };

        float dist = DirectX::XMVectorGetX(DirectX::XMVector3Length(DirectX::XMLoadFloat3(&PlayerToBall)));

        constexpr float acquireDistance = 0.2f;
        constexpr float releaseDistance = 0.6f;

        if (dist < acquireDistance)
        {
            manager.SetBallOwner(ControledIndex);
        }
        else if (player->GetHasBall() && dist > releaseDistance)
        {
            manager.SetBallOwner(-1);
        }
    }

    if (player)
    {
        // 移動方向を取得
        DirectX::XMFLOAT3 direction =
            m_playerController.GetMoveDirection();

        const int ownerIndex = bb.Get<int>(
            BallKeys::OwnerIndex).value_or(-1);

        const bool hasBall = ownerIndex == ControledIndex;

        // 選手はタッチで確定した方向へ進む
        const DirectX::XMFLOAT3 moveDirection = hasBall
            ? m_dribbleController.GetPlayerMoveDirection(direction) : direction;

        // プレイヤーの移動・旋回
        player->UpdateMovement(
            moveDirection,
            player->GetParameters().maxSpeed,
            elapsedTime);

        if (hasBall)
        {

            m_dribbleController.UpdateDribble(
                *player,
                m_ball,
                player->GetVelocity(),
                direction,
                elapsedTime);
        }
        else
        {
            m_dribbleController.Reset();
        }
    }
    else
    {
        m_dribbleController.Reset();
    }

    // ボール更新
    m_ball.Update(elapsedTime);

    m_soccerAISystem.Update(manager.GetPlayers(), &m_ball, bb, elapsedTime);

    // プレイヤーのアニメーション更新
    PlayerManager::Instance().Update(elapsedTime);
}

void SoccerWorld::Render()
{
	//ボール
	m_ball.Render();
	//プレイヤー
    PlayerManager::Instance().Render();
}

void SoccerWorld::DrawImGui(bool showBall, bool showPlayer)
{
#ifdef USE_IMGUI

    ImGui::SetNextWindowSize(
        ImVec2(650.0f, 600.0f),
        ImGuiCond_FirstUseEver);

    if (ImGui::Begin("Soccer Debug"))
    {
        if (ImGui::BeginTabBar("DebugTabs"))
        {
            if (showPlayer && ImGui::BeginTabItem("Players"))
            {
                ImGui::PushID("Players");
                PlayerManager::Instance().DrawImGui();
                ImGui::PopID();

                ImGui::EndTabItem();
            }

            if (showBall && ImGui::BeginTabItem("Ball"))
            {
                ImGui::PushID("Ball");
                m_ball.DrawImGui();
                ImGui::PopID();

                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("AI"))
            {
                m_soccerAISystem.DrawImGui();
                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }
    }

    ImGui::End();
#endif
}


