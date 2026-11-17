#include "PlayerManager.h"

#include "Soccer/Ball/SoccerBall.h"

#ifdef USE_IMGUI
#include <imgui.h>
#endif

void PlayerManager::CreateTeam(
	int teamNumber, 
	int playerCount, 
	const PlayerConfig& config)
{
	for (int i = 0; i < playerCount; ++i)
	{
		auto player = std::make_unique<SoccerPlayer>();

		player->SetConfig(config);
		player->SetTeamNumber(teamNumber);
		player->Initialize();

		float x = static_cast<float>(i) * 1.0f;
		float z = (teamNumber == 0) ? -2.0f : 2.0f;

		player->SetPosition({ x,0.64f,z });
		player->SetScale({ 0.005f,0.005f,0.005f });
		
		players.push_back(std::move(player));
	}
}

void PlayerManager::SetBallOwner(int ownerIndex)
{
    SoccerPlayer* player = GetPlayer(ownerIndex);

    if (!player)
    {
        ownerIndex = -1;
    }

    for (int i = 0; i < static_cast<int>(players.size()); ++i)
    {
        players[i]->SetHasBall(i == ownerIndex);
    }

    sharedBB->Set(BallKeys::OwnerIndex, ownerIndex);
    sharedBB->Set(BallKeys::OwnerTeam, player ? player->GetTeamNumber() : -1);

    if (ownerIndex >= 0)
    {
        ClearPass();
    }
}

void PlayerManager::Finalize()
{
    for (const auto& player : players)
    {
        player->Finalize();
    }

    players.clear();

    sharedBB->Set(PlayerKeys::ControlledIndex, -1);
    sharedBB->Set(BallKeys::OwnerIndex, -1);
    sharedBB->Set(BallKeys::OwnerTeam, -1);
}

void PlayerManager::Update(float elapsedTime)
{
	for (const std::unique_ptr<SoccerPlayer>& player : players)
	{
		player->Update(elapsedTime);
	}
}

void PlayerManager::Render()
{
	for (const std::unique_ptr<SoccerPlayer>& player : players)
	{
		player->Render();
	}
}

void PlayerManager::DrawImGui(SoccerBall& ball)
{
#ifdef USE_IMGUI
    if (players.empty())
    {
        ImGui::TextUnformatted("No players");
        return;
    }

    const int maxIndex = static_cast<int>(players.size()) - 1;

    // 操作対象
    int controlledIndex = sharedBB->Get<int>(
        PlayerKeys::ControlledIndex).value_or(-1);

    if (ImGui::SliderInt(
        "Controlled player",
        &controlledIndex,
        -1,
        maxIndex,
        "%d",
        ImGuiSliderFlags_AlwaysClamp))
    {
        sharedBB->Set(PlayerKeys::ControlledIndex, controlledIndex);
    }

    // ボール保持者：デバッグ用の指定
    int ownerIndex = sharedBB->Get<int>(
        BallKeys::OwnerIndex).value_or(-1);

    if (ImGui::SliderInt(
        "Ball owner",
        &ownerIndex,
        -1,
        maxIndex,
        "%d",
        ImGuiSliderFlags_AlwaysClamp))
    {
        /*sharedBB->Set(BallKeys::OwnerIndex, ownerIndex);

        SoccerPlayer* owner = GetPlayer(ownerIndex);

        sharedBB->Set(
            BallKeys::OwnerTeam,
            owner ? owner->GetTeamNumber() : -1);*/

        SetBallOwner(ownerIndex);

        if (SoccerPlayer* owner = GetPlayer(ownerIndex))
        {
            const auto pos = owner->GetPosition();
            const auto forward = owner->GetForward();

            ball.SetPosition({
                pos.x + forward.x * 0.2f,
                ball.GetPosition().y,
                pos.z + forward.z * 0.2f
                });

            ball.Stop();
        }
    }

    ImGui::TextDisabled("-1 = None");
    ImGui::Separator();

    // 設定を確認する選手
    if (inspectedPlayerIndex < 0 || inspectedPlayerIndex > maxIndex)
    {
        inspectedPlayerIndex = 0;
    }

    std::string preview =
        "Player " + std::to_string(inspectedPlayerIndex);

    if (ImGui::BeginCombo("Inspect player", preview.c_str()))
    {
        for (int i = 0; i <= maxIndex; ++i)
        {
            SoccerPlayer* player = GetPlayer(i);

            std::string label =
                "Player " + std::to_string(i) +
                " / Team " + std::to_string(player->GetTeamNumber());

            if (ImGui::Selectable(
                label.c_str(),
                inspectedPlayerIndex == i))
            {
                inspectedPlayerIndex = i;
            }
        }

        ImGui::EndCombo();
    }

    ImGui::Separator();

    // 選んだ選手の既存設定を表示
    if (SoccerPlayer* player = GetPlayer(inspectedPlayerIndex))
    {
        ImGui::PushID(inspectedPlayerIndex);
        player->DrawImGui();
        ImGui::PopID();
    }
#endif
}

void PlayerManager::ClearPass()
{
    sharedBB->Set(BallKeys::PassTeam, -1);
    sharedBB->Set(BallKeys::PassTarget, -1);
}

