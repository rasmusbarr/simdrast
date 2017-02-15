//
//  Quad.cpp
//  DirectXA4
//
//  Created by Rasmus Barringer on 2013-06-07.
//  Copyright (c) 2013 Rasmus Barringer. All rights reserved.
//

#include "Quad.h"

Quad::Quad() {
	Vertex vertices[] = {
		Vertex(srast::float3(-1.0f, -1.0f, 1.0f-1e-6f), srast::float2(0.0f, 1.0f)),
		Vertex(srast::float3(+1.0f, -1.0f, 1.0f-1e-6f), srast::float2(1.0f, 1.0f)),
		Vertex(srast::float3(-1.0f, +1.0f, 1.0f-1e-6f), srast::float2(0.0f, 0.0f)),
		Vertex(srast::float3(+1.0f, +1.0f, 1.0f-1e-6f), srast::float2(1.0f, 0.0f)),
	};

	D3D11_BUFFER_DESC bd = { 0 };
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(vertices);
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = 0;

	D3D11_SUBRESOURCE_DATA initData = { 0 };
	initData.pSysMem = vertices;
	DX_THROW_ON_FAIL(DxContext::getDevice()->CreateBuffer(&bd, &initData, &vertexBuffer));
}

const D3D11_INPUT_ELEMENT_DESC* Quad::layout() const {
	static const D3D11_INPUT_ELEMENT_DESC layout[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ 0 },
	};
	return layout;
}

void Quad::render() {
	UINT stride = sizeof(Vertex);
	UINT offset = 0;

	ID3D11Buffer* buffers[] = {
		vertexBuffer,
	};
	DxContext::getContext()->IASetVertexBuffers(0, 1, buffers, &stride, &offset);
	DxContext::getContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	DxContext::getContext()->Draw(4, 0);
}
