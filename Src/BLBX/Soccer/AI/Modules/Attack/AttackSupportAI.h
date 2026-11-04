#pragma once

#include"Soccer/AI/Core/SoccerAIModule.h"

class AttackAupportAI : public SoccerAIModule
{
public:
	void Initialize() override;

	void SubmitRequests(
		int  TeamNumber,
		const BlackBoard& blackboard,
		std::vector<std::unique_ptr<SoccerPlayer>>& players,
		AIMoveResolver& resolver,
		float elapsedTime) override;

	void DrawImGui() override;
};