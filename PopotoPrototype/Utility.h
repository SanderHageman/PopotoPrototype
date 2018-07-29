#pragma once

#include <sstream>

namespace Utility {
	
	namespace RootParameterIndices {
		enum : UINT {
			Textures,
			Object,
			Material,
			Light,
			MainPass,
			NUM_ROOTPARAMETERS
		};
	};

	namespace CBShaderRegister {
		enum : UINT {
			Object,
			Material,
			Light,
			MainPass
		};
	};

	// Access memory blocks as objects with N-byte alignment
	template<typename T, size_t alignment = 256, size_t Size = Math::AlignUp(sizeof(T), alignment)>
	struct PaddedBlock {
		PaddedBlock() = delete;
		PaddedBlock(const PaddedBlock&) = delete;
		PaddedBlock(PaddedBlock&&) = delete;
		
		template<typename U> PaddedBlock(U&& data) : data{ std::forward<U>(data) } {}

		PaddedBlock& operator=(const PaddedBlock& rhs) {
			this->data = rhs.data;
			return *this;
		}

		PaddedBlock& operator=(PaddedBlock&& rhs) {
			this->data = std::move(rhs.data);
			return *this;
		}

		static_assert(std::is_standard_layout_v<T>, "Block must be standard layout");
		static_assert(sizeof(T) <= Size, "PaddedBlock incorrectly Sized");

		constexpr size_t GetAlignedOffset(const size_t index) const { return Size * index; }
		T& getData() { return data; }
	private:
		union {
			T data;
			byte size_array[Size];
		};
	};

	inline void ThrowIfFailed(HRESULT hr)
	{
		if (FAILED(hr))
		{
			throw std::exception();
		}
	}

	// Assign a name to the resource to aid with debugging.
#if defined(_DEBUG)
	inline void SetName(ID3D12Resource* pResource, LPCWSTR name)
	{
		pResource->SetName(name);
	}
	inline void SetNameIndexed(ID3D12Resource* pResource, LPCWSTR name, UINT index)
	{
		WCHAR fullName[50];
		if (swprintf_s(fullName, L"%s[%u]", name, index) > 0)
		{
			pResource->SetName(fullName);
		}
	}
#else
	inline void SetName(ID3D12Object*, LPCWSTR)
	{
	}
	inline void SetNameIndexed(ID3D12Object*, LPCWSTR, UINT)
	{
	}
#endif
	// Naming helper for ComPtr<T>.
	// Assigns the name of the variable as the name of the object.
	// The indexed variant will include the index in the name of the object.
#define NAME_D3D12_RES(x) SetName(x.Get(), L#x)
#define NAME_D3D12_RES_INDEXED(x, n) SetNameIndexed(x[n].Get(), L#x, n)

	inline Microsoft::WRL::ComPtr<ID3D12Resource> CreateDefaultBuffer(
		ID3D12Device* device,
		ID3D12GraphicsCommandList* cmdList,
		const void* initData,
		UINT64 byteSize,
		Microsoft::WRL::ComPtr<ID3D12Resource>& uploadBuffer,
		D3D12_RESOURCE_STATES targetState)
	{
		const auto defaultHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
		const auto uploadHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);

		Microsoft::WRL::ComPtr<ID3D12Resource> defaultBuffer;
		const auto bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(byteSize);

		// Construct the defaultbuffer
		ThrowIfFailed(
			device->CreateCommittedResource(
				&defaultHeapProperties,
				D3D12_HEAP_FLAG_NONE,
				&bufferDesc,
				D3D12_RESOURCE_STATE_COPY_DEST,
				nullptr,
				IID_PPV_ARGS(&defaultBuffer)));
		NAME_D3D12_RES(defaultBuffer);

		// Construct the uploadbuffer
		ThrowIfFailed(
			device->CreateCommittedResource(
				&uploadHeapProperties,
				D3D12_HEAP_FLAG_NONE,
				&bufferDesc,
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr,
				IID_PPV_ARGS(&uploadBuffer)));
		NAME_D3D12_RES(uploadBuffer);

		// Create a description of the to be copied data
		D3D12_SUBRESOURCE_DATA subResourceData{};
		subResourceData.pData = initData;
		subResourceData.RowPitch = byteSize;
		subResourceData.SlicePitch = subResourceData.RowPitch;

		// Schedule a copy from the upload to the defaultheap
		const auto transitionBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
			defaultBuffer.Get(),
			D3D12_RESOURCE_STATE_COPY_DEST,
			targetState/*D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER*/);

		UpdateSubresources<1>(cmdList, defaultBuffer.Get(), uploadBuffer.Get(), 0, 0, 1, &subResourceData);
		cmdList->ResourceBarrier(1, &transitionBarrier);

		return defaultBuffer;
	}
} // namespace Utility