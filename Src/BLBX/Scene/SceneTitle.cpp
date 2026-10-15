#include "../Graphics/Graphics.h"
#include "SceneTitle.h"
#include "SceneManager.h"
#include "../Renderer/ShapeRenderer2D.h"


//初期化
void SceneTitle::Initialize()
{
	sprites[0] = std::make_unique<Sprite>(Graphics::Instance().GetDevice(), L"./Data/Sprite/SoccerCourt.png");
}

//終了化
void SceneTitle::Finalize()
{
}

//更新処理
void SceneTitle::Update(float elaspedTime)
{
}

//描画処理
void SceneTitle::Render()
{
	//2Dスプライト描画
	{
		sprites[0]->render(Graphics::Instance().GetContext(), 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 1, 1, 1, 1, 0);
	}


}

//GUI描画
void SceneTitle::DrawGUI()
{
}