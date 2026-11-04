#pragma once

#include <DirectXMath.h>

enum class AIMoveRequestType
{
	Base,
	Soft,
	Action,
	Hard,
	Modifier
};

enum class AIMoveMode
{
	Arrive,
	Continuous
};

enum class AIMoveReason
{
	None,
	TeamShape,
	Press,
	Cover,
	Mark,
	ForwardOffBall,
	ReceivePass,
	Support,
	BallOwner,
	ManualControl,
	FreeBallChase
};

struct AIMoveRequest
{
	bool active = false;

	AIMoveRequestType type = AIMoveRequestType::Base;
	AIMoveReason reason = AIMoveReason::None;

	AIMoveMode mode = AIMoveMode::Arrive;

	int priority = 0;
	float weight = 1.0f;

	//この行動を選ぶ価値
	float utility = 0.0f;

	//どれだけ素早く反応すべきか
	float urgency = 0.0f;

	//行動に採用するまで確認する時間
	float confirmationTime = 0.20f;

	//採用後に最低限維持する時間
	float minCommitmentTime = 0.50f;

	DirectX::XMFLOAT3 targetPosition = { 0.0f,0.0f,0.0f };

	float moveSpeed = 0.0f;
	float stopDistance = 0.05f;
};

struct AIMoveRequestSet
{
	AIMoveRequest base;

	AIMoveRequest soft[4];
	int softCount = 0;

	AIMoveRequest action[4];
	int actionCount = 0;

	AIMoveRequest modifier[4];
	int modifierCount = 0;

	AIMoveRequest hard;
	bool hasHard = false;
};

struct ResolverMoveCommand
{
	bool active = false;

	AIMoveRequestType sourceType = AIMoveRequestType::Base;

	AIMoveReason reason = AIMoveReason::None;

	AIMoveMode mode = AIMoveMode::Arrive;

	DirectX::XMFLOAT3 targetPosition = { 0.0f,0.0f,0.0f };

	float desireMoveSpeed = 0.0f;
	float stopDistance = 0.05f;
};

inline const char* ToString(AIMoveRequestType type)
{
	switch (type)
	{
	case AIMoveRequestType::Base:
		return "Base";
	case AIMoveRequestType::Soft:
		return "Soft";
	case AIMoveRequestType::Action:
		return "Action";
	case AIMoveRequestType::Hard:
		return "Hard";
	case AIMoveRequestType::Modifier:
		return "Modifier";
	}

	return "Unknown";
}

inline const char* ToString(AIMoveMode mode)
{
	switch (mode)
	{
	case AIMoveMode::Arrive:
		return "Arrive";

	case AIMoveMode::Continuous:
		return "Continuous";
	}

	return "Unknown";
}

inline const char* ToString(AIMoveReason reason)
{
	switch (reason)
	{
	case AIMoveReason::None:           return "None";
	case AIMoveReason::TeamShape:      return "TeamShape";
	case AIMoveReason::Press:          return "Press";
	case AIMoveReason::Cover:          return "Cover";
	case AIMoveReason::Mark:          return "Mark";
	case AIMoveReason::ForwardOffBall: return "ForwardOffBall";
	case AIMoveReason::ReceivePass:    return "ReceivePass";
	case AIMoveReason::Support:        return "Support";
	case AIMoveReason::BallOwner:      return "BallOwner";
	case AIMoveReason::ManualControl:  return "ManualControl";
	case AIMoveReason::FreeBallChase:  return "FreeBallChase";
	}

	return "Unknown";
}