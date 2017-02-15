//
//  SceneWindow.h
//  DirectXA4
//
//  Created by Rasmus Barringer on 2013-06-07.
//  Copyright (c) 2013 Rasmus Barringer. All rights reserved.
//

#ifndef DirectXA4_SceneWindow_h
#define DirectXA4_SceneWindow_h

#include "Scene.h"
#include "SceneRenderer.h"

class SceneWindow : public Window {
private:
	unsigned sceneIndex;
	RENDERERTYPE rendererType;
	Scene* scene;
	SceneRenderer* sceneRenderer;

public:
	SceneWindow(unsigned width, unsigned height);

	~SceneWindow();

private:
	void recreateRenderer();

	virtual void onRender();

	virtual void onKeyboard(char c);
};

#endif
