#include "SoccerPlayer.h"
#include "Graphics/Graphics.h"
#include <cmath>

#ifdef  USE_IMGUI
#include "imgui.h"
#endif //  USE_IMGUI


void SoccerPlayer::Initialize()
{
	if (!m_config.modelPath.empty())
	{
		LoadModel(m_config.modelPath.c_str(), false);

		if (m_model)
		{
			//ノードの取得
			m_animatedNodes = m_model->GetNodes();

			m_transform.position = { 0.0f,0.64f,-0.38f };
			m_transform.scale = { 0.005f,0.005f,0.005f };
		}
	}

	//能力値から内部パラメーターを生成
	m_parameters = BuildPlayerParameters(m_config.ability);

	m_animationTime = 0.0f;
}

void SoccerPlayer::Finalize()
{
	ModelObject::Finalize();
	m_animatedNodes.clear();
}

void SoccerPlayer::Update(float elapsedTime)
{
	if (!m_model)
	{
		return;
	}

	// モデルの初期姿勢に戻す
	m_animatedNodes = m_model->GetNodes();

	// Idleなら初期姿勢のまま描画
	if (m_animationState == PlayerAnimationState::Idle)
	{
		return;
	}

	const auto& animation = m_model->GetAnimations();

	//現在のアニメーション設定
	const auto& setting = m_config.animations[
		static_cast<size_t>(m_animationState)];

	//現在からアニメーションを検索
	for (size_t i = 0; i < animation.size(); ++i)
	{
		if (animation[i].name != setting.name)
		{
			continue;
		}

		float duration = animation[i].duration;

		if (duration <= 0.0f)
		{
			return;
		}

		m_animationTime += elapsedTime * setting.playbackSpeed;

		//ループ処理
		if (setting.loop)
		{
			m_animationTime = std::fmod(m_animationTime, duration);
		}
		else
		{
			m_animationTime = std::min(m_animationTime, duration);
		}

		//初期姿勢を取得
		m_animatedNodes = m_model->GetNodes();

		//アニメーションを再生
		m_model->animate(i, m_animationTime, m_animatedNodes);

		break;
	}
}

void SoccerPlayer::Render()
{
	if (!m_model)
	{
		return;
	}

	//ModelObject::Render();

	// ワールド行列を計算
		DirectX::XMFLOAT4X4 world;
		DirectX::XMStoreFloat4x4(
			&world,
			m_transform.GetMatrix()
		);

		// アニメーションを反映して描画
		m_model->render(
			Graphics::Instance().GetContext(),
			world,
			m_animatedNodes
		);
}

void SoccerPlayer::DrawImGui()
{
#ifdef USE_IMGUI
	if (ImGui::Begin("Player Settings"))
	{
		//位置
		ImGui::DragFloat3(
			"Position",
			&m_transform.position.x,
			0.01f);

		// 拡大縮小
		ImGui::DragFloat3(
			"Scale",
			&m_transform.scale.x,
			0.001f,
			0.001f,
			1.0f);


		PlayerConfig config = m_config;

		int speed = static_cast<int>(config.ability.speed);
		int explosiveness = static_cast<int>(config.ability.explosiveness);
		int technique = static_cast<int>(config.ability.technique);

		bool changed = false;

		changed |= ImGui::Combo(
			"Speed", &speed, "Low\0Normal\0High\0");

		changed |= ImGui::Combo(
			"Explosiveness", &explosiveness, "Low\0Normal\0High\0");

		changed |= ImGui::Combo(
			"Technique", &technique, "Low\0Normal\0High\0");

		if (changed)
		{
			config.ability.speed = static_cast<AbilityLevel>(speed);
			config.ability.explosiveness = static_cast<AbilityLevel>(explosiveness);
			config.ability.technique = static_cast<AbilityLevel>(technique);

			SetConfig(config);
		}

		if (ImGui::CollapsingHeader("Internal Parameters"))
		{
			ImGui::Text("Max Speed: %.2f", m_parameters.maxSpeed);
			ImGui::Text("Acceleration: %.2f", m_parameters.acceleration);
			ImGui::Text("Deceleration: %.2f", m_parameters.deceleration);
			ImGui::Text("Turn Speed: %.2f", m_parameters.turnSpeed);
			ImGui::Text("Ball Control: %.2f", m_parameters.ballControl);
		}

		if (ImGui::CollapsingHeader(
			"Animation Debug",
			ImGuiTreeNodeFlags_DefaultOpen))
		{
			if (m_model)
			{
				const auto& animations = m_model->GetAnimations();

				ImGui::Text("Animation Count: %zu", animations.size());
				ImGui::Text("Current Time: %.3f", m_animationTime);
				ImGui::Text("Node Count: %zu", m_animatedNodes.size());

				if (!animations.empty())
				{
					ImGui::Text("Animation Name: %s",
						animations[0].name.c_str());

					ImGui::Text("Duration: %.3f",
						animations[0].duration);

					ImGui::Text("Channel Count: %zu",
						animations[0].channels.size());
				}
			}

			const auto& animations = m_model->GetAnimations();

			const auto& setting = m_config.animations[
				static_cast<size_t>(m_animationState)];

			ImGui::Text("Selected: %s", setting.name.c_str());
			ImGui::Text("Time: %.2f", m_animationTime);

			ImGui::Separator();

			for (size_t i = 0; i < animations.size(); ++i)
			{
				ImGui::Text("%zu: %s (%.2fs)",
					i,
					animations[i].name.c_str(),
					animations[i].duration);
			}
		}

		if (ImGui::CollapsingHeader(
			"Animation Control",
			ImGuiTreeNodeFlags_DefaultOpen))
		{
			if (ImGui::Button("Idle"))
			{
				ChangeAnimation(PlayerAnimationState::Idle, true);
			}

			ImGui::SameLine();

			if (ImGui::Button("Walk"))
			{
				ChangeAnimation(PlayerAnimationState::Walk, true);
			}

			ImGui::SameLine();

			if (ImGui::Button("Run"))
			{
				ChangeAnimation(PlayerAnimationState::Run, true);
			}

			ImGui::SameLine();

			if (ImGui::Button("Dribble"))
			{
				ChangeAnimation(PlayerAnimationState::Dribble, true);
			}

			ImGui::Text("Time: %.2f", m_animationTime);
		}


	}

	ImGui::End();

#endif
}

void SoccerPlayer::Move(const DirectX::XMFLOAT3& move)
{
	FaceToMoveDirection(move);

	DirectX::XMVECTOR MoveVector = DirectX::XMLoadFloat3(&move);

	DirectX::XMFLOAT3 scaleMove;

	DirectX::XMStoreFloat3(&scaleMove, MoveVector);

	m_transform.position.x += scaleMove.x;
	m_transform.position.y += scaleMove.y;
	m_transform.position.z += scaleMove.z;
}

void SoccerPlayer::FaceToMoveDirection(const DirectX::XMFLOAT3& move)
{
	float lengthSq = move.x * move.x + move.z * move.z;

	if (lengthSq < 0.00001f)
	{
		return;
	}

	//移動方向の角度
	float yaw = atan2f(move.x, move.z);
	
	m_transform.rotation.y = yaw;
}

void SoccerPlayer::ChangeAnimation(PlayerAnimationState state,bool restart)
{
	if (state == PlayerAnimationState::Count)
	{
		return;
	}

	// 同じアニメーションなら変更しない
	// ただしrestartがtrueなら再生し直す
	if (m_animationState == state && !restart)
	{
		return;
	}

	m_animationState = state;
	m_animationTime = 0.0f;
}

void SoccerPlayer::SetConfig(const PlayerConfig config)
{
	m_config = config;
	m_parameters = BuildPlayerParameters(m_config.ability);
}

DirectX::XMFLOAT3 SoccerPlayer::GetForward() const
{
	float yaw = m_transform.rotation.y;

	DirectX::XMFLOAT3 forward;
	forward.x = sinf(yaw);
	forward.y = 0.0f;
	forward.z = cosf(yaw);

	return forward;
}
