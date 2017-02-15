//
//  ThreadLocalAllocatorArray.h
//  SimdRast
//
//  Created by Rasmus Barringer on 2012-10-31.
//  Copyright (c) 2012 Rasmus Barringer. All rights reserved.
//

#ifndef SimdRast_ThreadLocalAllocatorArray_h
#define SimdRast_ThreadLocalAllocatorArray_h

#include "ThreadLocalAllocator.h"
#include "PoolAllocator.h"
#include "ThreadPool.h"
#include <new>

namespace srast {

class ThreadLocalAllocatorArray {
private:
	unsigned threadCount;
	ThreadLocalAllocator** array;
	
public:
	ThreadLocalAllocatorArray(PoolAllocator& poolAllocator, ThreadPool& threadPool) {
		threadCount = threadPool.getThreadCount();
		array = static_cast<ThreadLocalAllocator**>(simd_malloc(sizeof(ThreadLocalAllocator*)*threadCount, 64));
		
		for (unsigned i = 0; i < threadCount; ++i) {
			array[i] = static_cast<ThreadLocalAllocator*>(simd_malloc(sizeof(ThreadLocalAllocator), 64));
			new (array[i]) ThreadLocalAllocator(poolAllocator);
		}
	}
	
	void reset() {
		for (unsigned i = 0; i < threadCount; ++i)
			array[i]->reset();
	}
	
	ThreadLocalAllocator* operator [] (unsigned index) const {
		return array[index];
	}
	
	~ThreadLocalAllocatorArray() {
		for (unsigned i = 0; i < threadCount; ++i) {
			array[i]->~ThreadLocalAllocator();
			simd_free(array[i]);
		}
		
		simd_free(array);
	}
};

}

#endif
