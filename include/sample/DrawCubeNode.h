#include <DirectXMath.h>

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
			template <typename T>
			struct MappedBuffer
			{
				ComPtr<ID3D12Resource> mBufferResource = nullptr;
				T* mWritePtr = nullptr;
				D3D12MA::Allocation* mAllocation = nullptr;
			};

			struct ModelViewProjection
			{
				DirectX::XMMATRIX mWorld;
				DirectX::XMMATRIX mWorldView;
				DirectX::XMMATRIX mWorldViewProj;
			};

			class DrawCubeNode
			{
			public:
				DrawCubeNode(RenderContext& renderContext, ResourceManager& resourceManager);
				~DrawCubeNode();
				DrawCubeNode(const DrawCubeNode&) = delete;
				DrawCubeNode(DrawCubeNode&&) = delete;

				void Draw(RenderContext& renderContext, Framegraph& framegraph, DescriptorAllocator& descriptorAllocator, ResourceHandle quadHandle, ComPtr<IDXGISwapChain3> swapchain, uint8_t frameIndex);

			private:
				std::shared_ptr<ExecutionContext> mExecutionCtx = nullptr;
				ComPtr<ID3D12RootSignature> mRootSig = nullptr;
				ComPtr<ID3D12PipelineState> mPso = nullptr;
				ResourceHandle mVertexBuffer;
				ResourceHandle mIndexBuffer;
				//ResourceHandle mConstantBuffer;
				ModelViewProjection mTransforms;
				MappedBuffer<ModelViewProjection> mConstantBuffer;
			};
		}
	}
}