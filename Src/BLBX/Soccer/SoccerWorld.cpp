#include "SoccerWorld.h"
#include "Soccer/AI/Keys/BlackBoardKeys.h"

#include "Graphics/Camera/Camera.h"
#include "Scene/SceneManager.h"
#include "Scene/SceneGame.h"
#include "Scene/SceneTitle.h"

#include <ImGuiCtrl.h>
#include <imgui.h>
#include <limits>
#include <Windows.h>

void SoccerWorld::Initialize(const PlayerConfig& config)
{
	//ボール初期化
	m_ball.Initialize();
	//プレイヤー初期化
    PlayerManager::Instance().CreateTeam(0, PLAYER_COUNT, config);
    PlayerManager::Instance().CreateTeam(1, PLAYER_COUNT, config);
    //選手を管理するPlayerManager本体を取得
    auto& playerManager = PlayerManager::Instance();
    //Managerが持つ選手一覧を参照で取得
    auto& manager = PlayerManager::Instance().GetPlayers();
    //AIシステム初期化
    m_soccerAISystem.Initialize(static_cast<int>(manager.size()));
    //開始選手(KickOffでボールを持たせる選手)
    SoccerPlayer* starter = playerManager.GetPlayer(0);
    //プレイヤーを初期位置に設定
    PlacePlayers();
    //画像
    sprites = std::make_unique<Sprite>(Graphics::Instance().GetDevice(), L"./Data/Sprite/Goal.png");

    if (starter)
    {
        m_ball.Stop();

        playerManager.GetSharedBlackBoard().Set(
            PlayerKeys::ControlledIndex, 0);

        playerManager.SetBallOwner(0);
    }

    //変数の初期化
    m_passElapsed = 0.0f;
    m_kickoffTargetIndex = -1;
    m_dribbleController.Reset();
    m_matchPhase = MatchPhase::KickOff;
    m_spriteX = SCREEN_WIDTH / 4;
    m_spriteY = SCREEN_HEIGHT / 4;
    m_spriteW = SCREEN_WIDTH / 2;
    m_spriteH = SCREEN_HEIGHT / 2;
    
    m_isGoal = false;
    m_goalElapsed = 0.0f;
    m_drawImGui = false;
}

void SoccerWorld::Finalize()
{
	//ボール終了化
	m_ball.Finalize();
	//プレイヤー終了化
    PlayerManager::Instance().Finalize();
}

void SoccerWorld::Update(float elapsedTime)
{
    // ゴール後はゲームの更新を止める
    if (m_isGoal)
    {
        m_goalElapsed += elapsedTime;
        //仮に四秒後にタイトルに戻す。
        if (m_goalElapsed > 4.0f)
        {
            SceneManager::Instance().ChangeScene(new SceneTitle);
        }
        return;
    }

    auto& manager = PlayerManager::Instance();
    auto& bb = manager.GetSharedBlackBoard();

    if (m_matchPhase == MatchPhase::KickOff)
    {
        UpdateKickoff();
        m_ball.Update(elapsedTime);
        manager.Update(elapsedTime);

        return;
    }
    //ボール取得
    UpdateBallOwner();

    const int ownerIndex = bb.Get<int>(BallKeys::OwnerIndex).value_or(-1);

    SoccerPlayer* owner = manager.GetPlayer(ownerIndex);

    const int currentControlPlayer = bb.Get<int>(PlayerKeys::ControlledIndex).value_or(-1);

    if (owner && owner->GetTeamNumber() == 0 && currentControlPlayer != ownerIndex)
    {
        bb.Set(PlayerKeys::ControlledIndex, ownerIndex);
        m_dribbleController.Reset();
    }

    const int ControledIndex = bb.Get<int>(PlayerKeys::ControlledIndex).value_or(-1);

    SoccerPlayer* player = manager.GetPlayer(ControledIndex);


    if (player)
    {
        //パス相手探し
        const int supportIndex = FinPassTarget(*player);
        SoccerPlayer* support = manager.GetPlayer(supportIndex);

        const auto state = player->GetAnimationState();

        bool pass = state == PlayerAnimationState::Pass_Right
            || state == PlayerAnimationState::Pass_Left;

        if (pass)
        {

        }

        const DirectX::XMFLOAT3 GoalPos = bb.Get<DirectX::XMFLOAT3>(TeamKeys::GoalPosition).value_or(DirectX::XMFLOAT3{});

        if (m_playerController.Update(*player, m_ball, support, m_dribbleController, GoalPos, elapsedTime))
        {
            manager.SetBallOwner(-1);
            const auto state = player->GetAnimationState();

            //パスをしたなら、ブラックボードに情報を設定する。
            if (state == PlayerAnimationState::Pass_Right ||
                state == PlayerAnimationState::Pass_Left)
            {
                bb.Set(BallKeys::PassTeam, player->GetTeamNumber());
                bb.Set(BallKeys::PassTarget, supportIndex);
                bb.Set(PlayerKeys::ControlledIndex, supportIndex);
                m_passElapsed = 0.0f;
            }
        }

    }
    else
    {
        m_dribbleController.Reset();
    }

    // ボール更新
    m_ball.Update(elapsedTime);

    if (IsGoal())
    {
        m_isGoal = true;
        m_goalElapsed = 0.0f;
        m_ball.Stop();
        return;
    }
    //パス処理
    UpdatePass(elapsedTime);
    //AIシステム更新
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

    auto& graphics = Graphics::Instance();
    auto* renderer = graphics.GetPrimitiveRenderer();

    if (!renderer)
    {
        return;
    }

    m_soccerAISystem.DrawDebug(
        *renderer,
        PlayerManager::Instance().GetPlayers());

    const auto& camera = Camera::Instance();

    // デバッグ線は、モデルや地面に隠れず見えるようにする
    graphics.SetDepthState(1);

    renderer->Render(
        graphics.GetContext(),
        camera.GetView(),
        camera.GetProjection(),
        D3D11_PRIMITIVE_TOPOLOGY_LINELIST);

    graphics.SetDepthState(0);

    if (m_isGoal && sprites)
    {
        float t = m_goalElapsed / m_goalDuration;

        if (t > 1.0f)
        {
            t = 1.0f;
        }

        // 最初は速く、最後はゆっくり拡大
        float scale = 1.0f - (1.0f - t) * (1.0f - t) * (1.0f - t);

        float width = m_spriteW * scale;
        float height = m_spriteH * scale;

        // 中心を固定して拡大
        float x = m_spriteX + (m_spriteW - width) * 0.5f;
        float y = m_spriteY + (m_spriteH - height) * 0.5f;

        sprites->render(
            Graphics::Instance().GetContext(),
            x, y,
            width, height,
            1, 1, 1, 1,
            0);

        m_drawImGui = true;
    }
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
                PlayerManager::Instance().DrawImGui(m_ball);
                m_playerController.DrawImgui();
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

        ImGui::DragFloat(
            "SpriteX",
            &m_spriteX,
            0.1f);

        ImGui::DragFloat(
            "SpriteY",
            &m_spriteY,
            0.1f);

        ImGui::DragFloat(
            "SpriteW",
            &m_spriteW,
            0.1f);

        ImGui::DragFloat(
            "SpriteH",
            &m_spriteH,
            0.1f);

    ImGui::End();

    // チェックボックスの設定を使って描画
    if (!m_drawImGui)
    {
        m_soccerAISystem.DrawDebugOverlay(PlayerManager::Instance().GetPlayers());
    }

#endif
}

void SoccerWorld::UpdateBallOwner()
{
    auto& manager = PlayerManager::Instance();
    auto& bb = manager.GetSharedBlackBoard();
    const auto ballPos = m_ball.GetPosition();

    //ボールの取得処理
    constexpr float acquireDistance = 0.2f;
    constexpr float releaseDistance = 0.6f;

    const int ownerIndex = bb.Get<int>(BallKeys::OwnerIndex).value_or(-1);

    SoccerPlayer* owner = manager.GetPlayer(ownerIndex);

    if (owner)
    {
        const auto ownerPos = owner->GetPosition();

        const float dx = ballPos.x - ownerPos.x;
        const float dz = ballPos.z - ownerPos.z;

        const float length = std::sqrt(dx * dx + dz * dz);

        if (length > releaseDistance)
        {
            manager.SetBallOwner(-1);
        }

        return;
    }

    const auto& players = manager.GetPlayers();

    //保持者がいなければ、一番近い人を探す。
    int bestIndex = -1;
    float bestLength = std::numeric_limits<float>::infinity();

    for (int i = 0; i < static_cast<int>(players.size()); ++i)
    {
        if (!players[i])
        {
            continue;
        }

        DirectX::XMFLOAT3 playerPos = players[i]->GetPosition();

        const float dx = ballPos.x - playerPos.x;
        const float dz = ballPos.z - playerPos.z;

        const float length = std::sqrt(dx * dx + dz * dz);

        if (length < bestLength)
        {
            bestLength = length;
            bestIndex = i;
        }
    }

    if (bestIndex >= 0 && bestLength < acquireDistance)
    {
        manager.SetBallOwner(bestIndex);
        if (players[bestIndex]->GetTeamNumber() == 0)
        {
            bb.Set(BallKeys::OwnerIndex, bestIndex);
            m_dribbleController.Reset();
        }
        else
        {
            SceneManager::Instance().ChangeScene(new SceneGame);
        }
    }

}

void SoccerWorld::UpdatePass(float elapsedTime)
{


    auto& manager = PlayerManager::Instance();
    auto& bb = manager.GetSharedBlackBoard();

    //パスをミスしたり、届かなかった場合は解除
    const int passTarget = bb.Get<int>(BallKeys::PassTarget).value_or(-1);

    if (passTarget >= 0)
    {
        m_passElapsed += elapsedTime;

        const auto velocity = m_ball.GetVelocity();
        const float speedSq = velocity.x * velocity.x + velocity.z * velocity.z;

        constexpr float stopSpeed = 0.1f;
        constexpr float maxPassTime = 2.0f;

        const bool stopped = speedSq < stopSpeed * stopSpeed;
        const bool timedOut = m_passElapsed >= maxPassTime;
        const bool invalidTarget = manager.GetPlayer(passTarget) == nullptr;

        if (stopped || timedOut || invalidTarget)
        {
            manager.ClearPass();
            m_passElapsed = 0.0f;
        }
    }
    else
    {
        m_passElapsed = 0.0f;
    }
}

int SoccerWorld::FinPassTarget(SoccerPlayer& player)
{
    auto& manager = PlayerManager::Instance();
    const auto& players = manager.GetPlayers();

    //パス相手の設定
    int supportIndex = -1;
    float bestScore = -std::numeric_limits<float>::infinity();

    const auto passerPos = player.GetPosition();

    auto aimDirection = m_playerController.GetMoveDirection();

    // 入力がなければ選手の向きを使う
    if (aimDirection.x * aimDirection.x +
        aimDirection.z * aimDirection.z < 0.00001f)
    {
        aimDirection = player.GetForward();
    }

    for (int i = 0; i < static_cast<int>(players.size()); ++i)
    {
        SoccerPlayer* candidate = players[i].get();

        if (!candidate || candidate == &player ||
            candidate->GetTeamNumber() != player.GetTeamNumber())
        {
            continue;
        }

        const auto candidatePos = candidate->GetPosition();

        const float dx = candidatePos.x - passerPos.x;
        const float dz = candidatePos.z - passerPos.z;

        const float distance = std::sqrt(dx * dx + dz * dz);

        if (distance < 0.00001f)
        {
            continue;
        }

        // 味方への方向を正規化して、狙い方向との内積を取る
        const float dot = (dx * aimDirection.x + dz * aimDirection.z) / distance;

        // 狙い方向より後ろの味方は除外
        if (dot <= 0.0f)
        {
            continue;
        }

        // 近いほど1に近くなる
        const float nearScore = 1.0f / (1.0f + distance);

        // 仮の配分。方向を重視する
        const float score = dot * 0.6f + nearScore * 0.4f;

        if (score > bestScore)
        {
            bestScore = score;
            supportIndex = i;
        }
    }

    return supportIndex;
}

bool SoccerWorld::IsGoal() const
{
    const auto position = m_ball.GetPosition();

    // ゴール幅・高さの範囲内で、奥のラインへ到達
    return position.x >= -0.55f &&
        position.x <= 0.76f &&
        position.y >= 0.0f &&
        position.y <= 1.44f &&
        position.z >= 10.78f;
}

void SoccerWorld::UpdateKickoff()
{
    auto& manager = PlayerManager::Instance();
    auto& bb = manager.GetSharedBlackBoard();

    SoccerPlayer* starter = manager.GetPlayer(0);

    if (!starter )
    {
        return;
    }

    const auto state = starter->GetAnimationState();

    const bool isPassing =
        state == PlayerAnimationState::Pass_Right ||
        state == PlayerAnimationState::Pass_Left;

    const bool passInput = (GetAsyncKeyState('P') & 0x8000) != 0;

    // 開始した瞬間の入力方向で相手を決める
    if (!isPassing)
    {
        const auto direction = m_playerController.GetMoveDirection();

        if (direction.x * direction.x + direction.z * direction.z > 0.001f)
        {
            starter->FaceToMoveDirection(direction);
        }

        if (!passInput)
        {
            return;
        }

        m_kickoffTargetIndex = FinPassTarget(*starter);

        
    }

    SoccerPlayer* receiver = manager.GetPlayer(m_kickoffTargetIndex);

    if (!receiver)
    {
        return;
    }

    // 実際に蹴るタイミングまではfalseが返る
    if (!starter->Pass(m_ball, receiver->GetPosition(), 10.0f))
    {
        return;
    }

    // ここからは、実際に蹴った瞬間の処理
    manager.SetBallOwner(-1);

    bb.Set(BallKeys::PassTeam, starter->GetTeamNumber());
    bb.Set(BallKeys::PassTarget, m_kickoffTargetIndex);
    bb.Set(PlayerKeys::ControlledIndex, m_kickoffTargetIndex);

    m_passElapsed = 0.0f;
    m_dribbleController.Reset();

    m_kickoffTargetIndex = -1;
    m_matchPhase = MatchPhase::Playing;
}

void SoccerWorld::PlacePlayers()
{
    auto& manager = PlayerManager::Instance();

    SoccerPlayer* starter = manager.GetPlayer(0);

    if (!starter)
    {
        return;
    }

    const auto ballPos = m_ball.GetPosition();
    const auto forward = starter->GetForward();

    const DirectX::XMFLOAT3 startPosition[2][3] =
    {

        {
            {ballPos.x,starter->GetPosition().y,ballPos.z - 0.2f}, // Player0：中央のボール
            { -2.0f, 0.64f,  1.5f }, // Player1：左前
            {  2.0f, 0.64f,  1.5f }  // Player2：右前
        },
        {
            { -0.0f, 0.64f, 4.5f },  // Player3
            {  -2.0f, 0.64f, 5.5 },  // Player4
            {  2.0f, 0.64f, 5.5f }   // Player5
        }
    };

    for (int team = 0; team < 2; ++team)
    {
        for (int i = 0; i < PLAYER_COUNT && i < 3; ++i)
        {
            SoccerPlayer* player = manager.GetPlayer(team * PLAYER_COUNT + i);

            if (!player)
            {
                continue;
            }

            player->SetPosition(startPosition[team][i]);

            player->FaceToMoveDirection({ 0.0f,0.0f,team == 0 ? 1.0f : -1.0f });
        }
    }
}


