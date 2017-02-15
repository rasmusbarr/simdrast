//
//  MsaaRenderTarget.cpp
//  DirectXA4
//
//  Created by Rasmus Barringer on 2013-06-07.
//  Copyright (c) 2013 Rasmus Barringer. All rights reserved.
//

#include "MsaaRenderTarget.h"

MsaaRenderTarget::MsaaRenderTarget(unsigned width, unsigned height, unsigned samples, unsigned mipCount, DXGI_FORMAT format) : width(width), height(height), samples(samples) {
	D3D11_TEXTURE2D_DESC desc = { 0 };
	desc.Width = width;
	desc.Height = height;
	desc.MipLevels = mipCount; 
	desc.ArraySize = 1;
	desc.Format = format;
	desc.SampleDesc.Count = samples;
	desc.SampleDesc.Quality = 0;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.BindFlags = samples > 1 ? D3D11_BIND_RENDER_TARGET : D3D11_BIND_RENDER_TARGET|D3D11_BIND_SHADER_RESOURCE;
	desc.CPUAccessFlags = 0;
	desc.MiscFlags = 0;
	DX_THROW_ON_FAIL(DxContext::getDevice()->CreateTexture2D(&desc, 0, &msaaTexture));

	if (samples > 1) {
		desc.SampleDesc.Count = 1;
		desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		DX_THROW_ON_FAIL(DxContext::getDevice()->CreateTexture2D(&desc, 0, &resolvedTexture));
	}

	D3D11_RENDER_TARGET_VIEW_DESC renderTargetViewDesc = { (DXGI_FORMAT)0 };
	renderTargetViewDesc.Format = desc.Format;
	renderTargetViewDesc.ViewDimension = samples > 1 ? D3D11_RTV_DIMENSION_TEXTURE2DMS : D3D11_RTV_DIMENSION_TEXTURE2D;
	renderTargetViewDesc.Texture2D.MipSlice = 0;
	DX_THROW_ON_FAIL(DxContext::getDevice()->CreateRenderTargetView(msaaTexture, &renderTargetViewDesc, &renderTargetView));

	D3D11_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc = { (DXGI_FORMAT)0 };
	shaderResourceViewDesc.Format = desc.Format;
	shaderResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	shaderResourceViewDesc.Texture2D.MostDetailedMip = 0;
	shaderResourceViewDesc.Texture2D.MipLevels = mipCount;
	DX_THROW_ON_FAIL(DxContext::getDevice()->CreateShaderResourceView(samples > 1 ? resolvedTexture : msaaTexture, &shaderResourceViewDesc, &shaderView));

	ReleasePointer<ID3D11Texture2D> depthStencil;
	D3D11_TEXTURE2D_DESC depthDesc = { 0 };
	depthDesc.Width = width;
	depthDesc.Height = height;
	depthDesc.MipLevels = 1;
	depthDesc.ArraySize = 1;
	depthDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	depthDesc.SampleDesc.Count = samples;
	depthDesc.SampleDesc.Quality = 0;
	depthDesc.Usage = D3D11_USAGE_DEFAULT;
	depthDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	depthDesc.CPUAccessFlags = 0;
	depthDesc.MiscFlags = 0;
	DX_THROW_ON_FAIL(DxContext::getDevice()->CreateTexture2D(&depthDesc, NULL, &depthStencil));

	D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc = { (DXGI_FORMAT)0 };
	dsvDesc.Format = depthDesc.Format;
	dsvDesc.ViewDimension = samples > 1 ? D3D11_DSV_DIMENSION_TEXTURE2DMS : D3D11_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Texture2D.MipSlice = 0;
	DX_THROW_ON_FAIL(DxContext::getDevice()->CreateDepthStencilView(depthStencil, &dsvDesc, &depthStencilView));
}

void MsaaRenderTarget::resolve() {
	if (samples > 1)
		DxContext::getContext()->ResolveSubresource(resolvedTexture, 0, msaaTexture, 0, DXGI_FORMAT_R8G8B8A8_UNORM);
}

void MsaaRenderTarget::bind() {
	ID3D11RenderTargetView* renderTargets[] = { renderTargetView };
	DxContext::getContext()->OMSetRenderTargets(1, renderTargets, depthStencilView);

	D3D11_VIEWPORT vp;
	vp.Width = (FLOAT)width;
	vp.Height = (FLOAT)height;
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	DxContext::getContext()->RSSetViewports(1, &vp);
}

void MsaaRenderTarget::bindTexture(unsigned slot) {
	ID3D11ShaderResourceView* views[] = { shaderView };
	DxContext::getContext()->PSSetShaderResources(slot, 1, views);
}

void MsaaRenderTarget::clear(const srast::float4& color) {
	float clearColor[4] = { color.x, color.y, color.z, color.w };
	DxContext::getContext()->ClearRenderTargetView(renderTargetView, clearColor);
	DxContext::getContext()->ClearDepthStencilView(depthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);
}
