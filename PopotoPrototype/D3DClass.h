#pragma once

#include "CameraClass.h"
#include "ModelClass.h"
#include "ShadowMapClass.h"

struct Light
{
	DirectX::XMFLOAT3 Strength = { 0.5f, 0.5f, 0.5f };
	float FalloffStart = 1.0f;                          // point/spot light only
	DirectX::XMFLOAT3 Direction = { 0.0f, -1.0f, 0.0f };// directional/spot light only
	float FalloffEnd = 10.0f;                           // point/spot light only
	DirectX::XMFLOAT3 Position = { 0.0f, 0.0f, 0.0f };  // point/spot light only
	float SpotPower = 64.0f;                            // spot light only
};

#define MaxLights 16

/*
	Directional:
		[0, 1]
	Point:
		[2]
	Spot:
		-
*/

struct MainPassConstantBuffer {
	Math::Vector3 eyePosition{ 0.0f, 0.0f, 0.0f };
	Math::Vector4 ambientLight{ 0.0f, 0.0f, 0.0f, 0.0f };
	Math::Matrix4 vpMat{ Math::kIdentity };
	Math::Matrix4 directionalLightVpMat{ Math::kIdentity };
	std::array<Math::Matrix4, 6> pointLightVpMats{ Math::Matrix4{ Math::kIdentity } };
	Light lights[MaxLights];
};

struct LightPassConstantBuffer {
	Math::Matrix4 lightVpMat{ Math::kIdentity };
};

struct ShadowCaster {
	std::unique_ptr<CameraClass> transform[6];
	std::unique_ptr<ShadowMapClass> shadowMap;
	Math::Matrix4 projMatrix{ Math::kIdentity };
};

class InputClass;
class D3DClass {
public:

	// Width, Height, Far, Near, VSYNC, and hWnd
	D3DClass(int sWidth, int sHeight, float fFar, float fNear, bool vSync, HWND hWnd);
	~D3DClass();

	void Render();
	void UpdateModel(ModelClass& aModel);

	// Delete functions
	D3DClass(D3DClass const& rhs) = delete;
	D3DClass& operator=(D3DClass const& rhs) = delete;

	D3DClass(D3DClass&& rhs) = delete;
	D3DClass& operator=(D3DClass&& rhs) = delete;

private:
	void PopulateCommandList();
	void RenderSceneToShadowMap(const ShadowCaster& sc);
	void WaitForGpu();
	void MoveToNextFrame();
	void LoadAssets();
	void LoadScene(std::string assetPath, bool invertTexY = false);


	void RenderAllModels(const std::vector<UINT>* shadowMapTextureIDs = nullptr);
	void UpdateMainPass();

public:
	static const UINT FrameCount = 3;
	static const UINT TexturePixelSize = 4;	// The number of bytes used to represent a pixel in the texture.
	const float m_aspectRatio;
	const float m_nearClip;
	const float m_farClip;
	const bool m_vsync_enabled;
	std::unique_ptr<CameraClass> m_camera;
	std::vector<ModelClass> m_models;

	ShadowCaster m_directionalLight;
	ShadowCaster m_pointLight;

private:
	// Pipeline objects
	CD3DX12_VIEWPORT m_viewport;
	CD3DX12_RECT m_scissorRect;
	Microsoft::WRL::ComPtr<ID3D12Device> m_device;
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_commandQueue;
	Microsoft::WRL::ComPtr<IDXGISwapChain3> m_swapChain;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_renderTargets[FrameCount];
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_commandAllocators[FrameCount];
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_commandList;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_srvHeapGlobal;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_srvHeapDynamic[FrameCount];
	Microsoft::WRL::ComPtr<ID3D12Resource> m_dsvBuffer;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_dsvHeap;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> m_defaultPipelineState;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> m_ReceiveNoShadowPipelineState;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> m_shadowMapPipelineState;
	Microsoft::WRL::ComPtr<ID3D12RootSignature> m_rootSignature;
	UINT m_rtvDescriptorSize = 0;

	// Synchronization objects
	UINT m_frameIndex;
	Microsoft::WRL::ComPtr<ID3D12Fence> m_fence;
	HANDLE m_fenceEvent;
	UINT64 m_fenceValues[FrameCount];
	
	// CBVs
	Microsoft::WRL::ComPtr<ID3D12Resource> m_modelConstantBufferResource[FrameCount];
	Utility::PaddedBlock<ModelConstantBuffer>* m_modelConstantBufferData[FrameCount];

	Microsoft::WRL::ComPtr<ID3D12Resource> m_mainPassConstantBufferResource[FrameCount];
	Utility::PaddedBlock<MainPassConstantBuffer>* m_mainPassConstantBufferData[FrameCount];
	Utility::PaddedBlock<LightPassConstantBuffer>* m_lightPassConstantBufferData[FrameCount];
	MainPassConstantBuffer m_mainPassConstantBuffer{};

	Microsoft::WRL::ComPtr<ID3D12Resource> m_materialConstantBufferResource[FrameCount];
	Utility::PaddedBlock<MaterialClass::MaterialConstantBuffer>* m_materialConstantBufferData[FrameCount];


	// Debug Variables
#if defined(_DEBUG)
	Microsoft::WRL::ComPtr<ID3D12Debug> m_debugController;
#endif
};