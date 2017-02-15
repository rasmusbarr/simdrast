//
//  Font.h
//  DirectXA4
//
//  Created by Rasmus Barringer on 2013-06-07.
//  Copyright (c) 2013 Rasmus Barringer. All rights reserved.
//

#ifndef DirectXA4_Font_h
#define DirectXA4_Font_h

#include "Shader.h"
#include "DxContext.h"
#include "ReleasePointer.h"
#include "../../SimdRast/VectorMath.h"

class Font {
private:
	struct Vertex {
		srast::float3 pos;
		srast::float2 uv;
		Vertex(const srast::float3& pos, const srast::float2& uv) : pos(pos), uv(uv) {}
	};

	static const unsigned maxQuadCount = 512;

	srast::float2 uvDelta;

	Shader shader;
	ReleasePointer<ID3D11Texture2D> tex;
	ReleasePointer<ID3D11ShaderResourceView> textureView;
	ReleasePointer<ID3D11SamplerState> sampler;
	ReleasePointer<ID3D11BlendState> transparencyBlendState;
	ReleasePointer<ID3D11Buffer> vertexBuffer;
	ReleasePointer<ID3D11Buffer> indexBuffer;

public:
	Font();

	void draw(float x, float y, const std::string& str);
};

#endif
