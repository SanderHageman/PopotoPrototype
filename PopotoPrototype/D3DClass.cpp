#include "stdafx.h"
#include "D3DClass.h"
#include "InputClass.h"

using Vertex = GeometryClass::Vertex;
using Microsoft::WRL::ComPtr;
using namespace Utility;

// Function to find and select the graphics adapter with the largest amount of video memory which can be assumed to be the 'best' choice
void GetHardwareAdapter(IDXGIFactory2* pFactory, IDXGIAdapter1** ppAdapter)
{
	ComPtr<IDXGIAdapter1> adapter;
	*ppAdapter = nullptr;

	SIZE_T highestMemoryAmount{};
	UINT preferredAdapterIndex = UINT_MAX;

	for (UINT adapterIndex = 0; DXGI_ERROR_NOT_FOUND != pFactory->EnumAdapters1(adapterIndex, &adapter); ++adapterIndex)
	{
		DXGI_ADAPTER_DESC1 desc;
		adapter->GetDesc1(&desc);

		// Check to see if the adapter supports Direct3D 12, but don't create the
		// actual device yet.
		if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr)))
		{
			if (desc.DedicatedVideoMemory > highestMemoryAmount) {
				highestMemoryAmount = desc.DedicatedVideoMemory;
				preferredAdapterIndex = adapterIndex;
			}
		}
	}

	if (DXGI_ERROR_NOT_FOUND != pFactory->EnumAdapters1(preferredAdapterIndex, &adapter)) {
		*ppAdapter = adapter.Detach();
	}
}

D3DClass::D3DClass(int sWidth, int sHeight, float fFar, float fNear, bool vSync, HWND hWnd) :
	m_frameIndex{},
	m_fenceValues{},
	m_vsync_enabled(vSync), 
	m_aspectRatio(static_cast<float>(sWidth) / static_cast<float>(sHeight)),
	m_farClip(fFar),
	m_nearClip(fNear),
	m_viewport(0.0f, 0.0f, static_cast<float>(sWidth), static_cast<float>(sHeight)),
	m_scissorRect(0, 0, static_cast<LONG>(sWidth), static_cast<LONG>(sHeight)) {

	UINT dxgiFactoryFlags = 0;

#if defined(_DEBUG)
	// Enable the debug layer (requires the Graphics Tools "optional feature").
	// NOTE: Enabling the debug layer after device creation will invalidate the active device.
	{
		if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&m_debugController))))
		{
			m_debugController->EnableDebugLayer();

			// Enable additional debug layers.
			dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
		}
	}
#endif

	ComPtr<IDXGIFactory4> factory;
	ThrowIfFailed(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&factory)));

	// Poll for the best available hw adapter and create the device
	ComPtr<IDXGIAdapter1> hardwareAdapter;
	GetHardwareAdapter(factory.Get(), &hardwareAdapter);
	ThrowIfFailed(D3D12CreateDevice(
		hardwareAdapter.Get(),
		D3D_FEATURE_LEVEL_11_0,
		IID_PPV_ARGS(&m_device)
	));

	// Setup command queue
	D3D12_COMMAND_QUEUE_DESC queueDesc{};
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	ThrowIfFailed(m_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_commandQueue)));

	// Setup swap chain
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};
	swapChainDesc.BufferCount = FrameCount;
	swapChainDesc.Width = sWidth;
	swapChainDesc.Height = sHeight;
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.SampleDesc.Count = 1;

	ComPtr<IDXGISwapChain1> swapChain;
	ThrowIfFailed(factory->CreateSwapChainForHwnd(
		m_commandQueue.Get(),
		hWnd,
		&swapChainDesc,
		nullptr,
		nullptr,
		&swapChain
	));

	ThrowIfFailed(swapChain.As(&m_swapChain));
	m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();

	// Disable fullscreen
	ThrowIfFailed(factory->MakeWindowAssociation(hWnd, DXGI_MWA_NO_ALT_ENTER));

	// Create descriptor heaps
	{
		D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc{};
		rtvHeapDesc.NumDescriptors = FrameCount;
		rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		ThrowIfFailed(m_device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_rtvHeap)));

		D3D12_DESCRIPTOR_HEAP_DESC srvHeapDescGlobal{};
		srvHeapDescGlobal.NumDescriptors = 1024U;
		srvHeapDescGlobal.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		srvHeapDescGlobal.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		ThrowIfFailed(m_device->CreateDescriptorHeap(&srvHeapDescGlobal, IID_PPV_ARGS(&m_srvHeapGlobal)));

		D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc{};
		dsvHeapDesc.NumDescriptors = 14U;
		dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
		dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		ThrowIfFailed(m_device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&m_dsvHeap)));

		m_rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	}

	// Create frame resources
	{
		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart());

		D3D12_DESCRIPTOR_HEAP_DESC srvHeapDescDynamic{};
		srvHeapDescDynamic.NumDescriptors = 1024U;
		srvHeapDescDynamic.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		srvHeapDescDynamic.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

		for (UINT n = 0; n < FrameCount; n++) {

			// RTV
			ThrowIfFailed(m_swapChain->GetBuffer(
				n, 
				IID_PPV_ARGS(&m_renderTargets[n])
			));

			m_device->CreateRenderTargetView(
				m_renderTargets[n].Get(), 
				nullptr, 
				rtvHandle
			);
			NAME_D3D12_RES_INDEXED(m_renderTargets, n);

			rtvHandle.Offset(1, m_rtvDescriptorSize);

			// Create the command allocator
			ThrowIfFailed(
				m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_commandAllocators[n]))
			);

			ThrowIfFailed(m_device->CreateDescriptorHeap(&srvHeapDescDynamic, IID_PPV_ARGS(&m_srvHeapDynamic[n])));
		}
	}

	m_camera = std::make_unique<CameraClass>(XM_PIDIV4 * 1.3f, m_aspectRatio, m_nearClip, m_farClip);
	m_camera->SetPosition({ 0.553669f, -0.185295f, 0.0333168f });
	LoadAssets();
}

D3DClass::~D3DClass() {
	try {
		WaitForGpu();
		CloseHandle(m_fenceEvent);
	}
	catch (const std::exception&) {
		std::terminate();
	}
}

void D3DClass::Render() {
	m_camera->Update();

	if (GetAsyncKeyState(VK_F7)) {
		const auto curPos = m_camera->GetPosition();
		m_models[0].m_Transform.SetTranslation(curPos);
		for (UINT i = 0; i < 6; ++i) {
			m_pointLight.transform[i]->SetPosition(curPos);
			m_pointLight.transform[i]->Update();
		}
	}
	static bool moveCamera = false;
	if (GetAsyncKeyState(VK_F4)) moveCamera = !moveCamera;
	if(moveCamera){
		const auto vecStart = Vector3{ -0.697373f, -0.33885f, 0.0881591f };
		const auto vecEnd = Vector3{ 0.692562f, -0.171149f, -0.00233012f };
		const auto speed = 0.0005f;
		const auto vecDiff = vecEnd - vecStart;

		static float t = 0;
		const auto curPos = Vector3(DirectX::XMVectorLerp(vecStart, vecEnd, t));
		m_models[0].m_Transform.SetTranslation(curPos);

		static bool dir = true;
		if (dir) {
			t += speed;

			if (t >= 1.0f)
				dir = false;
		}
		else {
			t -= speed;

			if (t <= 0.0f)
				dir = true;
		}

		for (UINT i = 0; i < 6; ++i) {
			m_pointLight.transform[i]->SetPosition(curPos);
			m_pointLight.transform[i]->Update();
		}
	}

	PopulateCommandList();

	ID3D12CommandList* ppCommandLists[] = { m_commandList.Get() };
	m_commandQueue->ExecuteCommandLists(std::extent_v<decltype(ppCommandLists)>, ppCommandLists);

	if (m_vsync_enabled) {
		ThrowIfFailed(m_swapChain->Present(1, 0));
	}
	else {
		ThrowIfFailed(m_swapChain->Present(0, 0));
	}

	MoveToNextFrame();
}

void D3DClass::UpdateModel(ModelClass& aModel) {
	aModel.m_modelConstantBuffer.worldMat = Matrix4(aModel.m_Transform) * Matrix4::MakeScale(aModel.m_UniformScale);
	m_modelConstantBufferData[m_frameIndex][aModel.m_id] = aModel.m_modelConstantBuffer;

	auto& material = aModel.m_material;
	m_materialConstantBufferData[m_frameIndex][material.m_id] = material.m_materialConstantBuffer;
}

const std::array<const char*, 11> g_bannedModelNames{
	"PlanarReflection_1",
	"PlanarReflection2",
	"Cube2",
	"Cube3",
	"Cube4",
	"Cube5_3",
	"Cube_2",
	"Cube6_2",
	"Cube7",
	"sponza_04",
	"BP_Sky_Sphere_256"
};

void D3DClass::PopulateCommandList() {

	// Reset command allocator and lists
	ThrowIfFailed(m_commandAllocators[m_frameIndex]->Reset());
	ThrowIfFailed(m_commandList->Reset(m_commandAllocators[m_frameIndex].Get(), m_defaultPipelineState.Get()));

	// Set required state
	ID3D12DescriptorHeap* ppHeaps[] = { m_srvHeapDynamic[m_frameIndex].Get() };
	m_commandList->SetDescriptorHeaps(std::extent_v<decltype(ppHeaps)>, ppHeaps);

	m_commandList->SetGraphicsRootSignature(m_rootSignature.Get());

	UpdateMainPass();

	m_commandList->SetGraphicsRootConstantBufferView(
		RootParameterIndices::MainPass, 
		m_mainPassConstantBufferResource[m_frameIndex]->GetGPUVirtualAddress());

	for (auto& model : m_models) {
		UpdateModel(model);
	}

	m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	RenderSceneToShadowMap(m_directionalLight);
	RenderSceneToShadowMap(m_pointLight);

	m_commandList->RSSetViewports(1, &m_viewport);
	m_commandList->RSSetScissorRects(1, &m_scissorRect);

	// Signal the commandlist that the back buffer will be used as the render target
	{
		const auto transitionBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
			m_renderTargets[m_frameIndex].Get(),
			D3D12_RESOURCE_STATE_PRESENT,
			D3D12_RESOURCE_STATE_RENDER_TARGET
		);

		m_commandList->ResourceBarrier(1, &transitionBarrier);
	}

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(
		m_rtvHeap->GetCPUDescriptorHandleForHeapStart(),
		m_frameIndex, 
		m_rtvDescriptorSize
	);

	CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(
		m_dsvHeap->GetCPUDescriptorHandleForHeapStart()
	);

	m_commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);
	//m_commandList->SetPipelineState(m_defaultPipelineState.Get());

	// Issue commands to the commandlist
	{
		// Start clearing the rendertarget
		constexpr std::array<FLOAT, 4> clearColour = { 0.3f, 0.5f, 0.8f, 1.0f };
		m_commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0UI8, 0U, nullptr);
		m_commandList->ClearRenderTargetView(rtvHandle, clearColour.data(), 0, nullptr);

		std::vector<UINT> shadowMapIDs{ m_directionalLight.shadowMap->GetTextureID(), m_pointLight.shadowMap->GetTextureID() };
		RenderAllModels(&shadowMapIDs);
	}

	// Signal the commandlist that the back buffer is to be presented
	{
		const auto transitionBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
			m_renderTargets[m_frameIndex].Get(),
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			D3D12_RESOURCE_STATE_PRESENT
		);

		m_commandList->ResourceBarrier(1, &transitionBarrier);
	}

	ThrowIfFailed(m_commandList->Close());
}

void D3DClass::RenderSceneToShadowMap(const ShadowCaster& sc) {

	{
		if (GetAsyncKeyState(VK_F2)) {
			m_directionalLight.transform[0]->SetDirection(Matrix3::MakeYRotation(0.0005f) * m_directionalLight.transform[0]->GetForward(), Vector3(kYUnitVector));
			m_directionalLight.transform[0]->SetPosition(-1.0f * 1.25f * m_directionalLight.transform[0]->GetForward());
		}
		if (GetAsyncKeyState(VK_F3)) {
			m_directionalLight.transform[0]->SetDirection(Matrix3::MakeYRotation(0.01f) * m_directionalLight.transform[0]->GetForward(), Vector3(kYUnitVector));
			m_directionalLight.transform[0]->SetPosition(-1.0f * 1.25f * m_directionalLight.transform[0]->GetForward());
		}

		m_directionalLight.transform[0]->Update();
	}

	const auto& shadowMap = sc.shadowMap;

	m_commandList->RSSetViewports(1, &shadowMap->m_viewport);
	m_commandList->RSSetScissorRects(1, &shadowMap->m_scissorRect);

	{
		const auto transitionBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
			shadowMap->Resource().Get(),
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			D3D12_RESOURCE_STATE_DEPTH_WRITE
		);

		m_commandList->ResourceBarrier(1, &transitionBarrier);
	}

	const UINT numDSVs = sc.shadowMap->m_cubemap ? 6U : 1U;

	for (UINT i = 0U; i < numDSVs; ++i) {
		m_lightPassConstantBufferData[m_frameIndex][shadowMap->m_id + i] = { sc.projMatrix * sc.transform[i]->GetViewMatrix() };

		const auto lightPassOffset =
			m_lightPassConstantBufferData[m_frameIndex]->GetAlignedOffset(shadowMap->m_id + i);

		const D3D12_GPU_VIRTUAL_ADDRESS mainPassBaseOffset =
			m_mainPassConstantBufferResource[m_frameIndex]->GetGPUVirtualAddress() +
			m_mainPassConstantBufferData[m_frameIndex]->GetAlignedOffset(1);

		m_commandList->SetGraphicsRootConstantBufferView(
			RootParameterIndices::Light,
			mainPassBaseOffset + lightPassOffset);

		const auto dsvCPUDescriptorHandle = shadowMap->GetDSV(i);

		m_commandList->ClearDepthStencilView(dsvCPUDescriptorHandle,
			D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL,
			1.0f, 0UI8, 0U, nullptr);

		m_commandList->OMSetRenderTargets(0U, NULL, FALSE, &dsvCPUDescriptorHandle);
		//m_commandList->SetPipelineState(m_shadowMapPipelineState.Get());

		RenderAllModels();
	}

	{
		const auto transitionBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
			shadowMap->Resource().Get(),
			D3D12_RESOURCE_STATE_DEPTH_WRITE,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
		);

		m_commandList->ResourceBarrier(1, &transitionBarrier);
	}
}

void D3DClass::RenderAllModels(const std::vector<UINT>* shadowMapTextureIDs) {
	const bool renderToShadowMap = !static_cast<bool>(shadowMapTextureIDs);

	for (auto& model : m_models) {
		bool skip = false;
		for (const auto& name : g_bannedModelNames) {
			if (model.m_name == name) {
				skip = true;
				break;
			};

			if (renderToShadowMap && !model.m_castShadows) {
				skip = true;
				break;
			}
		}

		if (skip) continue;

		const auto pipeLineState = [&] {
			if (renderToShadowMap)
				return m_shadowMapPipelineState.Get();
			else {
				return model.m_receiveShadows ? m_defaultPipelineState.Get() : m_ReceiveNoShadowPipelineState.Get();
			}
		};

		m_commandList->SetPipelineState(pipeLineState());

		model.DrawModel(m_commandList,
			m_modelConstantBufferResource[m_frameIndex],
			m_materialConstantBufferResource[m_frameIndex],
			m_srvHeapGlobal,
			m_srvHeapDynamic[m_frameIndex],
			shadowMapTextureIDs);
	}
}

void D3DClass::WaitForGpu() {
	// Add a signal command to the command queue
	ThrowIfFailed(m_commandQueue->Signal(m_fence.Get(), m_fenceValues[m_frameIndex]));

	// Wait for the fence to pass
	ThrowIfFailed(m_fence->SetEventOnCompletion(m_fenceValues[m_frameIndex], m_fenceEvent));
	WaitForSingleObject(m_fenceEvent, INFINITE);

	// Increment the current frames fence value
	m_fenceValues[m_frameIndex]++;
}

void D3DClass::MoveToNextFrame() {
	// Add a signal command to the command queue
	const auto currentFenceValue = m_fenceValues[m_frameIndex];
	ThrowIfFailed(m_commandQueue->Signal(m_fence.Get(), currentFenceValue));

	//Update the frame index
	m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();

	// Wait for the previous frame to finish rendering
	if (m_fence->GetCompletedValue() < m_fenceValues[m_frameIndex]) {
		ThrowIfFailed(m_fence->SetEventOnCompletion(m_fenceValues[m_frameIndex], m_fenceEvent));
		WaitForSingleObject(m_fenceEvent, INFINITE);
	}

	// Set the fence value for the next frame
	m_fenceValues[m_frameIndex] = currentFenceValue + 1;
}

void D3DClass::UpdateMainPass() {
	
	// General information
	m_mainPassConstantBuffer.vpMat = m_camera->GetViewProjMatrix();
	m_mainPassConstantBuffer.eyePosition = m_camera->GetPosition();
	m_mainPassConstantBuffer.ambientLight = { 0.25f, 0.25f, 0.35f, 1.0f };

	m_mainPassConstantBuffer.directionalLightVpMat = m_directionalLight.projMatrix * m_directionalLight.transform[0]->GetViewMatrix();

	for (int i = 0; i < 6; ++i) {
		m_mainPassConstantBuffer.pointLightVpMats[i] = m_pointLight.transform[i]->GetViewProjMatrix();
	}
	
	// Directional Lights
	m_mainPassConstantBuffer.lights[0].Direction = { 
		m_directionalLight.transform[0]->GetForward().GetX(), 
		m_directionalLight.transform[0]->GetForward().GetY(),
		m_directionalLight.transform[0]->GetForward().GetZ() };
	m_mainPassConstantBuffer.lights[0].Strength = { 0.8f, 0.5f, 0.3f };
	m_mainPassConstantBuffer.lights[1].Direction = { -0.57735f, -0.57735f, 0.57735f };
	m_mainPassConstantBuffer.lights[1].Strength = { 0.3f, 0.2f, 0.1f };

	// Point light
	const auto& pos = m_camera->GetPosition();
	m_mainPassConstantBuffer.lights[2].Strength = { 0.609375f, 0.1640625f, 0.0f }; // Light red fire
	//m_mainPassConstantBuffer.lights[2].Strength = { 0.5f, 0.03515625f, 0.03515625f }; // Dark red fire
	m_mainPassConstantBuffer.lights[2].Position = { 
		m_pointLight.transform[0]->GetPosition().GetX(),
		m_pointLight.transform[0]->GetPosition().GetY(),
		m_pointLight.transform[0]->GetPosition().GetZ() };
	m_mainPassConstantBuffer.lights[2].FalloffStart = 0.0f;
	m_mainPassConstantBuffer.lights[2].FalloffEnd = 0.75f;


	// Spot Lights
	const auto& dir = m_camera->GetForward();
	m_mainPassConstantBuffer.lights[3].Strength = { 0.5f, 0.5f, 0.0f };
	m_mainPassConstantBuffer.lights[3].Direction = { dir.GetX(), dir.GetY(), dir.GetZ() };
	m_mainPassConstantBuffer.lights[3].Position = { pos.GetX(), pos.GetY(), pos.GetZ() };
	m_mainPassConstantBuffer.lights[3].FalloffStart = 0.2f;
	m_mainPassConstantBuffer.lights[3].FalloffEnd = 1.7f;
	m_mainPassConstantBuffer.lights[3].SpotPower = 5.0f;

	m_mainPassConstantBufferData[m_frameIndex][0] = m_mainPassConstantBuffer;

}

void D3DClass::LoadAssets() {
	{
		std::array<CD3DX12_DESCRIPTOR_RANGE1, 1> ranges{};
		ranges[0].Init(
			D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 
			MaterialClass::NUM_SRVS_PER_MATERIAL,
			0U, 
			0U, 
			D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE
		);

		std::array<CD3DX12_ROOT_PARAMETER1, RootParameterIndices::NUM_ROOTPARAMETERS> rootParameters{};
		rootParameters[RootParameterIndices::Textures].InitAsDescriptorTable(static_cast<UINT>(ranges.size()),ranges.data(), D3D12_SHADER_VISIBILITY_PIXEL); // Textures
		rootParameters[RootParameterIndices::Object].InitAsConstantBufferView(CBShaderRegister::Object); // Per object CB
		rootParameters[RootParameterIndices::Material].InitAsConstantBufferView(CBShaderRegister::Material); // Per material CB
		rootParameters[RootParameterIndices::Light].InitAsConstantBufferView(CBShaderRegister::Light); // Per light CB
		rootParameters[RootParameterIndices::MainPass].InitAsConstantBufferView(CBShaderRegister::MainPass); // Per pass CB

		std::array<CD3DX12_STATIC_SAMPLER_DESC, 2> samplers{
			CD3DX12_STATIC_SAMPLER_DESC(
				0, // shaderRegister
				D3D12_FILTER_COMPARISON_ANISOTROPIC, // filter
				D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
				D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
				D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressW
				0.0f,                             // mipLODBias
				8),                               // maxAnisotropy

			CD3DX12_STATIC_SAMPLER_DESC(
				1, // shaderRegister
				D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT, // filter
				D3D12_TEXTURE_ADDRESS_MODE_BORDER,  // addressU
				D3D12_TEXTURE_ADDRESS_MODE_BORDER,  // addressV
				D3D12_TEXTURE_ADDRESS_MODE_BORDER,  // addressW
				0.0f,                               // mipLODBias
				16,                                 // maxAnisotropy
				D3D12_COMPARISON_FUNC_LESS_EQUAL,
				D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK)
		};

		CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc{};
		rootSignatureDesc.Init_1_1(
			static_cast<UINT>(rootParameters.size()),
			rootParameters.data(), 
			static_cast<UINT>(samplers.size()), 
			samplers.data(),
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
		);

		ComPtr<ID3DBlob> signature;
		ComPtr<ID3DBlob> error;

		auto hrResult = D3D12SerializeVersionedRootSignature(&rootSignatureDesc, &signature, &error);
		
		if (error != nullptr){
			::OutputDebugStringA((char*)error->GetBufferPointer());
		}

		ThrowIfFailed(hrResult);

		ThrowIfFailed(
			m_device->CreateRootSignature(
				0, 
				signature->GetBufferPointer(), 
				signature->GetBufferSize(), 
				IID_PPV_ARGS(&m_rootSignature)
			)
		);
	}

	{
		D3D12_INPUT_ELEMENT_DESC inputElementDescs[] = {
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 16,  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 32,  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,	  0, 48, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
		};

//#if defined(_DEBUG)
//		UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
//#else
		UINT compileFlags = 0;
//#endif

		ComPtr<ID3DBlob>
			vertexShader,
			pixelShader,
			pixelShaderNoShadow,
			shadowMapVertexShader,
			shadowMapPixelShader,
			shaderError;

		constexpr D3D_SHADER_MACRO Macro_Receive_Shadows[2]{
			"RECEIVE_SHADOWS", "0", 
			NULL, NULL}; // Trailing NULL NULL as a 'closing' of the struct

		if (FAILED(D3DCompileFromFile(
			L"Shaders/PopotoVertexShader.hlsl",
			nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "main", "vs_5_0", compileFlags, 0U, &vertexShader, &shaderError))) {

			if (shaderError.Get())
			{
				OutputDebugStringA((char*)shaderError.Get()->GetBufferPointer());
				shaderError.Get()->Release();
			}
		}

		if (FAILED(D3DCompileFromFile(
			L"Shaders/PopotoPixelShader.hlsl",
			Macro_Receive_Shadows, D3D_COMPILE_STANDARD_FILE_INCLUDE, "main", "ps_5_0", compileFlags, 0U, &pixelShader, &shaderError))) {

			if (shaderError.Get())
			{
				OutputDebugStringA((char*)shaderError.Get()->GetBufferPointer());
				shaderError.Get()->Release();
			}
		}

		if (FAILED(D3DCompileFromFile(
			L"Shaders/PopotoPixelShader.hlsl",
			nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "main", "ps_5_0", compileFlags, 0U, &pixelShaderNoShadow, &shaderError))) {

			if (shaderError.Get())
			{
				OutputDebugStringA((char*)shaderError.Get()->GetBufferPointer());
				shaderError.Get()->Release();
			}
		}

		if (FAILED(D3DCompileFromFile(
			L"Shaders/ShadowMapVertexShader.hlsl",
			nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "main", "vs_5_0", compileFlags, 0U, &shadowMapVertexShader, &shaderError))) {

			if (shaderError.Get())
			{
				OutputDebugStringA((char*)shaderError.Get()->GetBufferPointer());
				shaderError.Get()->Release();
			}
		}

		if (FAILED(D3DCompileFromFile(
			L"Shaders/ShadowMapPixelShader.hlsl",
			nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "main", "ps_5_0", compileFlags, 0U, &shadowMapPixelShader, &shaderError))) {

			if (shaderError.Get())
			{
				OutputDebugStringA((char*)shaderError.Get()->GetBufferPointer());
				shaderError.Get()->Release();
			}
		}

		const CD3DX12_SHADER_BYTECODE VertexShader			{ vertexShader.Get()->GetBufferPointer(),			vertexShader.Get()->GetBufferSize() };
		const CD3DX12_SHADER_BYTECODE PixelShader			{ pixelShader.Get()->GetBufferPointer(),			pixelShader.Get()->GetBufferSize() };
		const CD3DX12_SHADER_BYTECODE PixelShaderNoShadow	{ pixelShaderNoShadow.Get()->GetBufferPointer(),	pixelShaderNoShadow.Get()->GetBufferSize() };
		const CD3DX12_SHADER_BYTECODE ShadowMapVertexShader	{ shadowMapVertexShader.Get()->GetBufferPointer(),	shadowMapVertexShader.Get()->GetBufferSize() };
		const CD3DX12_SHADER_BYTECODE ShadowMapPixelShader	{ shadowMapPixelShader.Get()->GetBufferPointer(),	shadowMapPixelShader.Get()->GetBufferSize() };

		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
		psoDesc.InputLayout = { inputElementDescs, std::extent_v<decltype(inputElementDescs)> };
		psoDesc.pRootSignature = m_rootSignature.Get();
		psoDesc.VS = VertexShader;
		psoDesc.PS = PixelShader;
		psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		//psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
		//psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
		psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
		psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
		psoDesc.SampleMask = UINT_MAX;
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		psoDesc.NumRenderTargets = 1;
		psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		psoDesc.SampleDesc.Count = 1;

		ThrowIfFailed(m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_defaultPipelineState)));

		D3D12_GRAPHICS_PIPELINE_STATE_DESC noShadowPsoDesc{ psoDesc };
		noShadowPsoDesc.PS = PixelShaderNoShadow;
		ThrowIfFailed(m_device->CreateGraphicsPipelineState(&noShadowPsoDesc, IID_PPV_ARGS(&m_ReceiveNoShadowPipelineState)));

		D3D12_GRAPHICS_PIPELINE_STATE_DESC shadowMapPsoDesc{ psoDesc };
		shadowMapPsoDesc.RasterizerState.DepthBias = 100;
		shadowMapPsoDesc.RasterizerState.DepthBiasClamp = 0.0f;
		shadowMapPsoDesc.RasterizerState.SlopeScaledDepthBias = 2.0f;
		shadowMapPsoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
		shadowMapPsoDesc.pRootSignature = m_rootSignature.Get();
		//shadowMapPsoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
		shadowMapPsoDesc.VS = ShadowMapVertexShader;
		shadowMapPsoDesc.PS = ShadowMapPixelShader;
		shadowMapPsoDesc.RTVFormats[0] = DXGI_FORMAT_UNKNOWN;
		shadowMapPsoDesc.NumRenderTargets = 0;
		ThrowIfFailed(m_device->CreateGraphicsPipelineState(&shadowMapPsoDesc, IID_PPV_ARGS(&m_shadowMapPipelineState)));
	}

	ThrowIfFailed(
		m_device->CreateCommandList(
			0,
			D3D12_COMMAND_LIST_TYPE_DIRECT,
			m_commandAllocators[m_frameIndex].Get(),
			nullptr,
			IID_PPV_ARGS(&m_commandList)
		)
	);

	m_commandList->SetName(L"DefaultCommandlist");
	
	const auto defaultHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	const auto uploadHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);

	// Create Depth/Stencil
	{
		auto viewport2DTex = CD3DX12_RESOURCE_DESC::Tex2D(
			DXGI_FORMAT_D32_FLOAT,
			static_cast<UINT64>(m_viewport.Width),
			static_cast<UINT64>(m_viewport.Height),
			1UI16,
			1UI16,
			1U,
			0U,
			D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL
		);

		D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
		dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
		dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
		dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
		dsvDesc.Texture2D.MipSlice = 0;

		D3D12_CLEAR_VALUE dsvClearVal{};
		dsvClearVal.Format = DXGI_FORMAT_D32_FLOAT;
		dsvClearVal.DepthStencil.Depth = 1.0f;
		dsvClearVal.DepthStencil.Stencil = 0ui8;

		ThrowIfFailed(
			m_device->CreateCommittedResource(
				&defaultHeapProperties,
				D3D12_HEAP_FLAG_NONE,
				&viewport2DTex,
				D3D12_RESOURCE_STATE_DEPTH_WRITE,
				&dsvClearVal,
				IID_PPV_ARGS(&m_dsvBuffer)
			)
		);
		NAME_D3D12_RES(m_dsvBuffer);

		m_device->CreateDepthStencilView(m_dsvBuffer.Get(), &dsvDesc, m_dsvHeap->GetCPUDescriptorHandleForHeapStart());
	}

	// Create ShadowMap for directional light
	{
		//const int mapResolution = 8192;
		const int mapResolution = 4096;
		//const int mapResolution = 2048;
		//const int mapResolution = 1024;

		const float
			nearDirLight = 0.1f,
			farDirLight = 2.0f,
			sizeDirLight = 2.25f;

		m_directionalLight.shadowMap = std::make_unique<ShadowMapClass>(m_device, mapResolution, mapResolution, FALSE);

		m_directionalLight.shadowMap->BuildDescriptors(
			m_srvHeapGlobal->GetCPUDescriptorHandleForHeapStart(),
			m_srvHeapGlobal->GetGPUDescriptorHandleForHeapStart(),
			m_dsvHeap->GetCPUDescriptorHandleForHeapStart(),
			m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV),
			m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV)
		);

		m_directionalLight.transform[0] = std::make_unique<CameraClass>(XM_PIDIV4, 1.0f, nearDirLight, farDirLight);
		m_directionalLight.transform[0]->SetDirection({ -0.649087f, -0.734546f, -0.197807f }, Vector3(kYUnitVector));
		m_directionalLight.transform[0]->SetPosition({ 0.934585f, 1.44004f, 0.317185f });

		m_directionalLight.transform[0]->SetDirection(Matrix3::MakeYRotation(0.0005f) * m_directionalLight.transform[0]->GetForward(), Vector3(kYUnitVector));
		m_directionalLight.transform[0]->SetPosition(-1.0f * 1.25f * m_directionalLight.transform[0]->GetForward());

		m_directionalLight.transform[0]->Update();

		m_directionalLight.projMatrix = Matrix4{ XMMatrixOrthographicRH(sizeDirLight, sizeDirLight, nearDirLight, farDirLight) };
	}

	// Create ShadowMap for pointlight
	{
		//const int mapResolution = 8192;
		//const int mapResolution = 4096;
		const int mapResolution = 2048;
		//const int mapResolution = 1024;

		const float
			nearPointLight = 0.01f,
			farPointLight = 1.0f;

		m_pointLight.shadowMap = std::make_unique<ShadowMapClass>(m_device, mapResolution, mapResolution, TRUE);
		m_pointLight.shadowMap->BuildDescriptors(
			m_srvHeapGlobal->GetCPUDescriptorHandleForHeapStart(),
			m_srvHeapGlobal->GetGPUDescriptorHandleForHeapStart(),
			m_dsvHeap->GetCPUDescriptorHandleForHeapStart(),
			m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV),
			m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV)
		);

		const std::array<Vector3, 6> directions {
			-Vector3(kXUnitVector), Vector3(kXUnitVector),
			Vector3(kYUnitVector), -Vector3(kYUnitVector),
			Vector3(kZUnitVector), -Vector3(kZUnitVector)
		};

		const std::array<Vector3, 6> ups{
			Vector3(kYUnitVector), Vector3(kYUnitVector),
			-Vector3(kZUnitVector), Vector3(kZUnitVector),
			Vector3(kYUnitVector), Vector3(kYUnitVector)
		};

		for (int i = 0; i < 6; ++i) {
			m_pointLight.transform[i] = std::make_unique<CameraClass>(Math::XMConvertToRadians(90.0f), 1.0f, nearPointLight, farPointLight);
			m_pointLight.transform[i]->SetDirection(directions[i], ups[i]);
			m_pointLight.transform[i]->SetPosition({ 0.406655f, -0.256986f, -0.0975294f });
			m_pointLight.transform[i]->Update();
		}

		m_pointLight.projMatrix = { m_pointLight.transform[0]->GetProjMatrix() };
	}

	{
		LoadScene("assets/sphere.obj");
		m_models[0].m_Transform.SetTranslation(m_pointLight.transform[0]->GetPosition());
		m_models[0].m_UniformScale = 0.01f;
		m_models[0].m_castShadows = false;
		m_models[0].m_receiveShadows = false;
	}
	//std::string assetPath("assets\\churchscene\\churchscene.obj");
	//std::string assetPath("assets/sponza.obj");
	//std::string assetPath("assets/rungholt/house.obj");
	//std::string assetPath("assets/chicken.obj");
	LoadScene("assets/sponza.obj");
	//LoadScene("assets/elemental/Elemental.obj");
	//LoadScene("assets/mchouse/house.obj");

	// Initialize the matrices and create the CBVs
	{
		const auto bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);
		const auto bufferRange = CD3DX12_RANGE{ 0,0 };

		for (UINT n = 0; n < FrameCount; n++) {
			{ // Model CBs
				ThrowIfFailed(
					m_device->CreateCommittedResource(
						&uploadHeapProperties,
						D3D12_HEAP_FLAG_NONE,
						&bufferDesc,
						D3D12_RESOURCE_STATE_GENERIC_READ,
						nullptr,
						IID_PPV_ARGS(&m_modelConstantBufferResource[n])
					)
				);
				NAME_D3D12_RES_INDEXED(m_modelConstantBufferResource, n);

				ThrowIfFailed(m_modelConstantBufferResource[n]->Map(0, &bufferRange, reinterpret_cast<void**>(&m_modelConstantBufferData[n])));
				
				for (auto& model : m_models) {
					if (m_modelConstantBufferData[n]->GetAlignedOffset(model.m_id) >= D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT) {
						throw std::exception("Too many models to fit in constant buffer");
					}
					
					m_modelConstantBufferData[n][model.m_id] = model.m_modelConstantBuffer;
				}
			}
			{// Main Pass/Light Pass CBs
				ThrowIfFailed(
					m_device->CreateCommittedResource(
						&uploadHeapProperties,
						D3D12_HEAP_FLAG_NONE,
						&bufferDesc,
						D3D12_RESOURCE_STATE_GENERIC_READ,
						nullptr,
						IID_PPV_ARGS(&m_mainPassConstantBufferResource[n])
					)
				);
				NAME_D3D12_RES_INDEXED(m_mainPassConstantBufferResource, n);

				ThrowIfFailed(m_mainPassConstantBufferResource[n]->Map(0, &bufferRange, reinterpret_cast<void**>(&m_mainPassConstantBufferData[n])));

				m_mainPassConstantBufferData[n][0] = m_mainPassConstantBuffer;
				m_lightPassConstantBufferData[n] = reinterpret_cast<Utility::PaddedBlock<LightPassConstantBuffer>*>(&m_mainPassConstantBufferData[n][1]);
			}
			{// Material CBs
				ThrowIfFailed(
					m_device->CreateCommittedResource(
						&uploadHeapProperties,
						D3D12_HEAP_FLAG_NONE,
						&bufferDesc,
						D3D12_RESOURCE_STATE_GENERIC_READ,
						nullptr,
						IID_PPV_ARGS(&m_materialConstantBufferResource[n])
					)
				);
				NAME_D3D12_RES_INDEXED(m_materialConstantBufferResource, n);

				ThrowIfFailed(m_materialConstantBufferResource[n]->Map(0, &bufferRange, reinterpret_cast<void**>(&m_materialConstantBufferData[n])));

				for (auto& model : m_models) {
					m_materialConstantBufferData[n][model.m_material.m_id] = model.m_material.m_materialConstantBuffer;
				}
			}
		}
	}

	// Close command list
	ThrowIfFailed(m_commandList->Close());

	// Execute the commandlist to begin GPU initialization
	ID3D12CommandList* ppCommandLists[] = { m_commandList.Get() };
	m_commandQueue->ExecuteCommandLists(std::extent_v<decltype(ppCommandLists)>, ppCommandLists);

	// Create synchronization objects and wait for assets to upload to the GPU
	{
		ThrowIfFailed(m_device->CreateFence(
			m_fenceValues[m_frameIndex],
			D3D12_FENCE_FLAG_NONE,
			IID_PPV_ARGS(&m_fence)
		));
		m_fenceValues[m_frameIndex]++;

		// Create event to use for frame synchornization
		m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
		if (m_fenceEvent == nullptr) {
			ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
		}

		// Wait for the command list to execute.
		WaitForGpu();
	}
}


#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

void D3DClass::LoadScene(std::string assetPath, bool invertTexY) {
	// Obtain directory of the file
	const auto found = assetPath.find_last_of("/\\");
	const auto workingDirectory = assetPath.substr(0, found).append("\\");

	using namespace Assimp;
	Importer assetLoader;
	assetLoader.SetPropertyBool(AI_CONFIG_PP_PTV_NORMALIZE, true);
	UINT assimpImportFlags = aiProcess_Triangulate | aiProcess_PreTransformVertices | aiProcess_CalcTangentSpace;
	assert(assetLoader.ValidateFlags(assimpImportFlags)); // Throw this shit if the flags don't work

	const aiScene* pScene = assetLoader.ReadFile(assetPath, assimpImportFlags);
	if (pScene == nullptr) {
		OutputDebugString(L"UNABLE TO LOAD ASSET\n");
	}
	assetPath.append(" has been loaded. \n");
	OutputDebugString(std::wstring(assetPath.begin(), assetPath.end()).c_str());

	const auto& aMeshes = pScene->mMeshes;
	const auto nMeshes = pScene->mNumMeshes;
	
	const auto modelOffset = m_models.size();
	m_models.resize(modelOffset + static_cast<size_t>(nMeshes));

	for (UINT i = 0; i < nMeshes; ++i) {

		auto& model = m_models[modelOffset + i];
		const auto& mesh = *aMeshes[i];

		model.m_name = { mesh.mName.C_Str() };

		model.m_mesh.m_indices.reserve(static_cast<size_t>(mesh.mNumFaces * 3));
		model.m_mesh.m_vertices.reserve(static_cast<size_t>(mesh.mNumVertices));

		for (UINT j = 0; j < mesh.mNumFaces; ++j) {
			const auto& face = mesh.mFaces[j];
			assert(!(face.mNumIndices % 3));

			model.m_mesh.m_indices.emplace_back(face.mIndices[1]);
			model.m_mesh.m_indices.emplace_back(face.mIndices[0]);
			model.m_mesh.m_indices.emplace_back(face.mIndices[2]);
		}

		for (UINT j = 0; j < mesh.mNumVertices; ++j) {
			if (!mesh.HasTangentsAndBitangents()) { 
				OutputDebugString(L"No Tangents and Bitangents found, skipping mesh.\n"); 
				continue;
			}
			const auto& vertex = mesh.mVertices[j];
			const auto& tangent = mesh.mTangents[j];
			const auto& normal = mesh.mNormals[j];
			const auto& texcoord = mesh.mTextureCoords[0][j];

			model.m_mesh.m_vertices.emplace_back(
				Vertex{
				{ vertex.x, vertex.y, vertex.z },
				{ normal.x, normal.y, normal.z },
				{ tangent.x, tangent.y, tangent.z },
				{ texcoord.x, invertTexY ? (1.0f - texcoord.y) : texcoord.y } });
		}

		model.ConstructBuffers(m_device, m_commandList);
		//model.m_UniformScale = 0.0005f;

		const auto& assimpMaterial = pScene->mMaterials[mesh.mMaterialIndex];

		const aiTextureType usedTextureTypes[]{
			aiTextureType_DIFFUSE,
			aiTextureType_HEIGHT,
			aiTextureType_SPECULAR
		};

		static_assert(std::extent_v<decltype(usedTextureTypes)> == MaterialClass::NUM_TEXTURES_PER_MATERIAL,
			"The amount of to load texture types does not equal the amount of textures per material.");

		for(auto j = 0; j < MaterialClass::NUM_TEXTURES_PER_MATERIAL; ++j){

			std::wstring wTexPath = L"assets\\default_normal.dds";

			if (assimpMaterial->GetTextureCount(usedTextureTypes[j])) {
				aiString aiTexturePath;
				assimpMaterial->GetTexture(usedTextureTypes[j], 0, &aiTexturePath);

				std::string texPath(aiTexturePath.C_Str());
				texPath.insert(0, workingDirectory);
				wTexPath = { texPath.begin(), texPath.end() };
			}
			else {
				switch (usedTextureTypes[j]) {
				case aiTextureType_DIFFUSE: {
					wTexPath = L"assets\\default_diffuse.dds";
					break;
				}
				case aiTextureType_HEIGHT: {
					wTexPath = L"assets\\default_normal.dds";
					break;
				}
				case aiTextureType_SPECULAR: {
					wTexPath = L"assets\\default_specular.dds";
					break;
				}
				default: {
					OutputDebugString(L"No Texture found, loading default.\n");
					break;
				}}
			}

			bool skipBecauseItAlreadyExists = false;

			for (auto& otherModel : m_models) {
				if (otherModel.m_id >= model.m_id) break;

				if (otherModel.m_material.m_textures[j]->m_fileName == wTexPath) {
					model.m_material.m_textures[j] = otherModel.m_material.m_textures[j];
					skipBecauseItAlreadyExists = true;
					break;
				}
			}

			if (!skipBecauseItAlreadyExists) {
				model.m_material.m_textures[j] = std::make_shared<MaterialClass::Texture>();
				model.m_material.m_textures[j]->Load(m_commandList, m_device, m_srvHeapGlobal, wTexPath.data());
			}
		}
	}
}