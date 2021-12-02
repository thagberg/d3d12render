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
			std::shared_ptr<ExecutionContext> executionContext,
			RenderPassCallback callback)
		{
			assert(mFrameStarted);
			mNodes.emplace_back(
				std::move(passRenderTargets), 
				std::move(passInputs), 
				std::move(passOutputs), 
				executionContext, 
				callback);
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

			// TEMP: use a fence to ensure resource copies have finished

			auto commandList = mRenderContext->CreateGraphicsCommandList();

			for (const auto& node : mNodes)
			{
				// prepare rendertargets
				ResourceMapping rtMap;
				for (const auto& rt : node.mRenderTargets)
				{
					auto resolvedRt = mResourceManager->ResolveResource(rt, false);
					assert(resolvedRt);
					rtMap.insert({ rt, *resolvedRt });
				}

				// prepare inputs
				ResourceMapping inputMap;
				for (const auto& input : node.mInputs)
				{
					// resolve resource
					auto resolvedInput = mResourceManager->ResolveResource(input, false);
					assert(resolvedInput);
					
					// add to input map
					inputMap.insert({ input, *resolvedInput });
				}

				ResourceMapping outputMap;
				for (const auto& output : node.mOutputs)
				{
					// resolve resource
					auto resolvedOutput = mResourceManager->ResolveResource(output, true);
					assert(resolvedOutput);

					// add to output map
					outputMap.insert({ output, *resolvedOutput });
				}
				
				for (const auto& mappedRT : rtMap)
				{
					if (mappedRT.second.mState != D3D12_RESOURCE_STATE_RENDER_TARGET) {
						D3D12_RESOURCE_BARRIER rtBarrier = {};
						rtBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
						rtBarrier.Transition.pResource = mappedRT.second.mResolvedResource.Get();
						rtBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
						rtBarrier.Transition.StateBefore = mappedRT.second.mState;
						rtBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
						commandList->ResourceBarrier(1, &rtBarrier);
						mappedRT.second.mState = D3D12_RESOURCE_STATE_RENDER_TARGET;
					}
				}

				// for each input
				//		add resource transition
				for (const auto& mappedInput : inputMap)
				{
					D3D12_RESOURCE_STATES stateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
					if (mappedInput.second.mType == ResourceType::VertexBuffer) {
						stateAfter = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
					}
					if (mappedInput.second.mState != stateAfter) {
						D3D12_RESOURCE_BARRIER inputBarrier = {};
						inputBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
						inputBarrier.Transition.pResource = mappedInput.second.mResolvedResource.Get();
						inputBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
						inputBarrier.Transition.StateBefore = mappedInput.second.mState;
						inputBarrier.Transition.StateAfter = stateAfter;
						commandList->ResourceBarrier(1, &inputBarrier);
						mappedInput.second.mState = stateAfter;
					}
				}

				// for each output
				//		add resource transition
				for (const auto& mappedOutput : outputMap)
				{
					if (mappedOutput.second.mState != D3D12_RESOURCE_STATE_UNORDERED_ACCESS) {
						D3D12_RESOURCE_BARRIER outputBarrier = {};
						outputBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
						outputBarrier.Transition.pResource = mappedOutput.second.mResolvedResource.Get();
						outputBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
						outputBarrier.Transition.StateBefore = mappedOutput.second.mState;
						outputBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
						commandList->ResourceBarrier(1, &outputBarrier);
						mappedOutput.second.mState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
					}
				}

				// call node callback
				node.mCallback(commandList, *node.mExecutionContext, rtMap, inputMap, outputMap);
				commandList->Close();
				ID3D12CommandList* commandLists[] = { commandList.Get() };
				mRenderContext->GetCommandQueue()->ExecuteCommandLists(1, commandLists);
			}

			mNodes.clear();
		}
	}
}