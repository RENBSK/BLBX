#pragma once
#include <d3d11.h>
#include <directxmath.h>
#include <WICTextureLoader.h>
#include <wrl.h>
#include "Graphics/Resource/Texture.h"
#include "Graphics/Shader/Shader.h"


class Sprite
{
public:
	Microsoft::WRL::ComPtr< ID3D11VertexShader> vertex_shader;
	Microsoft::WRL::ComPtr< ID3D11PixelShader> pixel_shader;
	Microsoft::WRL::ComPtr< ID3D11InputLayout> input_layout;
	Microsoft::WRL::ComPtr< ID3D11Buffer> vertex_buffer;
	Microsoft::WRL::ComPtr< ID3D11ShaderResourceView> shader_resouce_view;
	//ID3D11VertexShader* vertex_shader;
	//ID3D11PixelShader* pixel_shader;
	//ID3D11InputLayout* input_layout;
	//ID3D11Buffer* vertex_buffer;
	//ID3D11ShaderResourceView* shader_resouce_view;
	D3D11_TEXTURE2D_DESC texture2d_desc;

	void render(ID3D11DeviceContext* immediate_context,
		float dx, float dy,		//短形の左上の座標(スクリーン座標)
		float dw, float dh,		//単形のサイズ(スクリーン座標)
		float r, float g, float b, float a,		//色
		float angle/*degree*/);		//回転角



	void render(ID3D11DeviceContext* immediate_context,
		float dx, float dy,		//短形の左上の座標(スクリーン座標)
		float dw, float dh,		//単形のサイズ(スクリーン座標)
		float r, float g, float b, float a,		//色
		float angle/*degree*/,		//回転角
		float sx, float sy, float sw, float sh);

	void renderCore(ID3D11DeviceContext* immediate_context,
		float dx, float dy,		//短形の左上の座標(スクリーン座標)
		float dw, float dh,		//単形のサイズ(スクリーン座標)
		float r, float g, float b, float a,		//色
		float angle/*degree*/,
		float u0, float v0, float u1, float v1);


	void textout(ID3D11DeviceContext* immediate_context, std::string s,
		float x, float y, float w, float h, float r, float g, float b, float a);



	Sprite(ID3D11Device* device, const wchar_t* filename);
	~Sprite();

	void Finalize();

	struct vertex
	{
		DirectX::XMFLOAT3 position;
		DirectX::XMFLOAT4 color;
		DirectX::XMFLOAT2 texcoord;
	};
};