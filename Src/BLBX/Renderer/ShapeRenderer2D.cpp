#include "ShapeRenderer2D.h"

#include "Graphics/Shader/Shader.h"

void ShapeRenderer2D::Initialize(ID3D11Device* device)
{
	Finalize();

	this->m_device = device;

	D3D11_BUFFER_DESC vbDesc = {};
	vbDesc.ByteWidth = sizeof(Vertex) * 6;
	vbDesc.Usage = D3D11_USAGE_DYNAMIC;
	vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	device->CreateBuffer(&vbDesc, nullptr, &m_vertexBuffer);

	D3D11_BUFFER_DESC cbDesc{};
	cbDesc.ByteWidth = sizeof(CB);
	cbDesc.Usage = D3D11_USAGE_DEFAULT;
	cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

	this->m_device->CreateBuffer(&cbDesc, nullptr, &m_constantBuffer);

	D3D11_INPUT_ELEMENT_DESC layout[] =
	{
		{"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,0,D3D11_INPUT_PER_VERTEX_DATA,0},
		{"COLOR",0,DXGI_FORMAT_R32G32B32A32_FLOAT,0,12,D3D11_INPUT_PER_VERTEX_DATA,0},
		{"TEXCOORD",0,DXGI_FORMAT_R32G32_FLOAT,0,28,D3D11_INPUT_PER_VERTEX_DATA,0},
		{"TEXCOORD",1,DXGI_FORMAT_R32_FLOAT,0,36,D3D11_INPUT_PER_VERTEX_DATA,0},
	};

	create_vs_from_cso(m_device, "./Data/Shader/Shape2D_vs.cso", &m_vs, &m_inputLayout, layout, 4);
	create_ps_from_cso(m_device, "./Data/Shader/Shape2D_ps.cso", &m_ps);


}

#define SAFE_RELEASE(p) if (p) { p->Release(); p = nullptr; }

void ShapeRenderer2D::Finalize()
{
	SAFE_RELEASE(m_vertexBuffer);
	SAFE_RELEASE(m_constantBuffer);

	SAFE_RELEASE(m_vs);
	SAFE_RELEASE(m_ps);
	SAFE_RELEASE(m_inputLayout);

	m_device = nullptr;
	m_context = nullptr;
}

void ShapeRenderer2D::Begin(ID3D11DeviceContext* dc)
{
	m_context = dc;

	UINT stride = sizeof(Vertex);
	UINT offset = 0;

	m_context->IASetVertexBuffers(0, 1, &m_vertexBuffer, &stride, &offset);
	m_context->IASetInputLayout(m_inputLayout);

	m_context->VSSetShader(m_vs, nullptr, 0);
	m_context->PSSetShader(m_ps, nullptr, 0);

	m_context->VSSetConstantBuffers(0, 1, &m_constantBuffer);

	m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	D3D11_VIEWPORT vp{};
	vp.Width = 1280.0f;
	vp.Height = 720.0f;
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	m_context->RSSetViewports(1, &vp);

	CB cb;
	cb.projection = DirectX::XMMatrixOrthographicOffCenterLH(
		0.0f, 1280.0f,
		720.0f, 0.0f,
		0.0f, 1.0f
	);

	m_context->UpdateSubresource(m_constantBuffer, 0, nullptr, &cb, 0, 0);
}

void ShapeRenderer2D::End()
{
}

void ShapeRenderer2D::DrawPoint(float x, float y, float size, DirectX::XMFLOAT4 color)
{
	Vertex v[6];

	float half = size * 0.5f;

	float left = x - half;
	float right = x + half;
	float top = y - half;
	float bottom = y + half;

	v[0] = { {left,top,0},color,{0,0},1.0f };
	v[1] = { {right,top,0},color, {1,0},1.0f };
	v[2] = { {left,bottom,0},color,{0,1},1.0f };

	v[3] = { {left,bottom,0},color,{0,1},1.0f };
	v[4] = { {right,top,0},color,{1,0},1.0f };
	v[5] = { {right,bottom,0},color,{1,1},1.0f };

	D3D11_MAPPED_SUBRESOURCE mapped;
	m_context->Map(m_vertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
	memcpy(mapped.pData, v, sizeof(v));
	m_context->Unmap(m_vertexBuffer, 0);

	//m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	m_context->Draw(6, 0);
}

void ShapeRenderer2D::DrawBox(float x, float y, float w, float h, DirectX::XMFLOAT4 color)
{
	Vertex v[6];

	float left = x;
	float right = x + w;
	float top = y;
	float bottom = y + h;

	v[0] = { {left,top,0},color,{0,0},0.0f };
	v[1] = { {right,top,0},color,{1,0},0.0f };
	v[2] = { {left,bottom,0},color,{0,1},0.0f };

	v[3] = { {left,bottom,0},color,{0,1},0.0f };
	v[4] = { {right,top,0},color,{1,0},0.0f };
	v[5] = { {right,bottom,0},color,{1,1},0.0f };

	D3D11_MAPPED_SUBRESOURCE mapped;
	m_context->Map(m_vertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
	memcpy(mapped.pData, v, sizeof(v));
	m_context->Unmap(m_vertexBuffer, 0);

	m_context->Draw(6, 0);
}
