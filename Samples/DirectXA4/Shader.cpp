//
//  Shader.cpp
//  DirectXA4
//
//  Created by Rasmus Barringer on 2013-06-07.
//  Copyright (c) 2013 Rasmus Barringer. All rights reserved.
//

#include "Shader.h"
#include "WideString.h"
#include <D3DCompiler.h>

static HRESULT compileShaderFromFile(WCHAR* fileName, LPCSTR entryPoint, LPCSTR shaderModel, ID3DBlob** blobOut, const D3D_SHADER_MACRO* defines) {
	ReleasePointer<ID3DBlob> errorBlob;
	HRESULT hr = D3DCompileFromFile(fileName, defines, NULL, entryPoint, shaderModel, D3DCOMPILE_ENABLE_STRICTNESS, 0, blobOut, &errorBlob);

	if (FAILED(hr)) {
		if (errorBlob)
			OutputDebugStringA((char*)errorBlob->GetBufferPointer());
		return hr;
	}

	return S_OK;
}

Shader::Shader(const char* filename, const D3D11_INPUT_ELEMENT_DESC* layout, const D3D_SHADER_MACRO* defines) {
	ReleasePointer<ID3DBlob> vsBlob;

	DX_THROW_ON_FAIL(compileShaderFromFile(WideString(filename), "VS", "vs_4_0", &vsBlob, defines));
	DX_THROW_ON_FAIL(DxContext::getDevice()->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), NULL, &vertexShader));

	UINT numElements = 0;
	while (layout[numElements].SemanticName)
		++numElements;

	DX_THROW_ON_FAIL(DxContext::getDevice()->CreateInputLayout(layout, numElements, vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &vertexLayout));

	ReleasePointer<ID3DBlob> psBlob;

	DX_THROW_ON_FAIL(compileShaderFromFile(WideString(filename), "PS", "ps_4_0", &psBlob, defines));
	DX_THROW_ON_FAIL(DxContext::getDevice()->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), NULL, &pixelShader));
}

void Shader::bind() {
	DxContext::getContext()->IASetInputLayout(vertexLayout);
	DxContext::getContext()->VSSetShader(vertexShader, NULL, 0);
	DxContext::getContext()->PSSetShader(pixelShader, NULL, 0);
}
