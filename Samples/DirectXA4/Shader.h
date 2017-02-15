//
//  Shader.h
//  DirectXA4
//
//  Created by Rasmus Barringer on 2013-06-07.
//  Copyright (c) 2013 Rasmus Barringer. All rights reserved.
//

#ifndef DirectXA4_Shader_h
#define DirectXA4_Shader_h

#include "DxContext.h"
#include "ReleasePointer.h"

class Shader {
private:
	ReleasePointer<ID3D11InputLayout> vertexLayout;
	ReleasePointer<ID3D11VertexShader> vertexShader;
	ReleasePointer<ID3D11PixelShader> pixelShader;

public:
	Shader(const char* filename, const D3D11_INPUT_ELEMENT_DESC* layout, const D3D_SHADER_MACRO* defines = 0);

	void bind();
};

#endif
