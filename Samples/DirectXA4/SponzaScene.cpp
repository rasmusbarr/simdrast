//
//  SponzaScene.cpp
//  DirectXA4
//
//  Created by Rasmus Barringer on 2013-06-07.
//  Copyright (c) 2013 Rasmus Barringer. All rights reserved.
//

#include "SponzaScene.h"
#include "Window.h"

#define M_PI (3.141592654f)

static std::vector<srast::int2> shadowMaps() {
	std::vector<srast::int2> sizes;
	sizes.push_back(srast::int2(1024, 512));
	return sizes;
}

SponzaScene::SponzaScene() : Scene("crytek-sponza/sponza.obj", shadowMaps()) {
	rot = M_PI*0.4f;

	// Tweak the loaded mesh:
	// - Move the vases up to avoid intersection with ground.
	// - Remove any untextured objects to avoid shading issues.

	const std::vector<fx::Mesh::DrawCall>& drawCalls = getMesh()->drawCalls;

	for (size_t i = 0; i < drawCalls.size(); ) {
		const std::string& name = drawCalls[i].material;

		bool isVase = (name == "vase_round" || name == "Material__57" || name == "vase" || name == "vase_hanging");

		if (isVase) {
			for (unsigned j = 0; j < drawCalls[i].vertexCount; ++j) {
				drawCalls[i].vertices[j].p.y += 2.0f;
				drawCalls[i].attributes[j].p.y += 2.0f;
			}
		}

		if (drawCalls[i].texture == fx::Mesh::noTexture)
			getMesh()->eraseDrawCall((unsigned)i);
		else
			++i;
	}
}

void SponzaScene::updateUniforms(Window* window, float timeStep, SceneUniforms& uniforms) {
	using namespace srast;

	if (isAnimating())
		rot += timeStep*0.1f;

	float3 camera = float3(0.0f, 150.0f, 550.0f);
	float targetY = 190.0f;

	float4x4 proj = float4x4::perspectiveProjection(M_PI*0.2f, (int)window->getWidth()/(float)(int)window->getHeight(), 1.0f, 2200.0f);
	float4x4 modelView = float4x4::lookAt(camera, float3(0.0f, targetY, 0.0f), float3(0.0f, 1.0f, 0.0f)) * float4x4::rotationY(rot);

	float4x4 mvi = modelView.inverse();
	uniforms.view = float3(mvi.c3.x, mvi.c3.y, mvi.c3.z);

	getMesh()->sortDrawCalls(modelView);
	
	uniforms.modelViewProj = proj * modelView;
	
	float4x4 shadowProj = float4x4::orthoProjection(-1300.0f, 600.0f, 150.0f, 550.0f, 500.0f, 2500.0f);
	float4x4 shadowModelView = float4x4::rotationZ(-M_PI*0.24f) * float4x4::lookAt(float3(500.0f, 2000.0f, 500.0f), float3(0.0f, 0.0f, 0.0f), float3(0.0f, 1.0f, 0.0f));
	
	uniforms.shadowTransform[0] = shadowProj*shadowModelView;
	
	for (size_t i = 0; i < getShadowMapSizes().size(); ++i) {
		float4 v = uniforms.shadowTransform[i].inverse() * float4(0.0f, 0.0f, -1.0f, 0.0f);
		float3 l = normalize(float3(v.x, v.y, v.z));
		uniforms.light[i] = float4(l.x, l.y, l.z, 0.0);
	}
}
