//
//  DrawCall.h
//  SimdRast
//
//  Created by Rasmus Barringer on 2012-10-19.
//  Copyright (c) 2012 Rasmus Barringer. All rights reserved.
//

#ifndef SimdRast_DrawCall_h
#define SimdRast_DrawCall_h

#include "VectorMath.h"
#include "ThreadPool.h"
#include "VertexRenderState.h"
#include "FragmentRenderState.h"

namespace srast {

enum {
	FACEFLAG_BACK = 1,
	FACEFLAG_VALID = 1 << 1,
	FACEFLAG_BACK_BEFORE_SNAP = 1 << 2,
	FACEFLAG_IMPORTANT = 1 << 3,
};

struct DrawBuffer {
	void* data;
	unsigned stride, count;
	
	DrawBuffer() {
		clear();
	}
	
	void clear() {
		data = 0;
		stride = 0;
		count = 0;
	}
};

struct DrawCall {
	VertexRenderState vertexRenderState;
	FragmentRenderState fragmentRenderState;
	RenderState attributeRenderState;
	
	DrawBuffer vertexBuffer;
	DrawBuffer attributeBuffer;
	DrawBuffer indexBuffer;

	float4* shadedPositions;
	float4* edges;
	double* highpEdgeZ0;
	double* highpEdgeZ1;
	double* highpEdgeZ2;
	unsigned* adjacency;
	unsigned char* flags;

	ThreadPoolTask* task;
};

}

#endif
