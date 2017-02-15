//
//  main.cpp
//  DirectXA4
//
//  Created by Rasmus Barringer on 2013-06-07.
//  Copyright (c) 2013 Rasmus Barringer. All rights reserved.
//

#include "SceneWindow.h"

static const unsigned windowWidth = 1024;
static const unsigned windowHeight = 768;

int main() {
	std::cout << "using " << srast::ThreadPool::cpuCount() << " cpu cores." << std::endl;
	
#ifdef SRAST_AVX
	std::cout << "using AVX instructions." << std::endl;
#else
	std::cout << "using SSE instructions." << std::endl;
#endif

	SceneWindow sceneWindow(windowWidth, windowHeight);
	sceneWindow.enterLoop();
	return 0;
}
