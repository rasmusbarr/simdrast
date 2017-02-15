//
//  Window.h
//  DirectXA4
//
//  Created by Rasmus Barringer on 2013-06-07.
//  Copyright (c) 2013 Rasmus Barringer. All rights reserved.
//

#ifndef DirectXA4_Window_h
#define DirectXA4_Window_h

#include "DxContext.h"
#include "ReleasePointer.h"

class Window {
private:
	HWND window;
	unsigned width, height;
	ReleasePointer<ID3D11Device> device;
	ReleasePointer<ID3D11DeviceContext> context;
	ReleasePointer<IDXGISwapChain> swapChain;
	ReleasePointer<ID3D11RenderTargetView> renderTargetView;
	ReleasePointer<ID3D11DepthStencilView> depthStencilView;

public:
	Window(unsigned width, unsigned height, bool useWarp = false);

	unsigned getWidth() const {
		return width;
	}

	unsigned getHeight() const {
		return height;
	}

	void recreateDevice(bool useWarp = false);

	void makeCurrent();

	void bindRenderTarget();

	void enterLoop();

	void close();

	~Window();

private:
	void render();
	
	virtual void onRender() = 0;

	virtual void onKeyboard(char c) = 0;

	static HWND createWindow(unsigned width, unsigned height);

	static LRESULT CALLBACK windowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam);
};

#endif
