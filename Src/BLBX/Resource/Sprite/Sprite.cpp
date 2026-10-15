#include "sprite.h"
#include "Core/Misc.h"
#include <sstream>


Sprite::Sprite(ID3D11Device* device, const wchar_t* filename)
{
	HRESULT hr = S_OK;

	vertex vertices[]
	{
		{ { -1.0, +1.0, 0 }, { 1, 1, 1, 1 }, { 0, 0 } },
		{ { +1.0, +1.0, 0 }, { 1, 1, 1, 1 }, { 1, 0 } },
		{ { -1.0, -1.0, 0 }, { 1, 1, 1, 1 }, { 0, 1 } },
		{ { +1.0, -1.0, 0 }, { 1, 1, 1, 1 }, { 1, 1 } },
	};


	//②頂点バッファオブジェクトの生成
	{
		D3D11_BUFFER_DESC buffer_desc{};
		buffer_desc.ByteWidth = sizeof(vertices);
		buffer_desc.Usage = D3D11_USAGE_DYNAMIC;
		buffer_desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		buffer_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		buffer_desc.MiscFlags = 0;
		buffer_desc.StructureByteStride = 0;
		D3D11_SUBRESOURCE_DATA subresource_data{};
		subresource_data.pSysMem = vertices;
		subresource_data.SysMemPitch = 0;
		subresource_data.SysMemSlicePitch = 0;
		hr = device->CreateBuffer(&buffer_desc, &subresource_data, vertex_buffer.GetAddressOf());
		_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

		Microsoft::WRL::ComPtr<ID3D11Resource> resource;

		hr = DirectX::CreateWICTextureFromFile(
			device,
			filename,
			resource.GetAddressOf(),
			shader_resouce_view.GetAddressOf()
		);
		_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

		// テクスチャ情報を取得
		Microsoft::WRL::ComPtr<ID3D11Texture2D> texture2d;
		hr = resource.As(&texture2d);
		_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

		texture2d->GetDesc(&texture2d_desc);

		//③頂点シェーダーオブジェクトの生成

		const char* cso_name{ "./Data/Shader/Sprite_vs.cso" };

		//④入力レイアウトオブジェクトの生成
		D3D11_INPUT_ELEMENT_DESC input_element_desc[]
		{
			{"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,
			  D3D11_APPEND_ALIGNED_ELEMENT,D3D11_INPUT_PER_VERTEX_DATA,0},
			{"COLOR",0,DXGI_FORMAT_R32G32B32A32_FLOAT,0,
			  D3D11_APPEND_ALIGNED_ELEMENT,D3D11_INPUT_PER_VERTEX_DATA,0},
			{"TEXCOORD",0,DXGI_FORMAT_R32G32_FLOAT,0,
			  D3D11_APPEND_ALIGNED_ELEMENT,D3D11_INPUT_PER_VERTEX_DATA,0},
		};

		create_vs_from_cso(device, cso_name, vertex_shader.GetAddressOf(), &input_layout, input_element_desc, _countof(input_element_desc));
	}

	//⑤ピクセルシェーダーオブジェクトの生成
	{
		const char* cso_name{ "./Data/Shader/Sprite_ps.cso" };
		create_ps_from_cso(device, cso_name, pixel_shader.GetAddressOf());
	}


}

Sprite::~Sprite()
{
	/*if (vertex_buffer)
	{
		vertex_buffer->Release();
		vertex_buffer = nullptr;
	}

	if (shader_resouce_view)
	{
		shader_resouce_view->Release();
		shader_resouce_view = nullptr;
	}

	if (vertex_shader)
	{
		vertex_shader->Release();
		vertex_shader = nullptr;
	}

	if (pixel_shader)
	{
		pixel_shader->Release();
		pixel_shader = nullptr;
	}

	if (input_layout)
	{
		input_layout->Release();
		input_layout = nullptr;
	}*/

	Finalize();
}

void Sprite::Finalize()
{
	shader_resouce_view.Reset();

	vertex_buffer.Reset();

	input_layout.Reset();
	pixel_shader.Reset();
	vertex_shader.Reset();
}

void Sprite::render(ID3D11DeviceContext* immediate_context, float dx, float dy, float dw, float dh, float r, float g, float b, float a, float angle)
{
	Sprite::renderCore(immediate_context, dx, dy, dw, dh, r, g, b, a, angle, 0.0f, 0.0f, 1.0f, 1.0f);
}

void Sprite::render(ID3D11DeviceContext* immediate_context, float dx, float dy, float dw, float dh, float r, float g, float b, float a, float angle, float sx, float sy, float sw, float sh)
{
	float u0 = sx / texture2d_desc.Width;
	float v0 = sy / texture2d_desc.Height;
	float u1 = (sx + sw) / texture2d_desc.Width;
	float v1 = (sy + sh) / texture2d_desc.Height;
	Sprite::renderCore(immediate_context, dx, dy, dw, dh, r, g, b, a, angle, u0, v0, u1, v1);
}

void Sprite::renderCore(ID3D11DeviceContext* immediate_context,
	float dx, float dy, float dw, float dh,
	float r, float g, float b, float a,
	float angle,
	float u0, float v0, float u1, float v1)
{

	//スクリーン（ビューポート）のサイズを取得する
	D3D11_VIEWPORT viewport{};
	UINT num_viewports{ 1 };
	immediate_context->RSGetViewports(&num_viewports, &viewport);

	//①頂点バッファーのバインド
	UINT stride{ sizeof(vertex) };
	UINT offset{ 0 };
	immediate_context->IASetVertexBuffers(0, 1, vertex_buffer.GetAddressOf(), &stride, &offset);


	//②プリミティブタイプおよびデータの順序に関する情報のバインド
	immediate_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);


	//③入力レイアウトオブジェクトのバインド
	immediate_context->IASetInputLayout(input_layout.Get());


	//④シェーダーのバインド
	immediate_context->VSSetShader(vertex_shader.Get(), nullptr, 0);
	immediate_context->PSSetShader(pixel_shader.Get(), nullptr, 0);
	immediate_context->PSSetShaderResources(0, 1, shader_resouce_view.GetAddressOf());


	// ⑤プリミティブの描画


	//render　メンバー関数の引数(dx,dy,dw,dh)から短形の各頂点の位置(スクリーン座標系)を計算する
	//(x0,y0)*--*(x1,y1)
	//		|  /|
	//		| / |
	//		|/  |
	//(x2,y2)*--*(x3,y3)

	//left-top
	float x0{ dx };
	float y0{ dy };
	//right-top
	float x1{ dx + dw };
	float y1{ dy };
	//left-bottom
	float x2{ dx };
	float y2{ dy + dh };
	//right-bottom
	float x3{ dx + dw };
	float y3{ dy + dh };

	//float u0 = sx / texture2d_desc.Width;
	//float v0 = sy / texture2d_desc.Height;
	//float u1 = (sx + sw) / texture2d_desc.Width;
	//float v1 = (sy + sh) / texture2d_desc.Height;

	//点(x,y)が点(cx,cy)を中心に角(angle)で回転した時の座標を計算する関数オブジェクト(ラムダ式)
	auto rotate = [](float& x, float& y, float cx, float cy, float angle)
		{
			x -= cx;
			y -= cy;

			float cos{ cosf(DirectX::XMConvertToRadians(angle)) };
			float sin{ sinf(DirectX::XMConvertToRadians(angle)) };
			float tx{ x }, ty{ y };
			x = cos * tx + -sin * ty;
			y = sin * tx + cos * ty;

			x += cx;
			y += cy;
		};

	//回転の中心を短形の中心点にした場合
	float cx = dx + dw * 0.5f;
	float cy = dy + dh * 0.5f;
	rotate(x0, y0, cx, cy, angle);
	rotate(x1, y1, cx, cy, angle);
	rotate(x2, y2, cx, cy, angle);
	rotate(x3, y3, cx, cy, angle);

	//スクリーン座標系からNDCへの座標変換を行う
	x0 = 2.0f * x0 / viewport.Width - 1.0f;
	y0 = 1.0f - 2.0f * y0 / viewport.Height;
	x1 = 2.0f * x1 / viewport.Width - 1.0f;
	y1 = 1.0f - 2.0f * y1 / viewport.Height;
	x2 = 2.0f * x2 / viewport.Width - 1.0f;
	y2 = 1.0f - 2.0f * y2 / viewport.Height;
	x3 = 2.0f * x3 / viewport.Width - 1.0f;
	y3 = 1.0f - 2.0f * y3 / viewport.Height;

	//計算結果で頂点バッファオブジェクトを更新する
	HRESULT hr{ S_OK };
	D3D11_MAPPED_SUBRESOURCE mapped_subresource{};
	hr = immediate_context->Map(vertex_buffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_subresource);
	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

	vertex* vertices{ reinterpret_cast<vertex*>(mapped_subresource.pData) };
	if (vertices != nullptr)
	{
		vertices[0].position = { x0,y0,0 };
		vertices[1].position = { x1,y1,0 };
		vertices[2].position = { x2,y2,0 };
		vertices[3].position = { x3,y3,0 };
		vertices[0].color = vertices[1].color = vertices[2].color = vertices[3].color = { r,g,b,a };

		vertices[0].texcoord = { u0,v0 };
		vertices[1].texcoord = { u1,v0 };
		vertices[2].texcoord = { u0,v1 };
		vertices[3].texcoord = { u1,v1 };
	}
	immediate_context->Unmap(vertex_buffer.Get(), 0);

	immediate_context->Draw(4, 0);

}

void Sprite::textout(ID3D11DeviceContext* immediate_context, std::string s, float x, float y, float w, float h, float r, float g, float b, float a)
{
	float sw = static_cast<float>(texture2d_desc.Width / 16);
	float sh = static_cast<float>(texture2d_desc.Height / 16);
	float carriage = 0;
	for (const char c : s)
	{
		render(immediate_context, x + carriage, y, w, h, r, g, b, a, 0,
			sw * (c & 0x0F), sh * (c >> 4), sw, sh);
		carriage += w;
	}
}


