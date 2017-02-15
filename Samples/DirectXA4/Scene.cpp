//
//  Scene.cpp
//  DirectXA4
//
//  Created by Rasmus Barringer on 2013-06-07.
//  Copyright (c) 2013 Rasmus Barringer. All rights reserved.
//

#include "Scene.h"

Scene::Scene(const char* filename, const std::vector<srast::int2>& shadowMapSizes) : mesh(filename), shadowMapSizes(shadowMapSizes) {
	animate = true;
}

void Scene::toggleAnimate() {
	animate = !animate;
}

bool Scene::isAnimating() const {
	return animate;
}

fx::Mesh* Scene::getMesh() {
	return &mesh;
}

const std::vector<srast::int2>& Scene::getShadowMapSizes() const {
	return shadowMapSizes;
}

Scene::~Scene() {
}
