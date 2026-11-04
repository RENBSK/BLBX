#pragma once

#include <vector>
#include <memory>

#include "Soccer/AI/Request/AIMoveRequest.h"

class SoccerPlayer;

struct AIMoveRequestDebug
{
	AIMoveRequestType type = AIMoveRequestType::Base;
	AIMoveReason reason = AIMoveReason::None;

	AIMoveMode mode = AIMoveMode::Arrive;

	float utility = 0.0f;
	float urgency = 0.0f;

	float confirmationTime = 0.0f;
	float minCommitmentTime = 0.0f;

	int priority = 0;
	float weight = 1.0f;

	DirectX::XMFLOAT3 targetPosition{ 0.0f, 0.0f, 0.0f };

	float moveSpeed = 0.0f;
	float stopDistance = 0.0f;
};

struct AIPlayerMoveDebug
{
	bool hasFinalMove = false;

	AIMoveRequestType finalType =
		AIMoveRequestType::Base;

	AIMoveMode finalMode =
		AIMoveMode::Arrive;

	AIMoveReason finalReason =
		AIMoveReason::None;

	DirectX::XMFLOAT3 finalTarget
	{
		0.0f,
		0.0f,
		0.0f
	};

	float finalMoveSpeed = 0.0f;
	float finalStopDistance = 0.0f;

	bool hasCurrentAction = false;

	AIMoveReason currentActionReason =
		AIMoveReason::None;

	float currentActionElapsedTime = 0.0f;
	float currentActionMinCommitmentTime = 0.0f;

	bool hasCandidateAction = false;

	AIMoveReason candidateActionReason =
		AIMoveReason::None;

	float candidateElapsedTime = 0.0f;
	float candidateRequiredTime = 0.0f;

	std::vector<AIMoveRequestDebug> requests;
};

struct AIPlayerActionState
{
	bool hasCurrentAction = false;

	AIMoveRequest currentAction;

	float currentActionElapsedTime = 0.0f;

	bool hasCandidateAction = false;

	AIMoveRequest candidateAction;

	float candidateElapsedTime = 0.0f;
};

class AIMoveResolver
{
public:
	void Initialize(int playerCount);
	void Clear();

	void Submit(int playerIndex, const AIMoveRequest& request);

	void ResolveAndExecute(
		std::vector<std::unique_ptr<SoccerPlayer>>& players,
		int controlledIndex,
		float elapsedTime);
	
	const std::vector<AIPlayerMoveDebug>& GetDebugInfos() const
	{
		return m_debugInfos;
	}
	
private:
	void MovePlayerToPosition(
		SoccerPlayer* player,
		const DirectX::XMFLOAT3& targetPosition,
		float moveSpeed,
		float stopDistance,
		AIMoveMode mode,
		float elapsedTime);

	bool IsSameAction(const AIMoveRequest& a, const AIMoveRequest& b)const;

	const AIMoveRequest* FindBestActionRequest(const AIMoveRequestSet& requestSet)const;

	const AIMoveRequest* FindCurrentActionRequest(const AIMoveRequestSet& requestSet, const AIMoveRequest& currentAction) const;

	void UpdateActionState(AIPlayerActionState& state,const AIMoveRequestSet& requestSet,float elapsedTime);

	ResolverMoveCommand MakeMoveCommand(const AIMoveRequest& request) const;

private:
	std::vector<AIMoveRequestSet> m_requestSets;
	std::vector<AIPlayerMoveDebug> m_debugInfos;

	std::vector<AIPlayerActionState> m_actionStates;
};