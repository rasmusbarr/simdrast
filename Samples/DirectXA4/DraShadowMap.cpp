//
//  DraShadowMap.cpp
//  DirectXA4
//
//  Created by Rasmus Barringer on 2013-06-07.
//  Copyright (c) 2013 Rasmus Barringer. All rights reserved.
//

#include "DraShadowMap.h"

static const D3D11_INPUT_ELEMENT_DESC layout[] = {
	{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ 0 },
};

DraShadowMap::DraShadowMap(unsigned width, unsigned height) : shader("ShadowMap.fx", layout), renderTarget(width, height, 1, DXGI_FORMAT_R32_FLOAT) {
	D3D11_SAMPLER_DESC sampDesc = { (D3D11_FILTER)0 };
	sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
	sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	sampDesc.MinLOD = 0;
	sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
	DX_THROW_ON_FAIL(DxContext::getDevice()->CreateSamplerState(&sampDesc, &sampler));
}

void DraShadowMap::setWorldToShadowTransform(const srast::float4x4& transform) {
	uniforms.data = transform;
	uniforms.update();
}

srast::float4x4 DraShadowMap::getWorldToShadowTransform() {
	return uniforms.data;
}

void DraShadowMap::bind() {
	renderTarget.clear(1.0f);
	renderTarget.bind();
	uniforms.bind();
	shader.bind();
}

void DraShadowMap::bindSamplerAndTexture(unsigned slot) {
	ID3D11SamplerState* samplers[] = { sampler };
	DxContext::getContext()->PSSetSamplers(slot, 1, samplers);
	renderTarget.bindTexture(slot);
}

DraSurface DraShadowMap::map() {
	return renderTarget.map(0);
}

void DraShadowMap::unmap() {
	renderTarget.unmap(0);
}
