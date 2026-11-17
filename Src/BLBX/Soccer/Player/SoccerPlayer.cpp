#include "SoccerPlayer.h"
#include "Soccer/Ball/SoccerBall.h"

#include "Graphics/Graphics.h"

#include <algorithm>
#include <cmath>

#ifdef  USE_IMGUI
#include "imgui.h"
#endif //  USE_IMGUI


void SoccerPlayer::Initialize()
{
    //モデル読み込み
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


    m_passKicked = false;
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

    // ========================================
    // アニメーション更新
    // ========================================

    // 毎フレーム初期姿勢からアニメーションを計算する
	m_animatedNodes = m_model->GetNodes();

    if (m_animationState != PlayerAnimationState::Tpose)
    {
        UpdateAnimation(elapsedTime);
    }

    // ========================================
    // アニメーションブレンド
    // ========================================
    if (m_isBlending && m_blendDuration > 0.0f)
    {
        m_blendTime += elapsedTime;

        // ブレンドの進行率を0～1に制限する
        float blendFactor = std::clamp(
            m_blendTime / m_blendDuration,
            0.0f,
            1.0f);

        // 切り替え前の姿勢から現在の姿勢へ補間する
        m_model->BlendPoses(
            m_blendFromNodes,
            m_animatedNodes,
            blendFactor);

        // ブレンド完了時に保存した姿勢を破棄する
        if (blendFactor >= 1.0f)
        {
            m_isBlending = false;
            m_blendFromNodes.clear();
        }
    }

    // ========================================
    // アニメーション終了判定
    // ========================================
    const auto& setting = m_config.animations[static_cast<size_t>(m_animationState)];

    // 操作ロック付きアニメーションの終了後はIdleへ戻す
    if (m_autoAnimation && setting.lockControl && m_animationFinished)
    {
        ChangeAnimation(PlayerAnimationState::Idle, false);
    }
}

void SoccerPlayer::Render()
{
	if (!m_model)
	{
		return;
	}

    // ワールド行列を計算
		DirectX::XMFLOAT4X4 world;
		DirectX::XMStoreFloat4x4(
			&world,
			m_transform.GetMatrix()
		);

		// アニメーションを反映したモデルを描画
		m_model->render(
			Graphics::Instance().GetContext(),
			world,
			m_animatedNodes
		);
}

void SoccerPlayer::Move(const DirectX::XMFLOAT3& move)
{
	DirectX::XMVECTOR MoveVector = DirectX::XMLoadFloat3(&move);

	DirectX::XMFLOAT3 scaleMove;
	DirectX::XMStoreFloat3(&scaleMove, MoveVector);

    // 移動量を現在位置へ加算
	m_transform.position.x += scaleMove.x;
	m_transform.position.y += scaleMove.y;
	m_transform.position.z += scaleMove.z;
}

bool SoccerPlayer::Pass(
    SoccerBall& ball, 
    const DirectX::XMFLOAT3& targetPosition, 
    float passSpeed)
{
    if (passSpeed <= 0.0f)
    {
        return false;
    }

    const DirectX::XMFLOAT3 BallPos = ball.GetPosition();
    float dx = targetPosition.x - BallPos.x;
    float dz = targetPosition.z - BallPos.z;

    const float distance = std::sqrt(dx * dx + dz * dz);

    if (distance < 0.00001f)
    {
        return false;
    }

    if (GetAnimationState() != PlayerAnimationState::Pass_Right)
    {
        ChangeAnimation(PlayerAnimationState::Pass_Right, false);
        m_passKicked = false;
    }

    //仮でアニメションの35%で蹴る
    constexpr float kickPhase = 0.30f;

    if (GetAnimationPhase() < kickPhase||m_passKicked)
    {
        return false;
    }

    const float scale = passSpeed / distance;

    ball.Kick({ dx * scale,0.0f,dz * scale });
    m_passKicked = true;
    return true;
}

bool SoccerPlayer::Shoot(DirectX::XMFLOAT3 GoalPos, SoccerBall& ball, float shootSpeed)
{
    if (!GetHasBall() || shootSpeed <= 0.0f)
    {
        return false;
    }

    ChangeAnimation(PlayerAnimationState::Shoot, false);

    //仮でアニメションの35%で蹴る
    constexpr float kickPhase = 0.30f;

    if (GetAnimationPhase() < kickPhase)
    {
        return false;
    }

    const auto forward = GetForward();


    ball.Kick({ forward.x * shootSpeed,0.0f,forward.z * shootSpeed });
    return true;
}

void SoccerPlayer::FaceToMoveDirection(const DirectX::XMFLOAT3& move)
{
	float lengthSq = move.x * move.x + move.z * move.z;

	if (lengthSq < 0.00001f)
	{
		return;
	}

    // 移動方向からY軸の回転角度を求める
	float yaw = atan2f(move.x, move.z);
	
	m_transform.rotation.y = yaw;
}

void SoccerPlayer::UpdateMovement(
    const DirectX::XMFLOAT3& direction, 
    float moveSpeed, 
    float elapsedTime)
{
    m_Playermovement.Update(*this, direction, moveSpeed, elapsedTime);
}


void SoccerPlayer::UpdateAnimation(float elapsedTime)
{
	const auto& animation = m_model->GetAnimations();

    // 現在のアニメーション設定を取得
    const auto& setting = m_config.animations[static_cast<size_t>(m_animationState)];

    // ========================================
    // アニメーションの検索・時間更新
    // ========================================
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

        // 基本の再生速度
        float playbackSpeed = setting.playbackSpeed;

        // 自動アニメーション中は移動速度に合わせて再生速度を調整
        if (m_autoAnimation &&
            (m_animationState == PlayerAnimationState::Walk ||
                m_animationState == PlayerAnimationState::Run))
        {
            // 各モーションを通常速度で再生するときの基準移動速度
            float referenceSpeed =
                (m_animationState == PlayerAnimationState::Walk)
                ? m_parameters.maxSpeed * 0.35f
                : m_parameters.maxSpeed * 0.85f;

            if (referenceSpeed > 0.0001f)
            {
                float speedRatio = std::clamp(
                    m_movementSpeed / referenceSpeed,
                    0.5f,
                    1.5f);

                playbackSpeed *= speedRatio;
            }
        }

        // 移動速度に応じてアニメーションを進める
        m_animationTime += elapsedTime * playbackSpeed;

        // ループ設定に応じて再生時間を制御
		if (setting.loop)
		{
			m_animationTime = std::fmod(m_animationTime, duration);
			m_animationFinished = false;
		}
		else
		{
			m_animationTime = std::min(m_animationTime, duration);
			m_animationFinished = (m_animationTime >= duration);
		}

        // ========================================
        // アニメーション姿勢の計算
        // ========================================

        // 初期姿勢を取得してアニメーションを適用する
		m_animatedNodes = m_model->GetNodes();

		m_model->animate(i, m_animationTime, m_animatedNodes);

		break;
	}
}

void SoccerPlayer::ChangeAnimation(PlayerAnimationState state,bool restart)
{
	if (state == PlayerAnimationState::Count)
	{
		return;
	}

    // 同じアニメーションは再開始要求がない限り変更しない
	if (m_animationState == state && !restart)
	{
		return;
	}

    // ========================================
    // ブレンドの準備
    // ========================================
    const AnimationClipSetting& setting = m_config.animations[static_cast<size_t>(state)];

	m_blendDuration = setting.blendDuration;
	m_blendTime = 0.0f;

    // ブレンド可能なら現在の姿勢を保存する
	if (m_model &&
		m_blendDuration > 0.0f &&
		m_animatedNodes.size() == m_model->GetNodes().size())
	{
		m_blendFromNodes = m_animatedNodes;
		m_isBlending = true;
	}
	else
	{
		m_blendFromNodes.clear();
		m_isBlending = false;
	}

    // ========================================
    // アニメーション状態の切り替え
    // ========================================
	m_animationState = state;
	m_animationTime = 0.0f;
	m_animationFinished = false;
}

bool SoccerPlayer::IsControlLocked() const
{
	const auto& setting = m_config.animations[static_cast<size_t>(m_animationState)];

    // ロック対象のアニメーションが再生中か判定
	return setting.lockControl && !m_animationFinished;
}

void SoccerPlayer::SetConfig(const PlayerConfig config)
{
	m_config = config;

    // 設定の変更を内部パラメーターに反映
	m_parameters = BuildPlayerParameters(m_config.ability);
}

float SoccerPlayer::GetAnimationPhase() const
{
    if (!m_model)
    {
        return 0.0f;
    }

    const auto& animations = m_model->GetAnimations();

    const auto& setting = m_config.animations[static_cast<size_t>(m_animationState)];

    for (const auto& animation : animations)
    {
        if (animation.name != setting.name)
        {
            continue;
        }

        if (animation.duration <= 0.0f)
        {
            return 0.0f;
        }

        return std::clamp(
            m_animationTime / animation.duration,
            0.0f,
            1.0f);
    }

    return 0.0f;
}

DirectX::XMFLOAT3 SoccerPlayer::GetForward() const
{
	float yaw = m_transform.rotation.y;

    // Y軸の回転角度から前方ベクトルを求める
	DirectX::XMFLOAT3 forward;
	forward.x = sinf(yaw);
	forward.y = 0.0f;
	forward.z = cosf(yaw);

	return forward;
}

void SoccerPlayer::DrawImGui()
{
#ifdef USE_IMGUI

    {
        // ========================================
        // Transform
        // ========================================
        if (ImGui::CollapsingHeader(
            "Transform",
            ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::DragFloat3(
                "Position",
                &m_transform.position.x,
                0.01f);

            ImGui::DragFloat3(
                "Scale",
                &m_transform.scale.x,
                0.001f,
                0.001f,
                1.0f);
        }

        // ========================================
        // Ability
        // ========================================
        if (ImGui::CollapsingHeader("Ability"))
        {
            PlayerConfig config = m_config;

            int speed =
                static_cast<int>(config.ability.speed);

            int explosiveness =
                static_cast<int>(config.ability.explosiveness);

            int technique =
                static_cast<int>(config.ability.technique);

            bool changed = false;

            changed |= ImGui::Combo(
                "Speed", &speed, "Low\0Normal\0High\0");

            changed |= ImGui::Combo(
                "Explosiveness", &explosiveness,
                "Low\0Normal\0High\0");

            changed |= ImGui::Combo(
                "Technique", &technique,
                "Low\0Normal\0High\0");

            if (changed)
            {
                config.ability.speed =
                    static_cast<AbilityLevel>(speed);

                config.ability.explosiveness =
                    static_cast<AbilityLevel>(explosiveness);

                config.ability.technique =
                    static_cast<AbilityLevel>(technique);

                SetConfig(config);
            }
        }

        // ========================================
        // Internal Parameters
        // ========================================
        if (ImGui::CollapsingHeader("Internal Parameters"))
        {
            ImGui::SliderFloat(
                "Max Speed",
                &m_parameters.maxSpeed,
                0.1f,
                10.0f,
                "%.2f");

            ImGui::SliderFloat(
                "Acceleration",
                &m_parameters.acceleration,
                0.1f,
                10.0f,
                "%.2f");

            ImGui::SliderFloat(
                "Deceleration",
                &m_parameters.deceleration,
                0.1f,
                10.0f,
                "%.2f");

            ImGui::SliderFloat(
                "Turn Speed",
                &m_parameters.turnSpeed,
                0.1f,
                10.0f,
                "%.2f");

            ImGui::SliderFloat(
                "Ball Control",
                &m_parameters.ballControl,
                0.1f,
                10.0f,
                "%.2f");

        }

        // ========================================
        // Animation
        // ========================================
        if (ImGui::CollapsingHeader(
            "Animation",
            ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::Checkbox(
                "Auto Animation",
                &m_autoAnimation);

            ImGui::Separator();

            // ------------------------------------
            // アニメーション選択
            // ------------------------------------
            for (size_t i = 0; i < m_config.animations.size(); ++i)
            {
                const AnimationClipSetting& clip =
                    m_config.animations[i];

                if (i > 0 && i % 4 != 0)
                {
                    ImGui::SameLine();
                }

                ImGui::PushID(static_cast<int>(i));

                const char* name = clip.name.empty()
                    ? "(Unnamed)"
                    : clip.name.c_str();

                if (ImGui::Button(name))
                {
                    m_autoAnimation = false;

                    ChangeAnimation(
                        static_cast<PlayerAnimationState>(i),
                        true);
                }

                ImGui::PopID();
            }

            ImGui::Separator();

            // ------------------------------------
            // 選択中のアニメーション設定
            // ------------------------------------
            size_t index =
                static_cast<size_t>(m_animationState);

            if (index < m_config.animations.size())
            {
                AnimationClipSetting& setting =
                    m_config.animations[index];

                ImGui::Text(
                    "Selected: %s",
                    setting.name.c_str());

                ImGui::Checkbox(
                    "Loop",
                    &setting.loop);

                ImGui::Checkbox(
                    "Lock Control",
                    &setting.lockControl);

                ImGui::SliderFloat(
                    "Playback Speed",
                    &setting.playbackSpeed,
                    0.0f,
                    2.0f,
                    "%.2fx");

                ImGui::SliderFloat(
                    "Blend Duration",
                    &setting.blendDuration,
                    0.0f,
                    1.0f,
                    "%.2f s");
            }

            ImGui::Separator();

            // ------------------------------------
            // アニメーション再生状況
            // ------------------------------------
            ImGui::Text(
                "Animation Time: %.3f",
                m_animationTime);

            ImGui::Text(
                "Finished: %s",
                m_animationFinished ? "YES" : "NO");

            ImGui::Text(
                "Blending: %s",
                m_isBlending ? "YES" : "NO");

            ImGui::Text(
                "Blend Time: %.3f / %.3f",
                m_blendTime,
                m_blendDuration);

            if (m_blendDuration > 0.0f)
            {
                float progress = std::clamp(
                    m_blendTime / m_blendDuration,
                    0.0f,
                    1.0f);

                ImGui::ProgressBar(progress);
            }

            // ------------------------------------
            // GLB内のアニメーション一覧
            // ------------------------------------
            if (m_model &&
                ImGui::TreeNode("Loaded Animations"))
            {
                const auto& animations =
                    m_model->GetAnimations();

                ImGui::Text(
                    "Animation Count: %zu",
                    animations.size());

                ImGui::Text(
                    "Node Count: %zu",
                    m_animatedNodes.size());

                ImGui::Separator();

                for (size_t i = 0; i < animations.size(); ++i)
                {
                    ImGui::Text(
                        "%zu: %s (%.2fs)",
                        i,
                        animations[i].name.c_str(),
                        animations[i].duration);
                }

                ImGui::TreePop();
            }
        }
    }


#endif
}