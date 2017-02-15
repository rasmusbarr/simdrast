//
//  BinListArray.h
//  SimdRast
//
//  Created by Rasmus Barringer on 2012-10-31.
//  Copyright (c) 2012 Rasmus Barringer. All rights reserved.
//

#ifndef SimdRast_BinListArray_h
#define SimdRast_BinListArray_h

#include "ZMode.h"
#include "BinList.h"
#include "SimdMath.h"
#include "ThreadPool.h"

namespace srast {

static const int tileSizeLog2 = 3;

class BinListArray {
private:
	unsigned threadCount;
	BinList** threadBinListArrays;
	unsigned width, height;
	unsigned* zmax;

public:
	BinListArray(ThreadPool& threadPool);
	
	void resize(unsigned width, unsigned height);

	SRAST_FORCEINLINE BinList* operator () (unsigned x, unsigned y, unsigned thread, unsigned frameNumber, unsigned*& zmax) const {
		x >>= tileSizeLog2;
		y >>= tileSizeLog2;

		unsigned idx = y*width + x;
		
		BinList* list = threadBinListArrays[thread] + idx;
		zmax = this->zmax + idx;

		if (list->frameNumber != frameNumber) {
			list->frameNumber = frameNumber;
			list->reset();
		}
		return list;
	}
	
	SRAST_FORCEINLINE unsigned finalize(BinList::Reader* readers, PoolAllocator& poolAllocator, unsigned x, unsigned y, unsigned frameNumber) const {
		x >>= tileSizeLog2;
		y >>= tileSizeLog2;
		
		unsigned idx = y*width + x;
		
		for (unsigned thread = 0; thread < threadCount; ++thread) {
			const BinList& list = threadBinListArrays[thread][idx];
			readers[thread] = list.frameNumber != frameNumber ? BinList::Reader() : list.finalize(poolAllocator);
		}

		return zmax[idx];
	}
	
	void clear();

	void clearZ();

	~BinListArray();
	
private:
	void clear(unsigned thread);
};

}

#endif
