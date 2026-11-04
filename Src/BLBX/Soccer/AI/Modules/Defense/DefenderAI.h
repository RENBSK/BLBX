#pragma once

#include "Soccer/AI/Core/SoccerAIModule.h"
#include "Soccer/AI/Modules/CalAIeva.h"

class DefenderAI : public SoccerAIModule
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

private:
	struct DefenderState
	{
		float timer = 0.0f;
		bool hasTarget = false;
		DirectX::XMFLOAT3 targetPosition{};
	};

	struct EvaluationDebug
	{
		bool active = false;
		int playerIndex = -1;
		Evaluation evaluation{};
	};

	CalAIeva m_calAIeva;

	EvaluationDebug m_evaluationDebug[2]{};

	std::unordered_map<int, DefenderState> m_states;

	float m_MarkDistance = 1.0f;
	float m_PressDistance = 0.5f;
	float m_PressAngle = 120.0f;
	float m_MoveSpeed = 2.25f;
	float m_TargetUpdateInterval = 0.1f;
};