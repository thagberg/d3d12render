#include "ResourceManager.h"

#include "Render.h"

#include <assert.h>

namespace hvk
{
	namespace render
	{
		void freeAllocator(D3D12MA::Allocator* allocator)
		{
			allocator->Release();
		}

		ResourceManager::ResourceManager(std::shared_ptr<RenderContext> renderContext)
			: mRenderContext(renderContext)
			, mAllocator(nullptr, freeAllocator)
			, mLastHandle(0)
			, mResources()
		{
			D3D12MA::ALLOCATOR_DESC allocDesc = {};
			allocDesc.pDevice = mRenderContext->GetDevice().Get();
			allocDesc.pAdapter = mRenderContext->GetHardwareAdapter().Get();

			D3D12MA::Allocator* allocator;
			HRESULT hr = D3D12MA::CreateAllocator(&allocDesc, &allocator);
			mAllocator.reset(allocator);

			mResources.reserve(kNumResources);
		}

		ResourceManager::~ResourceManager()
		{
			// allocator must be released first
			mAllocator.reset(nullptr);
		}

		ResourceHandle ResourceManager::CreateTexture(DXGI_FORMAT format, uint64_t width, uint64_t height, uint8_t arraySize, uint8_t numMips)
		{
			ResourceHandle newHandle = mLastHandle++;

			ResourceDescription newResource = {
				ResourceType::Texture,
				width,
				height,
				arraySize,
				numMips,
				format
			};
			mResources.insert({ newHandle, newResource });

			return newHandle;
		}

		ComPtr<ID3D12Resource> ResourceManager::ResolveResource(ResourceHandle handle, bool uavRequired)
		{
			auto foundResource = mResources.find(handle);
			if (foundResource != mResources.end())
			{
				// is this resource already resolved?
				if (foundResource->second.mResolvedResource == nullptr)
				{
					// TODO: need to analytically determine if resource requires unordered access flag
					//	possibly through parameter passed by framegraph?
					D3D12MA::ALLOCATION_DESC allocDesc = {};
					allocDesc.HeapType = D3D12_HEAP_TYPE_DEFAULT;

					D3D12_RESOURCE_DIMENSION dimension = foundResource->second.mType == ResourceType::Texture ? 
						D3D12_RESOURCE_DIMENSION_TEXTURE2D : D3D12_RESOURCE_DIMENSION_BUFFER;

					D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE;
					if (uavRequired)
					{
						flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
					}

					D3D12_RESOURCE_DESC resourceDesc = {};
					resourceDesc.Dimension = dimension;
					resourceDesc.Alignment = 0;
					resourceDesc.Width = foundResource->second.mWidth;
					resourceDesc.Height = foundResource->second.mHeight;
					resourceDesc.DepthOrArraySize = foundResource->second.mDepthOrArraySize;
					resourceDesc.MipLevels = foundResource->second.mMipLevels;
					resourceDesc.Format = foundResource->second.mFormat;
					resourceDesc.SampleDesc.Count = 1;
					resourceDesc.SampleDesc.Quality = 0;
					resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
					resourceDesc.Flags = flags;

					D3D12MA::Allocation* allocation;
					HRESULT hr = mAllocator->CreateResource(
						&allocDesc,
						&resourceDesc,
						D3D12_RESOURCE_STATE_COMMON,
						nullptr,
						&allocation,
						IID_PPV_ARGS(&foundResource->second.mResolvedResource)
					);
					assert(SUCCEEDED(hr));
				}
				
				return foundResource->second.mResolvedResource;
			}

			return nullptr;
		}

	}
}