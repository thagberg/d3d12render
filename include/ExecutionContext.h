#include <d3d12.h>
#include <wrl/client.h>
#include <span>
#include <optional>
#include <functional>

#include <dxc/dxcapi.h>

using namespace Microsoft::WRL;

namespace hvk
{
	namespace render
	{
		using ShaderCodeRef = std::optional<std::reference_wrapper<std::span<uint8_t>>>;
		class ExecutionContext
		{
		public:
			ExecutionContext(ComPtr<ID3D12PipelineState> pipelineState, ComPtr<ID3D12RootSignature> rootSig, ComPtr<IDxcBlob> vs = nullptr, ComPtr<IDxcBlob> ps = nullptr);
			~ExecutionContext();

		private:
			ComPtr<ID3D12PipelineState> mPipelineState;
			ComPtr<ID3D12RootSignature> mRootSig;
			ComPtr<IDxcBlob> mVertexShader;
			ComPtr<IDxcBlob> mPixelShader;
		};
	}
}