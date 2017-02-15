//
//  PoolAllocator.cpp
//  SimdRast
//
//  Created by Rasmus Barringer on 2012-10-20.
//  Copyright (c) 2012 Rasmus Barringer. All rights reserved.
//

#include "Config.h"
#include "PoolAllocator.h"
#include "Atomics.h"
#include <cstring>
#include <cassert>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/mman.h>
#endif

namespace srast {

inline unsigned align(unsigned x, unsigned boundary) {
	unsigned alignMask = boundary-1;
	return (x+alignMask) & (~alignMask);
}

PoolAllocator::PoolAllocator(unsigned size) {
	offset = 0;
	this->size = align(size, 64);
	
#ifdef _WIN32
	start = VirtualAlloc(0, this->size,  MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	assert(start);
#else
	start = mmap(0, this->size, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANON, 0, 0);
	assert(start != MAP_FAILED);
#endif
}

void PoolAllocator::reset() {
	offset = 0;
}

void* PoolAllocator::allocate(unsigned size) {
	size = align(size, 64);
	
	unsigned newOffset = Atomics::add(&offset, size);
	assert(newOffset < this->size);
	
	return static_cast<char*>(start) + (newOffset-size);
}

void* PoolAllocator::clone(const void* source, unsigned size) {
	if (!size)
		return 0;
	
	void* dest = allocate(size);
	std::memcpy(dest, source, size);
	return dest;
}

PoolAllocator::~PoolAllocator() {
#ifdef _WIN32
	VirtualFree(start, 0, MEM_RELEASE);
#else
	munmap(start, size);
#endif
}

}
