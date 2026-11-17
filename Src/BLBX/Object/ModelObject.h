#pragma once

#include <memory>
#include "GameObject.h"

#include "Resource/Model/GltfModel.h"

class ModelObject : public GameObject
{
public:
	~ModelObject() override;

	void Finalize() override;
	void Render() override;

	GltfModel* GetModel() const
	{
		return m_model.get();
	}

protected:
	void LoadModel(const char* filename, bool batching);

protected:
	std::shared_ptr<GltfModel> m_model;
};