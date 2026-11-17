#pragma once

#include"Soccer/AI/Core/SoccerAIModule.h"
#include "Soccer/AI/Modules/CalAIeva.h"

class AttackAupportAI : public SoccerAIModule
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

private:
	CalAIeva m_calAIeva;

	float m_forwardDistance = 5.0f;
	float m_sideDistance = 3.0f;
	float m_MoveSpeed = 5.0f;

	bool m_useLeft[2]{};

	std::optional<float> m_debugMinSideSlip[2];
	std::optional<float> m_debugRightMinSideSlip[2];
	std::optional<float> m_debugLeftMinSideSlip[2];

};