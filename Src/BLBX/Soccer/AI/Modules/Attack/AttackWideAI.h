#pragma once

#include"Soccer/AI/Core/SoccerAIModule.h"
#include "Soccer/AI/Modules/CalAIeva.h"

class AttackWideAI : public SoccerAIModule
{
public:
	void SubmitRequests(
		int  TeamNumber,
		const BlackBoard& blackboard,
		std::vector<std::unique_ptr<SoccerPlayer>>& players,
		const std::vector<int>& markTargets,
		AIMoveResolver& resolver,
		float elapsedTime) override;

	void DrawImGui() override;


private:
	CalAIeva m_calAI;

	float m_WodeDistance = 3.5f;
};