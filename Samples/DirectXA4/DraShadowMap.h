//
//  DraShadowMap.h
//  DirectXA4
//
//  Created by Rasmus Barringer on 2013-06-07.
//  Copyright (c) 2013 Rasmus Barringer. All rights reserved.
//

#ifndef DirectXA4_DraShadowMap_h
#define DirectXA4_DraShadowMap_h

#include "Shader.h"
#include "DraRenderTarget.h"
#include "ConstantBuffer.h"

class DraShadowMap {
private:
	Shader shader;
	DraRenderTarget renderTarget;
	ConstantBuffer<srast::float4x4, 0> uniforms;
	ReleasePointer<ID3D11SamplerState> sampler;

public:
	DraShadowMap(unsigned width, unsigned height);

	void setWorldToShadowTransform(const srast::float4x4& transform);

	srast::float4x4 getWorldToShadowTransform();

	void bind();

	void bindSamplerAndTexture(unsigned slot);

	DraSurface map();

	void unmap();
};

#endif
