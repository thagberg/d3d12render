#include "ExecutionContext.h"

namespace hvk
{
	namespace render
	{
		ExecutionContext::ExecutionContext(
			ComPtr<ID3D12PipelineState> pipelineState,
			ComPtr<ID3D12RootSignature> rootSig,
			ComPtr<IDxcBlob> vs,
			ComPtr<IDxcBlob> ps)
			: mPipelineState(pipelineState)
			, mRootSig(rootSig)
			, mVertexShader(vs)
			, mPixelShader(ps)
		{

		}

		ExecutionContext::~ExecutionContext()
		{

		}
	}
}