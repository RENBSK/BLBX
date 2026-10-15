#pragma once
#include <DirectXMath.h>

class Transform
{
public:
	DirectX::XMFLOAT3 position{ 0,0,0 };
	DirectX::XMFLOAT3 rotation{ 0,0,0 };
	DirectX::XMFLOAT3 scale{ 1,1,1 };

public:
	DirectX::XMMATRIX GetMatrix() const
	{
		using namespace DirectX;

		return
			XMMatrixScaling(scale.x, scale.y, scale.z) *
			XMMatrixRotationRollPitchYaw(rotation.x, rotation.y, rotation.z) *
			XMMatrixTranslation(position.x, position.y, position.z);
	}

	DirectX::XMFLOAT3 GetPosition() const
	{
		return position;
	}

	void Reset()
	{
		position = { 0.0f,0.0f,0.0f };
		rotation = { 0.0f,0.0f,0.0f };
		scale = { 1.0f,1.0f,1.0f };
	}
};
