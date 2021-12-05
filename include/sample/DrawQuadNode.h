#include "RenderContext.h"
#include "Framegraph.h"
#include "ResourceManager.h"
#include "DescriptorAllocator.h"

namespace hvk
{
	namespace render
	{
		namespace sample
		{
			// shader
			// vertex buffer
			// render target texture
			// execution context
			// descriptor allocator

			class DrawQuadNode
			{
			public:
				DrawQuadNode(RenderContext& renderContext, ResourceManager& resourceManager);
				~DrawQuadNode();
				DrawQuadNode(const DrawQuadNode&) = delete;
				DrawQuadNode(DrawQuadNode&&) = delete;

				void Draw(RenderContext& renderContext, Framegraph& framegraph, DescriptorAllocator& descriptorAllocator);

				ResourceHandle GetQuadTexture() { return mRenderTarget; }

			private:
				std::shared_ptr<ExecutionContext> mExecutionCtx = nullptr;
				ComPtr<ID3D12RootSignature> mRootSig = nullptr;
				ComPtr<ID3D12PipelineState> mPso = nullptr;
				ResourceHandle mRenderTarget;
				ResourceHandle mVertexBuffer;
			};
		}
	}
}