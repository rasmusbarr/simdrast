//
//  BinListArray.cpp
//  SimdRast
//
//  Created by Rasmus Barringer on 2012-10-31.
//  Copyright (c) 2012 Rasmus Barringer. All rights reserved.
//

#include "BinListArray.h"

namespace srast {

BinListArray::BinListArray(ThreadPool& threadPool) {
	width = 0;
	height = 0;
	zmax = 0;
	threadCount = threadPool.getThreadCount();
	threadBinListArrays = static_cast<BinList**>(simd_malloc(sizeof(BinList*) * threadCount, 64));
	std::fill(threadBinListArrays, threadBinListArrays + threadPool.getThreadCount(), static_cast<BinList*>(0));
}

void BinListArray::resize(unsigned width, unsigned height) {
	width = (width + (1<<tileSizeLog2)-1) >> tileSizeLog2;
	height = (height + (1<<tileSizeLog2)-1) >> tileSizeLog2;
	
	if (this->width == width && this->height == height)
		return;
	
	this->width = width;
	this->height = height;
	
	for (unsigned i = 0; i < threadCount; ++i) {
		if (threadBinListArrays[i])
			simd_free(threadBinListArrays[i]);
		
		threadBinListArrays[i] = static_cast<BinList*>(simd_malloc(sizeof(BinList)*width*height, 64));
	}

	if (zmax)
		simd_free(zmax);
	
	zmax = static_cast<unsigned*>(simd_malloc(sizeof(unsigned)*width*height, 64));
	
	clear();
}

void BinListArray::clear() {
	for (unsigned i = 0; i < threadCount; ++i)
		clear(i);
}

void BinListArray::clearZ() {
	float* dst = reinterpret_cast<float*>(zmax);
	unsigned size = width*height;
	
	__m128 z = _mm_set1_ps(SRAST_FAR_Z);
	
	for (unsigned i = 0; i < size; i += 16) {
		_mm_stream_ps(dst + i, z);
		_mm_stream_ps(dst + i + 4, z);
		_mm_stream_ps(dst + i + 8, z);
		_mm_stream_ps(dst + i + 12, z);
	}
}

void BinListArray::clear(unsigned thread) {
	float* dst = reinterpret_cast<float*>(threadBinListArrays[thread]);
	unsigned size = (sizeof(BinList)/sizeof(float))*width*height;
	
	__m128 z = _mm_setzero_ps();
	
	for (unsigned i = 0; i < size; i += 16) {
		_mm_stream_ps(dst + i, z);
		_mm_stream_ps(dst + i + 4, z);
		_mm_stream_ps(dst + i + 8, z);
		_mm_stream_ps(dst + i + 12, z);
	}
}

BinListArray::~BinListArray() {
	for (unsigned i = 0; i < threadCount; ++i) {
		if (threadBinListArrays[i])
			simd_free(threadBinListArrays[i]);
	}
	simd_free(threadBinListArrays);
	if (zmax)
		simd_free(zmax);
}
	
}
