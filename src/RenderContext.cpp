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
		{
			HRESULT hr = S_OK;
			hr = hvk::render::CreateFactory(mFactory);
			THROW_IF_FAILED(hr)

			hvk::render::GetHardwareAdapter(mFactory.Get(), &mHardwareAdapter);

			hr = hvk::render::CreateDevice(mFactory, mHardwareAdapter, mDevice);
			THROW_IF_FAILED(hr)

			hr = hvk::render::CreateCommandQueue(mDevice, mCommandQueue);
			THROW_IF_FAILED(hr)
		}

		RenderContext::~RenderContext()
		{

		}
	}
}