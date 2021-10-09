#include "RenderContext.h"
#include "Render.h"

#include <exception>

#define THROW_IF_FAILED(hr) if(!SUCCEEDED(hr)) { throw std::exception();}

namespace hvk
{
	namespace render
	{
		RenderContext::RenderContext()
			: mFactory(nullptr)
			, mHardwareAdapter(nullptr)
			, mDevice(nullptr)
			, mCommandQueue(nullptr)
			, mCommandAllocator(nullptr)
			//, mAllocator(nullptr, freeAllocator)
			, mAllocator(nullptr)
		{
			HRESULT hr = S_OK;
			hr = hvk::render::CreateFactory(mFactory);
			THROW_IF_FAILED(hr)

			hvk::render::GetHardwareAdapter(mFactory.Get(), &mHardwareAdapter);

			hr = hvk::render::CreateDevice(mFactory, mHardwareAdapter, mDevice);
			THROW_IF_FAILED(hr)

			hr = hvk::render::CreateCommandQueue(mDevice, mCommandQueue);
			THROW_IF_FAILED(hr)

			hr = hvk::render::CreateCommandAllocator(mDevice, mCommandAllocator);
			THROW_IF_FAILED(hr)

			//D3D12MA::Allocator* allocator;
			D3D12MA::ALLOCATOR_DESC allocDesc = {};
			allocDesc.pDevice = mDevice.Get();
			allocDesc.pAdapter = mHardwareAdapter.Get();
			hr = D3D12MA::CreateAllocator(&allocDesc, &mAllocator);
			THROW_IF_FAILED(hr);
			//mAllocator.reset(allocator);
			//mAllocator = std::make_shared(allocator);
			//mAllocator.reset(allocator);
		}

		RenderContext::~RenderContext()
		{
			mAllocator->Release();
		}

		ComPtr<ID3D12GraphicsCommandList4> RenderContext::CreateGraphicsCommandList()
		{
			ComPtr<ID3D12GraphicsCommandList4> newCommandList;
			HRESULT hr = hvk::render::CreateCommandList(mDevice, mCommandAllocator, nullptr, newCommandList);
			assert(SUCCEEDED(hr));

			return newCommandList;
		}
	}
}