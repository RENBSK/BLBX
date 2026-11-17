#pragma once

namespace BallKeys
{
	inline constexpr char Position[] = "Ball.Position";
	inline constexpr char OwnerIndex[] = "Ball.OwnerIndex";
	inline constexpr char OwnerTeam[] = "Ball.OwnerTeam";

	//パスしても、Ownerをただ-1にせず、パス中など判断する
	inline constexpr char PassTeam[] = "Ball.PassTeam";
	inline constexpr char PassTarget[] = "Ball.PassTargetIndex";

	
}

namespace PlayerKeys
{
	inline constexpr char ControlledIndex[] = "Player.ControlledIndex";
	inline constexpr char PressPlayerIndex[] = "Player.PressPlayerIndex";
	inline constexpr char DefenceCoverIndex[] = "Player.DefenceCoverIndex";
	inline constexpr char AttackSupportPlayerIndex[] = "Player.AttackSupportPlayerIndex";
	inline constexpr char PressTargetIndex[] = "Player.PressTargetIndex";

	//攻撃時に幅を取る選手
	inline constexpr char AttackWidePlayerIndex[] = "Player.AttackWidePlayerIndex";
	//守備時にパス相手をマークする選手
	inline constexpr char PassMarkPlayerIndex[] = "Player.PassMarkPlayerIndex";
	//選手がマークするパス相手
	inline constexpr char PassMarkTargetIndex[] = "Player.PassMarkTargetIndex";

	inline constexpr char FreeBallPlayerIndex[] = "Player.FreeBallPlayerIndex";
}

namespace TeamKeys
{
	inline constexpr char IsAttacking[] = "Team.Attacking";
	inline constexpr char IsDefending[] = "Team.Defending";
	inline constexpr char GoalPosition[] = "Goal.Position";
	inline constexpr char DefendGoalPosition[] = "Goal.DefendGoalPosition";
}
