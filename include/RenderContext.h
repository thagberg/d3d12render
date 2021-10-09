#pragma once

#include <wrl/client.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <memory>
#include <functional>

#include "D3D12MemAlloc.h"

using namespace Microsoft::WRL;

namespace hvk
{
	namespace render
	{
		void freeAllocator(D3D12MA::Allocator*);
		
		//using AllocatorPtr = std::unique_ptr<D3D12MA::Allocator, std::function<void(D3D12MA::Allocator*)>>;
		using AllocatorPtr = std::shared_ptr<D3D12MA::Allocator>;

		class RenderContext
		{
		public:
			RenderContext();
			~RenderContext();

			ComPtr<ID3D12Device5> GetDevice() { return mDevice; }
			ComPtr<IDXGIAdapter1> GetHardwareAdapter() { return mHardwareAdapter; }
			ComPtr<ID3D12CommandQueue> GetCommandQueue() { return mCommandQueue; }
			D3D12MA::Allocator& GetAllocator() { return *mAllocator; }
			ComPtr<ID3D12GraphicsCommandList4> CreateGraphicsCommandList();
		private:
			ComPtr<IDXGIFactory4> mFactory;
			ComPtr<IDXGIAdapter1> mHardwareAdapter;
			ComPtr<ID3D12Device5> mDevice;
			ComPtr<ID3D12CommandQueue> mCommandQueue;
			ComPtr<ID3D12CommandAllocator> mCommandAllocator;

			//AllocatorPtr mAllocator;
			//D3D12MA::Allocator mAllocator;
			D3D12MA::Allocator* mAllocator;
		};
	}
}
