#pragma once
#include "GeometryClass.h"
#include "MaterialClass.h"

struct ModelConstantBuffer {
	Math::Matrix4 worldMat;
};

class ModelClass
{
public:
	static UINT TOTALMODELCOUNT;
	const UINT m_id{ TOTALMODELCOUNT++ };

	ModelClass() :
		m_mesh{} {}

	ModelClass(const GeometryClass::Mesh& mesh) :
		m_mesh{ mesh } {}

	void DrawModel(
		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> cmdList,
		Microsoft::WRL::ComPtr<ID3D12Resource> modelCBResource,
		Microsoft::WRL::ComPtr<ID3D12Resource> materialCBResource,
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> srvHeapGlobal,
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> srvHeapDynamic,
		const std::vector<UINT>* shadowMapTextureIDs = nullptr);

	void ConstructBuffers(
		Microsoft::WRL::ComPtr<ID3D12Device> device,
		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> cmdList);
public:
	std::string m_name;
	MaterialClass m_material;

	GeometryClass::Mesh m_mesh;
	Math::OrthogonalTransform m_Transform{ Math::kIdentity };
	ModelConstantBuffer m_modelConstantBuffer{};
	float m_UniformScale{ 1.0f };
	
	bool
		m_castShadows{ true },
		m_receiveShadows{ true };

private:
	Microsoft::WRL::ComPtr<ID3D12Resource> m_vertexBuffer{};
	Microsoft::WRL::ComPtr<ID3D12Resource> m_vertexBufferUpload{};
	D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;

	Microsoft::WRL::ComPtr<ID3D12Resource> m_indexBuffer{};
	Microsoft::WRL::ComPtr<ID3D12Resource> m_indexBufferUpload{};
	D3D12_INDEX_BUFFER_VIEW m_indexBufferView;
};

