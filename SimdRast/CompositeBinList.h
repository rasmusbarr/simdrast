//
//  CompositeBinList.h
//  SimdRast
//
//  Created by Rasmus Barringer on 2012-11-02.
//  Copyright (c) 2012 Rasmus Barringer. All rights reserved.
//

#ifndef SimdRast_CompositeBinList_h
#define SimdRast_CompositeBinList_h

#include "SimdMath.h"
#include "ThreadPool.h"
#include "BinListArray.h"

namespace srast {

struct CompositeBinList {
private:
	unsigned binCount;
	unsigned z;
	BinList::Reader* sortedBins;
	unsigned char padding[64-sizeof(unsigned)-sizeof(unsigned)-sizeof(BinList::Reader*)];
	
public:
	CompositeBinList(ThreadPool& threadPool) {
		binCount = threadPool.getThreadCount();
		sortedBins = static_cast<BinList::Reader*>(simd_malloc(sizeof(BinList::Reader)*binCount, 64));
	}
	
	SRAST_FORCEINLINE unsigned setup(PoolAllocator& poolAllocator, BinListArray& binArray, unsigned x, unsigned y, unsigned frameNumber) {
		unsigned tileZmax = binArray.finalize(sortedBins, poolAllocator, x, y, frameNumber);

		for (int i = binCount-2; i >= 0; --i)
			maintainOrder(i);

		return tileZmax;
	}
	
	SRAST_FORCEINLINE unsigned next() {
		unsigned idx = sortedBins[0].index();
		
		if ((idx & 0x80000000) == 0) {
			z = sortedBins[0].readDepth();
			sortedBins[0].readIndex();
			maintainOrder(0);
		}
		else {
			if (idx == 0x8fffffff)
				return idx;
			
			unsigned binsActivated = 1;
			
			sortedBins[0].readIndex();
			
			for (; binsActivated < binCount; ++binsActivated) {
				if (sortedBins[binsActivated].index() != idx)
					break;
				
				sortedBins[binsActivated].readIndex();
			}
			
			for (int i = (int)binsActivated-2; i >= 0; --i)
				maintainOrder(i);
		}
		return idx;
	}
	
	SRAST_FORCEINLINE unsigned depth() const {
		return z;
	}
	
	~CompositeBinList() {
		simd_free(sortedBins);
	}
	
private:
	SRAST_FORCEINLINE void maintainOrder(unsigned i) const {
		while (i+1 < binCount && sortedBins[i+1].index() < sortedBins[i].index()) {
			std::swap(sortedBins[i], sortedBins[i+1]);
			++i;
		}
	}
};

}

#endif
