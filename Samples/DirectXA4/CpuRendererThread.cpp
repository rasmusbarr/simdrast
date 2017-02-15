//
//  CpuRendererThread.cpp
//  DirectXA4
//
//  Created by Rasmus Barringer on 2013-06-07.
//  Copyright (c) 2013 Rasmus Barringer. All rights reserved.
//

#include "CpuRendererThread.h"
#include "TransferTask.h"

CpuRendererThread::CpuRendererThread(const Delegate<srast::Renderer*>& renderScene, unsigned width, unsigned height) : renderScene(renderScene), width(width), height(height) {
	exit = false;
	running = false;
	frameBuffer = static_cast<unsigned*>(srast::simd_malloc(sizeof(unsigned)*width*height, 64));
}

unsigned* CpuRendererThread::getFrameBuffer() const {
	return frameBuffer;
}

void CpuRendererThread::begin() {
	stateMutex.enter();
	running = true;
	backEndEnabled = false;
	stateMutex.notifyAll();
	stateMutex.exit();
}

void CpuRendererThread::enableBackEnd() {
	stateMutex.enter();
	backEndEnabled = true;
	stateMutex.notifyAll();
	stateMutex.exit();
}

void CpuRendererThread::end() {
	stateMutex.enter();
	while (running)
		stateMutex.wait();
	stateMutex.exit();
}

void CpuRendererThread::join() {
	stateMutex.enter();
	exit = true;
	stateMutex.notifyAll();
	stateMutex.exit();
	srast::Thread::join();
}

void CpuRendererThread::finish(DraRenderTarget* dra) {
	DraSurface surface = dra->map(0);
	
	srast::ThreadPool& threadPool = renderer.getThreadPool();
	threadPool.barrier();
	
	unsigned tileWidth = (renderer.getFrameBufferWidth() + (1 << srast::tileSizeLog2)-1) >> srast::tileSizeLog2;
	unsigned tileHeight = (renderer.getFrameBufferHeight() + (1 << srast::tileSizeLog2)-1) >> srast::tileSizeLog2;
	
	TransferTask<DraSurface> task(surface, renderer, tileWidth, tileHeight);
	threadPool.startTask(&task, tileWidth*tileHeight, 8);
	threadPool.barrier();
	
	dra->unmap(0);
	renderer.finish();
}

CpuRendererThread::~CpuRendererThread() {
	srast::simd_free(frameBuffer);
}

void* CpuRendererThread::run() {
	for (;;) {
		stateMutex.enter();
		while (!running && !exit)
			stateMutex.wait();
		stateMutex.exit();

		if (exit)
			return 0;

		renderer.bindFrameBuffer(srast::FRAMEBUFFERFORMAT_RGBA8, frameBuffer, width, height, width);

		renderScene(&renderer);
		renderer.beginFrontEndShadeAndHimRast();
		renderer.beginFrontEndBin();

		stateMutex.enter();
		while (!backEndEnabled)
			stateMutex.wait();
		stateMutex.exit();

		renderer.beginBackEnd();

		stateMutex.enter();
		running = false;
		stateMutex.notifyAll();
		stateMutex.exit();
	}
	return 0;
}
