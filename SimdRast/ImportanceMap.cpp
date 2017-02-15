//
//  ImportanceMap.cpp
//  SimdRast
//
//  Created by Rasmus Barringer on 2012-10-22.
//  Copyright (c) 2012 Rasmus Barringer. All rights reserved.
//

#include "ImportanceMap.h"

namespace srast {

ImportanceMap::ImportanceMap() {
	maxLevel = 0;
	pixels = 0;
}

void ImportanceMap::resize(unsigned width, unsigned height) {
	unsigned oldMaxLevel = maxLevel;
	maxLevel = 0;
	
	while (width >> maxLevel || height >> maxLevel)
		++maxLevel;
	
	if (oldMaxLevel != maxLevel) {
		if (pixels)
			simd_free(pixels);
		
		unsigned size2 = 1 << (maxLevel + maxLevel);
		pixels = static_cast<unsigned char*>(simd_malloc(size2, 64));
	}
	
	this->width = width;
	this->height = height;
	
	clear();
}

void ImportanceMap::build(ThreadPool& threadPool) {
	threadPool.startTask(this, (height + 127) >> 7, 1);
	threadPool.barrier();
	
	for (unsigned l = 8; l <= maxLevel; ++l) {
		unsigned stride = 1 << l;
		
		for (unsigned y = 0; y < height; y += stride) {
			for (unsigned x = 0; x < width; x += stride) {
				unsigned halfStride = stride >> 1;

				unsigned idx = (y << maxLevel) + x;
				unsigned char value = pixels[idx];
				
				if (value > l && (sampleChild(idx, halfStride, 0)+1u <= l ||
								  sampleChild(idx, 0, halfStride)+1u <= l ||
								  sampleChild(idx, halfStride, halfStride)+1u <= l))
					pixels[idx] = (unsigned char)l;
			}
		}
	}
}

ImportanceMap::~ImportanceMap() {
	if (pixels)
		simd_free(pixels);
}

#define REDUCE2(s0, s1, r0)\
r0 = _mm_max_epu8(s0, incr1);\
s1 = _mm_max_epu8(s1, incr0);\
r0 = _mm_min_epu8(r0, s1);\
r0 = _mm_min_epu8(r0, _mm_srli_si128(r0, 1));\
r0 = _mm_blendv_epi8(r0, s0, mask1);

#define REDUCE4(s0, s1, s2, s3, r0, r2)\
REDUCE2(s0, s1, r0)\
REDUCE2(s2, s3, r2)\
s0 = r0;\
s1 = r2;\
r0 = _mm_max_epu8(s0, incr3);\
s1 = _mm_max_epu8(s1, incr2);\
r0 = _mm_min_epu8(r0, s1);\
r0 = _mm_min_epu8(r0, _mm_srli_si128(r0, 2));\
r0 = _mm_blendv_epi8(r0, s0, mask2);

void ImportanceMap::buildFirstLevelsSimd(unsigned tileOffset) {
	__m128i incr0 = _mm_set1_epi16(0x0101);
	__m128i incr1 = _mm_set1_epi16(0x0100);
	
	__m128i incr2 = _mm_set1_epi32(0x00020002);
	__m128i incr3 = _mm_set1_epi32(0x00020000);
	
	__m128i incr4 = _mm_setr_epi32(0x00000003, 0x00000003, 0x00000003, 0x00000003);
	__m128i incr5 = _mm_setr_epi32(0x00000000, 0x00000003, 0x00000000, 0x00000003);
	
	__m128i mask1 = _mm_set1_epi16((short)(unsigned short)0xff00);
	__m128i mask2 = _mm_set1_epi32(0xffffff00);
	__m128i mask3 = _mm_setr_epi32(0xffffff00, 0xffffffff, 0xffffff00, 0xffffffff);
	
	for (unsigned y = 0; y < 128; y += 8) {
		unsigned char* row0 = pixels + tileOffset + (y << maxLevel);
		unsigned char* row1 = row0 + (unsigned)(1 << maxLevel);
		unsigned char* row2 = row0 + (unsigned)(2 << maxLevel);
		unsigned char* row3 = row0 + (unsigned)(3 << maxLevel);
		unsigned char* row4 = row0 + (unsigned)(4 << maxLevel);
		unsigned char* row5 = row0 + (unsigned)(5 << maxLevel);
		unsigned char* row6 = row0 + (unsigned)(6 << maxLevel);
		unsigned char* row7 = row0 + (unsigned)(7 << maxLevel);
		
		for (unsigned x = 0; x < 128; x += 16) {
			__m128i d0, d1, d2, d3;

			__m128i s0 = _mm_load_si128(reinterpret_cast<const __m128i*>(row0 + x));
			__m128i s1 = _mm_load_si128(reinterpret_cast<const __m128i*>(row1 + x));
			__m128i s2 = _mm_load_si128(reinterpret_cast<const __m128i*>(row2 + x));
			__m128i s3 = _mm_load_si128(reinterpret_cast<const __m128i*>(row3 + x));
			
			REDUCE4(s0, s1, s2, s3, d0, d1);
			
			__m128i s4 = _mm_load_si128(reinterpret_cast<const __m128i*>(row4 + x));
			__m128i s5 = _mm_load_si128(reinterpret_cast<const __m128i*>(row5 + x));
			__m128i s6 = _mm_load_si128(reinterpret_cast<const __m128i*>(row6 + x));
			__m128i s7 = _mm_load_si128(reinterpret_cast<const __m128i*>(row7 + x));
			
			REDUCE4(s4, s5, s6, s7, d2, d3);
			
			s0 = d0;
			s1 = d2;
			
			__m128i r0 = _mm_max_epu8(s0, incr5);
			s1 = _mm_max_epu8(s1, incr4);
			r0 = _mm_min_epu8(r0, s1);
			r0 = _mm_min_epu8(r0, _mm_srli_si128(r0, 4));
			r0 = _mm_blendv_epi8(r0, s0, mask3);
			
			d0 = r0;
			
			_mm_store_si128(reinterpret_cast<__m128i*>(row0 + x), d0);
			_mm_store_si128(reinterpret_cast<__m128i*>(row2 + x), d1);
			_mm_store_si128(reinterpret_cast<__m128i*>(row4 + x), d2);
			_mm_store_si128(reinterpret_cast<__m128i*>(row6 + x), d3);
		}
	}
}

void ImportanceMap::buildTileRow(unsigned row) {
	unsigned ty = row << 7;
	
	for (unsigned tx = 0; tx < width; tx += 128) {
		unsigned tileOffset = (ty << maxLevel) + tx;
		buildFirstLevelsSimd(tileOffset);
		
		for (unsigned l = 4; l <= 7 && l <= maxLevel; ++l) {
			unsigned stride = 1 << l;
			
			for (unsigned y = 0; y < 128; y += stride) {
				for (unsigned x = 0; x < 128; x += stride) {
					unsigned halfStride = stride >> 1;

					unsigned idx = (y << maxLevel) + x + tileOffset;
					unsigned char value = pixels[idx];
					
					if (value > l && (sampleChild(idx, halfStride, 0)+1u <= l ||
									  sampleChild(idx, 0, halfStride)+1u <= l ||
									  sampleChild(idx, halfStride, halfStride)+1u <= l))
						pixels[idx] = (unsigned char)l;
				}
			}
		}
	}
}

void ImportanceMap::fill(__m128i z) {
	unsigned size2 = 1 << (maxLevel + maxLevel);
	unsigned char* maxPixels = pixels + size2;
	
	unsigned char* p = pixels;
	
	while (p < maxPixels) {
		_mm_stream_si128(reinterpret_cast<__m128i*>(p), z);
		_mm_stream_si128(reinterpret_cast<__m128i*>(p)+1, z);
		_mm_stream_si128(reinterpret_cast<__m128i*>(p)+2, z);
		_mm_stream_si128(reinterpret_cast<__m128i*>(p)+3, z);
		p += sizeof(float)*16;
	}
}

}
