//
//  DraRenderTarget.cpp
//  DirectXA4
//
//  Created by Rasmus Barringer on 2013-06-07.
//  Copyright (c) 2013 Rasmus Barringer. All rights reserved.
//

#include "DraRenderTarget.h"
#include "../Framework/External/IGFXExtensions/IGFXExtensionsHelper.h"
#include <iostream>
#include <cassert>

inline unsigned swizzle_x(unsigned x) { // x in bytes
	UINT x_low = (x & 0xF); 
	UINT x_high = (x & 0xFFFFFFF0);
	return (x_low | (x_high << 5)); // 5 bits are coming from Y in the tile. Whole Y tiles are accumulated to x.
}

DraRenderTarget::DraRenderTarget(unsigned width, unsigned height, unsigned mipCount, DXGI_FORMAT format) : width(width), height(height), mipCount(mipCount), quadShader("QuadColor.fx", quad.layout()) {
	D3D11_TEXTURE2D_DESC desc = { 0 };
	desc.Width = width;
	desc.Height = height;
	desc.MipLevels = mipCount;
	desc.ArraySize = 1;
	desc.Format = format;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.BindFlags = 0;
	desc.CPUAccessFlags = 0;
	desc.MiscFlags = 0;

	D3D11_TEXTURE2D_DESC gpuDesc = desc;
	gpuDesc.Usage = D3D11_USAGE_DEFAULT;
	gpuDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	gpuDesc.CPUAccessFlags = 0;

	D3D11_TEXTURE2D_DESC cpuDesc = desc;
	cpuDesc.Usage = D3D11_USAGE_STAGING;
	cpuDesc.BindFlags = 0;
	cpuDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE | D3D11_CPU_ACCESS_READ;

	DX_THROW_ON_FAIL(IGFX::CreateSharedTexture2D(DxContext::getDevice(), &cpuDesc, &cpuSharedTexture, &gpuDesc, &gpuSharedTexture, NULL));
	DxContext::getContext()->CopyResource(cpuSharedTexture, gpuSharedTexture); // Bind.

	D3D11_RENDER_TARGET_VIEW_DESC renderTargetViewDesc = { (DXGI_FORMAT)0 };
	renderTargetViewDesc.Format = desc.Format;
	renderTargetViewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	renderTargetViewDesc.Texture2D.MipSlice = 0;
	DX_THROW_ON_FAIL(DxContext::getDevice()->CreateRenderTargetView(gpuSharedTexture, &renderTargetViewDesc, &renderTargetView));

	D3D11_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc = { (DXGI_FORMAT)0 };
	shaderResourceViewDesc.Format = desc.Format;
	shaderResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	shaderResourceViewDesc.Texture2D.MostDetailedMip = 0;
	shaderResourceViewDesc.Texture2D.MipLevels = mipCount;
	DX_THROW_ON_FAIL(DxContext::getDevice()->CreateShaderResourceView(gpuSharedTexture, &shaderResourceViewDesc, &shaderView));

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
	DX_THROW_ON_FAIL(DxContext::getDevice()->CreateTexture2D(&depthDesc, NULL, &depthStencil));

	D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc = { (DXGI_FORMAT)0 };
	dsvDesc.Format = depthDesc.Format;
	dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Texture2D.MipSlice = 0;
	DX_THROW_ON_FAIL(DxContext::getDevice()->CreateDepthStencilView(depthStencil, &dsvDesc, &depthStencilView));
}

void DraRenderTarget::bind() {
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

void DraRenderTarget::bindTexture(unsigned slot) {
	ID3D11ShaderResourceView* views[] = { shaderView };
	DxContext::getContext()->PSSetShaderResources(slot, 1, views);
}

void DraRenderTarget::clear(const srast::float4& color) {
	float clearColor[4] = { color.x, color.y, color.z, color.w };
	DxContext::getContext()->ClearRenderTargetView(renderTargetView, clearColor);
	DxContext::getContext()->ClearDepthStencilView(depthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);

	// Avoid fast clear.
	bind();
	quadShader.bind();
	quadColor.data = color;
	quadColor.update();
	quadColor.bind();
	quad.render();
}

DraSurface DraRenderTarget::map(unsigned mip) {
	D3D11_MAPPED_SUBRESOURCE mapData;
	DX_THROW_ON_FAIL(DxContext::getContext()->Map(cpuSharedTexture, mip, D3D11_MAP_READ_WRITE, NULL, &mapData));

	assert(mapData.RowPitch >= sizeof(RESOURCE_DIRECT_ACCESS_MAP_DATA));
	RESOURCE_DIRECT_ACCESS_MAP_DATA* gpuSubResourceData = (RESOURCE_DIRECT_ACCESS_MAP_DATA*)mapData.pData;

	DraSurface surf;

	surf.pitch = swizzle_x(gpuSubResourceData->Pitch);
	surf.base = gpuSubResourceData->pBaseAddress;

	if (gpuSubResourceData->XOffset || gpuSubResourceData->YOffset)
		std::cout << "DRA warning: non-zero offset. Some code depend on zero offset." << std::endl;

	return surf;
}

void DraRenderTarget::unmap(unsigned mip) {
	DxContext::getContext()->Unmap(cpuSharedTexture, mip);
}

void DraRenderTarget::activateExtension() {
	DX_THROW_ON_FAIL(IGFX::Init(DxContext::getDevice()));
	IGFX::Extensions extensions = IGFX::getAvailableExtensions(DxContext::getDevice());

	if (!extensions.DirectResourceAccess)
		throw std::runtime_error("unable to activate IGFX extension DirectResourceAccess");
}
