#pragma once

#include <DirectXMath.h>
#include "Core/Transform.h"

class GameObject
{
public:
	virtual ~GameObject() = default;

	virtual void Initialize() {}
	virtual void Finalize() {}
	virtual void Update(float elapsedTime) {}
	virtual void Render() {}
	virtual void DrawImGui() {}

	bool IsActive() const { return m_active; }
	void SetActive(bool active) { m_active = active; }

	Transform& GetTransform() { return m_transform; }
	const Transform& GetTransform() const { return m_transform; }

	void SetPosition(const DirectX::XMFLOAT3& position)
	{
		m_transform.position = position;
	}

	void SetScale(const DirectX::XMFLOAT3& scale)
	{
		m_transform.scale = scale;
	}

	const DirectX::XMFLOAT3& GetPosition() const
	{
		return m_transform.position;
	}

protected:
	Transform m_transform;
	bool m_active = true;
};
