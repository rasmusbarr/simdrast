//
//  ThreadLocalAllocator.h
//  SimdRast
//
//  Created by Rasmus Barringer on 2012-10-31.
//  Copyright (c) 2012 Rasmus Barringer. All rights reserved.
//

#ifndef SimdRast_ThreadLocalAllocator_h
#define SimdRast_ThreadLocalAllocator_h

#include "PoolAllocator.h"

namespace srast {

class ThreadLocalAllocator {
private:
	static const unsigned blockSize = 16*1024;
	
	PoolAllocator& poolAllocator;
	void* start;
	char* currentBlock;
	unsigned currentBlockSize;
	
public:
	ThreadLocalAllocator(PoolAllocator& poolAllocator) : poolAllocator(poolAllocator) {
		start = poolAllocator.basePointer<void>();
		currentBlock = 0;
		currentBlockSize = 0;
	}
	
	template<class T>
	T* basePointer() const {
		return static_cast<T*>(start);
	}
	
	void reset() {
		currentBlock = 0;
		currentBlockSize = 0;
	}
	
	void* allocate(unsigned size) {
		if (currentBlockSize < size) {
			unsigned s = align(size, blockSize);
			currentBlock = static_cast<char*>(poolAllocator.allocate(s));
			currentBlockSize = s;
		}
		unsigned alignedSize = align(size, 64);
		void* p = currentBlock;
		currentBlock += alignedSize;
		currentBlockSize -= alignedSize;
		return p;
	}
	
	void* allocateTemporary(unsigned size) {
		if (currentBlockSize < size) {
			unsigned s = align(size, blockSize);
			currentBlock = static_cast<char*>(poolAllocator.allocate(s));
			currentBlockSize = s;
		}
		return currentBlock;
	}
	
private:
	static unsigned align(unsigned x, unsigned boundary) {
		unsigned alignMask = boundary-1;
		return (x+alignMask) & (~alignMask);
	}
};

}

#endif
