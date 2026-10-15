#pragma once
#include <DirectXMath.h>

struct SceneConstants
{
    DirectX::XMFLOAT4X4 view_projection;
    DirectX::XMFLOAT4 light_direction;
    DirectX::XMFLOAT4 camera_position;
};