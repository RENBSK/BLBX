#pragma once

#include "Soccer/AI/Core/SoccerAIModule.h"
#include "Soccer/AI/Modules/CalAIeva.h"


class CoverAI : public SoccerAIModule
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
	float m_MarkDistance = 0.5f;
	float m_MoveSpeed = 2.5f;


	struct EvaluationDebug
	{
		bool active = false;
		int playerIndex = -1;

		float coverScore = 0.0f;
		float repositionNeed = 0.0f;

		Evaluation evaluation{};
	};

	EvaluationDebug m_evaluationDebug[2]{};

	CalAIeva m_calAIeva;

	bool m_isSteppingIn[2]{};

	float m_coverWeight = 0.5f;
};
