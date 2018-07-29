#include "stdafx.h"
#include "ModelClass.h"

UINT ModelClass::TOTALMODELCOUNT{ 0 };

void ModelClass::DrawModel(
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> cmdList,
	Microsoft::WRL::ComPtr<ID3D12Resource> modelCBResource,
	Microsoft::WRL::ComPtr<ID3D12Resource> materialCBResource,
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> srvHeapGlobal,
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> srvHeapDynamic,
	const std::vector<UINT>* shadowMapTextureIDs) {

	cmdList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
	cmdList->IASetIndexBuffer(&m_indexBufferView);


	m_material.DrawMaterial(cmdList, materialCBResource, srvHeapGlobal, srvHeapDynamic, shadowMapTextureIDs);

	cmdList->SetGraphicsRootConstantBufferView(
		Utility::RootParameterIndices::Object, 
		modelCBResource->GetGPUVirtualAddress() + (m_id * Math::AlignUp(sizeof(ModelConstantBuffer), 256))
	);

	cmdList->DrawIndexedInstanced(static_cast<UINT>(m_mesh.m_indices.size()), 1, 0, 0, 0);
}

void ModelClass::ConstructBuffers(
	Microsoft::WRL::ComPtr<ID3D12Device> device,
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> cmdList) {

	// Create vertex buffer
	{
		const auto vertexBufferSize = static_cast<UINT>(sizeof(GeometryClass::Vertex) * m_mesh.m_vertices.size());

		m_vertexBuffer = Utility::CreateDefaultBuffer(
			device.Get(), 
			cmdList.Get(), 
			m_mesh.m_vertices.data(), 
			vertexBufferSize, 
			m_vertexBufferUpload, 
			D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

		// Initialize vertex buffer view
		m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
		m_vertexBufferView.StrideInBytes = sizeof(GeometryClass::Vertex);
		m_vertexBufferView.SizeInBytes = vertexBufferSize;
	}

	// Create index buffer
	{
		const auto indexBufferSize = static_cast<UINT>(sizeof(UINT32) * m_mesh.m_indices.size());

		m_indexBuffer = Utility::CreateDefaultBuffer(
			device.Get(), 
			cmdList.Get(), 
			m_mesh.m_indices.data(), 
			indexBufferSize, 
			m_indexBufferUpload, 
			D3D12_RESOURCE_STATE_INDEX_BUFFER);

		// Initialize index buffer view
		m_indexBufferView.BufferLocation = m_indexBuffer->GetGPUVirtualAddress();
		m_indexBufferView.Format = DXGI_FORMAT_R32_UINT;
		m_indexBufferView.SizeInBytes = indexBufferSize;
	}
}