#pragma once

#include <vector>
#include <memory>

#include "Soccer/AI/Core/BlackBoard.h"
#include "Soccer/AI/Core/SoccerAIModule.h"
#include "Soccer/AI/Request/AIMoveResolver.h"
#include "Soccer/AI/Meta/Meta.h"

#include "Soccer/AI/Keys/BlackBoardKeys.h"



class SoccerPlayer;
class SoccerBall;

class SoccerAISystem
{
public:
	void Initialize(int playerCount);

	void Update(
		std::vector<std::unique_ptr<SoccerPlayer>>& players,
		SoccerBall* ball,
		BlackBoard& sharedBB,
		float elapsedTime);

	void DrawImGui();

private:
	void UpdateSharedBlackBoard(const std::vector<std::unique_ptr<SoccerPlayer>>& players,
		const SoccerBall& ball,
		BlackBoard& sharedBB);

private:
	BlackBoard m_allyTeamBB;
	BlackBoard m_enemyTeamBB;

	AIMoveResolver m_moveResolver;

	Meta m_MetaAI;

	std::vector<std::unique_ptr<SoccerAIModule>> m_modules;
};
