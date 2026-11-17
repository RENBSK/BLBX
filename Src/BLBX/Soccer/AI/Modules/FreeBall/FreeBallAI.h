#pragma once

#include "Soccer/AI/Core/SoccerAIModule.h"
#include "Soccer/AI/Modules/CalAIeva.h"


class FreeBallAI : public SoccerAIModule
{
public:
	void Initialize() override;

	void SubmitRequests(
		int  TeamNumber,
		const BlackBoard& blackboard,
		std::vector<std::unique_ptr<SoccerPlayer>>& players,
		const std::vector<int>& markTargets,
		AIMoveResolver& resolver,
		float elapsedTime) override;

	void DrawImGui() override;

	const char* GetName() const override { return "Free Ball AI"; }
};
