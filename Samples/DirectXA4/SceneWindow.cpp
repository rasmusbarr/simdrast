//
//  SceneWindow.cpp
//  DirectXA4
//
//  Created by Rasmus Barringer on 2013-06-07.
//  Copyright (c) 2013 Rasmus Barringer. All rights reserved.
//

#include "SceneWindow.h"
#include "SponzaScene.h"
#include "ChessScene.h"

SceneWindow::SceneWindow(unsigned width, unsigned height) : Window(width, height, false) {
	sceneIndex = 0;
	rendererType = RENDERERTYPE_HW;
	scene = new SponzaScene();
	sceneRenderer = new SceneRenderer(rendererType, width, height, scene);
}

SceneWindow::~SceneWindow() {
	delete scene;
	delete sceneRenderer;
}

void SceneWindow::recreateRenderer() {
	delete sceneRenderer;
	sceneRenderer = 0;
	recreateDevice(rendererType == RENDERERTYPE_WARP);
	sceneRenderer = new SceneRenderer(rendererType, getWidth(), getHeight(), scene);
}

void SceneWindow::onRender() {
	sceneRenderer->render(this);
}

void SceneWindow::onKeyboard(char c) {
	if (c >= '1' && c <= '9') {
		unsigned i = c - '1';
		if (i < 2 && i != sceneIndex) {
			sceneIndex = i;

			delete sceneRenderer;
			delete scene;
			sceneRenderer = 0;
			scene = 0;

			switch (sceneIndex) {
			case 0:
				scene = new SponzaScene();
				break;
			case 1:
				scene = new ChessScene();
				break;
			}

			sceneRenderer = new SceneRenderer(rendererType, getWidth(), getHeight(), scene);
		}
	}
	else if (c == 'r') {
		rendererType = (RENDERERTYPE)((rendererType+1) % RENDERERTYPE_COUNT);
		recreateRenderer();
		return;
	}
	else if (c == 'm') {
		sceneRenderer->nextMode();
	}
	else if (c == 's') {
		scene->toggleAnimate();
	}
}
