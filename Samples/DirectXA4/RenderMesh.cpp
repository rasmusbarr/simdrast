//
//  RenderMesh.cpp
//  DirectXA4
//
//  Created by Rasmus Barringer on 2013-06-07.
//  Copyright (c) 2013 Rasmus Barringer. All rights reserved.
//

#include "RenderMesh.h"

RenderMesh::RenderMesh(fx::Mesh* mesh) : mesh(mesh) {
	for (size_t i = 0; i < mesh->textures.size(); ++i)
		textures.push_back(new Texture(mesh->textures[i]));

	for (size_t i = 0; i < mesh->drawCalls.size(); ++i) {
		const fx::Mesh::DrawCall& source = mesh->drawCalls[i];
		DrawCall* drawCall = new DrawCall();

		drawCall->texture = mesh->drawCalls[i].texture;

		D3D11_BUFFER_DESC bd = { 0 };
		bd.Usage = D3D11_USAGE_DEFAULT;
		bd.ByteWidth = sizeof(fx::Mesh::VertexAttributes) * source.vertexCount;
		bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		bd.CPUAccessFlags = 0;

		D3D11_SUBRESOURCE_DATA initData = { 0 };
		initData.pSysMem = &source.attributes[0];
		DX_THROW_ON_FAIL(DxContext::getDevice()->CreateBuffer(&bd, &initData, &drawCall->vertexBuffer));

		bd.Usage = D3D11_USAGE_DEFAULT;
		bd.ByteWidth = sizeof(unsigned) * source.indexCount;
		bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
		bd.CPUAccessFlags = 0;
		initData.pSysMem = source.indices;
		DX_THROW_ON_FAIL(DxContext::getDevice()->CreateBuffer(&bd, &initData, &drawCall->indexBuffer));

		drawCall->indexCount = source.indexCount;
		drawCalls.push_back(drawCall);
	}
}

const D3D11_INPUT_ELEMENT_DESC* RenderMesh::layout() const {
	static const D3D11_INPUT_ELEMENT_DESC layout[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ 0 },
	};
	return layout;
}

void RenderMesh::renderOpaque() {
	for (size_t i = 0; i < mesh->sortedDrawCalls.size(); ++i) {
		unsigned idx = (unsigned)(mesh->sortedDrawCalls[i] - &mesh->drawCalls[0]);

		DrawCall& drawCall = *drawCalls[idx];

		if (drawCall.texture != fx::Mesh::noTexture) {
			if (mesh->textures[drawCall.texture]->hasAlpha())
				continue;

			textures[drawCall.texture]->bind(0);
		}
		else {
			ID3D11ShaderResourceView* views[] = { 0 };
			DxContext::getContext()->PSSetShaderResources(0, 1, views);
		}

		UINT stride = sizeof(fx::Mesh::VertexAttributes);
		UINT offset = 0;

		ID3D11Buffer* buffers[] = {
			drawCall.vertexBuffer,
		};
		DxContext::getContext()->IASetVertexBuffers(0, 1, buffers, &stride, &offset);
		DxContext::getContext()->IASetIndexBuffer(drawCall.indexBuffer, DXGI_FORMAT_R32_UINT, 0);
		DxContext::getContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		DxContext::getContext()->DrawIndexed(drawCall.indexCount, 0, 0);
	}
}

void RenderMesh::renderTransparent() {
	for (size_t i = 0; i < mesh->sortedDrawCalls.size(); ++i) {
		unsigned idx = (unsigned)(mesh->sortedDrawCalls[i] - &mesh->drawCalls[0]);

		DrawCall& drawCall = *drawCalls[idx];

		if (drawCall.texture == fx::Mesh::noTexture)
			continue;

		if (!mesh->textures[drawCall.texture]->hasAlpha())
			continue;

		textures[drawCall.texture]->bind(0);

		UINT stride = sizeof(fx::Mesh::VertexAttributes);
		UINT offset = 0;

		ID3D11Buffer* buffers[] = {
			drawCall.vertexBuffer,
		};
		DxContext::getContext()->IASetVertexBuffers(0, 1, buffers, &stride, &offset);
		DxContext::getContext()->IASetIndexBuffer(drawCall.indexBuffer, DXGI_FORMAT_R32_UINT, 0);
		DxContext::getContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		DxContext::getContext()->DrawIndexed(drawCall.indexCount, 0, 0);
	}
}

RenderMesh::~RenderMesh() {
	for (size_t i = 0; i < textures.size(); ++i)
		delete textures[i];

	for (size_t i = 0; i < drawCalls.size(); ++i)
		delete drawCalls[i];
}
