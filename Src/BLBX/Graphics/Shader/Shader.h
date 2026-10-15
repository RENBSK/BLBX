#include "Core/Misc.h"
#include <d3d11.h>
#include <directxmath.h>
#include <wrl.h>
#include <string>
#include <map>
#include <sstream>
#include <wrl.h>

using namespace DirectX;
using namespace std;
using namespace Microsoft::WRL;

HRESULT create_vs_from_cso(ID3D11Device* device, const char* cso_name, ID3D11VertexShader** vertex_shader,
	ID3D11InputLayout** input, D3D11_INPUT_ELEMENT_DESC* input_element_desc, UINT num_elements);

HRESULT create_ps_from_cso(ID3D11Device* device, const char* cso_name, ID3D11PixelShader** pixel_shader);

