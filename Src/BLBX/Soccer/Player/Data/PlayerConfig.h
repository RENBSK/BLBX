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
	Tpose,
	Idle,
	Dribble,
	Walk,
	Run,
	StopWalking,
	StopRuning,
	LeftTurn,
	LeftBackTurn,
	RightTurn,
	RightBackTurn,
	Turn,
	Count
};

//各アニメーションの設定
struct AnimationClipSetting
{
	std::string name;
	float playbackSpeed = 1.0f;
	//ループ再生するか
	bool loop = true;

	//再生終了までプレイヤーの操作を禁止する
	bool lockControl = false;

	//アニメーション切り替えの補完時間
	float blendDuration = 0.2f;
};

//内部パラメーター
struct PlayerParameters
{
	float maxSpeed = 2.25f;
	float acceleration = 1.35f;
	float deceleration = 2.5f;
	float turnSpeed = 10.0f;
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

	std::array<
		AnimationClipSetting,
		static_cast<size_t>(PlayerAnimationState::Count)
	> animations{ {
		{ "Tpose",         1.0f, true,false,0.2f  },
		{ "Idle",          1.0f, true,false,0.2f  },
		{ "Dribble",       1.0f, true,false,0.2f  },
		{ "Walk",          1.0f, true,false,0.2f  },
		{ "Run",           1.0f, true,false,0.15f  },
		{ "StopWalking",   1.0f, false,true },
		{ "StopRunning",   1.0f, false,true,0.1f },
		{ "LeftTurn",      1.0f, false,false },
		{ "LeftBackTurn",  1.0f, false,false },
		{ "RightTurn",     1.0f, false,false },
		{ "RightBackTurn", 1.0f, false,false },
		{ "Turn",          1.0f, false,false }
	} };
};

// 能力から内部パラメータを生成
PlayerParameters BuildPlayerParameters(
	const PlayerAbility& ability);