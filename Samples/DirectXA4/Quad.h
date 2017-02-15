//
//  Quad.h
//  DirectXA4
//
//  Created by Rasmus Barringer on 2013-06-07.
//  Copyright (c) 2013 Rasmus Barringer. All rights reserved.
//

#ifndef DirectXA4_Quad_h
#define DirectXA4_Quad_h

#include "DxContext.h"
#include "ReleasePointer.h"
#include "../../SimdRast/VectorMath.h"

class Quad {
private:
	struct Vertex {
		srast::float3 pos;
		srast::float2 uv;
		Vertex(const srast::float3& pos, const srast::float2& uv) : pos(pos), uv(uv) {}
	};

	ReleasePointer<ID3D11Buffer> vertexBuffer;

public:
	Quad();

	const D3D11_INPUT_ELEMENT_DESC* layout() const;

	void render();
};

#endif
