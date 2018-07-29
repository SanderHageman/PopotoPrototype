#include "stdafx.h"
#include "MaterialClass.h"

#include "DDSTextureLoader.h"
#include "WICTextureLoader.h"
#include "DirectXHelpers.h"

UINT MaterialClass::TOTALMATERIALCOUNT{ 0 };
UINT MaterialClass::Texture::TOTALTEXTURECOUNT{ 0 };

using namespace DirectX;
using namespace Utility;

void MaterialClass::DrawMaterial(
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> cmdList,
	Microsoft::WRL::ComPtr<ID3D12Resource> materialCBResource,
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> srvHeapGlobal,
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> srvHeapDynamic,
	const std::vector<UINT>* shadowMapTextureIDs) {

	// Set the correct constantbuffer
	cmdList->SetGraphicsRootConstantBufferView(
		RootParameterIndices::Material, 
		materialCBResource->GetGPUVirtualAddress() + (m_id * Math::AlignUp(sizeof(MaterialConstantBuffer), 256))
	);

	// Obtain the device <- prevents us from having to pass it as argument
	Microsoft::WRL::ComPtr<ID3D12Device> device;
	srvHeapGlobal->GetDevice(IID_GRAPHICS_PPV_ARGS(device.GetAddressOf()));

	// Set the descriptorsize if it hasn't been set already
	if (!m_cbvSrvDescriptorSize) {
		m_cbvSrvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	}

	// Calculate the offset in descriptors for this material
	const auto requiredSRVs = NUM_SRVS_PER_MATERIAL;
	const auto dynamicDescriptorOffset = m_id * requiredSRVs;

	// If invoking for shadowmap; only draw the diffuse
	if (shadowMapTextureIDs == nullptr) {

		const CD3DX12_CPU_DESCRIPTOR_HANDLE srvDynamicCPUHandle{ 
			srvHeapDynamic->GetCPUDescriptorHandleForHeapStart(), 
			static_cast<INT>(dynamicDescriptorOffset),
			m_cbvSrvDescriptorSize 
		};

		const CD3DX12_CPU_DESCRIPTOR_HANDLE srvGlobalCPUHandle{
			srvHeapGlobal->GetCPUDescriptorHandleForHeapStart(),
			static_cast<INT>(m_textures[materialTexture_diffuse]->m_id),
			m_cbvSrvDescriptorSize
		};

		device->CopyDescriptorsSimple(
			1U,
			srvDynamicCPUHandle,
			srvGlobalCPUHandle,
			D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
		);
	}
	else {
		// Obtain handles to the global heap and set to the start of the heap
		std::array<CD3DX12_CPU_DESCRIPTOR_HANDLE, requiredSRVs> srvGlobalCPUHandles{};
		srvGlobalCPUHandles.fill(CD3DX12_CPU_DESCRIPTOR_HANDLE{ srvHeapGlobal->GetCPUDescriptorHandleForHeapStart() });

		// Offset the handles to the textures in the heap
		for (UINT i = 0; i < NUM_TEXTURES_PER_MATERIAL; ++i) {
			srvGlobalCPUHandles[i].Offset(m_textures[i]->m_id, m_cbvSrvDescriptorSize);
		}

		for (UINT i = NUM_TEXTURES_PER_MATERIAL; i < NUM_SRVS_PER_MATERIAL; ++i) {
			const auto vectorIndex = i - NUM_TEXTURES_PER_MATERIAL;
			srvGlobalCPUHandles[i].Offset((*shadowMapTextureIDs)[vectorIndex], m_cbvSrvDescriptorSize);
		}

		// Obtain handle to the dynamic heap and offset to the correct location CPU
		std::array<CD3DX12_CPU_DESCRIPTOR_HANDLE, requiredSRVs> srvDynamicCPUHandles{};
		srvDynamicCPUHandles.fill(CD3DX12_CPU_DESCRIPTOR_HANDLE{ srvHeapDynamic->GetCPUDescriptorHandleForHeapStart() });
		for (auto i = 0; i < srvDynamicCPUHandles.size(); ++i) {
			srvDynamicCPUHandles[i].Offset(dynamicDescriptorOffset + i, m_cbvSrvDescriptorSize);
		}

		device->CopyDescriptors(
			requiredSRVs,
			srvDynamicCPUHandles.data(),
			nullptr,
			requiredSRVs,
			srvGlobalCPUHandles.data(),
			nullptr,
			D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
		);
	}

	const CD3DX12_GPU_DESCRIPTOR_HANDLE srvDynamicGPUHandle{
		srvHeapDynamic->GetGPUDescriptorHandleForHeapStart(),
		static_cast<INT>(dynamicDescriptorOffset),
		m_cbvSrvDescriptorSize
	};

	cmdList->SetGraphicsRootDescriptorTable(RootParameterIndices::Textures, srvDynamicGPUHandle);
}

void MaterialClass::Texture::Load(
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> cmdList,
	Microsoft::WRL::ComPtr<ID3D12Device> device,
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> srvHeap,
	const wchar_t* fileName) {

	m_fileName = fileName;
	
	std::string fileExtension(m_fileName.begin(), m_fileName.end());
	const auto found = fileExtension.find_last_of(".");
	fileExtension = fileExtension.substr(found + 1, fileExtension.size());

	// Create the resources
	const auto uploadHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	std::unique_ptr<uint8_t[]> decodedImageData;
	std::vector<D3D12_SUBRESOURCE_DATA> subresources;

	// Load the file to memory
	if (fileExtension == "DDS" || fileExtension == "dds") {
		ThrowIfFailed(LoadDDSTextureFromFile(
			device.Get(), 
			fileName, 
			m_textureResource.ReleaseAndGetAddressOf(), 
			decodedImageData, 
			subresources));
	}	
	else {
		subresources.resize(1);
		ThrowIfFailed(LoadWICTextureFromFile(
			device.Get(), 
			fileName, 
			m_textureResource.ReleaseAndGetAddressOf(), 
			decodedImageData, 
			subresources[0]));
	}

	const auto numSubResources = static_cast<UINT>(subresources.size());
	m_textureResource->SetName(fileName);

	// Calculate the size of the buffer
	const auto uploadBufferSize = GetRequiredIntermediateSize(m_textureResource.Get(), 0, numSubResources);
	const auto bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize);

	// Construct the uploadheap
	ThrowIfFailed(device->CreateCommittedResource(
		&uploadHeapProperties,
		D3D12_HEAP_FLAG_NONE,
		&bufferDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&m_textureResourceUpload)
	));
	NAME_D3D12_RES(m_textureResourceUpload);

	UpdateSubresources(cmdList.Get(), m_textureResource.Get(), m_textureResourceUpload.Get(), 0, 0, numSubResources, subresources.data());

	{
		const auto transitionBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
			m_textureResource.Get(),
			D3D12_RESOURCE_STATE_COPY_DEST,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
		);

		cmdList->ResourceBarrier(1, &transitionBarrier);
	}

	CD3DX12_CPU_DESCRIPTOR_HANDLE srvHandle{ srvHeap->GetCPUDescriptorHandleForHeapStart() };
	srvHandle.Offset(m_id, device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));

	CreateShaderResourceView(device.Get(), m_textureResource.Get(), srvHandle);
}