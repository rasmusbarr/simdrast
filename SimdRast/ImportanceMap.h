//
//  ImportanceMap.h
//  SimdRast
//
//  Created by Rasmus Barringer on 2012-10-22.
//  Copyright (c) 2012 Rasmus Barringer. All rights reserved.
//

#ifndef SimdRast_ImportanceMap_h
#define SimdRast_ImportanceMap_h

#include "SimdMath.h"
#include "ThreadPool.h"

namespace srast {

class ImportanceMap : public ThreadPoolTask {
private:
	unsigned maxLevel;
	unsigned width, height;
	unsigned char* pixels;

public:
	ImportanceMap();
	
	unsigned getMaxLevel() const {
		return maxLevel;
	}
	
	void resize(unsigned width, unsigned height);
	
	void clear() {
		fill(_mm_set1_epi8((char)(unsigned char)0xdf));
	}
	
	void fill() {
		fill(_mm_set1_epi8(0x0));
	}
	
	virtual void run(unsigned start, unsigned end, unsigned thread) {
		buildTileRow(start);
	}
	
	void build(ThreadPool& threadPool);
	
	void clear(unsigned x, unsigned y) {
		pixels[(y << maxLevel) + x] = 0xdf;
	}
	
	void clear(unsigned idx) {
		pixels[idx] = 0xdf;
	}

	void setTo(unsigned x, unsigned y, unsigned val) {
		pixels[(y << maxLevel) + x] = (unsigned char)val;
	}

	void set(unsigned x, unsigned y) {
		pixels[(y << maxLevel) + x] = 0;
	}
	
	void set(unsigned idx) {
		pixels[idx] = 0;
	}
	
	bool isSet(unsigned level, unsigned x, unsigned y) const {
		/*unsigned m = ~((1 << level) - 1); // We expect input coordinates to adhere to this.
		x &= m;
		y &= m;*/
		return pixels[(y << maxLevel) + x] <= level;
	}
	
	~ImportanceMap();

private:
	unsigned char sampleChild(unsigned idx, unsigned cx, unsigned cy) {
		unsigned childIdx = idx + (cy << maxLevel) + cx;
		return pixels[childIdx];
	}

	void buildFirstLevelsSimd(unsigned tileOffset);
	
	void buildTileRow(unsigned row);
	
	void fill(__m128i z);
};

}

#endif
