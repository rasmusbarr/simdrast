//
//  MsaaRenderTarget.h
//  DirectXA4
//
//  Created by Rasmus Barringer on 2013-06-07.
//  Copyright (c) 2013 Rasmus Barringer. All rights reserved.
//

#ifndef DirectXA4_MsaaRenderTarget_h
#define DirectXA4_MsaaRenderTarget_h

#include "DxContext.h"
#include "RenderTarget.h"
#include "ReleasePointer.h"

class MsaaRenderTarget : public RenderTarget {
private:
	unsigned width, height, samples;
	ReleasePointer<ID3D11Texture2D> msaaTexture;
	ReleasePointer<ID3D11Texture2D> resolvedTexture;
	ReleasePointer<ID3D11ShaderResourceView> shaderView;
	ReleasePointer<ID3D11RenderTargetView> renderTargetView;
	ReleasePointer<ID3D11DepthStencilView> depthStencilView;

public:
	MsaaRenderTarget(unsigned width, unsigned height, unsigned samples, unsigned mipCount, DXGI_FORMAT format);

	virtual void resolve();

	virtual void bind();

	virtual void bindTexture(unsigned slot);

	virtual void clear(const srast::float4& color);
};

#endif
