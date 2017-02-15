//
//  Font.cpp
//  DirectXA4
//
//  Created by Rasmus Barringer on 2013-06-07.
//  Copyright (c) 2013 Rasmus Barringer. All rights reserved.
//

#include "Font.h"
#include <vector>
#include "../Framework/External/lodepng/lodepng.h"

static const D3D11_INPUT_ELEMENT_DESC layout[] = {
	{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ 0 },
};

Font::Font() : shader("QuadTexture.fx", layout) {
	unsigned width, height;
	std::vector<unsigned char> image;
	
	if (LodePNG::decode(image, width, height, "font.png"))
		throw std::runtime_error("unable to load font.png");

	uvDelta = srast::float2((int)width*(1.0f/16.0f), (int)height*(1.0f/6.0f));

	D3D11_TEXTURE2D_DESC desc = { 0 };
    desc.Width = width;
    desc.Height = height;
	desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    desc.CPUAccessFlags = 0;
    desc.MiscFlags = 0;

	D3D11_SUBRESOURCE_DATA initData = { 0 };

	initData.pSysMem = &image[0];
	initData.SysMemPitch = desc.Width*4;

    DX_THROW_ON_FAIL(DxContext::getDevice()->CreateTexture2D(&desc, &initData, &tex));

	D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc = { (DXGI_FORMAT)0 };
    SRVDesc.Format = desc.Format;
    SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    SRVDesc.Texture2D.MipLevels = desc.MipLevels;

    DX_THROW_ON_FAIL(DxContext::getDevice()->CreateShaderResourceView(tex, &SRVDesc, &textureView));

	D3D11_SAMPLER_DESC sampDesc = { (D3D11_FILTER)0 };
	sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
	sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	sampDesc.MinLOD = 0;
	sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
	DX_THROW_ON_FAIL(DxContext::getDevice()->CreateSamplerState(&sampDesc, &sampler));

	D3D11_BLEND_DESC blendStateDesc = { 0 };
	blendStateDesc.RenderTarget[0].BlendEnable = TRUE;
	blendStateDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	blendStateDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	blendStateDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	blendStateDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	blendStateDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
	blendStateDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	blendStateDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
	DX_THROW_ON_FAIL(DxContext::getDevice()->CreateBlendState(&blendStateDesc, &transparencyBlendState));

	std::vector<unsigned short> indices(maxQuadCount*6);

	for (unsigned i = 0; i < maxQuadCount; ++i) {
		indices[i*6]     = i*4;
		indices[i*6 + 1] = i*4 + 1;
		indices[i*6 + 2] = i*4 + 2;
		indices[i*6 + 3] = i*4 + 1;
		indices[i*6 + 5] = i*4 + 2;
		indices[i*6 + 4] = i*4 + 3;
	}

	D3D11_SUBRESOURCE_DATA indexData = { 0 };
	indexData.pSysMem = &indices[0];

	D3D11_BUFFER_DESC ibd = { 0 };
	ibd.Usage = D3D11_USAGE_DEFAULT;
	ibd.ByteWidth = sizeof(unsigned short)*maxQuadCount*6;
	ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	ibd.CPUAccessFlags = 0;

	DX_THROW_ON_FAIL(DxContext::getDevice()->CreateBuffer(&ibd, &indexData, &indexBuffer));

	D3D11_BUFFER_DESC vbd = { 0 };
	vbd.Usage = D3D11_USAGE_DYNAMIC;
	vbd.ByteWidth = sizeof(Vertex)*4*maxQuadCount;
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	DX_THROW_ON_FAIL(DxContext::getDevice()->CreateBuffer(&vbd, 0, &vertexBuffer));
}

void Font::draw(float x, float y, const std::string& str) {
	using namespace srast;

	UINT viewportCount = 1;
    D3D11_VIEWPORT viewport;

	DxContext::getContext()->RSGetViewports(&viewportCount, &viewport);

	float invWidth = 1.0f / viewport.Width;
	float invHeight = 1.0f / viewport.Height;

	D3D11_MAPPED_SUBRESOURCE mappedData;
	DxContext::getContext()->Map(vertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedData );

	Vertex* vertices = reinterpret_cast<Vertex*>(mappedData.pData);
	unsigned quadCount = 0;

	float uDelta = 1.0f/16.0f;
	float vDelta = 1.0f/6.0f;

	float xDelta = 2.0f*uvDelta.x*invWidth;
	float yDelta = 2.0f*uvDelta.y*invHeight;

	x = 2.0f*x*invWidth - 1.0f;
	y = 1.0f - 2.0f*y*invHeight - yDelta;

	float initialX = x;

	for (size_t i = 0; quadCount < maxQuadCount && i < str.size(); ++i) {
		char c = str[i];

		if (c != '\n') {
			unsigned index = c - 32;

			int ix = index & 0xf;
			int iy = index >> 4;

			float u = ix * uDelta;
			float v = iy * vDelta;

			*(vertices++) = Vertex(srast::float3(x, y, 0.5f), srast::float2(u, v+vDelta));
			*(vertices++) = Vertex(srast::float3(x+xDelta, y, 0.5f), srast::float2(u+uDelta, v+vDelta));
			*(vertices++) = Vertex(srast::float3(x,y+yDelta, 0.5f), srast::float2(u, v));
			*(vertices++) = Vertex(srast::float3(x+xDelta, y+yDelta, 0.5f), srast::float2(u+uDelta, v));

			x += xDelta;
			++quadCount;
		}
		else {
			x = initialX;
			y -= yDelta;
		}
	}

	DxContext::getContext()->Unmap(vertexBuffer, 0);

	ID3D11SamplerState* samplers[] = { sampler };
	DxContext::getContext()->PSSetSamplers(0, 1, samplers);

	ID3D11ShaderResourceView* views[] = { textureView };
	DxContext::getContext()->PSSetShaderResources(0, 1, views);

	DxContext::getContext()->OMSetBlendState(transparencyBlendState, NULL, 0xFFFFFF);
	shader.bind();
	
	UINT stride = sizeof(Vertex);
	UINT offset = 0;

	ID3D11Buffer* buffers[] = {
		vertexBuffer,
	};
	DxContext::getContext()->IASetVertexBuffers(0, 1, buffers, &stride, &offset);
	DxContext::getContext()->IASetIndexBuffer(indexBuffer, DXGI_FORMAT_R16_UINT, 0);
	DxContext::getContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	DxContext::getContext()->DrawIndexed(quadCount*6, 0, 0);

	DxContext::getContext()->OMSetBlendState(0, NULL, 0xFFFFFF);
}
