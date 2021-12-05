#include "sample/DrawQuadNode.h"

#include <d3d12.h>
#include <DirectXMath.h>

#include "Render.h"
#include "ShaderService.h"

namespace hvk
{
	namespace render
	{
		namespace sample
		{
			struct Vertex
			{
				DirectX::XMFLOAT3 pos;
				DirectX::XMFLOAT3 TexCoord;
			};

			const uint8_t kNumVertices = 6;
			const Vertex kVertices[kNumVertices] =
			{
				{DirectX::XMFLOAT3(-1.f, -1.f, 0.5f), DirectX::XMFLOAT3(0.f, 1.f, 0.f)},
				{DirectX::XMFLOAT3(-1.f, 1.f, 0.5f), DirectX::XMFLOAT3(1.f, 0.f, 0.f)},
				{DirectX::XMFLOAT3(1.f, -1.f, 0.5f), DirectX::XMFLOAT3(0.f, 0.f, 1.f)},
				{DirectX::XMFLOAT3(1.f, -1.f, 0.5f), DirectX::XMFLOAT3(1.f, 1.f, 0.f)},
				{DirectX::XMFLOAT3(-1.f, 1.f, 0.5f), DirectX::XMFLOAT3(0.f, 1.f, 1.f)},
				{DirectX::XMFLOAT3(1.f, 1.f, 0.5f), DirectX::XMFLOAT3(1.f, 0.f, 1.f)},

			};

			constexpr uint64_t kRenderWidth = 400;
			constexpr uint64_t kRenderHeight = 400;

			DrawQuadNode::DrawQuadNode(RenderContext& renderCtx, ResourceManager& resourceManager)
			{
				// auto create render target resource
				mRenderTarget = resourceManager.CreateTexture(
					DXGI_FORMAT_R8G8B8A8_UNORM,
					kRenderWidth,
					kRenderHeight,
					1,
					1,
					D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);

				// create VB for triangle
				mVertexBuffer = resourceManager.CreateVertexBuffer(sizeof(kVertices), [&](std::span<uint8_t> mappedBuffer) {
					memcpy(mappedBuffer.data(), kVertices, mappedBuffer.size_bytes());
				});

				std::shared_ptr<hvk::render::shader::ShaderService> shaderService = hvk::render::shader::ShaderService::Initialize();

				ComPtr<IDxcBlob> vertexByteCode;
				ComPtr<IDxcBlob> pixelByteCode;

				hvk::render::shader::ShaderDefinition vertexShaderDef{
					L"testshaders\\vertex.hlsl",
					L"main",
					L"vs_6_3"
				};
				hvk::render::shader::ShaderDefinition pixelShaderDef{
					L"testshaders\\pixel.hlsl",
					L"main",
					L"ps_6_3"
				};

				D3D12_INPUT_ELEMENT_DESC vertexInputs[] = {
					{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
					{"COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 3 * sizeof(float), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
				};
				D3D12_INPUT_LAYOUT_DESC vertexLayout{
					vertexInputs,
					_countof(vertexInputs)
				};

				auto hr = shaderService->CompileShader(vertexShaderDef, &vertexByteCode);
				hr = shaderService->CompileShader(pixelShaderDef, &pixelByteCode);

				// create root sig
				{
					hr = hvk::render::CreateRootSignature(renderCtx.GetDevice(), {}, {}, mRootSig);
				}

				// create PSO
				{
					hr = hvk::render::CreateGraphicsPipelineState(
						renderCtx.GetDevice(),
						vertexLayout,
						mRootSig,
						std::span<uint8_t>(reinterpret_cast<uint8_t*>(vertexByteCode->GetBufferPointer()), vertexByteCode->GetBufferSize()),
						std::span<uint8_t>(reinterpret_cast<uint8_t*>(pixelByteCode->GetBufferPointer()), pixelByteCode->GetBufferSize()),
						mPso	
					);

				}

				// create ExecutionContext
				mExecutionCtx = std::make_shared<hvk::render::ExecutionContext>(mPso, mRootSig, vertexByteCode, pixelByteCode);
			}

			DrawQuadNode::~DrawQuadNode()
			{

			}

			void DrawQuadNode::Draw(RenderContext& renderCtx, Framegraph& framegraph, DescriptorAllocator& descriptorAllocator)
			{
				framegraph.Insert({ mRenderTarget }, { mVertexBuffer }, {}, mExecutionCtx, [&](
					//const hvk::render::RenderContext& ctx,
					ComPtr<ID3D12GraphicsCommandList4> commandList,
					const hvk::render::ExecutionContext& context,
					const hvk::render::ResourceMapping& renderTargets,
					const hvk::render::ResourceMapping& inputs,
					const hvk::render::ResourceMapping& outputs) {

						D3D12_VIEWPORT viewport = {};
						viewport.Width = kRenderWidth;
						viewport.Height = kRenderHeight;
						viewport.TopLeftX = 0.f;
						viewport.TopLeftY = 0.f;
						viewport.MinDepth = 0.f;
						viewport.MaxDepth = 1.f;

						D3D12_RECT scissor = {};
						scissor.left = 0;
						scissor.right = kRenderWidth;
						scissor.top = 0;
						scissor.bottom = kRenderHeight;

						commandList->SetPipelineState(context.mPipelineState.Get());
						commandList->SetGraphicsRootSignature(context.mRootSig.Get());
						commandList->RSSetViewports(1, &viewport);
						commandList->RSSetScissorRects(1, &scissor);

						auto descriptorAllocation = descriptorAllocator.AllocDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 1);
						auto resolvedRT = renderTargets.at(mRenderTarget);
						renderCtx.GetDevice()->CreateRenderTargetView(resolvedRT.mResolvedResource.Get(), nullptr, descriptorAllocation.mPointer.mCpuHandle);
						const float clearColor[] = { 0.2f, 0.2f, 0.2f, 1.f };
						commandList->ClearRenderTargetView(descriptorAllocation.mPointer.mCpuHandle, clearColor, 0, nullptr);

						commandList->OMSetRenderTargets(1, &descriptorAllocation.mPointer.mCpuHandle, false, nullptr);

						//ComPtr<ID3D12Resource> swapchainRT;
						//swapchain->GetBuffer(frameIndex, IID_PPV_ARGS(&swapchainRT));
						//renderCtx->GetDevice()->CreateRenderTargetView(swapchainRT.Get(), nullptr, descriptorAllocation.mPointer.mCpuHandle);
						//commandList->OMSetRenderTargets(1, &descriptorAllocation.mPointer.mCpuHandle, false, nullptr);

						//{
						//	D3D12_RESOURCE_BARRIER backbuffer = {};
						//	backbuffer.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
						//	backbuffer.Transition.pResource = swapchainRT.Get();
						//	backbuffer.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
						//	backbuffer.Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;
						//	backbuffer.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
						//	commandList->ResourceBarrier(1, &backbuffer);
						//}

						//const float clearColor[] = { 0.2f, 0.2f, 0.2f, 1.f };
						//commandList->ClearRenderTargetView(descriptorAllocation.mPointer.mCpuHandle, clearColor, 0, nullptr);


						D3D12_VERTEX_BUFFER_VIEW vbView;
						vbView.BufferLocation = inputs.at(mVertexBuffer).mResolvedResource->GetGPUVirtualAddress();
						vbView.StrideInBytes = sizeof(DirectX::XMFLOAT3) * 2;
						vbView.SizeInBytes = sizeof(kVertices);
						commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
						commandList->IASetVertexBuffers(0, 1, &vbView);

						commandList->DrawInstanced(kNumVertices, 1, 0, 0);

						//{
						//	D3D12_RESOURCE_BARRIER present = {};
						//	present.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
						//	present.Transition.pResource = swapchainRT.Get();
						//	present.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
						//	present.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
						//	present.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
						//	commandList->ResourceBarrier(1, &present);
						//}

				});
			}
		}
	}
}