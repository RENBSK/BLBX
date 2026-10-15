#pragma once

#include <d3d11.h>
#include <wrl.h>
#include <mutex>
#include <memory>

#include "DirectXMath.h"
#include "../Core/Misc.h"
#include "SceneConstants.h"

#include "Renderer/PrimitiveRenderer.h"

template<typename T>
using ComPtr = Microsoft::WRL::ComPtr<T>;

enum class SAMPLER_STATE
{
	POINT,
	LINEAR,
	ANISOTROPIC,
	LINEAR_BORDER_BLACK,
	LINEAR_BORDER_WHITE
};

class Graphics
{
private:
	Graphics() = default;
	~Graphics() = default;
public:
	//シングルトン
	static Graphics& Instance()
	{
		static Graphics instance;
		return instance;
	}
	//初期化
	void Initialize(HWND hWnd);

	void Finalize();
	//画面クリア
	void Clear(float r, float g, float b, float a);
	// 即時コンテキスト取得
	void SetRenderTargets();
	//表示
	void Present(UINT syncInterval);
	//定数バッファー更新
	void UpdateSceneConstants(const SceneConstants& data);
	//Sampler
	void SetSampler(SAMPLER_STATE state, UINT slot = 0);

	//Depth
	void SetDepthState(UINT index);

	//Blend
	void SetBlendState(UINT index);

	//Rasterizer
	void SetRasterizerState(UINT index);

	// スクリーン幅取得
	float GetScreenWidth() const { return m_screenWidth; }

	// スクリーン高さ取得
	float GetScreenHeight() const { return m_screenHeight; }

	// プリミティブレンダラ取得
	PrimitiveRenderer* GetPrimitiveRenderer() const { return m_primitiveRenderer.get(); }

	//デバイス取得
	ID3D11Device* GetDevice() const { return m_device.Get(); }
	// RT と DSV を OM ステージに設定
	ID3D11DeviceContext* GetContext() const { return m_immediate_context.Get(); }
	//スワップチェーン取得
	IDXGISwapChain* GetSwapChain() const { return m_swap_chain.Get(); }

	ID3D11Buffer* GetConstantBuffer() const { return m_constant_buffer[0].Get(); }

	//ミューテックス取得
	std::mutex& GetMutex() { return mutex; }
private:
	void CreateDeviceAndSwapChain();
	void CreateRenderTarget();
	void CreateDepthStencil();
	void CreateViewport();
	void CreateSamplers();
	void CreateBlendStates();
	void CreateRasterizerStates();
	void CreateConstantBuffers();
private:
	HWND m_hwnd;

	Microsoft::WRL::ComPtr<ID3D11Device>			m_device;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext>		m_immediate_context;
	Microsoft::WRL::ComPtr<IDXGISwapChain>			m_swap_chain;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView>	m_render_target_view;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView>	m_depth_stencil_view;
	Microsoft::WRL::ComPtr<ID3D11SamplerState>		m_sampler_desc;
	Microsoft::WRL::ComPtr<ID3D11SamplerState>		m_sampler_states[5];
	Microsoft::WRL::ComPtr<ID3D11DepthStencilState>	m_depth_stencil_states[4];
	Microsoft::WRL::ComPtr<ID3D11BlendState>		m_blend_states[4];
	Microsoft::WRL::ComPtr<ID3D11RasterizerState>	m_rasterizer_states[3];
	D3D11_VIEWPORT									m_viewport;
	Microsoft::WRL::ComPtr<ID3D11Buffer>			m_constant_buffer[8];

	std::unique_ptr<PrimitiveRenderer>				m_primitiveRenderer;

	float	m_screenWidth = 0;
	float	m_screenHeight = 0;


	std::mutex mutex;
};