#pragma once
#define NOMINMAX

#include <d3d11.h>
#include <wrl.h>
#include <DirectXMath.h>

#include <cstddef>
#include <map>
#include <string>
#include <unordered_map>
#include <vector>

namespace tinygltf
{
	class Model;
	class Accessor;
}

class GltfModel
{
public:
	static constexpr size_t PRIMITIVE_MAX_JOINTS = 512;

public:
	GltfModel(
		ID3D11Device* device,
		const std::string& filename,
		bool static_batching = false
	);

	virtual ~GltfModel();

	void Finalize();

	// ============================================================
	// glTF 構造体
	// ============================================================

	struct scene
	{
		std::string name;
		std::vector<int> nodes;
	};

	struct node
	{
		std::string name;
		int skin{ -1 };
		int mesh{ -1 };

		std::vector<int> children;

		DirectX::XMFLOAT4 rotation{ 0.0f, 0.0f, 0.0f, 1.0f };
		DirectX::XMFLOAT3 scale{ 1.0f, 1.0f, 1.0f };
		DirectX::XMFLOAT3 translation{ 0.0f, 0.0f, 0.0f };

		DirectX::XMFLOAT4X4 global_transform
		{
			1,0,0,0,
			0,1,0,0,
			0,0,1,0,
			0,0,0,1
		};
	};

	// ============================================================
	// GPU バッファ構造
	// ============================================================

	struct buffer_view
	{
		int buffer{ -1 };

		DXGI_FORMAT format{ DXGI_FORMAT_UNKNOWN };
		UINT stride_in_bytes{ 0 };
		UINT size_in_bytes{ 0 };

		size_t count() const
		{
			if (stride_in_bytes == 0) return 0;
			return size_in_bytes / stride_in_bytes;
		}
	};

	// ============================================================
	// Mesh / Primitive
	// ============================================================

	struct mesh
	{
		std::string name;

		struct vertex
		{
			DirectX::XMFLOAT3 position{ 0.0f, 0.0f, 0.0f };
			DirectX::XMFLOAT3 normal{ 0.0f, 1.0f, 0.0f };
			DirectX::XMFLOAT4 tangent{ 1.0f, 0.0f, 0.0f, 1.0f };
			DirectX::XMFLOAT2 texcoord{ 0.0f, 0.0f };

			DirectX::XMUINT4 joints{ 0, 0, 0, 0 };
			DirectX::XMFLOAT4 weights{ 0.0f, 0.0f, 0.0f, 0.0f };
		};

		struct primitive
		{
			int material{ -1 };

			buffer_view vertex_buffer_view;
			buffer_view index_buffer_view;

			std::vector<vertex> cached_vertices;
			std::vector<unsigned char> cached_indices;

			std::map<std::string, DXGI_FORMAT> attributes;

			bool has(const std::string& attribute_name) const
			{
				return attributes.find(attribute_name) != attributes.end();
			}
		};

		std::vector<primitive> primitives;
	};

	// ============================================================
	// Static batching 用 Mesh
	// ============================================================

	struct batch_mesh
	{
		int material{ -1 };

		struct vertex
		{
			DirectX::XMFLOAT3 position{ 0.0f, 0.0f, 0.0f };
			DirectX::XMFLOAT3 normal{ 0.0f, 1.0f, 0.0f };
			DirectX::XMFLOAT4 tangent{ 1.0f, 0.0f, 0.0f, 1.0f };
			DirectX::XMFLOAT2 texcoord{ 0.0f, 0.0f };
		};

		buffer_view vertex_buffer_view;
		buffer_view index_buffer_view;

		std::vector<vertex> cached_vertices;
		std::vector<UINT> cached_indices;

		std::map<std::string, DXGI_FORMAT> attributes;

		bool has(const std::string& attribute_name) const
		{
			return attributes.find(attribute_name) != attributes.end();
		}
	};

	// ============================================================
	// Material / Texture
	// ============================================================

	struct texture_info
	{
		int index{ -1 };
		int texcoord{ 0 };
	};

	struct normal_texture_info
	{
		int index{ -1 };
		int texcoord{ 0 };
		float scale{ 1.0f };
	};

	struct occlusion_texture_info
	{
		int index{ -1 };
		int texcoord{ 0 };
		float strength{ 1.0f };
	};

	struct pbr_metallic_roughness
	{
		float basecolor_factor[4]{ 1, 1, 1, 1 };
		texture_info basecolor_texture;

		float metallic_factor{ 1.0f };
		float roughness_factor{ 1.0f };
		texture_info metallic_roughness_texture;
	};

	struct material
	{
		std::string name;

		struct cbuffer
		{
			float emissive_factor[3]{ 0, 0, 0 };
			int alpha_mode{ 0 };

			float alpha_cutoff{ 0.5f };
			int double_sided{ 0 };

			pbr_metallic_roughness pbr_metallic_roughness;

			normal_texture_info normal_texture;
			occlusion_texture_info occlusion_texture;
			texture_info emissive_texture;
		};

		cbuffer data;
	};

	struct texture
	{
		std::string name;
		int source{ -1 };
	};

	struct image
	{
		std::string name;

		int width{ -1 };
		int height{ -1 };
		int component{ -1 };
		int bits{ -1 };
		int pixel_type{ -1 };

		std::string mime_type;
		std::string uri;
		bool as_is{ false };

		std::vector<unsigned char> cache_data;
	};

	// ============================================================
	// Skin / Animation
	// ============================================================

	struct skin
	{
		std::vector<DirectX::XMFLOAT4X4> inverse_bind_matrices;
		std::vector<int> joints;
	};

	struct animation
	{
		std::string name;
		float duration{ 0.0f };

		struct channel
		{
			int sampler{ -1 };
			int target_node{ -1 };
			std::string target_path;
		};

		struct sampler
		{
			int input{ -1 };
			int output{ -1 };
			std::string interpolation;
		};

		std::vector<channel> channels;
		std::vector<sampler> samplers;

		std::unordered_map<int, std::vector<float>> timelines;
		std::unordered_map<int, std::vector<DirectX::XMFLOAT3>> scales;
		std::unordered_map<int, std::vector<DirectX::XMFLOAT4>> rotations;
		std::unordered_map<int, std::vector<DirectX::XMFLOAT3>> translations;
	};

	// ============================================================
	// Primitive 定数バッファ
	// ============================================================

	struct primitive_constants
	{
		DirectX::XMFLOAT4X4 world;
		int material{ -1 };
		int has_tangent{ 0 };
		int skin{ -1 };
		int pad;
	};

	struct primitive_joint_constants
	{
		DirectX::XMFLOAT4X4 matrices[PRIMITIVE_MAX_JOINTS];
	};

public:
	void render(
		ID3D11DeviceContext* immediate_context,
		const DirectX::XMFLOAT4X4& world,
		const std::vector<node>& animated_nodes = {}
	);

	void animate(
		size_t animation_index,
		float time,
		std::vector<node>& animated_nodes
	);

	const std::vector<node>& GetNodes() const { return m_nodes; }
	const std::vector<animation>& GetAnimations() const { return m_animations; }
	bool IsStaticBatching() const { return m_static_batching; }

private:
	// ============================================================
	// glTF 読み込み処理
	// ============================================================

	void fetch_nodes(const tinygltf::Model& gltf_model);
	void cumulate_transforms(std::vector<node>& nodes);

	void fetch_meshes(const tinygltf::Model& gltf_model);
	void fetch_and_batch_meshes(const tinygltf::Model& gltf_model);

	void fetch_materials(const tinygltf::Model& gltf_model);
	void fetch_textures(const tinygltf::Model& gltf_model);
	void fetch_animations(const tinygltf::Model& gltf_model);

	void create_and_upload_resources(ID3D11Device* device);

	void batch_render(
		ID3D11DeviceContext* immediate_context,
		const DirectX::XMFLOAT4X4& world
	);

	void UnbindResources(ID3D11DeviceContext* context);

	std::vector<node> m_nodes;

private:
	// ============================================================
	// メンバ変数
	// ============================================================

	std::string m_filename;

	bool m_static_batching{ false };
	int m_default_scene{ 0 };

	std::vector<scene> m_scenes;
	std::vector<mesh> m_meshes;
	std::vector<batch_mesh> m_batch_meshes;

	std::vector<material> m_materials;
	std::vector<image> m_images;
	std::vector<texture> m_textures;

	std::vector<skin> m_skins;
	std::vector<animation> m_animations;

	std::vector<Microsoft::WRL::ComPtr<ID3D11Buffer>> m_buffers;

	Microsoft::WRL::ComPtr<ID3D11VertexShader> m_vertex_shader;
	Microsoft::WRL::ComPtr<ID3D11PixelShader> m_pixel_shader;
	Microsoft::WRL::ComPtr<ID3D11InputLayout> m_input_layout;

	Microsoft::WRL::ComPtr<ID3D11SamplerState> m_sampler_states[3];

	Microsoft::WRL::ComPtr<ID3D11Buffer> m_primitive_cbuffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer> m_primitive_joint_cbuffer;

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_material_resource_view;

	std::vector<Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>> m_texture_resource_views;

	bool m_finalized = false;
};
