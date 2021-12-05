#pragma once

#include <unordered_map>
#include <functional>
#include <vector>

#include "ResourceManager.h"
#include "RenderContext.h"
#include "ExecutionContext.h"
#include "DescriptorAllocator.h"

namespace hvk
{
	namespace render
	{
		//using ResourceMapping = std::unordered_map<ResourceHandle, ComPtr<ID3D12Resource>>;
		using ResourceMapping = std::unordered_map<ResourceHandle, ResourceDescription&>;
		//using RenderPassCallback = std::function<void(
		//	const RenderContext& renderCtx, 
		//	const ResourceMapping& inputMap, 
		//	const ResourceMapping& outputMap)>;
		using RenderPassCallback = std::function<void(
			ComPtr<ID3D12GraphicsCommandList4> commandList, 
			const ExecutionContext& executionContext,
			const ResourceMapping& rtMap,
			const ResourceMapping& inputMap, 
			const ResourceMapping& outputMap)>;

		struct RenderNode
		{
			std::vector<ResourceHandle> mRenderTargets;
			std::vector<ResourceHandle> mInputs;
			std::vector<ResourceHandle> mOutputs;
			std::shared_ptr<ExecutionContext> mExecutionContext;
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
				std::shared_ptr<ExecutionContext> executionContext,
				RenderPassCallback);
			void EndFrame(DescriptorAllocator& descriptorAllocator);
			
		private:
			std::shared_ptr<RenderContext> mRenderContext;
			std::shared_ptr<ResourceManager> mResourceManager;
			std::vector<RenderNode> mNodes;
			bool mFrameStarted;
		};
	}
}
