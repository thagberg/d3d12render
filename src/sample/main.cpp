#define USE_PIX

#include <windows.h>
#include <tchar.h>
#include <iostream>
#include <DirectXMath.h>
#include <d3d12.h>

#include <Render.h>

#include "WinPixEventRuntime/pix3.h"

#include "RenderContext.h"
#include "Framegraph.h"
#include "ShaderService.h"
#include "DescriptorAllocator.h"
#include "sample/DrawQuadNode.h"
#include "sample/DrawCubeNode.h"

WCHAR kWindowClass[] = L"FramegraphTest";
WCHAR kWindowTitle[] = L"FramegraphTest";
constexpr int kWindowWidth = 800;
constexpr int kWindowHeight = 600;

struct Vertex
{
    DirectX::XMFLOAT3 pos;
    DirectX::XMFLOAT3 TexCoord;
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
	hvk::render::DescriptorAllocator descriptorAllocator(*renderCtx);
	//hvk::render::ResourceManager resourceManager(renderCtx);
	auto resourceManager = std::make_shared<hvk::render::ResourceManager>(renderCtx);
	hvk::render::Framegraph framegraph(renderCtx, resourceManager);

	auto swapchain = renderCtx->CreateSwapchain(window, 2, kWindowWidth, kWindowHeight);

	hvk::render::sample::DrawQuadNode drawQuadNode(*renderCtx, *resourceManager);
	hvk::render::sample::DrawCubeNode drawCubeNode(*renderCtx, *resourceManager);

	MSG msg;
	bool running = true;
	size_t framecount = 0;
	uint8_t frameIndex = 0;

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

		if (framecount == 0) {
			PIXCaptureParameters captureParams = {};
			captureParams.GpuCaptureParameters.FileName = L"C://Users//timot//code//test.wpix";
			PIXBeginCapture(PIX_CAPTURE_GPU, &captureParams);
		}

		framegraph.BeginFrame();

		// insert pass
		drawQuadNode.Draw(*renderCtx, framegraph, descriptorAllocator);
		drawCubeNode.Draw(*renderCtx, framegraph, descriptorAllocator, drawQuadNode.GetQuadTexture(), swapchain, frameIndex);

		framegraph.EndFrame(descriptorAllocator);

		swapchain->Present(1, 0);

		if (framecount == 0) {
			PIXEndCapture(false);
		}

		frameIndex = swapchain->GetCurrentBackBufferIndex();
		++framecount;
	}

	hvk::render::WaitForGraphics(renderCtx->GetDevice(), renderCtx->GetCommandQueue());

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
