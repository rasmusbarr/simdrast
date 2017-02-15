//
//  CpuRendererThread.h
//  DirectXA4
//
//  Created by Rasmus Barringer on 2013-06-07.
//  Copyright (c) 2013 Rasmus Barringer. All rights reserved.
//

#ifndef DirectXA4_CpuRendererThread_h
#define DirectXA4_CpuRendererThread_h

#include "../../SimdRast/Renderer.h"
#include "DraRenderTarget.h"
#include "DxContext.h"
#include "Delegate.h"

class CpuRendererThread : public srast::Thread {
private:
	bool exit, running, backEndEnabled;

	srast::Mutex stateMutex;
	srast::Renderer renderer;
	Delegate<srast::Renderer*> renderScene;

	unsigned* frameBuffer;
	unsigned width, height;

public:
	CpuRendererThread(const Delegate<srast::Renderer*>& renderScene, unsigned width, unsigned height);

	unsigned* getFrameBuffer() const;

	void begin();

	void enableBackEnd();

	void end();

	void join();

	void finish(DraRenderTarget* dra);

	~CpuRendererThread();

protected:
	virtual void* run();
};

#endif
