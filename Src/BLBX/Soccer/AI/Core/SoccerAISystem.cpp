#include "SoccerAISystem.h"

#include "Soccer/Ball/SoccerBall.h"
#include "Soccer/Player/SoccerPlayer.h"

#include "Soccer/AI/Modules/Defense/DefenderAI.h"
#include "Soccer/AI/Modules/Defense/CoverAI.h"

#ifdef USE_IMGUI
#include <imgui.h>
#endif

void SoccerAISystem::Initialize(int playerCount)
{

	m_modules.clear();

	//前回のチーム情報を初期化
	m_allyTeamBB = BlackBoard{};
	m_enemyTeamBB = BlackBoard{};

	const DirectX::XMFLOAT3 team0Goal{ 0.0f, 0.0f, 10.45f };
	const DirectX::XMFLOAT3 team1Goal{ 0.0f, 0.0f, -10.7f };

	// チーム0
	m_allyTeamBB.Set(TeamKeys::GoalPosition, team0Goal);
	m_allyTeamBB.Set(TeamKeys::DefendGoalPosition, team1Goal);

	// チーム1
	m_enemyTeamBB.Set(TeamKeys::GoalPosition, team1Goal);
	m_enemyTeamBB.Set(TeamKeys::DefendGoalPosition, team0Goal);

	//選手ごとの要求・行動状態を用意
	m_moveResolver.Initialize(playerCount);

	for (auto& module : m_modules)
	{
		if (!module)
		{
			continue;
		}

		module->Initialize();
	}

	//Module1に登録
	m_modules.emplace_back(std::make_unique<DefenderAI>());
	m_modules.emplace_back(std::make_unique<CoverAI>());
}

void SoccerAISystem::Update(std::vector<std::unique_ptr<SoccerPlayer>>& players, SoccerBall* ball, BlackBoard& sharedBB, float elapsedTime)
{
	if (!ball)
	{
		return;
	}

	m_moveResolver.Clear();

	//チームBBからの共有情報の読めるようにする
	m_allyTeamBB.SetParent(&sharedBB);
	m_enemyTeamBB.SetParent(&sharedBB);

	//現在の状況を更新
	UpdateSharedBlackBoard(players, *ball, sharedBB);

	//チーム方針を更新
	m_MetaAI.Update(sharedBB, m_allyTeamBB, m_enemyTeamBB, players, elapsedTime);

	//両チームについて、各Moduleから要求する
	for (int teamNumber = 0; teamNumber < 2; ++teamNumber)
	{
		const BlackBoard& teamBB = teamNumber == 0 ? m_allyTeamBB : m_enemyTeamBB;

		for (auto& module : m_modules)
		{
			if (!module)
			{
				continue;
			}

			module->SubmitRequests(
				teamNumber,
				teamBB,
				players,
				m_moveResolver,
				elapsedTime);

		}
	}

	const int controlledIndex = sharedBB.Get<int>(PlayerKeys::ControlledIndex).value_or(-1);

	//要求を選択して移動を実行
	//操作対象の番号も渡して、二重更新を防ぐ
	m_moveResolver.ResolveAndExecute(players, controlledIndex, elapsedTime);

}

void SoccerAISystem::DrawImGui()
{
#ifdef USE_IMGUI
	m_MetaAI.DrawImGui(m_allyTeamBB, m_enemyTeamBB);

	ImGui::Separator();

	for (auto& module : m_modules)
	{
		if (!module)
		{
			continue;
		}

		ImGui::PushID(module.get());
		module->DrawImGui();
		ImGui::PopID();
	}
#endif
}

void SoccerAISystem::UpdateSharedBlackBoard(const std::vector<std::unique_ptr<SoccerPlayer>>& players, const SoccerBall& ball, BlackBoard& sharedBB)
{
	sharedBB.Set(BallKeys::Position, ball.GetPosition());

	//既に登録されている保持者番号を取得
	const int ownerIndex = sharedBB.Get<int>(BallKeys::OwnerIndex).value_or(-1);

	//保持者がなければ、保持チームなし
	int ownerTeam = -1;

	if (ownerIndex >= 0 && ownerIndex < static_cast<int>(players.size()) && players[ownerIndex])
	{
		ownerTeam = players[ownerIndex]->GetTeamNumber();
	}

	sharedBB.Set(BallKeys::OwnerTeam, ownerTeam);
}
