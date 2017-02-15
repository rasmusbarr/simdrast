//
//  Renderer.h
//  SimdRast
//
//  Created by Rasmus Barringer on 2012-10-19.
//  Copyright (c) 2012 Rasmus Barringer. All rights reserved.
//

#ifndef SimdRast_Renderer_h
#define SimdRast_Renderer_h

#include "DrawCall.h"
#include "ThreadPool.h"
#include "ImportanceMap.h"
#include "BinListArray.h"
#include "PoolAllocator.h"
#include "CompositeBinListArray.h"
#include "ThreadLocalAllocatorArray.h"
#include <map>
#include <vector>

namespace srast {

enum SHADERKIND {
	SHADERKIND_VERT = 0,
	SHADERKIND_ATTR,
	SHADERKIND_FRAG
};

enum FRAMEBUFFERFORMAT {
	FRAMEBUFFERFORMAT_RGBA8 = 0,
};

class Renderer {
	template<class T>
	friend class TriangleSetupTask;
	
	friend class BinTask;
	
	template<class ZMode, bool Opaque>
	friend void binDrawCallInMode(Renderer& r, DrawCall& drawCall, unsigned start, unsigned end, int maxLevel, unsigned thread);

	friend void resolveTile(Renderer& r, unsigned x, unsigned y, unsigned thread);
	
private:
	ThreadPool threadPool;
	PoolAllocator poolAllocator;
	
	ImportanceMap importanceMap;
	BinListArray binListArray;
	CompositeBinListArray compositeBinListArray;
	ThreadLocalAllocatorArray localAllocators;
	
	bool dense;
	bool transparentImportance;
	
	unsigned clearColor;
	unsigned frameNumber;

	void* frameBuffer;
	FRAMEBUFFERFORMAT frameBufferFormat;
	unsigned frameBufferWidth, frameBufferHeight, frameBufferPitch;
	unsigned frameBufferSizeLog2;
	
	DrawCall currentDrawCall;
	
	Shader* currentShader[3];
	const void* currentUniforms[3];

	std::map<void*, unsigned*> adjacencyBuffers;
	std::vector<DrawCall> drawCalls;
	
	void (*rasterizeDrawCallToHim)(Renderer& r, DrawCall& drawCall);

public:
	Renderer();
	
	ThreadPool& getThreadPool() {
		return threadPool;
	}
	
	PoolAllocator& getPoolAllocator() {
		return poolAllocator;
	}
	
	ImportanceMap& getImportanceMap() {
		return importanceMap;
	}
	
	unsigned getFrameBufferWidth() const {
		return frameBufferWidth;
	}
	
	unsigned getFrameBufferHeight() const {
		return frameBufferHeight;
	}
	
	unsigned getFrameBufferPitch() const {
		return frameBufferPitch;
	}

	void* getFrameBuffer() const {
		return frameBuffer;
	}
	
	bool hasTransparentImportance() const {
		return transparentImportance;
	}
	
	VertexRenderState& getVertexRenderState();
	
	FragmentRenderState& getFragmentRenderState();
	
	void setClearColor(unsigned color);

	void forceDense();
	
	void forceTransparentImportance();
	
	void setupHimRasterization(void (*rasterizeDrawCallToHim)(Renderer& r, DrawCall& drawCall));
	
	void bindFrameBuffer(FRAMEBUFFERFORMAT format, void* frameBuffer, unsigned width, unsigned height, unsigned pitch);
	
	void bindIndexBuffer(void* indexBuffer, unsigned offset, unsigned size, unsigned count);
	
	void bindVertexBuffer(void* vertexBuffer, unsigned offset, unsigned stride, unsigned count);
	
	void bindAttributeBuffer(void* vertexBuffer, unsigned offset, unsigned stride, unsigned count);
	
	void invalidateIndexBuffer(void* indexBuffer); // Reset adjacency buffer.
	
	void bindShader(SHADERKIND shaderKind, Shader* shader, const void* uniforms, unsigned uniformSize);
	
	void drawList();
	
	void drawIndexed();
	
	void beginFrontEndShadeAndHimRast();

	void beginFrontEndBin();

	void beginBackEnd();

	void finish();
	
	void reset();
	
	~Renderer();
	
private:
	unsigned* generateAdjacencyBuffer(const void* indices, unsigned stride, unsigned count);
	
	void binDrawCall(DrawCall& drawCall);
	
	void resolveTiles();
	
	void setupShaders(DrawCall& drawCall);
};

}

#endif
