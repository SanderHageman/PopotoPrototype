#include "stdafx.h"
#include "ShadowMapClass.h"

UINT ShadowMapClass::TOTALSHADOWMAPCOUNT{ 0 };

ShadowMapClass::ShadowMapClass(Microsoft::WRL::ComPtr<ID3D12Device> device, UINT width, UINT height, BOOL cubemap) :
	m_device{ device }, m_width{ width }, m_height{ height }, 
	m_viewport{ 0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height), 0.0f, 1.0f },
	m_scissorRect{ 0, 0, static_cast<int>(width), static_cast<int>(height) },
	m_cubemap(cubemap) {
	BuildResource();
}

void ShadowMapClass::BuildDescriptors(
	D3D12_CPU_DESCRIPTOR_HANDLE cpuSRV, 
	D3D12_GPU_DESCRIPTOR_HANDLE gpuSRV, 
	D3D12_CPU_DESCRIPTOR_HANDLE cpuDSV,
	UINT CBVDescriptorSize,
	UINT DSVDescriptorSize) {

	m_cpuSRV = CD3DX12_CPU_DESCRIPTOR_HANDLE(cpuSRV, m_shadowMap->m_id, CBVDescriptorSize);
	m_gpuSRV = CD3DX12_GPU_DESCRIPTOR_HANDLE(gpuSRV, m_shadowMap->m_id, CBVDescriptorSize);
	m_cpuDSV[0] = CD3DX12_CPU_DESCRIPTOR_HANDLE(cpuDSV, m_shadowMap->m_id + 1, DSVDescriptorSize);

	if (m_cubemap) {
		for (int i = 1; i < 6; ++i) {
			m_cpuDSV[i] = CD3DX12_CPU_DESCRIPTOR_HANDLE(cpuDSV, m_shadowMap->m_id + 1 + i, DSVDescriptorSize);
		}
	}

	BuildDescriptors();
}

const Microsoft::WRL::ComPtr<ID3D12Resource> ShadowMapClass::Resource()
{
	return m_shadowMap->m_textureResource;
}

void ShadowMapClass::BuildDescriptors() {
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;

	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
	dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
	dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	
	if (m_cubemap) {
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
		srvDesc.TextureCube.MostDetailedMip = 0;
		srvDesc.TextureCube.MipLevels = 1;
		srvDesc.TextureCube.ResourceMinLODClamp = 0.0f;

		dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
		dsvDesc.Texture2DArray.MipSlice = 0;
		dsvDesc.Texture2DArray.ArraySize = 1;

		for (int i = 0; i < 6; ++i) {
			dsvDesc.Texture2DArray.FirstArraySlice = i;
			m_device->CreateDepthStencilView(m_shadowMap->m_textureResource.Get(), &dsvDesc, m_cpuDSV[i]);
		}
	}
	else{
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MostDetailedMip = 0;
		srvDesc.Texture2D.MipLevels = 1;
		srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
		srvDesc.Texture2D.PlaneSlice = 0;

		dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
		dsvDesc.Texture2D.MipSlice = 0;
		m_device->CreateDepthStencilView(m_shadowMap->m_textureResource.Get(), &dsvDesc, m_cpuDSV[0]);
	}

	m_device->CreateShaderResourceView(m_shadowMap->m_textureResource.Get(), &srvDesc, m_cpuSRV);
}

void ShadowMapClass::BuildResource() {
	m_shadowMap = std::make_unique<MaterialClass::Texture>();

	const auto texDesc = m_cubemap ? 
		(CD3DX12_RESOURCE_DESC::Tex2D(
			DXGI_FORMAT_R24G8_TYPELESS,
			m_width,
			m_height,
			6UI16,
			1UI16,
			1U,
			0U,
			D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL
		)) : 
		(CD3DX12_RESOURCE_DESC::Tex2D(
			DXGI_FORMAT_R24G8_TYPELESS,
			m_width,
			m_height,
			1UI16,
			1UI16,
			1U,
			0U,
			D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL
		));

	const CD3DX12_CLEAR_VALUE optClear(DXGI_FORMAT_D24_UNORM_S8_UINT, 1.0f, 0ui8);
	const CD3DX12_HEAP_PROPERTIES defaultHeapType(D3D12_HEAP_TYPE_DEFAULT);

	Utility::ThrowIfFailed(
		m_device->CreateCommittedResource(
			&defaultHeapType, 
			D3D12_HEAP_FLAG_NONE, 
			&texDesc, 
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			&optClear, 
			IID_PPV_ARGS(&m_shadowMap->m_textureResource)));

	std::wstringstream t_SStream;
	t_SStream << "ShadowmapNr" << m_shadowMap->m_id;
	m_shadowMap->m_textureResource->SetName(t_SStream.str().c_str());
}