#pragma once

#include <atomic>
#include <memory>
#include <unordered_map>
#include <functional>
#include <span>
#include <list>

#include <wrl/client.h>

//#include "D3D12MemAlloc.h"
#include "RenderContext.h"

using namespace Microsoft::WRL;

namespace hvk
{
	namespace render
	{
		using ResourceHandle = uint64_t;

		constexpr ResourceHandle kInvalidHandle = 0;
		constexpr size_t kNumResources = 128;

		//using ResourceHandle = std::atomic<uint64_t>;

		using ResourceUploadCallback = std::function<void(
			std::span<uint8_t> mappedBuffer
		)>;

		enum class ResourceType
		{
			Texture = 0,
			ConstantBuffer,
			VertexBuffer,
			IndexBuffer,
			Count
		};

		struct ResourceDescription
		{
			ResourceType mType;
			uint64_t mWidth;
			uint64_t mHeight;
			uint16_t mDepthOrArraySize;
			uint16_t mMipLevels;
			DXGI_FORMAT mFormat;
			D3D12_RESOURCE_FLAGS mFlags;
			D3D12MA::Allocation* mAllocation = nullptr;
			ComPtr<ID3D12Resource> mResolvedResource = nullptr;
		};

		struct CopyDescription
		{
			ResourceHandle mHandle;
			ResourceHandle mCopyTo = kInvalidHandle;
			D3D12MA::Allocation* mUploadAllocation = nullptr;
			ComPtr<ID3D12Resource> mUploadResource = nullptr;
			bool mCompleted = false;
		};

		void freeAllocator(D3D12MA::Allocator*);

		class ResourceManager
		{
		public:
			ResourceManager(std::shared_ptr<RenderContext> renderContext);
			~ResourceManager();
			ResourceManager(const ResourceManager&) = delete;
			ResourceManager& operator=(const ResourceManager&) = delete;

			ResourceHandle CreateTexture(DXGI_FORMAT format, uint64_t width, uint64_t height, uint8_t arraySize, uint8_t numMips, D3D12_RESOURCE_FLAGS flags);
			//ResourceHandle CreateVertexBuffer(std::span<uint8_t> vertexData, uint32_t vertexStride);
			//ResourceHandle UploadResource(uint64_t resourceSize, )
			ResourceHandle CreateVertexBuffer(uint64_t size, ResourceUploadCallback);
			ComPtr<ID3D12Resource> ResolveResource(ResourceHandle handle, bool uavRequired);
			void PerformResourceCopies(ComPtr<ID3D12GraphicsCommandList4> commandList);
			void Cleanup();

		private:
			std::shared_ptr<RenderContext> mRenderContext;
			//std::unique_ptr<D3D12MA::Allocator, std::function<void(D3D12MA::Allocator*)>> mAllocator;
			std::atomic<ResourceHandle> mLastHandle;
			std::unordered_map<ResourceHandle, ResourceDescription> mResources;
			std::list<CopyDescription> mCopyResources;
		};
	}
}
