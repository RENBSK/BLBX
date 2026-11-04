#include "PlayerConfig.h"

static float GetAbilityScale(AbilityLevel level)
{
	switch (level)
	{
	case AbilityLevel::Low:
		return 0.8f;
	case AbilityLevel::High:
		return 1.2f;
	case AbilityLevel::Normal:
		return 1.0f;
	}
}

PlayerParameters BuildPlayerParameters(
	const PlayerAbility& ability)
{
	PlayerParameters parameters{};

	parameters.maxSpeed *= GetAbilityScale(ability.speed);
	parameters.acceleration = GetAbilityScale(ability.explosiveness);
	parameters.ballControl *= GetAbilityScale(ability.technique);


	return parameters;
}