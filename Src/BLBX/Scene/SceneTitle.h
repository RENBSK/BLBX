#pragma once

#include "../Resource/Sprite/Sprite.h"
#include "Scene.h"

#include "../Core/Config.h"

#include "Soccer/Ball/SoccerBall.h"
#include "Soccer/Stage/Stadium.h"
#include "Graphics/Camera/FreeCameraController.h"

//タイトルシーン
class SceneTitle :public Scene
{
public:
	SceneTitle() {}
	~SceneTitle() override {}

	//初期化
	void Initialize() override;

	//終了化
	virtual void Finalize() override;

	//更新処理
	virtual void Update(float elapsedTime) override;

	//描画処理
	virtual void Render()  override;

	//GUI描画
	virtual void DrawGUI() override;

private:
	std::unique_ptr<Sprite> sprites[8];

	FreeCameraController m_freeCamera;

	Stadium m_stadium;

	SoccerBall m_ball;

	int m_ShootCount = 0;
	float m_shootTimer = 0.0f;

	float m_spriteX = 0.0f;
	float m_spriteY = 0.0f;

	float m_spriteW = 0.0f;
	float m_spriteH = 0.0f;
};