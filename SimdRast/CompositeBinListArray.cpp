//
//  CompositeBinListArray.cpp
//  SimdRast
//
//  Created by Rasmus Barringer on 2012-11-02.
//  Copyright (c) 2012 Rasmus Barringer. All rights reserved.
//

#include "CompositeBinListArray.h"
#include <new>

namespace srast {

CompositeBinListArray::CompositeBinListArray(ThreadPool& threadPool) {
	binCount = threadPool.getThreadCount();
	array = static_cast<CompositeBinList*>(simd_malloc(sizeof(CompositeBinList)*binCount, 64));
	
	for (unsigned i = 0; i < binCount; ++i)
		new (&array[i]) CompositeBinList(threadPool);
}

CompositeBinList& CompositeBinListArray::operator [] (unsigned index) const {
	return array[index];
}

CompositeBinListArray::~CompositeBinListArray() {
	for (unsigned i = 0; i < binCount; ++i)
		array[i].~CompositeBinList();
	
	simd_free(array);
}

}
