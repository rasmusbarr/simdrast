//
//  SponzaScene.h
//  DirectXA4
//
//  Created by Rasmus Barringer on 2013-06-07.
//  Copyright (c) 2013 Rasmus Barringer. All rights reserved.
//

#ifndef DirectXA4_SponzaScene_h
#define DirectXA4_SponzaScene_h

#include "Scene.h"

class SponzaScene : public Scene {
private:
	float rot;

public:
	SponzaScene();

	virtual void updateUniforms(Window* window, float timeStep, SceneUniforms& uniforms);
};

#endif
