#pragma once

#include "../Resource/Sprite/Sprite.h"
#include "Scene.h"

#include "../Core/Config.h"

#include "Soccer/Ball/SoccerBall.h"
#include "Graphics/Camera/FreeCameraController.h"

//タイトルシーン
class SceneResult :public Scene
{
public:
	SceneResult() {}
	~SceneResult() override {}

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
};