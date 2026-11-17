#pragma once

#include "Object/ModelObject.h"


class SoccerBall:public ModelObject
{
public:
	//初期化
	void Initialize() override;
	//終了化
	void Finalize();

	void Update(float elapsedTime) override;
	void Render() override;
	void DrawImGui() override;

	//物理処理
	void BounceOffWalls();

	//ドリブル
	void SetDribbleVelocity(
		const DirectX::XMFLOAT3& velocity);
	//パス
	void Kick(const DirectX::XMFLOAT3& velocity);
	//完全停止
	void Stop();
	//移動速度の取得
	const DirectX::XMFLOAT3& GetVelocity() const
	{
		return m_velocity;
	}
private:
	//重力
	float m_gravity = -9.81f;
	//オンオフスイッチ
	bool m_useGravity = false;
	//地面
	float m_ground = 0.7f;

	// ボールの移動速度
	DirectX::XMFLOAT3 m_velocity{ 0.0f, 0.0f, 0.0f };

	// 移動時の減速
	float m_friction = 1.0f;

	//反射係数
	float m_restitution = 0.8f;
};