//
//  DraTexture.cpp
//  DirectXA4
//
//  Created by Rasmus Barringer on 2013-06-07.
//  Copyright (c) 2013 Rasmus Barringer. All rights reserved.
//

#include "DraTexture.h"

static unsigned log2(unsigned x) {
	unsigned r = (x > 0xffff) << 4;
	x >>= r;
	
	unsigned shift = (x > 0xff) << 3;
	
	x >>= shift;
	r |= shift;
	
	shift = (x > 0xf) << 2;
	
	x >>= shift;
	r |= shift;
	
	shift = (x > 0x3) << 1;
	
	x >>= shift;
	r |= shift;
	
	r |= (x >> 1);
	return r;
}

DraTexture::DraTexture(unsigned width, unsigned height) {
	widthLog2 = log2(width);
	heightLog2 = log2(height);

	if ((1 << widthLog2) != width || (1 << heightLog2) != height)
		throw std::runtime_error("dra texture is not power of 2");

	localCopy.base = 0;
}

void DraTexture::setup(DraSurface surface) {
	this->surface = surface;

	unsigned maxTileOffset = (1 << (widthLog2 + 7)) + (surface.pitch << (heightLog2 - 5));
	unsigned tileCount = maxTileOffset >> 7;

	if (!localCopy.base) {
		localCopy.pitch = surface.pitch;
		localCopy.base = srast::simd_malloc(surface.pitch << (heightLog2-5), 64);
		fetchedTiles = static_cast<unsigned char*>(srast::simd_malloc(tileCount, 64));
	}

	__m128 zero = _mm_setzero_ps();

	for (unsigned i = 0; i < tileCount; i += 16)
		_mm_store_ps((float*)(fetchedTiles + i), zero);
}

DraTexture::~DraTexture() {
	if (localCopy.base) {
		srast::simd_free(localCopy.base);
		srast::simd_free(fetchedTiles);
	}
}
