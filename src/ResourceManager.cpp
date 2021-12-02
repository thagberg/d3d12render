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
			//, mAllocator(nullptr, freeAllocator)
			, mLastHandle(kInvalidHandle + 1)
			, mResources()
		{
			//D3D12MA::ALLOCATOR_DESC allocDesc = {};
			//allocDesc.pDevice = mRenderContext->GetDevice().Get();
			//allocDesc.pAdapter = mRenderContext->GetHardwareAdapter().Get();

			//D3D12MA::Allocator* allocator;
			//HRESULT hr = D3D12MA::CreateAllocator(&allocDesc, &allocator);
			//mAllocator.reset(allocator);

			mResources.reserve(kNumResources);
		}

		ResourceManager::~ResourceManager()
		{
			for (auto resource : mResources)
			{
				resource.second.mAllocation->Release();
			}

			for (auto resource : mCopyResources)
			{
				resource.mUploadAllocation->Release();
			}
		}

		ResourceHandle ResourceManager::CreateTexture(
			DXGI_FORMAT format, 
			uint64_t width, 
			uint64_t height, 
			uint8_t arraySize, 
			uint8_t numMips, 
			D3D12_RESOURCE_FLAGS flags)
		{
			ResourceHandle newHandle = mLastHandle++;

			ResourceDescription newResource = {
				ResourceType::Texture,
				width,
				height,
				arraySize,
				numMips,
				format,
				flags
			};
			mResources.insert({ newHandle, newResource });

			return newHandle;
		}

		ResourceHandle ResourceManager::CreateVertexBuffer(uint64_t size, ResourceUploadCallback uploadCallback)
		{
			ResourceHandle newHandle = mLastHandle++;

			D3D12MA::ALLOCATION_DESC allocDesc = {};
			allocDesc.HeapType = D3D12_HEAP_TYPE_UPLOAD;

			D3D12_RESOURCE_DESC uploadDesc = {};
			uploadDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
			uploadDesc.Alignment = 0;
			uploadDesc.Width = size;
			uploadDesc.Height = 1;
			uploadDesc.DepthOrArraySize = 1;
			uploadDesc.MipLevels = 1;
			uploadDesc.Format = DXGI_FORMAT_UNKNOWN;
			uploadDesc.SampleDesc.Count = 1;
			uploadDesc.SampleDesc.Quality = 0;
			uploadDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
			uploadDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

			CopyDescription toCopy = {};
			toCopy.mType = ResourceType::VertexBuffer;
			toCopy.mHandle = newHandle;

			HRESULT hr = mRenderContext->GetAllocator().CreateResource(
				&allocDesc,
				&uploadDesc,
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr,
				&toCopy.mUploadAllocation,
				IID_PPV_ARGS(&toCopy.mUploadResource)
			);
			assert(SUCCEEDED(hr));

			D3D12_RANGE readRange = {};
			readRange.Begin = 0;
			readRange.End = 0;

			uint8_t* vbBegin;
			toCopy.mUploadResource->Map(0, &readRange, reinterpret_cast<void**>(&vbBegin));

			std::span<uint8_t> mappedData{ vbBegin, size };
			uploadCallback(mappedData);

			toCopy.mUploadResource->Unmap(0, nullptr);

			mCopyResources.push_back(toCopy);

			return newHandle;
		}

		ResourceDescription* ResourceManager::ResolveResource(ResourceHandle handle, bool uavRequired)
		{
			auto foundResource = mResources.find(handle);
			assert(foundResource != mResources.end());
			if (foundResource != mResources.end())
			{
				// is this resource already resolved?
				if (foundResource->second.mResolvedResource == nullptr)
				{
					// TODO: need to analytically determine if resource requires unordered access flag
					//	possibly through parameter passed by framegraph?
					D3D12MA::ALLOCATION_DESC allocDesc = {};
					allocDesc.HeapType = D3D12_HEAP_TYPE_DEFAULT;

					const bool isTexture = foundResource->second.mType == ResourceType::Texture;

					D3D12_RESOURCE_DIMENSION dimension = isTexture ? 
						D3D12_RESOURCE_DIMENSION_TEXTURE2D : D3D12_RESOURCE_DIMENSION_BUFFER;

					D3D12_RESOURCE_FLAGS flags = foundResource->second.mFlags;
					if (uavRequired)
					{
						flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
					}

					D3D12_TEXTURE_LAYOUT layout = isTexture ? D3D12_TEXTURE_LAYOUT_UNKNOWN : D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

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
					resourceDesc.Layout = layout;
					resourceDesc.Flags = flags;

					//D3D12MA::Allocation* allocation;
					HRESULT hr = mRenderContext->GetAllocator().CreateResource(
						&allocDesc,
						&resourceDesc,
						D3D12_RESOURCE_STATE_COMMON,
						nullptr,
						//&allocation,
						&foundResource->second.mAllocation,
						IID_PPV_ARGS(&foundResource->second.mResolvedResource)
					);
					assert(SUCCEEDED(hr));
				}
				
				return &foundResource->second;
			}

			return nullptr;
		}

		void ResourceManager::PerformResourceCopies(ComPtr<ID3D12GraphicsCommandList4> commandList)
		{
			std::vector<D3D12_RESOURCE_BARRIER> postCopyBarriers;
			postCopyBarriers.reserve(mCopyResources.size());
			for (auto& copyDesc : mCopyResources)
			{
				assert(copyDesc.mHandle != kInvalidHandle);
				assert(copyDesc.mUploadResource != nullptr);

				// if no copy dest defined, this is probably an upload buffer
				// being copied to a new default heap resource, so create it now
				if (copyDesc.mCopyTo == kInvalidHandle)
				{
					auto resourceDesc = copyDesc.mUploadResource->GetDesc();

					ResourceDescription newResource = {
						copyDesc.mType,
						resourceDesc.Width,
						resourceDesc.Height,
						resourceDesc.DepthOrArraySize,
						resourceDesc.MipLevels,
						resourceDesc.Format,
						resourceDesc.Flags	
					};

					D3D12MA::ALLOCATION_DESC allocDesc = {};
					allocDesc.HeapType = D3D12_HEAP_TYPE_DEFAULT;

					//D3D12MA::Allocation* allocation;
					HRESULT hr = mRenderContext->GetAllocator().CreateResource(
						&allocDesc,
						&resourceDesc,
						D3D12_RESOURCE_STATE_COPY_DEST,
						nullptr,
						//&allocation,
						&newResource.mAllocation,
						IID_PPV_ARGS(&newResource.mResolvedResource)
					);

					commandList->CopyResource(newResource.mResolvedResource.Get(), copyDesc.mUploadResource.Get());
					copyDesc.mCompleted = true;

					D3D12_RESOURCE_BARRIER postCopy = {};
					postCopy.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
					postCopy.Transition.pResource = newResource.mResolvedResource.Get();
					postCopy.Transition.Subresource = 0;
					postCopy.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
					postCopy.Transition.StateAfter = D3D12_RESOURCE_STATE_COMMON;

					postCopyBarriers.push_back(postCopy);

					mResources.insert({ copyDesc.mHandle, newResource });
				}
			}

			commandList->ResourceBarrier(postCopyBarriers.size(), postCopyBarriers.data());
		}

		void ResourceManager::Cleanup()
		{
			auto it = mCopyResources.cbegin();
			while (it != mCopyResources.cend())
			{
				if (it->mCompleted)
				{
					it->mUploadAllocation->Release();
					auto deleteThis = it;
					++it;

					mCopyResources.erase(deleteThis);
				}
				else
				{
					++it;
				}
			}
		}
	}
}