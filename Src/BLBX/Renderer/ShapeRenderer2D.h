#include <d3d11.h>
#include <DirectXMath.h>

class ShapeRenderer2D
{
private:
	ShapeRenderer2D() = default;
	~ShapeRenderer2D() = default;
public:
	//ƒVƒ“ƒOƒ‹ƒgƒ“
	static ShapeRenderer2D& Instance()
	{
		static ShapeRenderer2D instance;
		return instance;
	}

	void Initialize(ID3D11Device* device);
	void Finalize();

	void Begin(ID3D11DeviceContext* dc);
	void End();

	void DrawPoint(float x, float y, float size, DirectX::XMFLOAT4 color);

	void DrawBox(float x, float y, float w, float h, DirectX::XMFLOAT4 color);

private:
	ID3D11Device* m_device = nullptr;
	ID3D11DeviceContext* m_context = nullptr;

	ID3D11Buffer* m_vertexBuffer = nullptr;
	ID3D11Buffer* m_constantBuffer = nullptr;

	ID3D11VertexShader* m_vs = nullptr;
	ID3D11PixelShader* m_ps = nullptr;
	ID3D11InputLayout* m_inputLayout = nullptr;

	struct Vertex
	{
		DirectX::XMFLOAT3 pos;
		DirectX::XMFLOAT4 col;
		DirectX::XMFLOAT2 uv;
		float type;
	};

	struct CB
	{
		DirectX::XMMATRIX projection;
	};
};
