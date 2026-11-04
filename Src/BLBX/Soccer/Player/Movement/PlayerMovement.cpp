#include "PlayerMovement.h"

#include "Soccer/Player/SoccerPlayer.h"
#include "Soccer/Ball/SoccerBall.h"

#include <cmath>
#include <algorithm>

void PlayerMovement::Update(
    SoccerPlayer& player,
    const DirectX::XMFLOAT3& direction,
    float moveSpeed,
    float elapsedTime)
{
    if (elapsedTime <= 0.0f)
    {
        return;
    }

    if (player.IsControlLocked())
    {
        m_velocity = { 0.0f, 0.0f, 0.0f };

        player.SetMovementSpeed(0.0f);
        return;
    }

    const PlayerParameters& parameters = player.GetParameters();

    //パラメーターからMaxSpeedを取ってくる
    const float maxSpeed = std::max(parameters.maxSpeed, 0.0f);
    //ドリブルの時にスピードを抑える
    const float speedLimit = player.GetHasBall() ? maxSpeed * 0.6f : maxSpeed;
    const float taregtSpeed = std::clamp(moveSpeed, 0.0f, speedLimit);

    const float inputLengthSq =
        direction.x * direction.x +
        direction.z * direction.z;

    const bool hasInput = inputLengthSq > 0.000001f;

    // 前フレームから引き継ぐのは「速さ」
    float speed = std::sqrt(
        m_velocity.x * m_velocity.x +
        m_velocity.z * m_velocity.z);

    const float previousSpeed = speed;

    DirectX::XMFLOAT3 moveDirection = { 0.0f, 0.0f, 0.0f };

    if (hasInput)
    {
        // 入力方向を正規化
        const float inputLength = std::sqrt(inputLengthSq);

        moveDirection =
        {
            direction.x / inputLength,
            0.0f,
            direction.z / inputLength
        };
        const float acceleration = std::max(parameters.acceleration, 0.0f);

        // 速さだけを徐々に上げる
        speed = std::min(speed + acceleration * elapsedTime, taregtSpeed);

        // 方向は即座に切り替える
        m_velocity =
        {
            moveDirection.x * speed,
            0.0f,
            moveDirection.z * speed
        };

        player.FaceToMoveDirection(moveDirection);
    }
    else
    {
        // 入力を離したら即停止
        speed = 0.0f;
        m_velocity = { 0.0f, 0.0f, 0.0f };
    }

    // 目標速度ではなく、実際に使用する速さを渡す
    player.SetMovementSpeed(speed);
    // ドリブルのアニメーションは上書きしない
    const bool isDribbling = player.GetAnimationState() == PlayerAnimationState::Dribble;

    if (player.IsAutoAnimationEnabled() )
    {
        const bool justStopped = !hasInput && previousSpeed > 0.01f;

        if (justStopped)
        {
            // 止まった瞬間だけ開始
            player.ChangeAnimation(PlayerAnimationState::StopRuning, false);
        }
        else if (speed > 0.01f)
        {
            if (player.GetHasBall())
            {
                player.ChangeAnimation(PlayerAnimationState::Dribble, false);
            }
            // 再入力されたら、停止アニメ中でも移動へ戻る
            else if (speed > parameters.maxSpeed * 0.6f)
            {
                player.ChangeAnimation(PlayerAnimationState::Run, false);
            }
            else
            {
                player.ChangeAnimation(PlayerAnimationState::Walk, false);
            }
        }
        else if (player.GetAnimationState() != PlayerAnimationState::StopRuning)
        {
            // 停止アニメの再生中は Idle で上書きしない
            player.ChangeAnimation(PlayerAnimationState::Idle, false);
        }
    }

    const DirectX::XMFLOAT3 move =
    {
        m_velocity.x * elapsedTime,
        0.0f,
        m_velocity.z * elapsedTime
    };

    player.Move(move);
}