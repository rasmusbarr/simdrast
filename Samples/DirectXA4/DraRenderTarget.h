//
//  DraRenderTarget.h
//  DirectXA4
//
//  Created by Rasmus Barringer on 2013-06-07.
//  Copyright (c) 2013 Rasmus Barringer. All rights reserved.
//

#ifndef DirectXA4_DraRenderTarget_h
#define DirectXA4_DraRenderTarget_h

#include "DxContext.h"
#include "Quad.h"
#include "Shader.h"
#include "DraSurface.h"
#include "RenderTarget.h"
#include "ConstantBuffer.h"

class DraRenderTarget : public RenderTarget {
private:
	static const unsigned bytesPerTexel = 4;

	unsigned width, height, mipCount;
	ReleasePointer<ID3D11Texture2D> cpuSharedTexture;
	ReleasePointer<ID3D11Texture2D> gpuSharedTexture;
	ReleasePointer<ID3D11ShaderResourceView> shaderView;
	ReleasePointer<ID3D11RenderTargetView> renderTargetView;
	ReleasePointer<ID3D11DepthStencilView> depthStencilView;
	Quad quad;
	Shader quadShader;
	ConstantBuffer<srast::float4, 0> quadColor;

public:
	DraRenderTarget(unsigned width, unsigned height, unsigned mipCount, DXGI_FORMAT format);

	virtual void bind();

	virtual void bindTexture(unsigned slot);

	virtual void clear(const srast::float4& color);

	DraSurface map(unsigned mip);

	void unmap(unsigned mip);

	static void activateExtension();
};

#endif
