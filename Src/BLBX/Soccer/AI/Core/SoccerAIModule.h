#pragma once

#include <vector>
#include <memory>

#include "Soccer/Player/SoccerPlayer.h"

#include "Soccer/AI/Request/AIMoveResolver.h"

#include "Soccer/AI/Core/BlackBoard.h"
#include "Soccer/AI/Keys/BlackBoardKeys.h"


#ifdef USE_IMGUI
#include <imgui.h>
#endif

class SoccerAIModule
{
public:
	virtual ~SoccerAIModule() = default;

	virtual void Initialize() {};

	virtual void SubmitRequests
	(
		int  TeamNumber,
		const BlackBoard& blackboard,
		std::vector<std::unique_ptr<SoccerPlayer>>& players,
		const std::vector<int>& markTargets,
		AIMoveResolver& resolver,
		float elapsedTime
	) = 0;

	virtual void DrawImGui() {};
};