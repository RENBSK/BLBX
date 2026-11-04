#pragma once

#include "Soccer/Player/Controller/PlayerController.h"

class SoccerPlayer;
class SoccerBall;

enum class DribbleTupe
{
	Close,
	Dash,
	Heavy
};

class DribbleController
{
public:
    void UpdateDribble(SoccerPlayer& player, SoccerBall& ball, const DirectX::XMFLOAT3& playerVelocity, DirectX::XMFLOAT3 direction, float elapsedTime);

    DirectX::XMFLOAT3 GetPlayerMoveDirection(const DirectX::XMFLOAT3& inputDirection) const;

    void Reset()
    {
        m_hasPreviousPhase = false;
        m_previousPhase = 0.0f;
        m_dribbleDirection = {};
    }
private:
    // アニメーション内のタッチタイミング
    float m_rightTouchPhase[2] = { 0.15f,1.0f };
    float m_leftTouchPhase[2] = { 0.55f,1.45f };

    // ボールの制御パラメーター
    float m_touchRadius = 0.2f;
    float m_footDistance = 0.2f;
    float m_followStrength = 4.0f;

    // 前フレームのアニメーション再生位置
    float m_previousPhase = 0.0f;
    bool m_hasPreviousPhase = false;

    float m_touchBoost = 0.0f;


    float m_dribbleDistance = 0.15f;
    DirectX::XMFLOAT3 m_dribbleDirection{};
};

