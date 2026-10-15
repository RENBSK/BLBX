#include "ModelObject.h"

#include <DirectXMath.h>

#include"Graphics/Graphics.h"

#include "Graphics/Debug/D3D11DebugUtil.h"

ModelObject::~ModelObject()
{
	Finalize();
}

void ModelObject::Finalize()
{
	ID3D11DeviceContext* context = Graphics::Instance().GetContext();

	D3D11DebugUtil::ClearPipeline(context);

	if (m_model)
	{
		m_model->Finalize();
		m_model.reset();
	}
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

	m_model = std::make_unique<GltfModel>(
		Graphics::Instance().GetDevice(),
		filename,
		batching
	);
}
