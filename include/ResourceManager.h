#pragma once

#include <atomic>
#include <memory>
#include <unordered_map>

#include <wrl/client.h>

#include "D3D12MemAlloc.h"
#include "RenderContext.h"

using namespace Microsoft::WRL;

namespace hvk
{
	namespace render
	{
		constexpr size_t kNumResources = 128;

		//using ResourceHandle = std::atomic<uint64_t>;
		using ResourceHandle = uint64_t;

		enum class ResourceType
		{
			Texture = 0,
			Buffer,
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
			ComPtr<ID3D12Resource> mResolvedResource = nullptr;
		};

		void freeAllocator(D3D12MA::Allocator*);

		class ResourceManager
		{
		public:
			ResourceManager(std::shared_ptr<RenderContext> renderContext);
			~ResourceManager();
			ResourceManager(const ResourceManager&) = delete;
			ResourceManager& operator=(const ResourceManager&) = delete;

			ResourceHandle CreateTexture(DXGI_FORMAT format, uint64_t width, uint64_t height, uint8_t arraySize, uint8_t numMips);
			ComPtr<ID3D12Resource> ResolveResource(ResourceHandle handle, bool uavRequired);

		private:
			std::shared_ptr<RenderContext> mRenderContext;
			std::unique_ptr<D3D12MA::Allocator, void(*)(D3D12MA::Allocator*)> mAllocator;
			std::atomic<ResourceHandle> mLastHandle;
			std::unordered_map<ResourceHandle, ResourceDescription> mResources;
		};
	}
}
