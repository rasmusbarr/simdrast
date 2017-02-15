//
//  Mesh.h
//  Framework
//
//  Created by Rasmus Barringer on 2012-10-22.
//  Copyright (c) 2012 Rasmus Barringer. All rights reserved.
//

#ifndef Framework_Mesh_h
#define Framework_Mesh_h

#include "LinearTexture.h"
#include <string>

namespace fx {

class Mesh {
public:
	static const unsigned noTexture = 0xffffffff;
	
	struct Vertex {
		srast::float3 p;
		float dummy;
	};
	
	struct VertexAttributes {
		srast::float3 p;
		srast::float3 n;
		srast::float2 uv;
	};
	
	struct DrawCall {
		unsigned texture;
		std::string material;
		
		srast::float3 bbMin, bbMax;
		
		Vertex* vertices;
		VertexAttributes* attributes;
		unsigned* indices;
		
		unsigned vertexCount;
		unsigned indexCount;
	};
	
	std::vector<DrawCall> drawCalls;
	std::vector<LinearTexture*> textures;
	std::vector<DrawCall*> sortedDrawCalls;
	
	Mesh(const char* filename);
	
	void sortDrawCalls(const srast::float4x4& modelViewMatrix);

	void eraseDrawCall(unsigned index);
	
	~Mesh();
};

}

#endif
