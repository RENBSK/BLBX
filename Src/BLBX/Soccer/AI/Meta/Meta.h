#pragma once

#include "Soccer/AI/Modules/CalAIeva.h"

class BlackBoard;

class SoccerPlayer;

class PrimitiveRenderer;

#include <vector>
#include <memory>

class Meta
{
public:
	void  Initialize();

	void Update(
		const BlackBoard& sharedBB, 
		BlackBoard& allyTeamBB,
		BlackBoard& enemyTeamBB,
		std::vector<std::unique_ptr<SoccerPlayer>>& players,
		float elapsedTime);

	void FindPressPlayerIndex(
		const BlackBoard& sharedBB,
		BlackBoard& allyTeamBB,
		BlackBoard& enemyTeamBB,
		std::vector < std::unique_ptr<SoccerPlayer>>& players,
		float elapsedTime);

	void FindDefenderSupportPlayerIndex(
		const BlackBoard& sharedBB,
		BlackBoard& allyTeamBB,
		BlackBoard& enemyTeamBB,
		std::vector < std::unique_ptr<SoccerPlayer>>& players,
		float elapsedTime);

	void FindAttackSupportPlayerIndex(
		const BlackBoard& sharedBB,
		BlackBoard& allyTeamBB,
		BlackBoard& enemyTeamBB,
		std::vector < std::unique_ptr<SoccerPlayer>>& players,
		float elapsedTime);

	void AssignMarks(
		const BlackBoard& sharedBB,
		BlackBoard& allyTeamBB,
		BlackBoard& enemyTeamBB,
		std::vector < std::unique_ptr<SoccerPlayer>>& players,
		float elapsedTime);

	void AssignAttack(
		const BlackBoard& sharedBB,
		BlackBoard& allyTeamBB,
		BlackBoard& enemyTeamBB,
		std::vector < std::unique_ptr<SoccerPlayer>>& players,
		float elapsedTime);

	void AssainFreeBallPlayerIndex(
		const BlackBoard& sharedBB,
		BlackBoard& allyTeamBB,
		BlackBoard& enemyTeamBB,
		std::vector < std::unique_ptr<SoccerPlayer>>& players,
		float elapsedTime);

	void DrawImGui(const BlackBoard& allyTeamBB, const BlackBoard& enemyTeamBB);

	void DrawDebug(
		PrimitiveRenderer& renderer,
		const std::vector<std::unique_ptr<SoccerPlayer>>& players,
		const BlackBoard& allyTeamBB,
		const BlackBoard& enemyTeamBB) const;

	const std::vector<int>& GetMarkTargets() const
	{
		return m_markTargets;
	}

private:
	struct PressAssignmentState
	{
		int candidateIndex = -1;
		float timer = 0.0f;
	};

	PressAssignmentState m_pressStates[2];

	//担当交代を決行するまでの時間
	float m_PressSwitchTimer = 0.3f;

	CalAIeva m_calAIeva;

	// 自分の選手index → マーク相手のindex
	std::vector<int> m_markTargets;

	// チームごとの選手index。選考とImGuiで使う
	std::vector<int> m_markPlayers[2];

	// 担当を１人変更する負担
	float m_markSwitchCost = 0.25f;

	
};
