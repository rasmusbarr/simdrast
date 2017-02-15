//
//  BinList.h
//  SimdRast
//
//  Created by Rasmus Barringer on 2012-10-30.
//  Copyright (c) 2012 Rasmus Barringer. All rights reserved.
//

#ifndef SimdRast_BinList_h
#define SimdRast_BinList_h

#include "Config.h"
#include "ThreadLocalAllocator.h"

namespace srast {

struct BinList {
	class Reader {
	private:
		unsigned* block;
		unsigned idx;
		
	public:
		SRAST_FORCEINLINE Reader() : block(0) {
			idx = 0x8fffffff;
		}
		
		SRAST_FORCEINLINE Reader(unsigned* block) : block(block) {
			idx = *(this->block++);
		}
		
		SRAST_FORCEINLINE unsigned index() const {
			return idx;
		}
		
		SRAST_FORCEINLINE void readIndex() {
			idx = *(block++);
			if (idx & 0x40000000) {
				block += idx & (~0x40000000);
				idx = *(block-1);
			}
		}
		
		SRAST_FORCEINLINE unsigned readDepth() {
			return *(block++);
		}
	};
	
	class Writer {
		friend struct BinList;
		
	private:
		unsigned* block;

	public:
		SRAST_FORCEINLINE Writer(unsigned* block) : block(block) {
		}
		
		SRAST_FORCEINLINE void writeTriangle(unsigned idx, unsigned z) {
			*(block++) = idx;
			*(block++) = z;
		}
	};

	unsigned frameNumber;
	unsigned firstBlockOffset;
	unsigned currentBlockOffset;
	unsigned short currentSize;
	unsigned short currentDrawCall;

	SRAST_FORCEINLINE Reader finalize(PoolAllocator& poolAllocator) const {
		unsigned* start = poolAllocator.basePointer<unsigned>();
		*(start + currentBlockOffset) = 0x8fffffff;
		return start + firstBlockOffset;
	}

	SRAST_FORCEINLINE void reset() {
		currentDrawCall = 0;
		firstBlockOffset = 0;
		currentBlockOffset = 0;
		currentSize = 0;
	}
	
	SRAST_FORCEINLINE Writer startWrite(unsigned drawCall, ThreadLocalAllocator& localAllocator) {
		unsigned* start = localAllocator.basePointer<unsigned>();
		
		// Reserve space for a SIMD width of triangles, draw call, and end-of-block marker.
		if (currentSize < 2+simd_float::width*2) {
			static const unsigned blockSize = 128 - 1;
			
			unsigned prevBlockOffset = currentBlockOffset;
			unsigned* prevBlock = start + prevBlockOffset;
			
			unsigned* currentBlock = static_cast<unsigned*>(localAllocator.allocate(sizeof(unsigned)*blockSize + sizeof(unsigned)));
			
			currentSize = blockSize;
			currentBlockOffset = (unsigned)(currentBlock - start);
			
			if (prevBlockOffset)
				*prevBlock = (unsigned)(currentBlock - prevBlock) | 0x40000000;
			else
				firstBlockOffset = currentBlockOffset;
		}
		
		unsigned* block = start + currentBlockOffset;
		
		if (currentDrawCall != drawCall || currentBlockOffset == firstBlockOffset)
			*(block++) = drawCall | 0x80000000;
		
		return block;
	}
	
	SRAST_FORCEINLINE void endWrite(Writer& writer, ThreadLocalAllocator& localAllocator) {
		unsigned* start = localAllocator.basePointer<unsigned>();
		unsigned* block = start + currentBlockOffset;
		
		unsigned written = (unsigned)(writer.block - block);
		
		if (written > 1) {
			if (written & 1)
				currentDrawCall = (unsigned short)(*block & (~0x80000000));
			
			currentBlockOffset += written;
			currentSize -= written;
		}
	}
};

}

#endif
