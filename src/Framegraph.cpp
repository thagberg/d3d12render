#include "Framegraph.h"

#include <assert.h>

#include <Render.h>

namespace hvk
{
	namespace render
	{
		constexpr size_t kDefaultNodes = 128;

		Framegraph::Framegraph(std::shared_ptr<RenderContext> renderContext, std::shared_ptr<ResourceManager> resourceManager)
			: mRenderContext(renderContext)
			, mResourceManager(resourceManager)
			, mNodes()
			, mFrameStarted(false)
		{
			assert(mRenderContext != nullptr);
			assert(mResourceManager != nullptr);
			mNodes.reserve(kDefaultNodes);
		}

		Framegraph::~Framegraph()
		{
			// TODO
			WaitForGraphics(mRenderContext->GetDevice(), mRenderContext->GetCommandQueue());
		}

		void Framegraph::BeginFrame()
		{
			assert(!mFrameStarted);
			assert(mNodes.size() == 0);

			mFrameStarted = true;

			WaitForGraphics(mRenderContext->GetDevice(), mRenderContext->GetCommandQueue());
			mResourceManager->Cleanup();
		}

		void Framegraph::Insert(
			std::vector<ResourceHandle> passRenderTargets,
			std::vector<ResourceHandle> passInputs,
			std::vector<ResourceHandle> passOutputs,
			const std::span<uint8_t>& vertexByteCode,
			const std::span<uint8_t>& pixelByteCode,
			const D3D12_INPUT_LAYOUT_DESC& inputLayout,
			RenderPassCallback callback)
		{
			assert(mFrameStarted);
			mNodes.emplace_back(std::move(passRenderTargets), std::move(passInputs), std::move(passOutputs), callback);
		}

		void Framegraph::EndFrame()
		{
			assert(mFrameStarted);
			mFrameStarted = false;

			auto commandQueue = mRenderContext->GetCommandQueue();

			auto copyList = mRenderContext->CreateGraphicsCommandList();
			mResourceManager->PerformResourceCopies(copyList);
			copyList->Close();
			ID3D12CommandList* commandLists[] = { copyList.Get() };
			commandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);

			auto commandList = mRenderContext->CreateGraphicsCommandList();

			for (const auto& node : mNodes)
			{
				// prepare inputs
				ResourceMapping inputMap;
				for (const auto& input : node.mInputs)
				{
					// resolve resource
					auto resolvedInput = mResourceManager->ResolveResource(input, false);
					
					// add to input map
					inputMap.insert({ input, resolvedInput });
				}

				ResourceMapping outputMap;
				for (const auto& output : node.mOutputs)
				{
					// resolve resource
					auto resolvedOutput = mResourceManager->ResolveResource(output, true);

					// add to output map
					outputMap.insert({ output, resolvedOutput });
				}

				// for each input
				//		add resource transition
				for (const auto& mappedInput : inputMap)
				{
					D3D12_RESOURCE_BARRIER inputBarrier = {};
					inputBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
					inputBarrier.Transition.pResource = mappedInput.second.Get();
					inputBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
					inputBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;
					inputBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
					commandList->ResourceBarrier(1, &inputBarrier);
				}

				// for each output
				//		add resource transition
				for (const auto& mappedOutput : outputMap)
				{
					D3D12_RESOURCE_BARRIER outputBarrier = {};
					outputBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
					outputBarrier.Transition.pResource = mappedOutput.second.Get();
					outputBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
					outputBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;
					outputBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
					commandList->ResourceBarrier(1, &outputBarrier);
				}

				// call node callback
				node.mCallback(commandList, inputMap, outputMap);
			}

			mNodes.clear();
		}
	}
}