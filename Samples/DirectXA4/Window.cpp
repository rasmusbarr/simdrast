//
//  Window.cpp
//  DirectXA4
//
//  Created by Rasmus Barringer on 2013-06-07.
//  Copyright (c) 2013 Rasmus Barringer. All rights reserved.
//

#include "Window.h"
#include <cassert>
#include <map>

static std::map<HWND, Window*> windowMap;

Window::Window(unsigned width, unsigned height, bool useWarp) : width(width), height(height) {
	window = createWindow(width, height);
	assert(window);
	ShowWindow(window, SW_SHOWNORMAL);

	windowMap[window] = this;

	recreateDevice(useWarp);
}

void Window::recreateDevice(bool useWarp) {
	device.release();
	context.release();
	swapChain.release();
	renderTargetView.release();
	depthStencilView.release();

	D3D_FEATURE_LEVEL featureLevel;

	DXGI_SWAP_CHAIN_DESC swapDesc = { 0 };
	swapDesc.BufferCount = 1;
	swapDesc.BufferDesc.Width = width;
	swapDesc.BufferDesc.Height = height;
	swapDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapDesc.BufferDesc.RefreshRate.Numerator = 60;
	swapDesc.BufferDesc.RefreshRate.Denominator = 1;
	swapDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapDesc.OutputWindow = window;
	swapDesc.SampleDesc.Count = 1;
	swapDesc.SampleDesc.Quality = 0;
	swapDesc.Windowed = TRUE;

	D3D_FEATURE_LEVEL featureLevels[] = {
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0,
    };
	UINT numFeatureLevels = ARRAYSIZE(featureLevels);

	DX_THROW_ON_FAIL(D3D11CreateDeviceAndSwapChain(NULL, useWarp ? D3D_DRIVER_TYPE_WARP : D3D_DRIVER_TYPE_HARDWARE, NULL, NULL, featureLevels, numFeatureLevels, D3D11_SDK_VERSION, &swapDesc, &swapChain, &device, &featureLevel, &context));
	assert(featureLevel >= D3D_FEATURE_LEVEL_10_0);

	ReleasePointer<ID3D11Texture2D> backBuffer;
	DX_THROW_ON_FAIL(swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBuffer));
	DX_THROW_ON_FAIL(device->CreateRenderTargetView(backBuffer, NULL, &renderTargetView));

	ReleasePointer<ID3D11Texture2D> depthStencil;
	D3D11_TEXTURE2D_DESC depthDesc = { 0 };
	depthDesc.Width = width;
	depthDesc.Height = height;
	depthDesc.MipLevels = 1;
	depthDesc.ArraySize = 1;
	depthDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	depthDesc.SampleDesc.Count = 1;
	depthDesc.SampleDesc.Quality = 0;
	depthDesc.Usage = D3D11_USAGE_DEFAULT;
	depthDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	depthDesc.CPUAccessFlags = 0;
	depthDesc.MiscFlags = 0;
	DX_THROW_ON_FAIL(device->CreateTexture2D(&depthDesc, NULL, &depthStencil));

	D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc = { (DXGI_FORMAT)0 };
	dsvDesc.Format = depthDesc.Format;
	dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Texture2D.MipSlice = 0;
	DX_THROW_ON_FAIL(device->CreateDepthStencilView(depthStencil, &dsvDesc, &depthStencilView));

	makeCurrent();
}

void Window::makeCurrent() {
	DxContext::setCurrent(device, context);
}

void Window::bindRenderTarget() {
	ID3D11RenderTargetView* renderTargets[] = { renderTargetView };
	context->OMSetRenderTargets(1, renderTargets, depthStencilView);

	D3D11_VIEWPORT vp;
	vp.Width = (FLOAT)width;
	vp.Height = (FLOAT)height;
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	context->RSSetViewports(1, &vp);
}

void Window::enterLoop() {
	MSG msg = { 0 };
	while (msg.message != WM_QUIT) {
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else {
			render();
		}
	}
}

void Window::close() {
	DestroyWindow(window);
	windowMap.erase(window);
	window = 0;
}

Window::~Window() {
	if (window)
		windowMap.erase(window);
}

void Window::render() {
	float clearColor[4] = { 0.0f, 0.125f, 0.3f, 1.0f };
	context->ClearRenderTargetView(renderTargetView, clearColor);
	context->ClearDepthStencilView(depthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);
	
	bindRenderTarget();
	onRender();
	
	if (!window)
		return;
	
	swapChain->Present(0, 0);
}

HWND Window::createWindow(unsigned width, unsigned height) {
	WNDCLASSEX wcex;
	wcex.cbSize = sizeof( WNDCLASSEX );
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = windowProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = GetModuleHandle(0);
	wcex.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = NULL;
	wcex.lpszClassName = L"DxWndClass";
	wcex.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
	RegisterClassEx(&wcex);

	RECT rc = { 0, 0, width, height };
	AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);
	return CreateWindow(L"DxWndClass", L"Window", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left, rc.bottom - rc.top, NULL, NULL, GetModuleHandle(0), NULL);
}

LRESULT CALLBACK Window::windowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
	std::map<HWND, Window*>::iterator i = windowMap.find(window);
	Window* obj = i == windowMap.end() ? 0 : (*i).second;

	if (!obj)
		return DefWindowProc(window, message, wParam, lParam);

	PAINTSTRUCT ps;
	HDC hdc;
	switch (message) {
		case WM_PAINT:
			hdc = BeginPaint(window, &ps);
			EndPaint(window, &ps);
			break;
		case WM_CHAR:
			obj->onKeyboard((char)wParam);
			break;
		case WM_DESTROY:
			PostQuitMessage(0);
			break;
		default:
			return DefWindowProc(window, message, wParam, lParam);
	}
	return 0;
}
