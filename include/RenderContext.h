#pragma once

#include <wrl/client.h>
#include <d3d12.h>
#include <dxgi1_6.h>

using namespace Microsoft::WRL;

namespace hvk
{
	namespace render
	{
		class RenderContext
		{
		public:
			RenderContext();
			~RenderContext();

			ComPtr<ID3D12Device5> GetDevice() { return mDevice; }
			ComPtr<IDXGIAdapter1> GetHardwareAdapter() { return mHardwareAdapter; }
		private:
			ComPtr<IDXGIFactory4> mFactory;
			ComPtr<IDXGIAdapter1> mHardwareAdapter;
			ComPtr<ID3D12Device5> mDevice;
			ComPtr<ID3D12CommandQueue> mCommandQueue;
		};
	}
}
