#include "Graphics.h"

void Graphics::Initialize(HWND hWnd)
{
	//DirectX初期化
	m_hwnd = hWnd;

	//ウィンドウのクライアント	領域サイズ
	RECT rc;
	GetClientRect(hWnd, &rc);
	UINT screenWidth = rc.right - rc.left;
	UINT screenHeight = rc.bottom - rc.top;

	// 画面サイズをメンバに保存
	m_screenWidth = static_cast<float>(screenWidth);
	m_screenHeight = static_cast<float>(screenHeight);

	// Direct3D デバイスとスワップチェーンを作成
	CreateDeviceAndSwapChain();

	// レンダーターゲットビュー（バックバッファ）を作成
	CreateRenderTarget();

	// 深度ステンシルバッファとステートを作成
	CreateDepthStencil();

	// ビューポート（描画領域）を作成
	CreateViewport();

	// サンプラーステート（テクスチャのサンプリング方法）を作成
	CreateSamplers();

	// ブレンドステート（アルファブレンド用）を作成
	CreateBlendStates();

	CreateRasterizerStates();

	// 定数バッファ（シーン用）を作成
	CreateConstantBuffers();

	// レンダラ生成
	m_primitiveRenderer = std::make_unique<PrimitiveRenderer>(m_device.Get());
}

void Graphics::Finalize()
{
	if (m_immediate_context)
	{
		m_immediate_context->ClearState();
		m_immediate_context->Flush();
	}

	m_primitiveRenderer.reset();

	m_constant_buffer[0].Reset();

	for (auto& state : m_sampler_states)
	{
		state.Reset();
	}

	for (auto& state : m_blend_states)
	{
		state.Reset();
	}

	for (auto& state : m_depth_stencil_states)
	{
		state.Reset();
	}

	for (auto& state : m_rasterizer_states)
	{
		state.Reset();
	}

	m_depth_stencil_view.Reset();
	m_render_target_view.Reset();

	if (m_immediate_context)
	{
		m_immediate_context->ClearState();
		m_immediate_context->Flush();
	}

	// Report前にContext / SwapChainも解放
	m_immediate_context.Reset();
	m_swap_chain.Reset();

#if defined(DEBUG) || defined(_DEBUG)
	if (m_device)
	{
		Microsoft::WRL::ComPtr<ID3D11Debug> debug;
		if (SUCCEEDED(m_device.As(&debug)))
		{
			OutputDebugStringA("===== Manual D3D11 Live Object Report =====\n");
			debug->ReportLiveDeviceObjects(
				D3D11_RLDO_DETAIL | D3D11_RLDO_IGNORE_INTERNAL
			);
		}
	}
#endif

	m_device.Reset();
}

void Graphics::Clear(float r, float g, float b, float a)
{
	float color[4] = { r,g,b,a };
	m_immediate_context->ClearRenderTargetView(m_render_target_view.Get(), color);
	m_immediate_context->ClearDepthStencilView(m_depth_stencil_view.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
}

void Graphics::SetRenderTargets()
{
	m_immediate_context->RSSetViewports(1, &m_viewport);
	m_immediate_context->OMSetRenderTargets(1, m_render_target_view.GetAddressOf(), m_depth_stencil_view.Get());
}

void Graphics::Present(UINT syncInterval)
{
	m_swap_chain->Present(syncInterval, 0);
}

void Graphics::SetSampler(SAMPLER_STATE state, UINT slot)
{
	ID3D11SamplerState* sampler = m_sampler_states[(int)state].Get();
	m_immediate_context->PSSetSamplers(slot, 1, &sampler);
}

void Graphics::SetDepthState(UINT index)
{
	m_immediate_context->OMSetDepthStencilState(
		m_depth_stencil_states[index].Get(), 0);
}

void Graphics::SetBlendState(UINT index)
{
	float blendFactor[4] = { 1,1,1,1 };
	m_immediate_context->OMSetBlendState(
		m_blend_states[index].Get(), blendFactor, 0xffffffff);
}

void Graphics::SetRasterizerState(UINT index)
{
	m_immediate_context->RSSetState(
		m_rasterizer_states[index].Get());
}

void Graphics::UpdateSceneConstants(const SceneConstants& data)
{
	auto* ctx = m_immediate_context.Get();
	ID3D11Buffer* cb = m_constant_buffer[0].Get();
	ctx->UpdateSubresource(cb, 0, nullptr, &data, 0, 0);
	ctx->VSSetConstantBuffers(1, 1, &cb);
	ctx->PSSetConstantBuffers(1, 1, &cb);
}

void Graphics::CreateDeviceAndSwapChain()
{
	HRESULT hr = S_OK;

	// DirectX デバイス作成フラグ（デバッグ時はログ出す）
	UINT create_device_flags = 0;
#if defined(DEBUG) || defined(_DEBUG)
	create_device_flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
	// 対応する Feature Level（DirectX 11）
	D3D_FEATURE_LEVEL feature_levels{ D3D_FEATURE_LEVEL_11_0 };

	//スワップチェーン
	DXGI_SWAP_CHAIN_DESC swap_chain_desc{};
	// スワップチェーンの設定
	swap_chain_desc.BufferCount = 2;
	swap_chain_desc.BufferDesc.Width = static_cast<UINT>(m_screenWidth);
	swap_chain_desc.BufferDesc.Height = static_cast<UINT>(m_screenHeight);
	swap_chain_desc.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	// 60fps 用のリフレッシュレート
	swap_chain_desc.BufferDesc.RefreshRate.Numerator = 60;
	swap_chain_desc.BufferDesc.RefreshRate.Denominator = 1;
	// バックバッファをレンダーターゲットとして使用
	swap_chain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swap_chain_desc.OutputWindow = m_hwnd;
	// マルチサンプリング
	swap_chain_desc.SampleDesc.Count = 1;
	swap_chain_desc.SampleDesc.Quality = 0;
	// ウィンドウモードで起動
	swap_chain_desc.Windowed = TRUE;
	swap_chain_desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swap_chain_desc.Flags = 0;

	hr = D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, create_device_flags,
		&feature_levels, 1, D3D11_SDK_VERSION, &swap_chain_desc,
		&m_swap_chain, m_device.GetAddressOf(), NULL, m_immediate_context.GetAddressOf());
	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));
}

void Graphics::CreateRenderTarget()
{
	HRESULT hr = S_OK;

	//レンダーターゲットビュー(RTV)
	// スワップチェーンの裏バッファを取り出して、それを描画のキャンバスにする
	Microsoft::WRL::ComPtr<ID3D11Texture2D> texture2d;
	hr = m_swap_chain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(texture2d.GetAddressOf()));
	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

	hr = m_device->CreateRenderTargetView(texture2d.Get(), nullptr, m_render_target_view.GetAddressOf());
	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

}

void Graphics::CreateDepthStencil()
{
	HRESULT hr = S_OK;


	// 深度ステンシルビュー（DSV）
	// 奥行き判定に必要な「深度バッファ」を作る
	{
		Microsoft::WRL::ComPtr<ID3D11Texture2D> texture2d;
		D3D11_TEXTURE2D_DESC texture2d_desc;
		texture2d_desc.Width = static_cast<UINT>(m_screenWidth);
		texture2d_desc.Height = static_cast<UINT>(m_screenHeight);
		texture2d_desc.MipLevels = 1;
		texture2d_desc.ArraySize = 1;
		texture2d_desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		texture2d_desc.SampleDesc.Count = 1;
		texture2d_desc.SampleDesc.Quality = 0;
		texture2d_desc.Usage = D3D11_USAGE_DEFAULT;
		texture2d_desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
		texture2d_desc.CPUAccessFlags = 0;
		texture2d_desc.MiscFlags = 0;
		hr = m_device->CreateTexture2D(&texture2d_desc, nullptr, texture2d.GetAddressOf());
		_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

		// DSV 作成
		D3D11_DEPTH_STENCIL_VIEW_DESC depth_stencil_view_desc{};
		depth_stencil_view_desc.Format = texture2d_desc.Format;
		depth_stencil_view_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
		depth_stencil_view_desc.Texture2D.MipSlice = 0;
		hr = m_device->CreateDepthStencilView(texture2d.Get(), nullptr, m_depth_stencil_view.GetAddressOf());
		_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

		//深度ステンシルステート（モデル用)
		D3D11_DEPTH_STENCIL_DESC depth_stencil_desc{};
		depth_stencil_desc.DepthEnable = TRUE;
		depth_stencil_desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
		depth_stencil_desc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
		hr = m_device->CreateDepthStencilState(&depth_stencil_desc, m_depth_stencil_states[0].GetAddressOf());
		_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

		//深度ステンシルステート（スプライト用)
		depth_stencil_desc.DepthEnable = FALSE;
		depth_stencil_desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
		depth_stencil_desc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
		hr = m_device->CreateDepthStencilState(&depth_stencil_desc, m_depth_stencil_states[1].GetAddressOf());
		_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));
	}
}

void Graphics::CreateViewport()
{

	//ビューポート(描画領域の設定）
	{
		m_viewport.Width = static_cast<float>(m_screenWidth);
		m_viewport.Height = static_cast<float>(m_screenHeight);
		m_viewport.MaxDepth = 1.0f;
		m_viewport.MinDepth = 0.0f;
		m_viewport.TopLeftX = 0.0f;
		m_viewport.TopLeftY = 0.0f;
	}
}

void Graphics::CreateSamplers()
{
	HRESULT hr = S_OK;

	//サンプラーステート（テクスチャの見え方設定）
	D3D11_SAMPLER_DESC sampler_desc;
	{
		//POINT
		sampler_desc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
		sampler_desc.AddressU = sampler_desc.AddressV = sampler_desc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
		sampler_desc.MipLODBias = 0;
		sampler_desc.MaxAnisotropy = 16;
		sampler_desc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
		sampler_desc.BorderColor[0] = 0;
		sampler_desc.BorderColor[1] = 0;
		sampler_desc.BorderColor[2] = 0;
		sampler_desc.BorderColor[3] = 0;
		sampler_desc.MinLOD = 0;
		sampler_desc.MaxLOD = D3D11_FLOAT32_MAX;
		hr = m_device->CreateSamplerState(&sampler_desc, m_sampler_states[static_cast<size_t>(SAMPLER_STATE::POINT)].GetAddressOf());
		_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

		//LINEAR
		sampler_desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
		sampler_desc.AddressU = sampler_desc.AddressV = sampler_desc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
		hr = m_device->CreateSamplerState(&sampler_desc, m_sampler_states[static_cast<size_t>(SAMPLER_STATE::LINEAR)].GetAddressOf());
		_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

		//ANISOTROPIC
		sampler_desc.Filter = D3D11_FILTER_ANISOTROPIC;
		sampler_desc.AddressU = sampler_desc.AddressV = sampler_desc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
		hr = m_device->CreateSamplerState(&sampler_desc, m_sampler_states[static_cast<size_t>(SAMPLER_STATE::ANISOTROPIC)].GetAddressOf());
		_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

		//BORDER（黒）
		sampler_desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
		sampler_desc.AddressU = sampler_desc.AddressV = sampler_desc.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
		sampler_desc.BorderColor[0] = 0;
		sampler_desc.BorderColor[1] = 0;
		sampler_desc.BorderColor[2] = 0;
		sampler_desc.BorderColor[3] = 0;
		hr = m_device->CreateSamplerState(&sampler_desc, m_sampler_states[static_cast<size_t>(SAMPLER_STATE::LINEAR_BORDER_BLACK)].GetAddressOf());
		_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

		//BORDER（白）
		sampler_desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
		sampler_desc.AddressU = sampler_desc.AddressV = sampler_desc.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
		sampler_desc.BorderColor[0] = 1;
		sampler_desc.BorderColor[1] = 1;
		sampler_desc.BorderColor[2] = 1;
		sampler_desc.BorderColor[3] = 1;
		hr = m_device->CreateSamplerState(&sampler_desc, m_sampler_states[static_cast<size_t>(SAMPLER_STATE::LINEAR_BORDER_WHITE)].GetAddressOf());
		_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));
	}
}

void Graphics::CreateBlendStates()
{
	HRESULT hr = S_OK;

	//ブレンドステート（透明描画）
	D3D11_BLEND_DESC blend_desc{};
	{
		//blend_desc.AlphaToCoverageEnable = FALSE;
		//blend_desc.IndependentBlendEnable = FALSE;
		//blend_desc.RenderTarget[0].BlendEnable = TRUE;
		//blend_desc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
		//blend_desc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
		//blend_desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
		//blend_desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
		//blend_desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
		//blend_desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
		//blend_desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
		//hr = m_device->CreateBlendState(&blend_desc, m_blend_states[0].GetAddressOf());
		//_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

		// 通常(alpha)
		{
			blend_desc.AlphaToCoverageEnable = FALSE;
			blend_desc.IndependentBlendEnable = FALSE;
			blend_desc.RenderTarget[0].BlendEnable = TRUE;
			blend_desc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
			blend_desc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
			blend_desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
			blend_desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
			blend_desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
			blend_desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
			blend_desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
			hr = m_device->CreateBlendState(&blend_desc, m_blend_states[0].GetAddressOf());
			_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));
		}
	}
}

void Graphics::CreateRasterizerStates()
{
	HRESULT hr = S_OK;

	D3D11_RASTERIZER_DESC rasterizer_desc{};
	rasterizer_desc.FillMode = D3D11_FILL_SOLID;
	rasterizer_desc.CullMode = D3D11_CULL_NONE;
	rasterizer_desc.FrontCounterClockwise = FALSE;
	rasterizer_desc.DepthClipEnable = TRUE;

	// 0: Solid / Cull None
	hr = m_device->CreateRasterizerState(
		&rasterizer_desc,
		m_rasterizer_states[0].GetAddressOf()
	);
	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

	// 1: Wireframe / Cull None
	rasterizer_desc.FillMode = D3D11_FILL_WIREFRAME;
	rasterizer_desc.CullMode = D3D11_CULL_NONE;

	hr = m_device->CreateRasterizerState(
		&rasterizer_desc,
		m_rasterizer_states[1].GetAddressOf()
	);
	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

	// 2: Solid / Cull Back
	rasterizer_desc.FillMode = D3D11_FILL_SOLID;
	rasterizer_desc.CullMode = D3D11_CULL_BACK;

	hr = m_device->CreateRasterizerState(
		&rasterizer_desc,
		m_rasterizer_states[2].GetAddressOf()
	);
	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));
}

void Graphics::CreateConstantBuffers()
{
	HRESULT hr = S_OK;

	//シーン定数バッファー
	D3D11_BUFFER_DESC buffer_desc = {};
	{
		buffer_desc.ByteWidth = sizeof(SceneConstants);
		buffer_desc.Usage = D3D11_USAGE_DEFAULT;
		buffer_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		buffer_desc.CPUAccessFlags = 0;
		buffer_desc.MiscFlags = 0;
		buffer_desc.StructureByteStride = 0;
		hr = m_device->CreateBuffer(&buffer_desc, nullptr, m_constant_buffer[0].GetAddressOf());
		_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));
	}
}
