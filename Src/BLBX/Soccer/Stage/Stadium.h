#pragma once

#include "Object/ModelObject.h"

class Stadium : public ModelObject
{
public:
	void Initialize() override;
	void Update(float elapsedTime) override;
	void Render() override;
	void DrawImGui() override;

private:
	bool m_visible = true;
};
