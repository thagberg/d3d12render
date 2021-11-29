#include <windows.h>
#include <tchar.h>
#include <iostream>
#include <DirectXMath.h>
#include <d3d12.h>

#include <Render.h>

#include "RenderContext.h"
#include "Framegraph.h"
#include "ShaderService.h"

WCHAR kWindowClass[] = L"FramegraphTest";
WCHAR kWindowTitle[] = L"FramegraphTest";
constexpr int kWindowWidth = 800;
constexpr int kWindowHeight = 600;

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

LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR lpCmdLine,
	_In_ int nCmdShow)
{
	{
		WNDCLASSEXW wcex;

		wcex.cbSize = sizeof(WNDCLASSEX);

		wcex.style          = CS_HREDRAW | CS_VREDRAW;
		wcex.lpfnWndProc    = WndProc;
		wcex.cbClsExtra     = 0;
		wcex.cbWndExtra     = 0;
		wcex.hInstance      = hInstance;
		wcex.hIcon			= NULL;
		wcex.hCursor		= NULL;
		wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
		wcex.lpszMenuName	= NULL;
		wcex.lpszClassName  = kWindowClass;
		wcex.hIconSm        = NULL;

		RegisterClassExW(&wcex);
	}

	HWND window = CreateWindowW(
		kWindowClass, 
		kWindowTitle, 
		WS_OVERLAPPEDWINDOW, 
		CW_USEDEFAULT, 
		CW_USEDEFAULT, 
		kWindowWidth, 
		kWindowHeight, 
		nullptr, 
		nullptr, 
		hInstance, 
		nullptr);

	if (!window)
	{
		return 1;
	}

	ShowWindow(window, nCmdShow);
	UpdateWindow(window);

	auto renderCtx = std::make_shared<hvk::render::RenderContext>();
	//hvk::render::ResourceManager resourceManager(renderCtx);
	auto resourceManager = std::make_shared<hvk::render::ResourceManager>(renderCtx);
	hvk::render::Framegraph framegraph(renderCtx, resourceManager);

	// auto create render target resource
	auto renderTarget = resourceManager->CreateTexture(
		DXGI_FORMAT_R8G8B8A8_UNORM,
		kWindowWidth,
		kWindowHeight,
		1,
		1,
		D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);

	// create VB for triangle
	auto vertexBuffer = resourceManager->CreateVertexBuffer(sizeof(kVertices), [&](std::span<uint8_t> mappedBuffer) {
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
	ComPtr<ID3D12RootSignature> rootSig;
	{
		hr = hvk::render::CreateRootSignature(renderCtx->GetDevice(), {}, {}, rootSig);
	}

	// create PSO
	ComPtr<ID3D12PipelineState> pso;
	{
		hr = hvk::render::CreateGraphicsPipelineState(
			renderCtx->GetDevice(),
			vertexLayout,
			rootSig,
			std::span<uint8_t>(reinterpret_cast<uint8_t*>(vertexByteCode->GetBufferPointer()), vertexByteCode->GetBufferSize()),
			std::span<uint8_t>(reinterpret_cast<uint8_t*>(pixelByteCode->GetBufferPointer()), pixelByteCode->GetBufferSize()),
			pso
		);

	}

	// create ExecutionContext
	auto executionContext = std::make_shared<hvk::render::ExecutionContext>(pso, rootSig, vertexByteCode, pixelByteCode);

	MSG msg;
	bool running = true;

	while (running)
	{
		while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			if (msg.message == WM_QUIT || msg.message == WM_CLOSE || msg.message == WM_DESTROY)
			{
				running = false;
			}
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		framegraph.BeginFrame();

		// insert pass
		framegraph.Insert({ renderTarget }, { vertexBuffer }, {}, executionContext, [&](
			//const hvk::render::RenderContext& ctx,
			ComPtr<ID3D12GraphicsCommandList4> commandList,
			const hvk::render::ExecutionContext& context,
			const hvk::render::ResourceMapping& inputs,
			const hvk::render::ResourceMapping& outputs) {

			std::cout << "In frame node callback" << std::endl;

		});

		framegraph.EndFrame();
	}

	return msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // Parse the menu selections:
            switch (wmId)
            {
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            // TODO: Add any drawing code that uses hdc here...
            EndPaint(hWnd, &ps);
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}
