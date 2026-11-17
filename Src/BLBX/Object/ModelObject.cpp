#include "ModelObject.h"

#include <DirectXMath.h>

#include"Graphics/Graphics.h"

#include "Graphics/Debug/D3D11DebugUtil.h"

#include <unordered_map>
#include <memory>
#include <string>

ModelObject::~ModelObject()
{
	Finalize();
}

void ModelObject::Finalize()
{
	ID3D11DeviceContext* context =
		Graphics::Instance().GetContext();

	if (context)
	{
		D3D11DebugUtil::ClearPipeline(context);
	}

	// 自分が持つ参照だけを解除
	m_model.reset();
}

void ModelObject::Render()
{
	if (!m_model) return;

	DirectX::XMFLOAT4X4 world;
	DirectX::XMStoreFloat4x4(&world, m_transform.GetMatrix());

	m_model->render(Graphics::Instance().GetContext(), world);
}

void ModelObject::LoadModel(const char* filename, bool batching)
{
	Finalize();

	// 生成済みモデルを共有するためのキャッシュ
	static std::unordered_map<
		std::string,
		std::shared_ptr<GltfModel>
	> modelCache;

	// 同じファイルでもバッチング設定が違えば別モデル
	std::string key = std::string(filename)
		+ (batching ? "|batch" : "|normal");

	auto it = modelCache.find(key);

	if (it != modelCache.end())
	{
		// 生成済みモデルを使う
		m_model = it->second;

		OutputDebugStringA("GltfModel Cache HIT\n");
		return;
	}

	// 初回のみモデルを生成
	m_model = std::make_shared<GltfModel>(
		Graphics::Instance().GetDevice(),
		filename,
		batching
	);

	// キャッシュに保存
	modelCache.emplace(key, m_model);

	OutputDebugStringA("GltfModel Cache MISS\n");
}
