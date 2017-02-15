//
//  RenderMesh.h
//  DirectXA4
//
//  Created by Rasmus Barringer on 2013-06-07.
//  Copyright (c) 2013 Rasmus Barringer. All rights reserved.
//

#ifndef DirectXA4_RenderMesh_h
#define DirectXA4_RenderMesh_h

#include "DxContext.h"
#include "Texture.h"
#include "Shader.h"
#include "../Framework/Mesh.h"

class RenderMesh {
private:
	struct DrawCall {
		ReleasePointer<ID3D11Buffer> vertexBuffer;
		ReleasePointer<ID3D11Buffer> indexBuffer;
		unsigned texture;
		unsigned indexCount;
	};

	fx::Mesh* mesh;
	std::vector<DrawCall*> drawCalls;
	std::vector<Texture*> textures;

public:
	RenderMesh(fx::Mesh* mesh);

	const D3D11_INPUT_ELEMENT_DESC* RenderMesh::layout() const;

	void renderOpaque();

	void renderTransparent();

	~RenderMesh();
};

#endif
