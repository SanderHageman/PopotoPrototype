#pragma once
#include "MaterialClass.h"

class ShadowMapClass
{
public:
	static UINT TOTALSHADOWMAPCOUNT;
	const UINT m_id{ TOTALSHADOWMAPCOUNT++ };

	ShadowMapClass(Microsoft::WRL::ComPtr<ID3D12Device> device, UINT width, UINT height, BOOL cubemap);
	~ShadowMapClass()=default;

	// Delete functions
	ShadowMapClass(ShadowMapClass const& rhs) = delete;
	ShadowMapClass& operator=(ShadowMapClass const& rhs) = delete;

	ShadowMapClass(ShadowMapClass&& rhs) = delete;
	ShadowMapClass& operator=(ShadowMapClass&& rhs) = delete;

public:
	UINT GetWidth() const { return m_width; }
	UINT GetHeight() const { return m_height; }

	CD3DX12_GPU_DESCRIPTOR_HANDLE GetSRV() const { return m_gpuSRV; }
	CD3DX12_CPU_DESCRIPTOR_HANDLE GetDSV() const { return m_cpuDSV[0]; }
	CD3DX12_CPU_DESCRIPTOR_HANDLE GetDSV(UINT index) const { return m_cpuDSV[index]; }
	
	void BuildDescriptors(
		D3D12_CPU_DESCRIPTOR_HANDLE cpuSRV, 
		D3D12_GPU_DESCRIPTOR_HANDLE gpuSRV, 
		D3D12_CPU_DESCRIPTOR_HANDLE cpuDSV,
		UINT CBVDescriptorSize,
		UINT DSVDescriptorSize);

	const D3D12_VIEWPORT m_viewport{};
	const D3D12_RECT m_scissorRect{};
	const BOOL m_cubemap{};

	const Microsoft::WRL::ComPtr<ID3D12Resource> Resource();
	const UINT GetTextureID() { return m_shadowMap->m_id; }
private:
	void BuildDescriptors();
	void BuildResource();

private:
	const Microsoft::WRL::ComPtr<ID3D12Device> m_device{};

	const UINT m_width{ 0 };
	const UINT m_height{ 0 };

	CD3DX12_CPU_DESCRIPTOR_HANDLE m_cpuSRV{};
	CD3DX12_GPU_DESCRIPTOR_HANDLE m_gpuSRV{};
	CD3DX12_CPU_DESCRIPTOR_HANDLE m_cpuDSV[6]{};

	std::unique_ptr<MaterialClass::Texture> m_shadowMap{};
};