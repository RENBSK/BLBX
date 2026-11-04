#pragma once

namespace BallKeys
{
	inline constexpr char Position[] = "Ball.Position";
	inline constexpr char OwnerIndex[] = "Ball.OwnerIndex";
	inline constexpr char OwnerTeam[] = "Ball.OwnerTeam";
}

namespace PlayerKeys
{
	inline constexpr char ControlledIndex[] = "Player.ControlledIndex";
	inline constexpr char PressPlayerIndex[] = "Player.PressPlayerIndex";
	inline constexpr char DefenceCoverIndex[] = "Player.DefenceCoverIndex";
	inline constexpr char AttackSupportPlayerIndex[] = "Player.AttackSupportPlayerIndex";
	inline constexpr char PressTargetIndex[] = "Player.PressTargetIndex";
}

namespace TeamKeys
{
	inline constexpr char IsAttacking[] = "Team.Attacking";
	inline constexpr char IsDefending[] = "Team.Defending";
	inline constexpr char GoalPosition[] = "Goal.Position";
	inline constexpr char DefendGoalPosition[] = "Goal.DefendGoalPosition";
}
