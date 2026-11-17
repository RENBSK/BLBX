#include "SoccerAISystem.h"

#include "Soccer/Ball/SoccerBall.h"
#include "Soccer/Player/SoccerPlayer.h"

#include "Soccer/AI/Modules/Defense/DefenderAI.h"
#include "Soccer/AI/Modules/Defense/CoverAI.h"
#include "Soccer/AI/Modules/Attack/AttackSupportAI.h"
#include "Soccer/AI/Modules/Attack/AttackWideAI.h"
#include "Soccer/AI/Modules/FreeBall/FreeBallAI.h"
#include "Graphics/Camera/Camera.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

#ifdef USE_IMGUI
#include <imgui.h>
#endif

void SoccerAISystem::Initialize(int playerCount)
{

	m_modules.clear();

	//前回のチーム情報を初期化
	m_allyTeamBB = BlackBoard{};
	m_enemyTeamBB = BlackBoard{};

	const DirectX::XMFLOAT3 team0Goal{ 0.0f, 0.0f, 10.45f };
	const DirectX::XMFLOAT3 team1Goal{ 0.0f, 0.0f, -10.7f };

	// チーム0
	m_allyTeamBB.Set(TeamKeys::GoalPosition, team0Goal);
	m_allyTeamBB.Set(TeamKeys::DefendGoalPosition, team1Goal);

	// チーム1
	m_enemyTeamBB.Set(TeamKeys::GoalPosition, team1Goal);
	m_enemyTeamBB.Set(TeamKeys::DefendGoalPosition, team0Goal);

	//選手ごとの要求・行動状態を用意
	m_moveResolver.Initialize(playerCount);

	for (auto& module : m_modules)
	{
		if (!module)
		{
			continue;
		}

		module->Initialize();
	}

	//Module1に登録
	m_modules.emplace_back(std::make_unique<DefenderAI>());
	m_modules.emplace_back(std::make_unique<CoverAI>());
	m_modules.emplace_back(std::make_unique<AttackAupportAI>());
	m_modules.emplace_back(std::make_unique<AttackWideAI>());
	m_modules.emplace_back(std::make_unique<FreeBallAI>());
}

void SoccerAISystem::Update(std::vector<std::unique_ptr<SoccerPlayer>>& players, SoccerBall* ball, BlackBoard& sharedBB, float elapsedTime)
{
	if (!ball)
	{
		return;
	}

	m_moveResolver.Clear();

	//チームBBからの共有情報の読めるようにする
	m_allyTeamBB.SetParent(&sharedBB);
	m_enemyTeamBB.SetParent(&sharedBB);

	//現在の状況を更新
	UpdateSharedBlackBoard(players, *ball, sharedBB);

	//チーム方針を更新
	m_MetaAI.Update(sharedBB, m_allyTeamBB, m_enemyTeamBB, players, elapsedTime);

	//両チームについて、各Moduleから要求する
	for (int teamNumber = 0; teamNumber < 2; ++teamNumber)
	{
		const BlackBoard& teamBB = teamNumber == 0 ? m_allyTeamBB : m_enemyTeamBB;

		for (auto& module : m_modules)
		{
			if (!module)
			{
				continue;
			}

			module->SubmitRequests(
				teamNumber,
				teamBB,
				players,
				m_MetaAI.GetMarkTargets(),
				m_moveResolver,
				elapsedTime);

		}
	}

	const int controlledIndex = sharedBB.Get<int>(PlayerKeys::ControlledIndex).value_or(-1);

	//要求を選択して移動を実行
	//操作対象の番号も渡して、二重更新を防ぐ
	m_moveResolver.ResolveAndExecute(players, controlledIndex, elapsedTime);

}

void SoccerAISystem::DrawImGui()
{
#ifdef USE_IMGUI
	ImGui::Checkbox("Role labels / arrows", &m_showRoleLabels);
	ImGui::Checkbox("Final move targets", &m_showMoveTargets);
	ImGui::Checkbox("Base / Soft targets", &m_showRequestTargets);

	ImGui::InputInt(
		"Detail player (-1 = all)",
		&m_debugPlayerIndex);

	const int lastIndex =
		static_cast<int>(m_moveResolver.GetDebugInfos().size()) - 1;

	m_debugPlayerIndex =
		std::clamp(m_debugPlayerIndex, -1, lastIndex);

	ImGui::Separator();
	DrawRequestImGui();

	m_MetaAI.DrawImGui(m_allyTeamBB, m_enemyTeamBB);

	ImGui::Separator();


	for (auto& module : m_modules)
	{
		if (!module)
		{
			continue;
		}

		ImGui::PushID(module.get());
		module->DrawImGui();
		ImGui::PopID();
	}
#endif
}

void SoccerAISystem::DrawDebug(
	PrimitiveRenderer& renderer,
	const std::vector<std::unique_ptr<SoccerPlayer>>& players) const
{
	m_MetaAI.DrawDebug(
		renderer,
		players,
		m_allyTeamBB,
		m_enemyTeamBB);
}

void SoccerAISystem::DrawDebugOverlay(
	const std::vector<std::unique_ptr<SoccerPlayer>>& players) const
{
#ifdef USE_IMGUI
	if (!m_showRoleLabels &&
		!m_showMoveTargets &&
		!m_showRequestTargets)
	{
		return;
	}

	const auto& camera = Camera::Instance();

	const DirectX::XMMATRIX viewProjection =
		DirectX::XMLoadFloat4x4(&camera.GetView()) *
		DirectX::XMLoadFloat4x4(&camera.GetProjection());

	ImGuiViewport* viewport = ImGui::GetMainViewport();

	// ImGuiのウィンドウより後ろ、ゲーム画面の上へ描く
	ImDrawList* draw =
		ImGui::GetBackgroundDrawList(viewport);

	// ワールド座標を画面座標へ変換する
	auto ToScreen = [&](const DirectX::XMFLOAT3& world, ImVec2& screen)
		{
			DirectX::XMFLOAT4 clip;

			DirectX::XMStoreFloat4(
				&clip,
				DirectX::XMVector4Transform(
					DirectX::XMVectorSet(
						world.x, world.y, world.z, 1.0f),
					viewProjection));

			if (!std::isfinite(clip.w) || clip.w <= 0.00001f)
			{
				return false;
			}

			const float x = clip.x / clip.w;
			const float y = clip.y / clip.w;
			const float z = clip.z / clip.w;

			if (!std::isfinite(x) ||
				!std::isfinite(y) ||
				!std::isfinite(z))
			{
				return false;
			}

			if (x < -1.0f || x > 1.0f ||
				y < -1.0f || y > 1.0f ||
				z < 0.0f || z > 1.0f)
			{
				return false;
			}

			screen.x =
				viewport->Pos.x + (x + 1.0f) * 0.5f * viewport->Size.x;

			screen.y =
				viewport->Pos.y + (1.0f - y) * 0.5f * viewport->Size.y;

			return true;
		};

	// 目標位置の円と、そこまでの線を描く
	auto DrawTarget = [&](
		const ImVec2& from,
		const DirectX::XMFLOAT3& worldTarget,
		const char* text,
		ImU32 color,
		bool finalTarget)
		{
			ImVec2 target;

			if (!ToScreen(worldTarget, target))
			{
				return;
			}

			if (finalTarget)
			{
				// 最終移動先は破線
				const float dx = target.x - from.x;
				const float dy = target.y - from.y;
				const float length = std::sqrt(dx * dx + dy * dy);

				if (length > 1.0f)
				{
					const float dirX = dx / length;
					const float dirY = dy / length;

					for (float start = 0.0f; start < length; start += 16.0f)
					{
						const float end =
							(std::min)(start + 9.0f, length);

						draw->AddLine(
							{
								from.x + dirX * start,
								from.y + dirY * start
							},
						{
							from.x + dirX * end,
							from.y + dirY * end
						},
							color,
							2.0f);
					}
				}
			}
			else
			{
				// 個別要求は細い実線
				draw->AddLine(from, target, color, 1.0f);
			}

			draw->AddCircle(
				target,
				finalTarget ? 7.0f : 4.0f,
				color,
				16,
				finalTarget ? 2.0f : 1.0f);

			draw->AddText(
				{ target.x + 9.0f, target.y - 7.0f },
				color,
				text);
		};

	const auto& marks = m_MetaAI.GetMarkTargets();
	const auto& moveInfos = m_moveResolver.GetDebugInfos();

	const int playerCount = static_cast<int>(players.size());

	const int controlledIndex =
		m_allyTeamBB.Get<int>(
			PlayerKeys::ControlledIndex).value_or(-1);

	for (int i = 0; i < playerCount; ++i)
	{
		if (!players[i])
		{
			continue;
		}

		const int team = players[i]->GetTeamNumber();

		if (team < 0 || team >= 2)
		{
			continue;
		}

		const BlackBoard& teamBB =
			team == 0 ? m_allyTeamBB : m_enemyTeamBB;

		const bool attacking =
			teamBB.Get<bool>(TeamKeys::IsAttacking).value_or(false);

		const bool defending =
			teamBB.Get<bool>(TeamKeys::IsDefending).value_or(false);

		const int owner =
			teamBB.Get<int>(BallKeys::OwnerIndex).value_or(-1);

		const int support =
			teamBB.Get<int>(
				PlayerKeys::AttackSupportPlayerIndex).value_or(-1);

		const int wide =
			teamBB.Get<int>(
				PlayerKeys::AttackWidePlayerIndex).value_or(-1);

		const int press =
			teamBB.Get<int>(
				PlayerKeys::PressPlayerIndex).value_or(-1);

		const int cover =
			teamBB.Get<int>(
				PlayerKeys::DefenceCoverIndex).value_or(-1);

		const char* role = "NONE";
		ImU32 roleColor = IM_COL32(200, 200, 200, 255);

		if (attacking)
		{
			if (i == owner)
			{
				role = "OWNER";
				roleColor = IM_COL32(255, 230, 30, 255);
			}
			else if (i == support)
			{
				role = "SUPPORT";
				roleColor = IM_COL32(60, 255, 100, 255);
			}
			else if (i == wide)
			{
				role = "WIDE";
				roleColor = IM_COL32(90, 160, 255, 255);
			}
		}
		else if (defending)
		{
			role = "MARK";

			if (i == press)
			{
				role = "PRESS";
				roleColor = IM_COL32(255, 80, 80, 255);
			}
			else if (i == cover)
			{
				role = "COVER";
				roleColor = IM_COL32(40, 255, 255, 255);
			}
		}

		const auto position = players[i]->GetPosition();

		auto footPos = position;
		footPos.y += 0.08f;

		ImVec2 footScreen;
		const bool footVisible = ToScreen(footPos, footScreen);

		const AIPlayerMoveDebug* move = nullptr;

		if (i < static_cast<int>(moveInfos.size()))
		{
			move = &moveInfos[i];
		}

		const int markTarget =
			i < static_cast<int>(marks.size()) ? marks[i] : -1;

		const bool validMark =
			defending &&
			markTarget >= 0 &&
			markTarget < playerCount &&
			players[markTarget] != nullptr &&
			players[markTarget]->GetTeamNumber() != team;

		if (m_showRoleLabels)
		{
			std::string label =
				"T" + std::to_string(team) +
				" P" + std::to_string(i) + "  " + role;

			if (validMark)
			{
				label += "\nMark -> P" + std::to_string(markTarget);
			}

			if (i == controlledIndex)
			{
				label += "\nManual";
			}
			else if (move && move->hasFinalMove)
			{
				label += "\nMove: ";
				label += ToString(move->finalType);
				label += " / ";
				label += ToString(move->finalReason);
			}

			// 頭上ラベルの高さ。モデルに合わせて調整可能
			auto labelPos = position;
			labelPos.y += 1.0f;

			ImVec2 screen;

			if (ToScreen(labelPos, screen))
			{
				const ImVec2 size = ImGui::CalcTextSize(label.c_str());

				const ImVec2 topLeft
				{
					screen.x - size.x * 0.5f,
					screen.y - size.y
				};

				draw->AddRectFilled(
					{ topLeft.x - 5.0f, topLeft.y - 3.0f },
					{
						topLeft.x + size.x + 5.0f,
						topLeft.y + size.y + 3.0f
					},
					IM_COL32(0, 0, 0, 190),
					4.0f);

				draw->AddText(topLeft, roleColor, label.c_str());
			}

			// 既存のマーク線へ、方向を示す矢印の先端を加える
			if (validMark && footVisible)
			{
				auto targetPos = players[markTarget]->GetPosition();
				targetPos.y += 0.08f;

				ImVec2 targetScreen;

				if (ToScreen(targetPos, targetScreen))
				{
					const float dx = targetScreen.x - footScreen.x;
					const float dy = targetScreen.y - footScreen.y;
					const float length = std::sqrt(dx * dx + dy * dy);

					if (length > 20.0f)
					{
						const float dirX = dx / length;
						const float dirY = dy / length;

						const ImVec2 center
						{
							footScreen.x + dx * 0.8f,
							footScreen.y + dy * 0.8f
						};

						draw->AddTriangleFilled(
							{
								center.x + dirX * 7.0f,
								center.y + dirY * 7.0f
							},
							{
								center.x - dirX * 5.0f - dirY * 4.0f,
								center.y - dirY * 5.0f + dirX * 4.0f
							},
							{
								center.x - dirX * 5.0f + dirY * 4.0f,
								center.y - dirY * 5.0f - dirX * 4.0f
							},
							IM_COL32(255, 255, 255, 255));
					}
				}
			}
		}

		if (!move || !footVisible)
		{
			continue;
		}

		// Base・Softの詳細は、指定された選手だけ表示できる
		const bool showDetail =
			m_debugPlayerIndex < 0 || m_debugPlayerIndex == i;

		if (m_showRequestTargets && showDetail)
		{
			for (const auto& request : move->requests)
			{
				if (request.type != AIMoveRequestType::Base &&
					request.type != AIMoveRequestType::Soft)
				{
					continue;
				}

				auto target = request.targetPosition;

				// 実際の移動と同じXZ平面で表示する
				target.y = footPos.y;

				char text[96];

				if (request.type == AIMoveRequestType::Soft)
				{
					std::snprintf(
						text, sizeof(text),
						"Soft %s  w=%.2f",
						ToString(request.reason),
						request.weight);

					DrawTarget(
						footScreen, target, text,
						IM_COL32(60, 220, 255, 180), false);
				}
				else
				{
					std::snprintf(
						text, sizeof(text),
						"Base %s",
						ToString(request.reason));

					DrawTarget(
						footScreen, target, text,
						IM_COL32(130, 160, 255, 180), false);
				}
			}
		}

		if (m_showMoveTargets && move->hasFinalMove)
		{
			auto target = move->finalTarget;
			target.y = footPos.y;

			DrawTarget(
				footScreen,
				target,
				"Final",
				IM_COL32(255, 220, 40, 255),
				true);
		}
	}
#endif
}

void SoccerAISystem::DrawRequestImGui()
{
#ifdef USE_IMGUI
	if (!ImGui::CollapsingHeader("Move requests"))
	{
		return;
	}

	const auto& infos = m_moveResolver.GetDebugInfos();

	const int controlledIndex =
		m_allyTeamBB.Get<int>(
			PlayerKeys::ControlledIndex).value_or(-1);

	for (int i = 0; i < static_cast<int>(infos.size()); ++i)
	{
		// コート上の詳細表示と同じ選手指定を使う
		if (m_debugPlayerIndex >= 0 && m_debugPlayerIndex != i)
		{
			continue;
		}

		const AIPlayerMoveDebug& info = infos[i];

		ImGui::PushID(i);

		const ImGuiTreeNodeFlags flags =
			m_debugPlayerIndex >= 0
			? ImGuiTreeNodeFlags_DefaultOpen
			: 0;

		if (!ImGui::TreeNodeEx(
			"PlayerRequests",
			flags,
			"Player %d  (%d requests)",
			i,
			static_cast<int>(info.requests.size())))
		{
			ImGui::PopID();
			continue;
		}

		if (i == controlledIndex)
		{
			ImGui::TextColored(
				ImVec4(1.0f, 0.9f, 0.2f, 1.0f),
				"Manual control");
		}
		else if (info.hasFinalMove)
		{
			ImGui::TextColored(
				ImVec4(1.0f, 0.9f, 0.2f, 1.0f),
				"Final: %s / %s",
				ToString(info.finalType),
				ToString(info.finalReason));

			ImGui::Text(
				"Target: (%.2f, %.2f, %.2f)",
				info.finalTarget.x,
				info.finalTarget.y,
				info.finalTarget.z);

			ImGui::Text(
				"Actual speed: %.2f",
				info.finalMoveSpeed);
		}
		else
		{
			ImGui::TextDisabled("No final move");
		}

		// 継続中のAction
		if (info.hasCurrentAction)
		{
			ImGui::Text(
				"Current Action: %s",
				ToString(info.currentActionReason));

			ImGui::Text(
				"Elapsed: %.2f / minimum %.2f s",
				info.currentActionElapsedTime,
				info.currentActionMinCommitmentTime);
		}

		// 切り替え確認中のAction
		if (info.hasCandidateAction)
		{
			ImGui::Text(
				"Checking: %s",
				ToString(info.candidateActionReason));

			ImGui::Text(
				"Confirmation: %.2f / %.2f s",
				info.candidateElapsedTime,
				info.candidateRequiredTime);

			const float progress =
				info.candidateRequiredTime > 0.0f
				? std::clamp(
					info.candidateElapsedTime /
					info.candidateRequiredTime,
					0.0f,
					1.0f)
				: 1.0f;

			ImGui::ProgressBar(
				progress,
				ImVec2(-1.0f, 0.0f));
		}

		if (info.requests.empty())
		{
			ImGui::TextDisabled("No requests submitted");
		}
		else
		{
			// 横にスクロールできる要求一覧
			const float tableHeight =
				ImGui::GetTextLineHeightWithSpacing() *
				(static_cast<float>(info.requests.size()) + 2.0f)
				+ 32.0f;

			const ImGuiTableFlags tableFlags =
				ImGuiTableFlags_Borders |
				ImGuiTableFlags_RowBg |
				ImGuiTableFlags_Resizable |
				ImGuiTableFlags_ScrollX |
				ImGuiTableFlags_SizingFixedFit;

			if (ImGui::BeginTable(
				"Requests",
				8,
				tableFlags,
				ImVec2(0.0f, tableHeight)))
			{
				ImGui::TableSetupColumn("Type");
				ImGui::TableSetupColumn("Reason");
				ImGui::TableSetupColumn("Priority");
				ImGui::TableSetupColumn("Utility");
				ImGui::TableSetupColumn("Urgency");
				ImGui::TableSetupColumn("Weight");
				ImGui::TableSetupColumn("Target X / Z");
				ImGui::TableSetupColumn("Req speed");
				ImGui::TableHeadersRow();

				for (const auto& request : info.requests)
				{
					ImGui::TableNextRow();

					ImGui::TableSetColumnIndex(0);
					ImGui::TextUnformatted(ToString(request.type));

					ImGui::TableSetColumnIndex(1);
					ImGui::TextUnformatted(ToString(request.reason));

					ImGui::TableSetColumnIndex(2);
					ImGui::Text("%d", request.priority);

					ImGui::TableSetColumnIndex(3);
					ImGui::Text("%.2f", request.utility);

					ImGui::TableSetColumnIndex(4);
					ImGui::Text("%.2f", request.urgency);

					ImGui::TableSetColumnIndex(5);
					ImGui::Text("%.2f", request.weight);

					ImGui::TableSetColumnIndex(6);
					ImGui::Text(
						"%.2f / %.2f",
						request.targetPosition.x,
						request.targetPosition.z);

					ImGui::TableSetColumnIndex(7);
					ImGui::Text("%.2f", request.moveSpeed);
				}

				ImGui::EndTable();
			}
		}

		ImGui::TreePop();
		ImGui::PopID();
	}
#endif
}

void SoccerAISystem::UpdateSharedBlackBoard(const std::vector<std::unique_ptr<SoccerPlayer>>& players, const SoccerBall& ball, BlackBoard& sharedBB)
{
	sharedBB.Set(BallKeys::Position, ball.GetPosition());

	//既に登録されている保持者番号を取得
	const int ownerIndex = sharedBB.Get<int>(BallKeys::OwnerIndex).value_or(-1);

	//保持者がなければ、保持チームなし
	int ownerTeam = -1;

	if (ownerIndex >= 0 && ownerIndex < static_cast<int>(players.size()) && players[ownerIndex])
	{
		ownerTeam = players[ownerIndex]->GetTeamNumber();
	}

	sharedBB.Set(BallKeys::OwnerTeam, ownerTeam);
}
