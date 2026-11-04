#pragma once

class BlackBoard;

class SoccerPlayer;

#include <vector>
#include <memory>

class Meta
{
public:
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

	void DrawImGui(const BlackBoard& allyTeamBB, const BlackBoard& enemyTeamBB);

private:
	struct PressAssignmentState
	{
		int candidateIndex = -1;
		float timer = 0.0f;
	};

	PressAssignmentState m_pressStates[2];

	float m_PressSwitchTimer = 2.0f;
};
