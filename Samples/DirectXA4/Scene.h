//
//  Scene.h
//  DirectXA4
//
//  Created by Rasmus Barringer on 2013-06-07.
//  Copyright (c) 2013 Rasmus Barringer. All rights reserved.
//

#ifndef DirectXA4_Scene_h
#define DirectXA4_Scene_h

#include "../../SimdRast/VectorMath.h"
#include "../Framework/Mesh.h"

class Window;

struct SceneUniforms {
	srast::float4x4 modelViewProj;
	srast::float4x4 shadowTransform[4];
	srast::float4 light[4];
	srast::float3 view;
	int shadowCount;
};

class Scene {
private:
	bool animate;
	fx::Mesh mesh;
	std::vector<srast::int2> shadowMapSizes;

public:
	Scene(const char* filename, const std::vector<srast::int2>& shadowMapSizes);

	void toggleAnimate();

	bool isAnimating() const;

	fx::Mesh* getMesh();

	const std::vector<srast::int2>& getShadowMapSizes() const;

	virtual void updateUniforms(Window* window, float timeStep, SceneUniforms& uniforms) = 0;

	virtual ~Scene();
};

#endif
