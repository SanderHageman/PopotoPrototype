#pragma once

class MaterialClass
{
public:
	static UINT TOTALMATERIALCOUNT;
	const UINT m_id{ TOTALMATERIALCOUNT++ };

	struct Texture {
		static UINT TOTALTEXTURECOUNT;
		const UINT m_id{ TOTALTEXTURECOUNT++ };

		std::wstring m_fileName{};

		Microsoft::WRL::ComPtr<ID3D12Resource> m_textureResource{};
		Microsoft::WRL::ComPtr<ID3D12Resource> m_textureResourceUpload{};

		void Load(
			Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> cmdList,
			Microsoft::WRL::ComPtr<ID3D12Device> device,
			Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> srvHeap,
			const wchar_t* fileName);
	};

	struct MaterialConstantBuffer {
		Math::Vector4 m_diffuseAlbedo{ 1.0f, 1.0f, 1.0f, 1.0f };
		Math::Vector3 m_fresnelR0{ 0.1f, 0.1f, 0.1f };
		float m_roughness = 0.3f;
	};

	enum materialTexture {
		materialTexture_diffuse,
		materialTexture_normal,
		materialTexture_specular,
		NUM_TEXTURES_PER_MATERIAL,
		NUM_SRVS_PER_MATERIAL = 5 // Plus two shadowmaps
	};

public:
	std::string m_name{};

	std::shared_ptr<Texture> m_textures[NUM_TEXTURES_PER_MATERIAL];

	MaterialConstantBuffer m_materialConstantBuffer{};
public:
	void DrawMaterial(
		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> cmdList,
		Microsoft::WRL::ComPtr<ID3D12Resource> materialCBResource,
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> srvHeapGlobal,
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> srvHeapDynamic,
		const std::vector<UINT>* shadowMapTextureIDs);
private:
	UINT m_cbvSrvDescriptorSize{};
};
