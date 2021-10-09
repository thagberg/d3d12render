#pragma once

#include "ResourceManager.h"
#include "RenderContext.h"

#include <unordered_map>
#include <functional>
#include <vector>

namespace hvk
{
	namespace render
	{
		using ResourceMapping = std::unordered_map<ResourceHandle, ComPtr<ID3D12Resource>>;
		//using RenderPassCallback = std::function<void(
		//	const RenderContext& renderCtx, 
		//	const ResourceMapping& inputMap, 
		//	const ResourceMapping& outputMap)>;
		using RenderPassCallback = std::function<void(
			ComPtr<ID3D12GraphicsCommandList4> commandList, 
			const ResourceMapping& inputMap, 
			const ResourceMapping& outputMap)>;

		struct RenderNode
		{
			std::vector<ResourceHandle> mRenderTargets;
			std::vector<ResourceHandle> mInputs;
			std::vector<ResourceHandle> mOutputs;
			//const RenderPassCallback& mCallback;
			RenderPassCallback mCallback;
		};

		class Framegraph
		{
		public:
			Framegraph(std::shared_ptr<RenderContext> renderContext, std::shared_ptr<ResourceManager> resourceManager);
			~Framegraph();

			void BeginFrame();
			//void Insert(std::vector<ResourceHandle> passInputs, std::vector<ResourceHandle> passOutputs, const RenderPassCallback&);
			void Insert(
				std::vector<ResourceHandle> passRenderTargets,
				std::vector<ResourceHandle> passInputs, 
				std::vector<ResourceHandle> passOutputs, 
				const std::span<uint8_t>& vertexByteCode,
				const std::span<uint8_t>& pixelByteCode,
				const D3D12_INPUT_LAYOUT_DESC& inputLayout,
				RenderPassCallback);
			void EndFrame();
			
		private:
			std::shared_ptr<RenderContext> mRenderContext;
			std::shared_ptr<ResourceManager> mResourceManager;
			std::vector<RenderNode> mNodes;
			bool mFrameStarted;
		};
	}
}
