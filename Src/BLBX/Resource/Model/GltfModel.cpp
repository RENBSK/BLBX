// INTERLEAVED_GLTF_MODEL
#include "GltfModel.h"
#include <functional>
#include <filesystem>
#include <fstream>
#include <algorithm>
#include <cstring>
#include <WICTextureLoader.h>

#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_NO_EXTERNAL_IMAGE
#define TINYGLTF_NO_STB_IMAGE
#define TINYGLTF_NO_STB_IMAGE_WRITE
#include "tinygltf-release/tiny_gltf.h"

#include "Core/Misc.h"
#include "Graphics/Shader/Shader.h"
#include "Graphics/Resource/Texture.h"

bool _null_load_image_data(tinygltf::Image*, const int, std::string*, std::string*, int, int, const unsigned char*, int, void*)
{
	return true;
}
GltfModel::GltfModel(ID3D11Device* device, const std::string& filename, bool static_batching) : m_filename(filename), m_static_batching(static_batching)
{
	tinygltf::TinyGLTF tiny_gltf;
	tiny_gltf.SetImageLoader(_null_load_image_data, nullptr);

	tinygltf::Model gltf_model;
	std::string error, warning;
	bool succeeded = false;
	if (filename.find(".glb") != std::string::npos)
	{
		succeeded = tiny_gltf.LoadBinaryFromFile(&gltf_model, &error, &warning, filename.c_str());
	}
	else if (filename.find(".gltf") != std::string::npos)
	{
		succeeded = tiny_gltf.LoadASCIIFromFile(&gltf_model, &error, &warning, filename.c_str());
	}

	_ASSERT_EXPR_A(warning.empty(), warning.c_str());
	_ASSERT_EXPR_A(error.empty(), error.c_str());
	_ASSERT_EXPR_A(succeeded, L"Failed to load glTF file");

	for (const tinygltf::Scene& gltf_scene : gltf_model.scenes)
	{
		scene& scene = m_scenes.emplace_back();
		scene.name = gltf_scene.name;
		scene.nodes = gltf_scene.nodes;
	}
	m_default_scene = gltf_model.defaultScene < 0 ? 0 : gltf_model.defaultScene;

	fetch_nodes(gltf_model);
	fetch_materials(gltf_model);
	fetch_textures(gltf_model);

	if (m_static_batching)
	{
		fetch_and_batch_meshes(gltf_model);
	}
	else
	{
		fetch_meshes(gltf_model);
		fetch_animations(gltf_model);
	}

	create_and_upload_resources(device);
}
GltfModel::~GltfModel()
{
	Finalize();
}
void GltfModel::Finalize()
{
	if (m_finalized)
	{
		return;
	}

	m_finalized = true;

	for (auto& srv : m_texture_resource_views)
	{
		srv.Reset();
	}
	m_texture_resource_views.clear();

	m_material_resource_view.Reset();

	m_primitive_joint_cbuffer.Reset();
	m_primitive_cbuffer.Reset();

	for (auto& sampler : m_sampler_states)
	{
		sampler.Reset();
	}

	m_input_layout.Reset();
	m_pixel_shader.Reset();
	m_vertex_shader.Reset();

	for (auto& buffer : m_buffers)
	{
		buffer.Reset();
	}
	m_buffers.clear();

	m_animations.clear();
	m_skins.clear();
	m_textures.clear();
	m_images.clear();
	m_materials.clear();
	m_batch_meshes.clear();
	m_meshes.clear();
	m_nodes.clear();
	m_scenes.clear();
}
void GltfModel::fetch_nodes(const tinygltf::Model& gltf_model)
{
	for (const tinygltf::Node& gltf_node : gltf_model.nodes)
	{
		node& node = m_nodes.emplace_back();
		node.name = gltf_node.name;
		node.skin = gltf_node.skin;
		node.mesh = gltf_node.mesh;
		node.children = gltf_node.children;
		if (!gltf_node.matrix.empty())
		{
			DirectX::XMFLOAT4X4 matrix;
			for (size_t row = 0; row < 4; row++)
			{
				for (size_t column = 0; column < 4; column++)
				{
					matrix(row, column) = static_cast<float>(gltf_node.matrix.at(4 * row + column));
				}
			}

			DirectX::XMVECTOR S, T, R;
			bool succeed = DirectX::XMMatrixDecompose(&S, &R, &T, DirectX::XMLoadFloat4x4(&matrix));
			_ASSERT_EXPR(succeed, L"Failed to decompose matrix.");

			DirectX::XMStoreFloat3(&node.scale, S);
			DirectX::XMStoreFloat4(&node.rotation, R);
			DirectX::XMStoreFloat3(&node.translation, T);
		}
		else
		{
			if (gltf_node.scale.size() > 0)
			{
				node.scale.x = static_cast<float>(gltf_node.scale.at(0));
				node.scale.y = static_cast<float>(gltf_node.scale.at(1));
				node.scale.z = static_cast<float>(gltf_node.scale.at(2));
			}
			if (gltf_node.translation.size() > 0)
			{
				node.translation.x = static_cast<float>(gltf_node.translation.at(0));
				node.translation.y = static_cast<float>(gltf_node.translation.at(1));
				node.translation.z = static_cast<float>(gltf_node.translation.at(2));
			}
			if (gltf_node.rotation.size() > 0)
			{
				node.rotation.x = static_cast<float>(gltf_node.rotation.at(0));
				node.rotation.y = static_cast<float>(gltf_node.rotation.at(1));
				node.rotation.z = static_cast<float>(gltf_node.rotation.at(2));
				node.rotation.w = static_cast<float>(gltf_node.rotation.at(3));
			}
		}
	}
	cumulate_transforms(m_nodes);
}
void GltfModel::cumulate_transforms(std::vector<node>& m_nodes)
{
	std::function<void(int, int)> traverse = [&](int parent_index, int node_index)->void
		{
			DirectX::XMMATRIX P = parent_index > -1 ? DirectX::XMLoadFloat4x4(&m_nodes.at(parent_index).global_transform) : DirectX::XMMatrixIdentity();

			node& node = m_nodes.at(node_index);
			DirectX::XMMATRIX S = DirectX::XMMatrixScaling(node.scale.x, node.scale.y, node.scale.z);
			DirectX::XMMATRIX R = DirectX::XMMatrixRotationQuaternion(DirectX::XMLoadFloat4(&node.rotation));
			DirectX::XMMATRIX T = DirectX::XMMatrixTranslation(node.translation.x, node.translation.y, node.translation.z);
			DirectX::XMStoreFloat4x4(&node.global_transform, S * R * T * P);

			for (int child_index : node.children)
			{
				traverse(node_index, child_index);
			}
		};
	for (int node_index : m_scenes.at(m_default_scene).nodes)
	{
		traverse(-1, node_index);
	}
}
DXGI_FORMAT _dxgi_format(const tinygltf::Accessor& accessor)
{
	switch (accessor.type)
	{
	case TINYGLTF_TYPE_SCALAR:
		switch (accessor.componentType)
		{
		case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
			return DXGI_FORMAT_R8_UINT;
		case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
			return DXGI_FORMAT_R16_UINT;
		case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
			return DXGI_FORMAT_R32_UINT;
		default:
			_ASSERT_EXPR(FALSE, L"This accessor component type is not supported.");
			return DXGI_FORMAT_UNKNOWN;
		}
	case TINYGLTF_TYPE_VEC2:
		switch (accessor.componentType)
		{
		case TINYGLTF_COMPONENT_TYPE_FLOAT:
			return DXGI_FORMAT_R32G32_FLOAT;
		default:
			_ASSERT_EXPR(FALSE, L"This accessor component type is not supported.");
			return DXGI_FORMAT_UNKNOWN;
		}
	case TINYGLTF_TYPE_VEC3:
		switch (accessor.componentType)
		{
		case TINYGLTF_COMPONENT_TYPE_FLOAT:
			return DXGI_FORMAT_R32G32B32_FLOAT;
		default:
			_ASSERT_EXPR(FALSE, L"This accessor component type is not supported.");
			return DXGI_FORMAT_UNKNOWN;
		}
	case TINYGLTF_TYPE_VEC4:
		switch (accessor.componentType)
		{
		case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
			return DXGI_FORMAT_R8G8B8A8_UINT;
		case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
			return DXGI_FORMAT_R16G16B16A16_UINT;
		case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
			return DXGI_FORMAT_R32G32B32A32_UINT;
		case TINYGLTF_COMPONENT_TYPE_FLOAT:
			return DXGI_FORMAT_R32G32B32A32_FLOAT;
		default:
			_ASSERT_EXPR(FALSE, L"This accessor component type is not supported.");
			return DXGI_FORMAT_UNKNOWN;
		}
		break;
	default:
		_ASSERT_EXPR(FALSE, L"This accessor type is not supported.");
		return DXGI_FORMAT_UNKNOWN;
	}
}

UINT _sizeof_component(DXGI_FORMAT format)
{
	switch (format)
	{
	case DXGI_FORMAT_R8_UINT: return 1;
	case DXGI_FORMAT_R16_UINT: return 2;
	case DXGI_FORMAT_R32_UINT: return 4;
	case DXGI_FORMAT_R32G32_FLOAT: return 8;
	case DXGI_FORMAT_R32G32B32_FLOAT: return 12;
	case DXGI_FORMAT_R8G8B8A8_UINT: return 4;
	case DXGI_FORMAT_R16G16B16A16_UINT: return 8;
	case DXGI_FORMAT_R32G32B32A32_UINT: return 16;
	case DXGI_FORMAT_R32G32B32A32_FLOAT: return 16;
	}
	_ASSERT_EXPR(FALSE, L"Not supported");
	return 0;
}

template<class T>
static void _copy(unsigned char* d_data, const size_t d_stride, const unsigned char* s_data, const size_t s_stride, size_t count)
{
	while (count-- > 0)
	{
		*reinterpret_cast<T*>(d_data) = *reinterpret_cast<const T*>(s_data);
		s_data += s_stride;
		d_data += d_stride;
	}
};
// INTERLEAVED_GLTF_MODEL
void GltfModel::fetch_meshes(const tinygltf::Model& gltf_model)
{
	for (const tinygltf::Mesh& gltf_mesh : gltf_model.meshes)
	{
		mesh& mesh = m_meshes.emplace_back();
		mesh.name = gltf_mesh.name;
		for (const tinygltf::Primitive& gltf_primitive : gltf_mesh.primitives)
		{
			mesh::primitive& primitive = mesh.primitives.emplace_back();
			primitive.material = gltf_primitive.material;

			// Create index buffer view
			if (gltf_primitive.indices > -1)
			{
				const tinygltf::Accessor& gltf_accessor = gltf_model.accessors.at(gltf_primitive.indices);
				const tinygltf::BufferView& gltf_buffer_view = gltf_model.bufferViews.at(gltf_accessor.bufferView);

				primitive.index_buffer_view.format = _dxgi_format(gltf_accessor);
				primitive.index_buffer_view.size_in_bytes = static_cast<UINT>(gltf_accessor.count) * _sizeof_component(primitive.index_buffer_view.format);
				primitive.cached_indices.resize(primitive.index_buffer_view.size_in_bytes);
				const unsigned char* data = gltf_model.buffers.at(gltf_buffer_view.buffer).data.data() + gltf_buffer_view.byteOffset + gltf_accessor.byteOffset;

				memcpy_s(primitive.cached_indices.data(), primitive.cached_indices.size(), data, primitive.index_buffer_view.size_in_bytes);
			}

			// Create vertex buffer view
			if (gltf_primitive.attributes.size() > 0 && gltf_primitive.attributes.find("POSITION") != gltf_primitive.attributes.end())
			{
				primitive.cached_vertices.resize(gltf_model.accessors.at(gltf_primitive.attributes.at("POSITION")).count);
			}
			else
			{
				continue;
			}
			for (std::map<std::string, int>::const_reference gltf_attribute : gltf_primitive.attributes)
			{
				const tinygltf::Accessor& gltf_accessor = gltf_model.accessors.at(gltf_attribute.second);
				const tinygltf::BufferView& gltf_buffer_view = gltf_model.bufferViews.at(gltf_accessor.bufferView);

				const unsigned char* s_data = gltf_model.buffers.at(gltf_buffer_view.buffer).data.data() + gltf_buffer_view.byteOffset + gltf_accessor.byteOffset;
				const size_t s_stride = gltf_accessor.ByteStride(gltf_buffer_view);
				const size_t d_stride = sizeof(mesh::vertex);
				const size_t count = gltf_accessor.count;
				_ASSERT_EXPR(count == primitive.cached_vertices.size(), L"The number of components on all vertices comprising the mesh must be the same.");
				if (gltf_attribute.first == "POSITION")
				{
					unsigned char* d_data = reinterpret_cast<unsigned char*>(&primitive.cached_vertices.data()->position);
					_copy<DirectX::XMFLOAT3>(d_data, d_stride, s_data, s_stride, count);
				}
				else if (gltf_attribute.first == "NORMAL")
				{
					unsigned char* d_data = reinterpret_cast<unsigned char*>(&primitive.cached_vertices.data()->normal);
					_copy<DirectX::XMFLOAT3>(d_data, d_stride, s_data, s_stride, count);
				}
				else if (gltf_attribute.first == "TANGENT")
				{
					unsigned char* d_data = reinterpret_cast<unsigned char*>(&primitive.cached_vertices.data()->tangent);
					_copy<DirectX::XMFLOAT4>(d_data, d_stride, s_data, s_stride, count);
				}
				else if (gltf_attribute.first == "TEXCOORD_0")
				{
					unsigned char* d_data = reinterpret_cast<unsigned char*>(&primitive.cached_vertices.data()->texcoord);
					_copy<DirectX::XMFLOAT2>(d_data, d_stride, s_data, s_stride, count);
				}
				else if (gltf_attribute.first == "JOINTS_0")
				{
					if (gltf_accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT)
					{
						unsigned char* d_data = reinterpret_cast<unsigned char*>(&primitive.cached_vertices.data()->joints);
						_copy<DirectX::XMINT4>(d_data, d_stride, s_data, s_stride, count);
					}
					else if (gltf_accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT)
					{
						for (size_t accessor_index = 0; accessor_index < gltf_accessor.count; ++accessor_index)
						{
							const USHORT* data = reinterpret_cast<const USHORT*>(s_data + accessor_index * s_stride);
							primitive.cached_vertices.at(accessor_index).joints.x = static_cast<UINT>(data[0]);
							primitive.cached_vertices.at(accessor_index).joints.y = static_cast<UINT>(data[1]);
							primitive.cached_vertices.at(accessor_index).joints.z = static_cast<UINT>(data[2]);
							primitive.cached_vertices.at(accessor_index).joints.w = static_cast<UINT>(data[3]);
						}
					}
					else if (gltf_accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE)
					{
						for (size_t accessor_index = 0; accessor_index < gltf_accessor.count; ++accessor_index)
						{
							const UCHAR* data = reinterpret_cast<const UCHAR*>(s_data + accessor_index * s_stride);
							primitive.cached_vertices.at(accessor_index).joints.x = static_cast<UINT>(data[0]);
							primitive.cached_vertices.at(accessor_index).joints.y = static_cast<UINT>(data[1]);
							primitive.cached_vertices.at(accessor_index).joints.z = static_cast<UINT>(data[2]);
							primitive.cached_vertices.at(accessor_index).joints.w = static_cast<UINT>(data[3]);
						}
					}
					else
					{
						_ASSERT_EXPR(FALSE, L"This component type is unsupported, please convert it yourself if necessary.");
					}
				}
				else if (gltf_attribute.first == "WEIGHTS_0")
				{
					if (gltf_accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT)
					{
						unsigned char* d_data = reinterpret_cast<unsigned char*>(&primitive.cached_vertices.data()->weights);
						_copy<DirectX::XMFLOAT4>(d_data, d_stride, s_data, s_stride, count);
					}
					else if (gltf_accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT)
					{
						for (size_t accessor_index = 0; accessor_index < gltf_accessor.count; ++accessor_index)
						{
							const USHORT* data = reinterpret_cast<const USHORT*>(s_data + accessor_index * s_stride);
							primitive.cached_vertices.at(accessor_index).weights.x = static_cast<FLOAT>(data[0]) / 0xFFFF;
							primitive.cached_vertices.at(accessor_index).weights.y = static_cast<FLOAT>(data[1]) / 0xFFFF;
							primitive.cached_vertices.at(accessor_index).weights.z = static_cast<FLOAT>(data[2]) / 0xFFFF;
							primitive.cached_vertices.at(accessor_index).weights.w = static_cast<FLOAT>(data[3]) / 0xFFFF;
						}
					}
					else if (gltf_accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE)
					{
						for (size_t accessor_index = 0; accessor_index < gltf_accessor.count; ++accessor_index)
						{
							const UCHAR* data = reinterpret_cast<const UCHAR*>(s_data + accessor_index * s_stride);
							primitive.cached_vertices.at(accessor_index).weights.x = static_cast<FLOAT>(data[0]) / 0xFF;
							primitive.cached_vertices.at(accessor_index).weights.y = static_cast<FLOAT>(data[1]) / 0xFF;
							primitive.cached_vertices.at(accessor_index).weights.z = static_cast<FLOAT>(data[2]) / 0xFF;
							primitive.cached_vertices.at(accessor_index).weights.w = static_cast<FLOAT>(data[3]) / 0xFF;
						}
					}
					else
					{
						_ASSERT_EXPR(FALSE, L"This component type is unsupported, please convert it yourself if necessary.");
					}
				}
				else
				{
					//_ASSERT_EXPR(FALSE, L"This attribute is unsupported.");
				}
				primitive.attributes.emplace(gltf_attribute.first, _dxgi_format(gltf_accessor));
			}
			primitive.vertex_buffer_view.size_in_bytes = static_cast<UINT>(primitive.cached_vertices.size() * sizeof(mesh::vertex));
			primitive.vertex_buffer_view.stride_in_bytes = static_cast<UINT>(sizeof(mesh::vertex));
		}
	}
}

// INTERLEAVED_GLTF_MODEL
void GltfModel::fetch_and_batch_meshes(const tinygltf::Model& gltf_model)
{
	m_batch_meshes.resize(gltf_model.materials.size());

	std::function<void(int)> traverse = [&](int node_index)->void {
		const node& node = m_nodes.at(node_index);
		if (node.mesh > -1)
		{
			const DirectX::XMMATRIX global_transform = DirectX::XMLoadFloat4x4(&node.global_transform);

			const tinygltf::Mesh& gltf_mesh = gltf_model.meshes.at(node.mesh);

			for (const tinygltf::Primitive& gltf_primitive : gltf_mesh.primitives)
			{
#if 1
				if (gltf_primitive.material < 0)
				{
					// TODO
					continue;
				}
#endif

				batch_mesh& batch_mesh = m_batch_meshes.at(gltf_primitive.material);
				batch_mesh.material = gltf_primitive.material;
				batch_mesh.index_buffer_view.format = DXGI_FORMAT_R32_UINT;
				if (gltf_primitive.indices > -1)
				{
					const tinygltf::Accessor& gltf_accessor = gltf_model.accessors.at(gltf_primitive.indices);
					const tinygltf::BufferView& gltf_buffer_view = gltf_model.bufferViews.at(gltf_accessor.bufferView);

					std::vector<UINT> cached_indices(gltf_accessor.count);
					const size_t vertex_offset = batch_mesh.cached_vertices.size();
					if (gltf_accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE)
					{
						const BYTE* data = gltf_model.buffers.at(gltf_buffer_view.buffer).data.data() + gltf_buffer_view.byteOffset + gltf_accessor.byteOffset;
						for (size_t accessor_index = 0; accessor_index < gltf_accessor.count; ++accessor_index)
						{
							cached_indices.at(accessor_index) = static_cast<UINT>(data[accessor_index] + vertex_offset);
						}
					}
					else if (gltf_accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT)
					{
						const USHORT* data = reinterpret_cast<const USHORT*>(gltf_model.buffers.at(gltf_buffer_view.buffer).data.data() + gltf_buffer_view.byteOffset + gltf_accessor.byteOffset);
						for (size_t accessor_index = 0; accessor_index < gltf_accessor.count; ++accessor_index)
						{
							cached_indices.at(accessor_index) = static_cast<UINT>(data[accessor_index] + vertex_offset);
						}
					}
					else if (gltf_accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT)
					{
						const UINT* data = reinterpret_cast<const UINT*>(gltf_model.buffers.at(gltf_buffer_view.buffer).data.data() + gltf_buffer_view.byteOffset + gltf_accessor.byteOffset);
						for (size_t accessor_index = 0; accessor_index < gltf_accessor.count; ++accessor_index)
						{
							cached_indices.at(accessor_index) = static_cast<UINT>(data[accessor_index] + vertex_offset);
						}
					}
					else
					{
						_ASSERT_EXPR(false, L"This index format is not supported.");
					}

					batch_mesh.cached_indices.insert(batch_mesh.cached_indices.end(), cached_indices.begin(), cached_indices.end());
					batch_mesh.index_buffer_view.size_in_bytes += static_cast<UINT>(gltf_accessor.count * sizeof(UINT));
				}

				std::vector<batch_mesh::vertex> cached_vertices;
				if (gltf_primitive.attributes.size() > 0 && gltf_primitive.attributes.find("POSITION") != gltf_primitive.attributes.end())
				{
					cached_vertices.resize(gltf_model.accessors.at(gltf_primitive.attributes.at("POSITION")).count);
				}
				else
				{
					continue;
				}

				for (std::map<std::string, int>::const_reference gltf_attribute : gltf_primitive.attributes)
				{
					const tinygltf::Accessor& gltf_accessor = gltf_model.accessors.at(gltf_attribute.second);
					const tinygltf::BufferView& gltf_buffer_view = gltf_model.bufferViews.at(gltf_accessor.bufferView);

					const unsigned char* s_data = gltf_model.buffers.at(gltf_buffer_view.buffer).data.data() + gltf_buffer_view.byteOffset + gltf_accessor.byteOffset;
					const size_t s_stride = gltf_accessor.ByteStride(gltf_buffer_view);
					const size_t d_stride = sizeof(batch_mesh::vertex);
					const size_t count = gltf_accessor.count;
					_ASSERT_EXPR(count == cached_vertices.size(), L"The number of components on all vertices comprising the mesh must be the same.");
					if (gltf_attribute.first == "POSITION")
					{
						unsigned char* d_data = reinterpret_cast<unsigned char*>(&cached_vertices.data()->position);
						_copy<DirectX::XMFLOAT3>(d_data, d_stride, s_data, s_stride, count);
					}
					else if (gltf_attribute.first == "NORMAL")
					{
						unsigned char* d_data = reinterpret_cast<unsigned char*>(&cached_vertices.data()->normal);
						_copy<DirectX::XMFLOAT3>(d_data, d_stride, s_data, s_stride, count);
					}
					else if (gltf_attribute.first == "TANGENT")
					{
						unsigned char* d_data = reinterpret_cast<unsigned char*>(&cached_vertices.data()->tangent);
						_copy<DirectX::XMFLOAT4>(d_data, d_stride, s_data, s_stride, count);
					}
					else if (gltf_attribute.first == "TEXCOORD_0")
					{
						unsigned char* d_data = reinterpret_cast<unsigned char*>(&cached_vertices.data()->texcoord);
						_copy<DirectX::XMFLOAT2>(d_data, d_stride, s_data, s_stride, count);
					}
					else
					{
						//_ASSERT_EXPR(FALSE, L"This attribute is unsupported.");
					}
					batch_mesh.attributes.emplace(gltf_attribute.first, _dxgi_format(gltf_accessor));
				}


				for (batch_mesh::vertex& cached_vertex : cached_vertices)
				{
					DirectX::XMStoreFloat3(&cached_vertex.position, DirectX::XMVector3TransformCoord(DirectX::XMLoadFloat3(&cached_vertex.position), global_transform));
					DirectX::XMStoreFloat3(&cached_vertex.normal, DirectX::XMVector3Normalize(DirectX::XMVector3TransformNormal(DirectX::XMLoadFloat3(&cached_vertex.normal), global_transform)));
					float sigma = cached_vertex.tangent.w;
					cached_vertex.tangent.w = 0;
					DirectX::XMStoreFloat4(&cached_vertex.tangent, DirectX::XMVector3Normalize(DirectX::XMVector3TransformNormal(DirectX::XMLoadFloat4(&cached_vertex.tangent), global_transform)));
					cached_vertex.tangent.w = sigma;
				}

				batch_mesh.cached_vertices.insert(batch_mesh.cached_vertices.end(), cached_vertices.begin(), cached_vertices.end());
				batch_mesh.vertex_buffer_view.size_in_bytes += static_cast<UINT>(cached_vertices.size() * sizeof(batch_mesh::vertex));
				batch_mesh.vertex_buffer_view.stride_in_bytes = static_cast<UINT>(sizeof(batch_mesh::vertex));

			}
		}
		for (std::vector<int>::value_type child_index : node.children)
		{
			traverse(child_index);
		}
		};
	for (std::vector<int>::value_type node_index : m_scenes.at(m_default_scene).nodes)
	{
		traverse(node_index);
	}
}

void GltfModel::fetch_materials(const tinygltf::Model& gltf_model)
{
	for (const tinygltf::Material& gltf_material : gltf_model.materials)
	{
		std::vector<material>::reference material = m_materials.emplace_back();

		material.name = gltf_material.name;

		material.data.emissive_factor[0] = static_cast<float>(gltf_material.emissiveFactor.at(0));
		material.data.emissive_factor[1] = static_cast<float>(gltf_material.emissiveFactor.at(1));
		material.data.emissive_factor[2] = static_cast<float>(gltf_material.emissiveFactor.at(2));

		material.data.alpha_mode = gltf_material.alphaMode == "OPAQUE" ? 0 : gltf_material.alphaMode == "MASK" ? 1 : gltf_material.alphaMode == "BLEND" ? 2 : 0;
		material.data.alpha_cutoff = static_cast<float>(gltf_material.alphaCutoff);
		material.data.double_sided = gltf_material.doubleSided ? 1 : 0;

		material.data.pbr_metallic_roughness.basecolor_factor[0] = static_cast<float>(gltf_material.pbrMetallicRoughness.baseColorFactor.at(0));
		material.data.pbr_metallic_roughness.basecolor_factor[1] = static_cast<float>(gltf_material.pbrMetallicRoughness.baseColorFactor.at(1));
		material.data.pbr_metallic_roughness.basecolor_factor[2] = static_cast<float>(gltf_material.pbrMetallicRoughness.baseColorFactor.at(2));
		material.data.pbr_metallic_roughness.basecolor_factor[3] = static_cast<float>(gltf_material.pbrMetallicRoughness.baseColorFactor.at(3));
		material.data.pbr_metallic_roughness.basecolor_texture.index = gltf_material.pbrMetallicRoughness.baseColorTexture.index;
		material.data.pbr_metallic_roughness.basecolor_texture.texcoord = gltf_material.pbrMetallicRoughness.baseColorTexture.texCoord;
		material.data.pbr_metallic_roughness.metallic_factor = static_cast<float>(gltf_material.pbrMetallicRoughness.metallicFactor);
		material.data.pbr_metallic_roughness.roughness_factor = static_cast<float>(gltf_material.pbrMetallicRoughness.roughnessFactor);
		material.data.pbr_metallic_roughness.metallic_roughness_texture.index = gltf_material.pbrMetallicRoughness.metallicRoughnessTexture.index;
		material.data.pbr_metallic_roughness.metallic_roughness_texture.texcoord = gltf_material.pbrMetallicRoughness.metallicRoughnessTexture.texCoord;

		material.data.normal_texture.index = gltf_material.normalTexture.index;
		material.data.normal_texture.texcoord = gltf_material.normalTexture.texCoord;
		material.data.normal_texture.scale = static_cast<float>(gltf_material.normalTexture.scale);

		material.data.occlusion_texture.index = gltf_material.occlusionTexture.index;
		material.data.occlusion_texture.texcoord = gltf_material.occlusionTexture.texCoord;
		material.data.occlusion_texture.strength = static_cast<float>(gltf_material.occlusionTexture.strength);

		material.data.emissive_texture.index = gltf_material.emissiveTexture.index;
		material.data.emissive_texture.texcoord = gltf_material.emissiveTexture.texCoord;
	}
}
void GltfModel::fetch_textures(const tinygltf::Model& gltf_model)
{
	for (const tinygltf::Texture& gltf_texture : gltf_model.textures)
	{
		texture& texture = m_textures.emplace_back();
		texture.name = gltf_texture.name;
		texture.source = gltf_texture.source;
	}
	for (const tinygltf::Image& gltf_image : gltf_model.images)
	{
		image& image = m_images.emplace_back();
		image.name = gltf_image.name;
		image.width = gltf_image.width;
		image.height = gltf_image.height;
		image.component = gltf_image.component;
		image.bits = gltf_image.bits;
		image.pixel_type = gltf_image.pixel_type;
		image.mime_type = gltf_image.mimeType;
		image.uri = gltf_image.uri;
		image.as_is = gltf_image.as_is;

		if (gltf_image.bufferView > -1)
		{
			const tinygltf::BufferView& buffer_view = gltf_model.bufferViews.at(gltf_image.bufferView);
			const tinygltf::Buffer& buffer = gltf_model.buffers.at(buffer_view.buffer);
			const unsigned char* data = buffer.data.data() + buffer_view.byteOffset;
			image.cache_data.resize(buffer_view.byteLength);
			memcpy_s(image.cache_data.data(), image.cache_data.size(), data, buffer_view.byteLength);
		}
	}

}

void GltfModel::fetch_animations(const tinygltf::Model& gltf_model)
{
	for (const tinygltf::Skin& transmission_skin : gltf_model.skins)
	{
		skin& skin = m_skins.emplace_back();
		const tinygltf::Accessor& gltf_accessor = gltf_model.accessors.at(transmission_skin.inverseBindMatrices);
		const tinygltf::BufferView& gltf_buffer_view = gltf_model.bufferViews.at(gltf_accessor.bufferView);
		_ASSERT_EXPR(gltf_accessor.type == TINYGLTF_TYPE_MAT4, L"");

		skin.inverse_bind_matrices.resize(gltf_accessor.count);
		memcpy(skin.inverse_bind_matrices.data(), gltf_model.buffers.at(gltf_buffer_view.buffer).data.data() + gltf_buffer_view.byteOffset + gltf_accessor.byteOffset, gltf_accessor.count * sizeof(DirectX::XMFLOAT4X4));

		skin.joints = transmission_skin.joints;
	}

	for (const tinygltf::Animation& gltf_animation : gltf_model.animations)
	{
		animation& animation = m_animations.emplace_back();
		animation.name = gltf_animation.name;
		for (const tinygltf::AnimationSampler& gltf_sampler : gltf_animation.samplers)
		{
			animation::sampler& sampler = animation.samplers.emplace_back();
			sampler.input = gltf_sampler.input;
			sampler.output = gltf_sampler.output;
			sampler.interpolation = gltf_sampler.interpolation;

			const tinygltf::Accessor& gltf_accessor = gltf_model.accessors.at(gltf_sampler.input);
			const tinygltf::BufferView& gltf_buffer_view = gltf_model.bufferViews.at(gltf_accessor.bufferView);
			_ASSERT_EXPR(gltf_accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT, L"");
			_ASSERT_EXPR(gltf_accessor.type == TINYGLTF_TYPE_SCALAR, L"");
			auto timelines = animation.timelines.emplace(gltf_sampler.input, std::vector<float>(gltf_accessor.count));
			if (timelines.second)
			{
				memcpy(timelines.first->second.data(), gltf_model.buffers.at(gltf_buffer_view.buffer).data.data() + gltf_buffer_view.byteOffset + gltf_accessor.byteOffset, gltf_accessor.count * sizeof(FLOAT));
			}
		}
		for (const tinygltf::AnimationChannel& gltf_channel : gltf_animation.channels)
		{
			animation::channel& channel = animation.channels.emplace_back();
			channel.sampler = gltf_channel.sampler;
			channel.target_node = gltf_channel.target_node;
			channel.target_path = gltf_channel.target_path;

			const tinygltf::AnimationSampler& gltf_sampler = gltf_animation.samplers.at(gltf_channel.sampler);
			const tinygltf::Accessor& gltf_accessor = gltf_model.accessors.at(gltf_sampler.output);
			const tinygltf::BufferView& gltf_buffer_view = gltf_model.bufferViews.at(gltf_accessor.bufferView);
			if (gltf_channel.target_path == "scale")
			{
				_ASSERT_EXPR(gltf_accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT, L"");
				_ASSERT_EXPR(gltf_accessor.type == TINYGLTF_TYPE_VEC3, L"");

				auto scales = animation.scales.emplace(gltf_sampler.output, std::vector<DirectX::XMFLOAT3>(gltf_accessor.count));
				if (scales.second)
				{
					memcpy(scales.first->second.data(), gltf_model.buffers.at(gltf_buffer_view.buffer).data.data() + gltf_buffer_view.byteOffset + gltf_accessor.byteOffset, gltf_accessor.count * sizeof(DirectX::XMFLOAT3));
				}
			}
			else if (gltf_channel.target_path == "rotation")
			{
				_ASSERT_EXPR(gltf_accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT, L"");
				_ASSERT_EXPR(gltf_accessor.type == TINYGLTF_TYPE_VEC4, L"");

				auto rotations = animation.rotations.emplace(gltf_sampler.output, std::vector<DirectX::XMFLOAT4>(gltf_accessor.count));
				if (rotations.second)
				{
					memcpy(rotations.first->second.data(), gltf_model.buffers.at(gltf_buffer_view.buffer).data.data() + gltf_buffer_view.byteOffset + gltf_accessor.byteOffset, gltf_accessor.count * sizeof(DirectX::XMFLOAT4));
				}
			}
			else if (gltf_channel.target_path == "translation")
			{
				_ASSERT_EXPR(gltf_accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT, L"");
				_ASSERT_EXPR(gltf_accessor.type == TINYGLTF_TYPE_VEC3, L"");
				auto translations = animation.translations.emplace(gltf_sampler.output, std::vector<DirectX::XMFLOAT3>(gltf_accessor.count));
				if (translations.second)
				{
					memcpy(translations.first->second.data(), gltf_model.buffers.at(gltf_buffer_view.buffer).data.data() + gltf_buffer_view.byteOffset + gltf_accessor.byteOffset, gltf_accessor.count * sizeof(DirectX::XMFLOAT3));
				}
			}
			else if (gltf_channel.target_path == "weights")
			{
				//_ASSERT_EXPR(FALSE, L"");
			}
			else
			{
				_ASSERT_EXPR(FALSE, L"");
			}
		}
	}
	// Find a longest animation duration in timeline of each channel.
	for (decltype(m_animations)::reference animation : m_animations)
	{
		for (decltype(animation.timelines)::reference timelines : animation.timelines)
		{
			animation.duration = std::max<float>(animation.duration, timelines.second.back());
		}
	}

}
void GltfModel::animate(size_t animation_index, float time, std::vector<node>& animated_nodes)
{
	_ASSERT_EXPR(m_animations.size() > animation_index, L"");
	_ASSERT_EXPR(animated_nodes.size() == m_nodes.size(), L"");

	std::function<size_t(const std::vector<float>&, float, float&)> indexof = [](const std::vector<float>& timelines, float time, float& interpolation_factor)->size_t {
		const size_t keyframe_count = timelines.size();
		if (time >= timelines.at(keyframe_count - 1))
		{
			interpolation_factor = 1.0f;
			return keyframe_count - 2;
		}
		else if (time < timelines.at(0))
		{
			interpolation_factor = 0.0f;
			return 0;
		}
		size_t keyframe_index = 0;
		for (size_t time_index = 1; time_index < keyframe_count; ++time_index)
		{
			if (time < timelines.at(time_index))
			{
				keyframe_index = time_index - 1;
				break;
			}
		}
		interpolation_factor = (time - timelines.at(keyframe_index + 0)) / (timelines.at(keyframe_index + 1) - timelines.at(keyframe_index + 0));
		return keyframe_index;
		};

	if (m_animations.size() > 0)
	{
		const animation& animation = m_animations.at(animation_index);
		for (std::vector<animation::channel>::const_reference channel : animation.channels)
		{
			const animation::sampler& sampler = animation.samplers.at(channel.sampler);
			const std::vector<float>& timeline = animation.timelines.at(sampler.input);
			if (timeline.size() == 0)
			{
				continue;
			}
			float interpolation_factor = {};
			size_t keyframe_index = indexof(timeline, time, interpolation_factor);
			if (channel.target_path == "scale")
			{
				const std::vector<DirectX::XMFLOAT3>& scales = animation.scales.at(sampler.output);
				DirectX::XMStoreFloat3(&animated_nodes.at(channel.target_node).scale, DirectX::XMVectorLerp(DirectX::XMLoadFloat3(&scales.at(keyframe_index + 0)), DirectX::XMLoadFloat3(&scales.at(keyframe_index + 1)), interpolation_factor));
			}
			else if (channel.target_path == "rotation")
			{
				const std::vector<DirectX::XMFLOAT4>& rotations = animation.rotations.at(sampler.output);
				DirectX::XMStoreFloat4(&animated_nodes.at(channel.target_node).rotation, DirectX::XMQuaternionNormalize(DirectX::XMQuaternionSlerp(DirectX::XMLoadFloat4(&rotations.at(keyframe_index + 0)), DirectX::XMLoadFloat4(&rotations.at(keyframe_index + 1)), interpolation_factor)));
			}
			else if (channel.target_path == "translation")
			{
				const std::vector<DirectX::XMFLOAT3>& translations = animation.translations.at(sampler.output);
				DirectX::XMStoreFloat3(&animated_nodes.at(channel.target_node).translation, DirectX::XMVectorLerp(DirectX::XMLoadFloat3(&translations.at(keyframe_index + 0)), DirectX::XMLoadFloat3(&translations.at(keyframe_index + 1)), interpolation_factor));
			}
			else if (channel.target_path == "weights")
			{

			}
			else
			{

			}
		}
		cumulate_transforms(animated_nodes);
	}
}

void GltfModel::create_and_upload_resources(ID3D11Device* device)
{
	HRESULT hr;
	D3D11_BUFFER_DESC buffer_desc = {};
	D3D11_SUBRESOURCE_DATA subresource_data = {};

	// Create and upload vertex and index m_buffers on GPU
	if (m_static_batching)
	{
		for (batch_mesh& batch_mesh : m_batch_meshes)
		{
			if (batch_mesh.index_buffer_view.size_in_bytes > 0)
			{
				batch_mesh.index_buffer_view.buffer = static_cast<int>(m_buffers.size());
				buffer_desc.ByteWidth = batch_mesh.index_buffer_view.size_in_bytes;
				buffer_desc.Usage = D3D11_USAGE_DEFAULT;
				buffer_desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
				buffer_desc.CPUAccessFlags = 0;
				buffer_desc.MiscFlags = 0;
				buffer_desc.StructureByteStride = 0;
				subresource_data.pSysMem = batch_mesh.cached_indices.data();
				subresource_data.SysMemPitch = 0;
				subresource_data.SysMemSlicePitch = 0;
				hr = device->CreateBuffer(&buffer_desc, &subresource_data, m_buffers.emplace_back().GetAddressOf());
				_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

				batch_mesh.cached_indices.clear();
			}

			if (batch_mesh.vertex_buffer_view.size_in_bytes > 0)
			{
				batch_mesh.vertex_buffer_view.buffer = static_cast<int>(m_buffers.size());
				buffer_desc.ByteWidth = batch_mesh.vertex_buffer_view.size_in_bytes;
				buffer_desc.Usage = D3D11_USAGE_DEFAULT;
				buffer_desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
				buffer_desc.CPUAccessFlags = 0;
				buffer_desc.MiscFlags = 0;
				buffer_desc.StructureByteStride = 0;
				subresource_data.pSysMem = batch_mesh.cached_vertices.data();
				subresource_data.SysMemPitch = 0;
				subresource_data.SysMemSlicePitch = 0;
				hr = device->CreateBuffer(&buffer_desc, &subresource_data, m_buffers.emplace_back().GetAddressOf());
				_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

				batch_mesh.cached_vertices.clear();
			}
		}
	}
	else
	{
		for (mesh& mesh : m_meshes)
		{
			for (mesh::primitive& primitive : mesh.primitives)
			{
				if (primitive.index_buffer_view.size_in_bytes > 0)
				{
					primitive.index_buffer_view.buffer = static_cast<int>(m_buffers.size());
					buffer_desc.ByteWidth = primitive.index_buffer_view.size_in_bytes;
					buffer_desc.Usage = D3D11_USAGE_DEFAULT;
					buffer_desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
					buffer_desc.CPUAccessFlags = 0;
					buffer_desc.MiscFlags = 0;
					buffer_desc.StructureByteStride = 0;
					subresource_data.pSysMem = primitive.cached_indices.data();
					subresource_data.SysMemPitch = 0;
					subresource_data.SysMemSlicePitch = 0;
					hr = device->CreateBuffer(&buffer_desc, &subresource_data, m_buffers.emplace_back().GetAddressOf());
					_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

					primitive.cached_indices.clear();
				}

				if (primitive.vertex_buffer_view.size_in_bytes > 0)
				{
					primitive.vertex_buffer_view.buffer = static_cast<int>(m_buffers.size());
					buffer_desc.ByteWidth = primitive.vertex_buffer_view.size_in_bytes;
					buffer_desc.Usage = D3D11_USAGE_DEFAULT;
					buffer_desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
					buffer_desc.CPUAccessFlags = 0;
					buffer_desc.MiscFlags = 0;
					buffer_desc.StructureByteStride = 0;
					subresource_data.pSysMem = primitive.cached_vertices.data();
					subresource_data.SysMemPitch = 0;
					subresource_data.SysMemSlicePitch = 0;
					hr = device->CreateBuffer(&buffer_desc, &subresource_data, m_buffers.emplace_back().GetAddressOf());
					_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

					primitive.cached_vertices.clear();
				}
			}
		}
	}

	// Create and upload m_materials on GPU
	std::vector<material::cbuffer> material_data;
	for (const material& material : m_materials)
	{
		material_data.emplace_back(material.data);
	}
	Microsoft::WRL::ComPtr<ID3D11Buffer> material_buffer;
	buffer_desc.ByteWidth = static_cast<UINT>(sizeof(material::cbuffer) * material_data.size());
	buffer_desc.Usage = D3D11_USAGE_DEFAULT;
	buffer_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	buffer_desc.CPUAccessFlags = 0;
	buffer_desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	buffer_desc.StructureByteStride = sizeof(material::cbuffer);
	subresource_data.pSysMem = material_data.data();
	subresource_data.SysMemPitch = 0;
	subresource_data.SysMemSlicePitch = 0;
	hr = device->CreateBuffer(&buffer_desc, &subresource_data, material_buffer.GetAddressOf());
	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));
	D3D11_SHADER_RESOURCE_VIEW_DESC shader_resource_view_desc = {};
	shader_resource_view_desc.Format = DXGI_FORMAT_UNKNOWN;
	shader_resource_view_desc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
	shader_resource_view_desc.Buffer.NumElements = static_cast<UINT>(material_data.size());
	hr = device->CreateShaderResourceView(material_buffer.Get(), &shader_resource_view_desc, m_material_resource_view.GetAddressOf());
	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

	// Create and upload m_textures on GPU
	for (image& image : m_images)
	{
		if (image.cache_data.size() > 0)
		{
			ID3D11ShaderResourceView* texture_resource_view = NULL;
			hr = load_texture_from_memory(device, image.cache_data.data(), image.cache_data.size(), &texture_resource_view);
			if (hr == S_OK)
			{
				m_texture_resource_views.emplace_back().Attach(texture_resource_view);
			}
			image.cache_data.clear();
		}
		else
		{
			const std::filesystem::path path(m_filename);

			std::wstring textureFilename =
				path.parent_path().wstring() +
				L"/" +
				std::wstring(image.uri.begin(), image.uri.end());

			Microsoft::WRL::ComPtr<ID3D11Resource> resource;
			Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv;

			hr = DirectX::CreateWICTextureFromFile(
				device,
				textureFilename.c_str(),
				resource.GetAddressOf(),
				srv.GetAddressOf()
			);

			if (SUCCEEDED(hr))
			{
				m_texture_resource_views.push_back(srv);
			}
		}
	}

	if (m_static_batching)
	{
		D3D11_INPUT_ELEMENT_DESC input_element_desc[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TANGENT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		};
		create_vs_from_cso(device, "./Data/Shader/GltfModelStaticBatching_vs.cso", m_vertex_shader.ReleaseAndGetAddressOf(), m_input_layout.ReleaseAndGetAddressOf(), input_element_desc, _countof(input_element_desc));
	}
	else
	{
		D3D11_INPUT_ELEMENT_DESC input_element_desc[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TANGENT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "JOINTS", 0, DXGI_FORMAT_R32G32B32A32_UINT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "WEIGHTS", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		};
		create_vs_from_cso(device, "./Data/Shader/GltfModel_vs.cso", m_vertex_shader.ReleaseAndGetAddressOf(), m_input_layout.ReleaseAndGetAddressOf(), input_element_desc, _countof(input_element_desc));
	}
	create_ps_from_cso(device, "./Data/Shader/GltfModel_ps.cso", m_pixel_shader.ReleaseAndGetAddressOf());

	// Create sampler states
	D3D11_SAMPLER_DESC sampler_desc = {};
	sampler_desc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	sampler_desc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	sampler_desc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	sampler_desc.MipLODBias = 0.0f;
	sampler_desc.MaxAnisotropy = 1;
	sampler_desc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	sampler_desc.BorderColor[0] = 0.0f;
	sampler_desc.BorderColor[1] = 0.0f;
	sampler_desc.BorderColor[2] = 0.0f;
	sampler_desc.BorderColor[3] = 0.0f;
	sampler_desc.MinLOD = 0.0f;
	sampler_desc.MaxLOD = D3D11_FLOAT32_MAX;

	// s0 : POINT
	sampler_desc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
	hr = device->CreateSamplerState(&sampler_desc, m_sampler_states[0].ReleaseAndGetAddressOf());
	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

	// s1 : LINEAR
	sampler_desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	hr = device->CreateSamplerState(&sampler_desc, m_sampler_states[1].ReleaseAndGetAddressOf());
	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

	// s2 : ANISOTROPIC
	sampler_desc.Filter = D3D11_FILTER_ANISOTROPIC;
	sampler_desc.MaxAnisotropy = 16;
	hr = device->CreateSamplerState(&sampler_desc, m_sampler_states[2].ReleaseAndGetAddressOf());
	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

	buffer_desc.ByteWidth = sizeof(primitive_constants);
	buffer_desc.Usage = D3D11_USAGE_DEFAULT;
	buffer_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	buffer_desc.CPUAccessFlags = 0;
	buffer_desc.MiscFlags = 0;
	buffer_desc.StructureByteStride = 0;
	hr = device->CreateBuffer(&buffer_desc, nullptr, m_primitive_cbuffer.ReleaseAndGetAddressOf());
	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

	buffer_desc.ByteWidth = sizeof(primitive_joint_constants);
	buffer_desc.Usage = D3D11_USAGE_DEFAULT;
	buffer_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	buffer_desc.CPUAccessFlags = 0;
	buffer_desc.MiscFlags = 0;
	buffer_desc.StructureByteStride = 0;
	hr = device->CreateBuffer(&buffer_desc, NULL, m_primitive_joint_cbuffer.ReleaseAndGetAddressOf());
	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));
}

void GltfModel::render(ID3D11DeviceContext* immediate_context, const DirectX::XMFLOAT4X4& world, const std::vector<node>& animated_nodes/*UNIT.37*/)
{
	immediate_context->VSSetShader(m_vertex_shader.Get(), nullptr, 0);
	immediate_context->PSSetShader(m_pixel_shader.Get(), nullptr, 0);
	immediate_context->IASetInputLayout(m_input_layout.Get());
	immediate_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	immediate_context->PSSetShaderResources(0, 1, m_material_resource_view.GetAddressOf());

	ID3D11SamplerState* sampler_states[] =
	{
		m_sampler_states[0].Get(),
		m_sampler_states[1].Get(),
		m_sampler_states[2].Get()
	};

	immediate_context->PSSetSamplers(0, 3, sampler_states);

	if (m_static_batching)
	{
		batch_render(immediate_context, world);
		UnbindResources(immediate_context);
		return;
	}

	const std::vector<node>& render_nodes = animated_nodes.size() > 0 ? animated_nodes : m_nodes;

	primitive_joint_constants primitive_joint_data{};
	std::function<void(int)> traverse = [&](int node_index)->void {
		const node& node = render_nodes.at(node_index);
		if (node.skin > -1)
		{
			const skin& skin = m_skins.at(node.skin);
			_ASSERT_EXPR(skin.joints.size() <= PRIMITIVE_MAX_JOINTS, L"The size of the joint array is insufficient, please expand it.");
			//primitive_joint_constants primitive_joint_data{};
			for (size_t joint_index = 0; joint_index < skin.joints.size(); ++joint_index)
			{
				DirectX::XMStoreFloat4x4(&primitive_joint_data.matrices[joint_index],
					DirectX::XMLoadFloat4x4(&skin.inverse_bind_matrices.at(joint_index)) *
					DirectX::XMLoadFloat4x4(&render_nodes.at(skin.joints.at(joint_index)).global_transform) *
					DirectX::XMMatrixInverse(NULL, DirectX::XMLoadFloat4x4(&node.global_transform))
				);
			}
			immediate_context->UpdateSubresource(m_primitive_joint_cbuffer.Get(), 0, 0, &primitive_joint_data, 0, 0);
			immediate_context->VSSetConstantBuffers(2, 1, m_primitive_joint_cbuffer.GetAddressOf());
		}
		if (node.mesh > -1)
		{
			const mesh& mesh = m_meshes.at(node.mesh);
			for (const mesh::primitive& primitive : mesh.primitives)
			{
				if (primitive.index_buffer_view.format == DXGI_FORMAT_R8_UINT)
					continue;

				if (primitive.vertex_buffer_view.buffer < 0)
					continue;

				// INTERLEAVED_GLTF_MODEL
				UINT stride = sizeof(mesh::vertex);
				UINT offset = 0;
				immediate_context->IASetVertexBuffers(0, 1, m_buffers.at(primitive.vertex_buffer_view.buffer).GetAddressOf(), &stride, &offset);

				primitive_constants primitive_data = {};
				primitive_data.material = primitive.material;
				primitive_data.has_tangent = primitive.has("TANGENT");
				primitive_data.skin = node.skin;
				DirectX::XMStoreFloat4x4(&primitive_data.world, DirectX::XMLoadFloat4x4(&node.global_transform) * DirectX::XMLoadFloat4x4(&world));
				immediate_context->UpdateSubresource(m_primitive_cbuffer.Get(), 0, 0, &primitive_data, 0, 0);
				immediate_context->VSSetConstantBuffers(0, 1, m_primitive_cbuffer.GetAddressOf());
				immediate_context->PSSetConstantBuffers(0, 1, m_primitive_cbuffer.GetAddressOf());

				const material& material = m_materials.at(primitive.material);
				const int texture_indices[] =
				{
					material.data.pbr_metallic_roughness.basecolor_texture.index,
					material.data.pbr_metallic_roughness.metallic_roughness_texture.index,
					material.data.normal_texture.index,
					material.data.emissive_texture.index,
					material.data.occlusion_texture.index,
				};
				ID3D11ShaderResourceView* null_shader_resource_view = {};
				std::vector<ID3D11ShaderResourceView*> shader_resource_views(_countof(texture_indices));
				for (int texture_index = 0; texture_index < shader_resource_views.size(); ++texture_index)
				{
					shader_resource_views.at(texture_index) = texture_indices[texture_index] > -1 ? m_texture_resource_views.at(m_textures.at(texture_indices[texture_index]).source).Get() : null_shader_resource_view;
				}
				immediate_context->PSSetShaderResources(1, static_cast<UINT>(shader_resource_views.size()), shader_resource_views.data());

				if (primitive.index_buffer_view.buffer > -1)
				{
					// INTERLEAVED_GLTF_MODEL
					immediate_context->IASetIndexBuffer(m_buffers.at(primitive.index_buffer_view.buffer).Get(), primitive.index_buffer_view.format, 0);
					immediate_context->DrawIndexed(primitive.index_buffer_view.size_in_bytes / _sizeof_component(primitive.index_buffer_view.format), 0, 0);
				}
				else
				{
					// INTERLEAVED_GLTF_MODEL
					immediate_context->Draw(primitive.vertex_buffer_view.size_in_bytes / primitive.vertex_buffer_view.stride_in_bytes, 0);
				}


			}
		}
		for (std::vector<int>::value_type child_index : node.children)
		{
			traverse(child_index);
		}
		};
	for (std::vector<int>::value_type node_index : m_scenes.at(m_default_scene).nodes)
	{
		traverse(node_index);
	}

	UnbindResources(immediate_context);
}
// INTERLEAVED_GLTF_MODEL
void GltfModel::batch_render(ID3D11DeviceContext* immediate_context, const DirectX::XMFLOAT4X4& world)
{
	_ASSERT_EXPR(m_static_batching, L"This function only works with m_static_batching data.");

	for (const batch_mesh& batch_mesh : m_batch_meshes)
	{
		if (batch_mesh.vertex_buffer_view.buffer < 0)
			continue;

		UINT stride = sizeof(batch_mesh::vertex);
		UINT offset = 0;
		immediate_context->IASetVertexBuffers(0, 1, m_buffers.at(batch_mesh.vertex_buffer_view.buffer).GetAddressOf(), &stride, &offset);

		primitive_constants primitive_data = {};
		primitive_data.material = batch_mesh.material;
		primitive_data.has_tangent = batch_mesh.has("TANGENT");
		primitive_data.skin = -1;
		primitive_data.world = world;
		immediate_context->UpdateSubresource(m_primitive_cbuffer.Get(), 0, 0, &primitive_data, 0, 0);
		immediate_context->VSSetConstantBuffers(0, 1, m_primitive_cbuffer.GetAddressOf());
		immediate_context->PSSetConstantBuffers(0, 1, m_primitive_cbuffer.GetAddressOf());

		const material& material = m_materials.at(batch_mesh.material);
		const int texture_indices[]
		{
			material.data.pbr_metallic_roughness.basecolor_texture.index,
			material.data.pbr_metallic_roughness.metallic_roughness_texture.index,
			material.data.normal_texture.index,
			material.data.emissive_texture.index,
			material.data.occlusion_texture.index,
		};
		ID3D11ShaderResourceView* null_shader_resource_view{};
		std::vector<ID3D11ShaderResourceView*> shader_resource_views(_countof(texture_indices));
		for (int texture_index = 0; texture_index < shader_resource_views.size(); ++texture_index)
		{
			shader_resource_views.at(texture_index) = texture_indices[texture_index] > -1 ? m_texture_resource_views.at(m_textures.at(texture_indices[texture_index]).source).Get() : null_shader_resource_view;
		}
		immediate_context->PSSetShaderResources(1, static_cast<UINT>(shader_resource_views.size()), shader_resource_views.data());

		if (batch_mesh.index_buffer_view.buffer > -1)
		{
			immediate_context->IASetIndexBuffer(m_buffers.at(batch_mesh.index_buffer_view.buffer).Get(), batch_mesh.index_buffer_view.format, 0);
			immediate_context->DrawIndexed(batch_mesh.index_buffer_view.size_in_bytes / _sizeof_component(batch_mesh.index_buffer_view.format), 0, 0);
		}
		else
		{
			immediate_context->Draw(batch_mesh.vertex_buffer_view.size_in_bytes / batch_mesh.vertex_buffer_view.stride_in_bytes, 0);
		}
	}
}

void GltfModel::UnbindResources(ID3D11DeviceContext* context)
{
	if (context == nullptr)
	{
		return;
	}

	ID3D11ShaderResourceView* nullSrvs[8] = {};
	context->PSSetShaderResources(0, 8, nullSrvs);
	context->VSSetShaderResources(0, 8, nullSrvs);
}
