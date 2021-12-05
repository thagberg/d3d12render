#include "sample/DrawCubeNode.h"

#include <corecrt_math_defines.h>

#include "ShaderService.h"

namespace hvk
{
	namespace render
	{
		namespace sample
		{
			struct CubeVertex
			{
				DirectX::XMFLOAT3 pos;
				DirectX::XMFLOAT2 uv;
			};

			/*
			    G			E
				  --------
				 /|     /|
			  B -------- |  D
		    F	| |    | |
				|/ ----|/     H
			  A --------   C
			*/
			const CubeVertex kVertices[] = {
				{DirectX::XMFLOAT3(-1.f, -1.f, 0.5f), DirectX::XMFLOAT2(0.f, 0.f)}, // A
				{DirectX::XMFLOAT3(-1.f, 1.f, 0.5f), DirectX::XMFLOAT2(0.f, 0.f)}, // B
				{DirectX::XMFLOAT3(1.f, -1.f, 0.5f), DirectX::XMFLOAT2(0.f, 0.f)}, // C
				{DirectX::XMFLOAT3(1.f, 1.f, 0.5f), DirectX::XMFLOAT2(0.f, 0.f)}, // D

				{DirectX::XMFLOAT3(1.f, 1.f, 2.5f), DirectX::XMFLOAT2(0.f, 0.f)}, // E
				{DirectX::XMFLOAT3(-1.f, -1.f, 2.5f), DirectX::XMFLOAT2(0.f, 0.f)}, // F
				{DirectX::XMFLOAT3(-1.f, 1.f, 2.5f), DirectX::XMFLOAT2(0.f, 0.f)}, // G
				{DirectX::XMFLOAT3(1.f, -1.f, 2.5f), DirectX::XMFLOAT2(0.f, 0.f)}, // H
			};

			const uint16_t kIndices[] = {
				0, 1, 2, // front face
				2, 1, 3,

				3, 7, 2, // right face
				7, 3, 4,

				4, 5, 7, // back face
				5, 4, 6,

				6, 0, 5, // left face
				0, 6, 1,

				1, 6, 3, // top face
				3, 6, 4,

				0, 7, 5, // bottom face
				7, 0, 2
			};

			constexpr uint64_t kRenderWidth = 1000;
			constexpr uint64_t kRenderHeight = 1000;

			DrawCubeNode::DrawCubeNode(RenderContext& renderContext, ResourceManager& resourceManager)
				: mTransforms()
			{
				mVertexBuffer = resourceManager.CreateVertexBuffer(sizeof(kVertices) * sizeof(CubeVertex), [&](std::span<uint8_t> mappedBuffer) {
					memcpy(mappedBuffer.data(), kVertices, sizeof(kVertices));
				});

				mIndexBuffer = resourceManager.CreateIndexBuffer(kIndices);

				// create camera
				auto view = DirectX::XMMatrixLookAtRH(
					DirectX::FXMVECTOR{ 0.f, 0.f, -0.5f, 1.f },  // position
					DirectX::FXMVECTOR{ 0.f, 0.f, 1.f, 1.f },  // focus
					DirectX::FXMVECTOR{ 0.f, 1.f, 0.f, 1.f }); // up
				auto proj = DirectX::XMMatrixPerspectiveFovRH(90.f * M_PI / 180.f, 1.f, 0.1f, 10.f);

				// create cube transform
				auto cubeWorld = DirectX::XMMatrixMultiply(
					DirectX::XMMatrixIdentity(), 
					DirectX::XMMatrixTranslation(0.f, 0.f, 2.f));

				mTransforms.mWorld = cubeWorld;
				mTransforms.mWorldView = DirectX::XMMatrixMultiply(cubeWorld, view);
				mTransforms.mWorldViewProj = DirectX::XMMatrixMultiply(mTransforms.mWorldView, proj);

				// create constant buffer
				{
					D3D12MA::ALLOCATION_DESC cbAllocDesc = {};
					cbAllocDesc.HeapType = D3D12_HEAP_TYPE_UPLOAD;

					D3D12_RESOURCE_DESC cbDesc = {};
					cbDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
					cbDesc.Alignment = 0;
					cbDesc.Width = Align(sizeof(ModelViewProjection), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
					cbDesc.Height = 1;
					cbDesc.DepthOrArraySize = 1;
					cbDesc.MipLevels = 1;
					cbDesc.Format = DXGI_FORMAT_UNKNOWN;
					cbDesc.SampleDesc.Count = 1;
					cbDesc.SampleDesc.Quality = 0;
					cbDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
					cbDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

					HRESULT hr = renderContext.GetAllocator().CreateResource(
						&cbAllocDesc,
						&cbDesc,
						D3D12_RESOURCE_STATE_GENERIC_READ,
						nullptr,
						&mConstantBuffer.mAllocation,
						IID_PPV_ARGS(&mConstantBuffer.mBufferResource)
					);
					assert(SUCCEEDED(hr));

					D3D12_RANGE readRange = { 0, 0 };
					mConstantBuffer.mBufferResource->Map(0, &readRange, reinterpret_cast<void**>(&mConstantBuffer.mWritePtr));
				}

				std::shared_ptr<hvk::render::shader::ShaderService> shaderService = hvk::render::shader::ShaderService::Initialize();

				ComPtr<IDxcBlob> vertexByteCode;
				ComPtr<IDxcBlob> pixelByteCode;

				hvk::render::shader::ShaderDefinition vertexShaderDef{
					L"testshaders\\cubeVertex.hlsl",
					L"main",
					L"vs_6_3"
				};
				hvk::render::shader::ShaderDefinition pixelShaderDef{
					L"testshaders\\cubePixel.hlsl",
					L"main",
					L"ps_6_3"
				};

				DXGI_FORMAT_R8G8_UNORM;
				D3D12_INPUT_ELEMENT_DESC vertexInputs[] = {
					{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
					{"TEXCOORD", 0, DXGI_FORMAT_R16G16_UNORM, 0, 3 * sizeof(float), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
				};
				D3D12_INPUT_LAYOUT_DESC vertexLayout{
					vertexInputs,
					_countof(vertexInputs)
				};

				auto hr = shaderService->CompileShader(vertexShaderDef, &vertexByteCode);
				hr = shaderService->CompileShader(pixelShaderDef, &pixelByteCode);

				// root sig
				{
					D3D12_DESCRIPTOR_RANGE cbvRange = {};
					cbvRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
					cbvRange.NumDescriptors = 1;
					cbvRange.BaseShaderRegister = 0;
					cbvRange.RegisterSpace = 0;
					cbvRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

					D3D12_DESCRIPTOR_RANGE srvRange = {};
					srvRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
					srvRange.NumDescriptors = 1;
					srvRange.BaseShaderRegister = 0;
					srvRange.RegisterSpace = 0;
					srvRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

					D3D12_DESCRIPTOR_RANGE ranges[] = { cbvRange, srvRange };

					D3D12_STATIC_SAMPLER_DESC bilinearSampler = {};
					bilinearSampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
					bilinearSampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
					bilinearSampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
					bilinearSampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
					bilinearSampler.MipLODBias = 0.f;
					bilinearSampler.MaxAnisotropy = 1;
					bilinearSampler.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
					bilinearSampler.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK;
					bilinearSampler.MinLOD = 0;
					bilinearSampler.MaxLOD = 0;
					bilinearSampler.ShaderRegister = 0;
					bilinearSampler.RegisterSpace = 0;
					bilinearSampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

					D3D12_ROOT_PARAMETER param = {};
					param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
					param.DescriptorTable.NumDescriptorRanges = _countof(ranges);
					param.DescriptorTable.pDescriptorRanges = ranges;
					param.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

					hr = hvk::render::CreateRootSignature(renderContext.GetDevice(), {param}, {bilinearSampler}, mRootSig);
				}

				// PSO
				{
					hr = hvk::render::CreateGraphicsPipelineState(
						renderContext.GetDevice(),
						vertexLayout,
						mRootSig,
						std::span<uint8_t>(reinterpret_cast<uint8_t*>(vertexByteCode->GetBufferPointer()), vertexByteCode->GetBufferSize()),
						std::span<uint8_t>(reinterpret_cast<uint8_t*>(pixelByteCode->GetBufferPointer()), pixelByteCode->GetBufferSize()),
						mPso
					);
				}

				mExecutionCtx = std::make_shared<hvk::render::ExecutionContext>(mPso, mRootSig, vertexByteCode, pixelByteCode);
			}

			DrawCubeNode::~DrawCubeNode()
			{
				mConstantBuffer.mBufferResource->Unmap(0, nullptr);
				mConstantBuffer.mAllocation->Release();
			}

			void DrawCubeNode::Draw(
				RenderContext& renderCtx,
				Framegraph& framegraph, 
				DescriptorAllocator& descriptorAllocator, 
				ResourceHandle quadHandle, 
				ComPtr<IDXGISwapChain3> swapchain,
				uint8_t frameIndex)
			{
				// update constants
				memcpy(mConstantBuffer.mWritePtr, &mTransforms, sizeof(mTransforms));

				framegraph.Insert({}, { mVertexBuffer, mIndexBuffer, quadHandle }, {}, mExecutionCtx, [&, swapchain](
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
						auto miscDescriptorAllocation = descriptorAllocator.AllocDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 2);

						ComPtr<ID3D12Resource> swapchainRT;
						swapchain->GetBuffer(frameIndex, IID_PPV_ARGS(&swapchainRT));
						renderCtx.GetDevice()->CreateRenderTargetView(swapchainRT.Get(), nullptr, descriptorAllocation.mPointer.mCpuHandle);
						commandList->OMSetRenderTargets(1, &descriptorAllocation.mPointer.mCpuHandle, false, nullptr);

						{
							D3D12_RESOURCE_BARRIER backbuffer = {};
							backbuffer.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
							backbuffer.Transition.pResource = swapchainRT.Get();
							backbuffer.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
							backbuffer.Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;
							backbuffer.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
							commandList->ResourceBarrier(1, &backbuffer);
						}

						const float clearColor[] = { 0.2f, 0.2f, 0.2f, 1.f };
						commandList->ClearRenderTargetView(descriptorAllocation.mPointer.mCpuHandle, clearColor, 0, nullptr);

						const auto cbDesc = mConstantBuffer.mBufferResource->GetDesc();
						const auto srDesc = inputs.at(quadHandle).mResolvedResource->GetDesc();

						auto descriptorTableHandle = miscDescriptorAllocation.mPointer.mCpuHandle;

						//D3D12_SHADER_RESOURCE_VIEW_DESC cbvViewDesc = {};
						//cbvViewDesc.Format = cbDesc.Format;
						//cbvViewDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
						//cbvViewDesc.Buffer.FirstElement = 0;
						//cbvViewDesc.Buffer.NumElements = 1;
						//cbvViewDesc.Buffer.StructureByteStride = sizeof(ModelViewProjection);
						//renderCtx.GetDevice()->CreateShaderResourceView(mConstantBuffer.mBufferResource.Get(), &cbvViewDesc, descriptorTableHandle);
						//descriptorTableHandle.ptr += renderCtx.GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
						D3D12_CONSTANT_BUFFER_VIEW_DESC cbViewDesc = {};
						cbViewDesc.BufferLocation = mConstantBuffer.mBufferResource->GetGPUVirtualAddress();
						cbViewDesc.SizeInBytes = Align(sizeof(ModelViewProjection), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
						renderCtx.GetDevice()->CreateConstantBufferView(&cbViewDesc, descriptorTableHandle);
						descriptorTableHandle.ptr += renderCtx.GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

						D3D12_SHADER_RESOURCE_VIEW_DESC srViewDesc = {};
						srViewDesc.Format = srDesc.Format;
						srViewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
						srViewDesc.Texture2D.MipLevels = 1;
						srViewDesc.Texture2D.MostDetailedMip = 0;
						srViewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
						renderCtx.GetDevice()->CreateShaderResourceView(inputs.at(quadHandle).mResolvedResource.Get(), &srViewDesc, descriptorTableHandle);

						commandList->SetGraphicsRootDescriptorTable(0, miscDescriptorAllocation.mPointer.mGpuHandle);

						D3D12_VERTEX_BUFFER_VIEW vbView;
						vbView.BufferLocation = inputs.at(mVertexBuffer).mResolvedResource->GetGPUVirtualAddress();
						vbView.StrideInBytes = sizeof(CubeVertex);
						vbView.SizeInBytes = sizeof(kVertices);
						commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
						commandList->IASetVertexBuffers(0, 1, &vbView);

						D3D12_INDEX_BUFFER_VIEW ibView = {};
						ibView.BufferLocation = inputs.at(mIndexBuffer).mResolvedResource->GetGPUVirtualAddress();
						ibView.SizeInBytes = sizeof(kIndices);
						ibView.Format = DXGI_FORMAT_R16_UINT;
						commandList->IASetIndexBuffer(&ibView);

						commandList->DrawIndexedInstanced(_countof(kIndices), 1, 0, 0, 0);

						{
							D3D12_RESOURCE_BARRIER present = {};
							present.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
							present.Transition.pResource = swapchainRT.Get();
							present.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
							present.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
							present.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
							commandList->ResourceBarrier(1, &present);
						}
				});
			}
		}
	}
}