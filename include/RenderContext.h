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

			ComPtr<ID3D12Device5> GetDevice() const { return mDevice; }
			ComPtr<IDXGIAdapter1> GetHardwareAdapter() const { return mHardwareAdapter; }
			ComPtr<ID3D12CommandQueue> GetCommandQueue() const { return mCommandQueue; }
			D3D12MA::Allocator& GetAllocator() const { return *mAllocator; }
			ComPtr<ID3D12GraphicsCommandList4> CreateGraphicsCommandList() const;
			ComPtr<IDXGISwapChain3> CreateSwapchain(HWND window, uint8_t numFramebuffers, uint16_t width, uint16_t height) const;
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
