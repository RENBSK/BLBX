#include "AIMoveResolver.h"

#include "Soccer/Player/SoccerPlayer.h"

#include <cmath>
#include <algorithm>

//選手ごとの移動要求など保存する領域を用意する
void AIMoveResolver::Initialize(int playerCount)
{
	//以前の一覧を破棄
	m_requestSets.clear();
	m_debugInfos.clear();
	m_actionStates.clear();
	//選手数と同じ数のセットを作成する
	m_requestSets.resize(playerCount);
	m_debugInfos.resize(playerCount);
	m_actionStates.resize(playerCount);
}

void AIMoveResolver::Clear()
{
	for (auto& request : m_requestSets)
	{
		request = AIMoveRequestSet{};
	}

	for (auto& debugInfo : m_debugInfos)
	{
		debugInfo = AIPlayerMoveDebug{};
	}
}

void AIMoveResolver::Submit(int playerIndex, const AIMoveRequest& request)
{
	if (playerIndex < 0) return;
	if (playerIndex >= static_cast<int>(m_requestSets.size())) return;
	if (!request.active)return;

	AIMoveRequestSet& requestSet = m_requestSets[playerIndex];

	switch (request.type)
	{
	case AIMoveRequestType::Base:
		if (!requestSet.base.active 
			||request.priority >requestSet.base.priority)
		{
			requestSet.base =request;
		}
		break;

	case AIMoveRequestType::Hard:
		if (!requestSet.hasHard 
			|| request.priority > requestSet.hard.priority)
		{
			requestSet.hard = request;
			requestSet.hasHard = true;
		}
		break;

	case AIMoveRequestType::Soft:
		if (requestSet.softCount < 4)
		{
			requestSet.soft[requestSet.softCount] = request;
			requestSet.softCount++;
		}
		break;

	case AIMoveRequestType::Modifier:
		if (requestSet.modifierCount < 4)
		{
			requestSet.modifier[requestSet.modifierCount] = request;
			requestSet.modifierCount++;
		}
		break;

	case AIMoveRequestType::Action:
		if (requestSet.actionCount < 4)
		{
			requestSet.action[requestSet.actionCount] = request;
			requestSet.actionCount++;
		}
		break;
	}

	if (playerIndex < static_cast<int>(m_debugInfos.size()))
	{
		AIMoveRequestDebug debugRequest;
		debugRequest.type = request.type;
		debugRequest.reason = request.reason;
		debugRequest.mode = request.mode;
		debugRequest.utility = request.utility;
		debugRequest.urgency = request.urgency;
		debugRequest.confirmationTime = request.confirmationTime;
		debugRequest.minCommitmentTime = request.minCommitmentTime;
		debugRequest.priority = request.priority;
		debugRequest.weight = request.weight;
		debugRequest.targetPosition = request.targetPosition;
		debugRequest.moveSpeed = request.moveSpeed;
		debugRequest.stopDistance = request.stopDistance;

		m_debugInfos[playerIndex].requests.push_back(debugRequest);
	}
}

void AIMoveResolver::ResolveAndExecute(std::vector<std::unique_ptr<SoccerPlayer>>& players, int controlledIndex, float elapsedTime)
{

	const int playerCount = static_cast<int>(std::min(players.size(), m_requestSets.size()));



	for (int i = 0; i < playerCount; ++i)
	{
		if (!players[i])
		{
			continue;
		}

		SoccerPlayer* player = players[i].get();

		AIPlayerActionState& actionState = m_actionStates[i];

		//操作プレイヤーは手動でうごかす
		if (i == controlledIndex)
		{
			actionState = AIPlayerActionState{};
			continue;
		}

		const AIMoveRequestSet& requestSet = m_requestSets[i];

		float currentMoveSpeed = player->GetMovementSpeed();

		// ========================================
		//　Actionの継続・切り替えを更新
		// ========================================
		if (requestSet.hasHard)
		{
			actionState = AIPlayerActionState{};
		}
		else
		{
			UpdateActionState(actionState, requestSet, elapsedTime);
		}

		// ========================================
		// 主要求を選択
		// ========================================
		const AIMoveRequest* mainRequest = nullptr;

		if (requestSet.hasHard)
		{
			mainRequest = &requestSet.hard;
		}
		else if (actionState.hasCurrentAction)
		{
			mainRequest = &actionState.currentAction;
		}
		else if (requestSet.base.active)
		{
			mainRequest = &requestSet.base;
		}

		ResolverMoveCommand command{};

		if (mainRequest)
		{
			command = MakeMoveCommand(*mainRequest);
		}

		// ========================================
		// BaseにSoft・Modifierを適用
		// ========================================

		if (command.active && command.sourceType == AIMoveRequestType::Base)
		{
			DirectX::XMFLOAT3 target = command.targetPosition;

			//Baseの重みは1として扱う
			float totalWeight = 1.0f;

			//Soft 目標位置を重み付きで混ぜる
			for (int s = 0; s < requestSet.softCount; ++s)
			{
				const AIMoveRequest& soft = requestSet.soft[s];

				target.x += soft.targetPosition.x * soft.weight;
				target.y += soft.targetPosition.y * soft.weight;
				target.z += soft.targetPosition.z * soft.weight;

				totalWeight += soft.weight;
			}

			target.x /= totalWeight;
			target.y /= totalWeight;
			target.z /= totalWeight;

			for (int m = 0; m < requestSet.modifierCount; ++m)
			{
				const AIMoveRequest& modifier = requestSet.modifier[m];

				target.x += modifier.targetPosition.x * modifier.weight;
				target.y += modifier.targetPosition.y * modifier.weight;
				target.z += modifier.targetPosition.z * modifier.weight;
			}

			command.targetPosition = target;
		}

		// ========================================
		// 移動または停止を依頼
		// ========================================
		if (command.active)
		{
			MovePlayerToPosition(
				player,
				command.targetPosition,
				command.desireMoveSpeed,
				command.stopDistance,
				command.mode,
				elapsedTime);
		}
		else
		{
			player->UpdateMovement(
				{ 0.0f, 0.0f, 0.0f },
				0.0f,
				elapsedTime);
		}


		// ========================================
		// デバッグ情報を記録
		// ========================================
		AIPlayerMoveDebug& debug = m_debugInfos[i];

		debug.hasFinalMove = command.active;
		debug.finalType = command.sourceType;
		debug.finalMode = command.mode;
		debug.finalReason = command.reason;
		debug.finalTarget = command.targetPosition;
		debug.finalStopDistance = command.stopDistance;

		// 希望速度ではなく、移動処理後の実速度
		debug.finalMoveSpeed = player->GetMovementSpeed();

		debug.hasCurrentAction =actionState.hasCurrentAction;

		if (actionState.hasCurrentAction)
		{
			debug.currentActionReason =actionState.currentAction.reason;

			debug.currentActionElapsedTime =actionState.currentActionElapsedTime;

			debug.currentActionMinCommitmentTime =actionState.currentAction.minCommitmentTime;
		}

		debug.hasCandidateAction =actionState.hasCandidateAction;

		if (actionState.hasCandidateAction)
		{
			const AIMoveRequest& candidate =actionState.candidateAction;

			debug.candidateActionReason = candidate.reason;

			debug.candidateElapsedTime =actionState.candidateElapsedTime;

			const float urgency =std::clamp(candidate.urgency, 0.0f, 1.0f);

			debug.candidateRequiredTime = std::max(candidate.confirmationTime, 0.0f) * (1.0f - urgency * 0.75f);
		}
	}
}

//AIプレイヤーをうごかしてる基盤
void AIMoveResolver::MovePlayerToPosition(
	SoccerPlayer* player, 
	const DirectX::XMFLOAT3& targetPosition, 
	float moveSpeed, 
	float stopDistance, 
	AIMoveMode mode, 
	float elapsedTime)
{
	if (!player) return;

	DirectX::XMFLOAT3 pos = player->GetPosition();

	float dx = targetPosition.x - pos.x;
	float dz = targetPosition.z - pos.z;

	float distSq = dx * dx + dz * dz;
	float stopDistanceSq = stopDistance * stopDistance;

	const bool hasArrives = mode == AIMoveMode::Arrive && distSq <= stopDistanceSq;

	if (hasArrives||distSq<0.00001)
	{
		player->UpdateMovement(
			{ 0.0f,0.0f,0.0f },
			0.0f,
			elapsedTime);

		return;
	}

	const float distance = std::sqrtf(distSq);

	const DirectX::XMFLOAT3 direction =
	{
		dx / distance,
		0.0f,
		dz / distance
	};

	float requestedSpeed = moveSpeed;

	if (mode == AIMoveMode::Arrive)
	{
		const float remainDistance = distance - stopDistance;

		requestedSpeed = std::min(requestedSpeed, remainDistance / elapsedTime);
	}


	player->UpdateMovement(direction, requestedSpeed, elapsedTime);
}

//二つの行動を同じ行動として扱うか判定する
bool AIMoveResolver::IsSameAction(const AIMoveRequest& a, const AIMoveRequest& b) const
{
	//行動の目的を移動方式が同じなら、同じ行動として扱う
	return a.reason == b.reason && a.mode == b.mode;
}

//最良のAction要求を探す
const AIMoveRequest* AIMoveResolver::FindBestActionRequest(const AIMoveRequestSet& requestSet) const
{
	const AIMoveRequest* best = nullptr;

	for (int i = 0; i < static_cast<int>(requestSet.actionCount); ++i)
	{
		const AIMoveRequest& action = requestSet.action[i];

		if (!action.active)
		{
			continue;
		}

		if (!best)
		{
			best = &action;
			continue;
		}

		//優先度が高い要求を採用する
		if (action.priority > best->priority)
		{
			best = &action;
		}
		//優先度が同じなら、weightが大きい要求を採用する
		else if (action.priority == best->priority && action.utility > best->utility)
		{
			best = &action;
		}
	}


	return best;
}

//今実行している行動と一致する要求を、今フレームからの要求から探す
const AIMoveRequest* AIMoveResolver::FindCurrentActionRequest(const AIMoveRequestSet& requestSet, const AIMoveRequest& currentAction) const
{
	const AIMoveRequest* latesRequest = nullptr;

	for (int i = 0; i < requestSet.actionCount; ++i)
	{
		const AIMoveRequest& action = requestSet.action[i];

		if (!action.active)
		{
			continue;
		}

		if (!IsSameAction(action, currentAction))
		{
			continue;
		}

		if (!latesRequest)
		{
			latesRequest = &action;
			continue;
		}


		//同じ行動の要求が複数ある場合、優先度が同じならutilityが高いものに更新する
		if (action.priority ==
			latesRequest->priority &&
			action.utility >
			latesRequest->utility)
		{
			latesRequest = &action;
		}
	}

	return latesRequest;
}

//実行中のの行動と切り替え候補を更新する
//どのActionを採用するか、その状態だけを管理する
void AIMoveResolver::UpdateActionState(AIPlayerActionState& state, const AIMoveRequestSet& requestSet, float elapsedTime)
{
	// ========================================
	// すでに実行中のActionがある場合
	// ========================================
	if (state.hasCurrentAction)
	{
		//現在の行動を採用してからも経過時間を進める
		state.currentActionElapsedTime += elapsedTime;

		//現在の行動と一致する要求が今フレームも送られているか調べる
		const AIMoveRequest* latestRequest =
			FindCurrentActionRequest(
				requestSet,
				state.currentAction);

		if (!latestRequest)
		{
			//現在の行動が要求されていないので解除する。
			state.hasCurrentAction = false;
			state.currentAction = AIMoveRequest{};
			state.currentActionElapsedTime = 0.0f;
		}

		const bool currentActionRequested = latestRequest != nullptr;

		//同じ行動が引き続き要求されている場合、目標位置や速度などを最新の内容へ更新する
		//経過時間はリセットしない
		if (latestRequest)
		{
			state.currentAction = *latestRequest;
		}


		//現在の行動を最低限続ける時間
		const float minimumCommitmentTime =
			std::max(
				state.currentAction.minCommitmentTime,
				0.0f);

		//最低継続時間に達していなければ、現在の行動を維持する
		//この間は別行動の確認時間も蓄積しない
		if (state.currentActionElapsedTime < minimumCommitmentTime)
		{
			state.hasCandidateAction = false;
			state.candidateAction =
				AIMoveRequest{};
			state.candidateElapsedTime = 0.0f;

			return;
		}

		//現在送られている中で市場良い良いAction
		const AIMoveRequest* bestAction =FindBestActionRequest(requestSet);


		// 現在と違うActionが最良なら切り替え候補
		if (bestAction &&!IsSameAction(*bestAction,state.currentAction))
		{
			//前フレームから同じ切り替え候補が続いてる場合
			if (state.hasCandidateAction &&IsSameAction(state.candidateAction,*bestAction))
			{
				//目標位置などを最新の内容に更新する
				state.candidateAction = *bestAction;
				//同じ候補が続いてる時間を加算する
				state.candidateElapsedTime += elapsedTime;
			}
			else
			{
				//新しい切り替え候補として登録する
				state.hasCandidateAction = true;
				state.candidateAction = *bestAction;
				//候補になった最初のフレーム分から時間を数える
				state.candidateElapsedTime = elapsedTime;
			}
			//緊急時を0～1に収める
			const float urgency =
				std::clamp(
					state.candidateAction.urgency,
					0.0f,
					1.0f);

			//候補を採用するまでに必要な確認時間
			float requiredConfirmationTime =
				std::max(
					state.candidateAction.confirmationTime,
					0.0f);

			//緊急度が高いほど確認時間を短くする
			requiredConfirmationTime *= 1.0f - urgency * 0.75f;

			//切り替え候補の確認終了
			if (state.candidateElapsedTime >=
				requiredConfirmationTime)
			{
				state.currentAction = state.candidateAction;
				//新しい行動の最低継続時間を、ここから数え直す。
				state.currentActionElapsedTime = 0.0f;
				//候補は実行中の行動になったので、候補状態から解除する
				state.hasCandidateAction = false;
				state.candidateAction = AIMoveRequest{};
				state.candidateElapsedTime = 0.0f;
			}

			//確認中も、切り替え完了時も、このフレームの更新は終了
			//確認中の場合は、以前の行動がそのまま残る
			return;
		}

		//別の行動へ切り替える状況ではないため、候補状態を解除する
		state.hasCandidateAction = false;
		state.candidateAction = AIMoveRequest{};
		state.candidateElapsedTime = 0.0f;

		//現在Actionがまだ要求されている
		if (currentActionRequested)
		{
			return;
		}

	}

	// ========================================
	// 実行中のActionがない場合
	// ========================================

	//今フレームの最良Actionを探す
	const AIMoveRequest* bestAction = FindBestActionRequest(requestSet);

	//Action要求が一つもなければ、候補も解除して終了
	if (!bestAction)
	{
		state.hasCandidateAction = false;

		state.candidateAction = AIMoveRequest{};

		state.candidateElapsedTime = 0.0f;

		return;
	}

	//候補の確認時間を更新
	if (state.hasCandidateAction &&
		IsSameAction(
			state.candidateAction,
			*bestAction))
	{
		//前フレームと同じ候補
		state.candidateAction = *bestAction;

		state.candidateElapsedTime += elapsedTime;
	}
	else
	{
		//別の候補に変わった
		state.hasCandidateAction = true;

		state.candidateAction = *bestAction;

		state.candidateElapsedTime = elapsedTime;
	}

	//緊急度に応じて候補の確認時間を短くする。
	const float urgency =
		std::clamp(
			state.candidateAction.urgency,
			0.0f,
			1.0f);

	float requiredConfirmationTime =
		std::max(
			state.candidateAction.confirmationTime,
			0.0f);

	requiredConfirmationTime *=
		1.0f - urgency * 0.75f;

	//確認時間を越えたらActionを採用
	if (state.candidateElapsedTime <
		requiredConfirmationTime)
	{
		return;
	}

	//必要な時間だけ同じ候補が続いたので、実行するActionとして採用する
	state.hasCurrentAction = true;
	state.currentAction = state.candidateAction;

	//採用した行動の継続時間を、ここから始める
	state.currentActionElapsedTime = 0.0f;

	//候補状態を解除
	state.hasCandidateAction = false;
	state.candidateAction = AIMoveRequest{};
	state.candidateElapsedTime = 0.0f;
}

//採用した移動要求を、最終的な移動指示の形式へ変換する
ResolverMoveCommand AIMoveResolver::MakeMoveCommand(const AIMoveRequest& request) const
{
	ResolverMoveCommand command;

	command.active = request.active;
	command.sourceType = request.type;
	command.reason = request.reason;
	command.mode = request.mode;
	command.targetPosition = request.targetPosition;
	command.desireMoveSpeed = request.moveSpeed;
	command.stopDistance = request.stopDistance;

	return command;

}
