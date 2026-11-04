#pragma once

#include "Soccer/Player/SoccerPlayer.h"
#include "Soccer/Player/Data/PlayerConfig.h"
#include "Soccer/AI/Core/BlackBoard.h"
#include "Soccer/AI/Keys/BlackBoardKeys.h"

#include <memory>
#include <vector>

class PlayerManager
{
private:
	PlayerManager()
	{
		sharedBB = std::make_unique<BlackBoard>();

		sharedBB->Set(PlayerKeys::ControlledIndex, -1);
		sharedBB->Set(BallKeys::OwnerIndex, -1);
	}
	~PlayerManager() {}

public:
	static PlayerManager& Instance()
	{
		static PlayerManager instance;
		return instance;
	}

	void Finalize();

	void Update(float elapsedTime);

	void Render();

	void DrawImGui();

	void CreateTeam(int teamNumber, int playerCount,const PlayerConfig& config);

	BlackBoard& GetSharedBlackBoard()
	{
		return *sharedBB;
	}

	SoccerPlayer* GetPlayer(int index)
	{
		if (index < 0 || index >= static_cast<int>(players.size()))
		{
			return nullptr;
		}

		return players[index].get();
	}

	std::vector<std::unique_ptr<SoccerPlayer>>& GetPlayers()
	{
		return players;
	}

	void SetBallOwner(int ownerIndex);

private:
	std::vector<std::unique_ptr<SoccerPlayer>> players;
private:
	int id = 0;
	std::shared_ptr<BlackBoard> sharedBB;

	int inspectedPlayerIndex = 0;
};