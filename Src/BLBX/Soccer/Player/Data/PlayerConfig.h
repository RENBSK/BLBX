#pragma once

#include <array>
#include<string>

//能力三段階
enum class AbilityLevel
{
	Low,
	Normal,
	High
};

//選手の能力値
struct PlayerAbility
{
	AbilityLevel technique = AbilityLevel::Normal;
	AbilityLevel speed = AbilityLevel::Normal;
	AbilityLevel explosiveness = AbilityLevel::Normal;
};

// アニメーションの状態
enum class PlayerAnimationState
{
	Idle,
	Dribble,
	Walk,
	Run,
	Count
};

//各アニメーションの設定
struct AnimationClipSetting
{
	std::string name;
	float playbackSpeed = 1.0f;
	bool loop = true;
};

//内部パラメーター
struct PlayerParameters
{
	float maxSpeed = 1.0f;
	float acceleration = 1.0f;
	float deceleration = 1.0f;
	float turnSpeed = 1.0f;
	float ballControl = 1.0f;
};

// 選手の設定
struct PlayerConfig
{
	//モデル読み込み
	std::string modelPath;

	PlayerAbility ability =
	{
		AbilityLevel::Normal,
		AbilityLevel::Normal,
		AbilityLevel::Normal
	};

	std::array<AnimationClipSetting, static_cast<size_t>(PlayerAnimationState::Count)> animations{{
		{ "Idle",    1.0f, true },
		{ "Dribble",    1.0f, true },
		{ "Walk",     1.0f, true },
		{ "Run", 1.0f, true }
	} };
};

// 能力から内部パラメータを生成
PlayerParameters BuildPlayerParameters(
	const PlayerAbility& ability);