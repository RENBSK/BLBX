#pragma once

#include "Soccer/AI/Modules/CalAIeva.h"

class BlackBoard;

class SoccerPlayer;

class PrimitiveRenderer;

#include <vector>
#include <memory>

class Meta
{
public:
	void  Initialize();

	void Update(
		const BlackBoard& sharedBB, 
		BlackBoard& allyTeamBB,
		BlackBoard& enemyTeamBB,
		std::vector<std::unique_ptr<SoccerPlayer>>& players,
		float elapsedTime);

	void AssignPress(
		int teamNumber,
		BlackBoard& teamBB,
		std::vector < std::unique_ptr<SoccerPlayer>>& players,
		float elapsedTime);

	void AssignCover(
		int teamNumber,
		BlackBoard& teamBB,
		std::vector < std::unique_ptr<SoccerPlayer>>& players);

	void AssignAttack(
		int teamNumber,
		BlackBoard& teamBB,
		const std::vector<std::unique_ptr<SoccerPlayer>>& players);

	void AssignMarks(
		int teamNumber,
		const BlackBoard& teamBB,
		const std::vector<std::unique_ptr<SoccerPlayer>>& players);

	void AssignChase(
		int teamNumber,
		BlackBoard& teamBB,
		const std::vector<std::unique_ptr<SoccerPlayer>>& players);

	void DrawImGui(const BlackBoard& allyTeamBB, const BlackBoard& enemyTeamBB);

	void DrawDebug(
		PrimitiveRenderer& renderer,
		const std::vector<std::unique_ptr<SoccerPlayer>>& players,
		const BlackBoard& allyTeamBB,
		const BlackBoard& enemyTeamBB) const;

	const std::vector<int>& GetMarkTargets() const
	{
		return m_markTargets;
	}

//共通の計算関数
public:
	//保持者、またはパスの受け手の番号を取得
	int GetReference(
		const BlackBoard& blackboard,
		const std::vector<std::unique_ptr<SoccerPlayer>>& players) const;

	//指定チームから、目標位置に一番近い選手を探す
	int FindNearest(
		const std::vector<std::unique_ptr<SoccerPlayer>>& players,
		int TeamNumber,
		const DirectX::XMFLOAT3& targetPosition,
		int excludeIndex = -1,
		int secondExcludeIndex = -1)const;

	std::vector<int>FindOrder(const std::vector<std::vector<float>>& costs) const;
	


private:
	struct PressAssignmentState
	{
		int candidateIndex = -1;
		float timer = 0.0f;
	};

	PressAssignmentState m_pressStates[2];

	//担当交代を決行するまでの時間
	float m_PressSwitchTimer = 0.3f;

	CalAIeva m_calAIeva;

	// 自分の選手index → マーク相手のindex
	std::vector<int> m_markTargets;

	// チームごとの選手index。選考とImGuiで使う
	std::vector<int> m_markPlayers[2];

	// 担当を１人変更する負担
	float m_markSwitchCost = 0.25f;

	
};
