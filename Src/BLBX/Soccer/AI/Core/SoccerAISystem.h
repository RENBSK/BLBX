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
class PrimitiveRenderer;

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

public:
	void DrawDebug(
		PrimitiveRenderer& renderer,
		const std::vector<std::unique_ptr<SoccerPlayer>>& players) const;

	void DrawDebugOverlay(
		const std::vector<std::unique_ptr<SoccerPlayer>>& players) const;

	void DrawRequestImGui();

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

	bool m_showRoleLabels = true;
	bool m_showMoveTargets = true;
	bool m_showRequestTargets = false;

	// -1なら全選手。Base・Softの詳細表示に使用
	int m_debugPlayerIndex = -1;

	//Meta機能の切り替え
	bool m_enableMeta = true;
};
