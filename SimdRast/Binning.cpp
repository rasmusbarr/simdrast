//
//  Binning.cpp
//  SimdRast
//
//  Created by Rasmus Barringer on 2012-10-26.
//  Copyright (c) 2012 Rasmus Barringer. All rights reserved.
//

#include "Binning.h"
#include "SimdMath.h"
#include "ZMode.h"

namespace srast {

inline vec4<simd4_float> decodeBoundingBox(const __m128i& bbXY, const __m128i& bbZW) {
	__m128i mask = _mm_set1_epi32(0xffff);

	vec4<simd4_float> bb;
		
	bb.x = _mm_cvtepi32_ps(_mm_and_si128(bbXY, mask));
	bb.y = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_si128(bbXY, 2), mask));
	bb.z = _mm_cvtepi32_ps(_mm_and_si128(bbZW, mask));
	bb.w = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_si128(bbZW, 2), mask));

	return bb;
}

#define GATHER_TRIANGLE_FF(m0, m1, m2, m3, idx) \
	__m128 m0, m1, m2, m3 = _mm_setzero_ps();\
while (i < end) {\
if (*(flags++) & FACEFLAG_BACK) {\
++i;\
continue;\
}\
m0 = _mm_load_ps(&edges[i*4+0].x);\
m1 = _mm_load_ps(&edges[i*4+1].x);\
m2 = _mm_load_ps(&edges[i*4+2].x);\
m3 = _mm_load_ps(&edges[i*4+3].x);\
laneMask += laneMask + 1;\
triangleIndex[idx] = i;\
++i;\
break;\
}

#define GATHER_TRIANGLE_FF_LO(m0, m1, m2, m3, idx) \
__m256 m0, m1, m2, m3 = _mm256_setzero_ps();\
while (i < end) {\
if (*(flags++) & FACEFLAG_BACK) {\
++i;\
continue;\
}\
m0 = _mm256_castps128_ps256(_mm_load_ps(&edges[i*4+0].x));\
m1 = _mm256_castps128_ps256(_mm_load_ps(&edges[i*4+1].x));\
m2 = _mm256_castps128_ps256(_mm_load_ps(&edges[i*4+2].x));\
m3 = _mm256_castps128_ps256(_mm_load_ps(&edges[i*4+3].x));\
laneMask += laneMask + 1;\
triangleIndex[idx] = i;\
++i;\
break;\
}

#define GATHER_TRIANGLE_FF_HI(m0, m1, m2, m3, idx) \
while (i < end) {\
if (*(flags++) & FACEFLAG_BACK) {\
++i;\
continue;\
}\
m0 = _mm256_insertf128_ps(m0, _mm_load_ps(&edges[i*4+0].x), 1);\
m1 = _mm256_insertf128_ps(m1, _mm_load_ps(&edges[i*4+1].x), 1);\
m2 = _mm256_insertf128_ps(m2, _mm_load_ps(&edges[i*4+2].x), 1);\
m3 = _mm256_insertf128_ps(m3, _mm_load_ps(&edges[i*4+3].x), 1);\
laneMask += laneMask + 1;\
triangleIndex[idx] = i;\
++i;\
break;\
}

template<class ZMode, bool Opaque>
void binDrawCallInMode(Renderer& r, DrawCall& drawCall, unsigned start, unsigned end, int maxLevel, unsigned thread) {
	float4* __restrict edges = drawCall.edges;
	unsigned char* __restrict flags = drawCall.flags + start;
	
	const ImportanceMap& importanceMap = r.importanceMap;
	BinListArray& binListArray = r.binListArray;
	ThreadLocalAllocator& localAllocator = *r.localAllocators[thread];
	
	unsigned drawCallIdx = (unsigned)(&drawCall - &r.drawCalls[0]);
	
	unsigned width = r.frameBufferWidth;
	unsigned height = r.frameBufferHeight;
	unsigned frameNumber = r.frameNumber;
	
	for (unsigned i = start; i < end; ) {
		unsigned laneMask = 0;
		unsigned triangleIndex[simd_float::width];
		
#ifdef SRAST_AVX
		GATHER_TRIANGLE_FF_LO(m00, m01, m02, m03, 0);
		
		if (laneMask == 0)
			break;
		
		GATHER_TRIANGLE_FF_LO(m10, m11, m12, m13, 1);
		GATHER_TRIANGLE_FF_LO(m20, m21, m22, m23, 2);
		GATHER_TRIANGLE_FF_LO(m30, m31, m32, m33, 3);
		
		GATHER_TRIANGLE_FF_HI(m00, m01, m02, m03, 4);
		GATHER_TRIANGLE_FF_HI(m10, m11, m12, m13, 5);
		GATHER_TRIANGLE_FF_HI(m20, m21, m22, m23, 6);
		GATHER_TRIANGLE_FF_HI(m30, m31, m32, m33, 7);
		
		SRAST_MM256_TRANSPOSE4_PS(m00, m10, m20, m30);
		SRAST_MM256_TRANSPOSE4_PS(m01, m11, m21, m31);
		SRAST_MM256_TRANSPOSE4_PS(m02, m12, m22, m32);
		SRAST_MM256_TRANSPOSE4_PS(m03, m13, m23, m33);
#else
		GATHER_TRIANGLE_FF(m00, m01, m02, m03, 0);
		
		if (laneMask == 0)
			break;
		
		GATHER_TRIANGLE_FF(m10, m11, m12, m13, 1);
		GATHER_TRIANGLE_FF(m20, m21, m22, m23, 2);
		GATHER_TRIANGLE_FF(m30, m31, m32, m33, 3);
		
		_MM_TRANSPOSE4_PS(m00, m10, m20, m30);
		_MM_TRANSPOSE4_PS(m01, m11, m21, m31);
		_MM_TRANSPOSE4_PS(m02, m12, m22, m32);
		_MM_TRANSPOSE4_PS(m03, m13, m23, m33);
#endif
		
		simd_float3 edge0(m00, m10, m20);
		simd_float3 edge1(m01, m11, m21);
		simd_float3 edge2(m02, m12, m22);
		
		simd_float ez0(m30);
		simd_float ez1(m31);
		simd_float ez2(m32);

		simd_float zminVertex = m23;
		simd_float zmaxVertex = m33;

		simd_float bbXY = m03;
		simd_float bbZW = m13;
		
		simd_float2 bbMin, bbMax;
		
#ifdef SRAST_AVX
		vec4<simd4_float> bbA = decodeBoundingBox(_mm_castps_si128(bbXY.low().mm), _mm_castps_si128(bbZW.low().mm));
		vec4<simd4_float> bbB = decodeBoundingBox(_mm_castps_si128(bbXY.high().mm), _mm_castps_si128(bbZW.high().mm));

		bbMin.x = simd_float_combine(bbA.x.mm, bbB.x.mm);
		bbMin.y = simd_float_combine(bbA.y.mm, bbB.y.mm);
		bbMax.x = simd_float_combine(bbA.z.mm, bbB.z.mm);
		bbMax.y = simd_float_combine(bbA.w.mm, bbB.w.mm);

		__m128i simdBBMin = _mm_min_epi16(_mm_castps_si128(bbXY.low().mm), _mm_castps_si128(bbXY.high().mm));
		__m128i simdBBMax = _mm_max_epi16(_mm_castps_si128(bbZW.low().mm), _mm_castps_si128(bbZW.high().mm));
#else
		vec4<simd4_float> bb = decodeBoundingBox(_mm_castps_si128(bbXY.mm), _mm_castps_si128(bbZW.mm));

		bbMin.x = bb.x;
		bbMin.y = bb.y;
		bbMax.x = bb.z;
		bbMax.y = bb.w;

		__m128i simdBBMin = _mm_castps_si128(bbXY.mm);
		__m128i simdBBMax = _mm_castps_si128(bbZW.mm);
#endif
		
		simdBBMin = _mm_min_epi16(simdBBMin, _mm_shuffle_epi32(simdBBMin, _MM_SHUFFLE(3,2,3,2)));
		simdBBMax = _mm_max_epi16(simdBBMax, _mm_shuffle_epi32(simdBBMax, _MM_SHUFFLE(3,2,3,2)));

		simdBBMin = _mm_min_epi16(simdBBMin, _mm_shuffle_epi32(simdBBMin, _MM_SHUFFLE(1,1,1,1)));
		simdBBMax = _mm_max_epi16(simdBBMax, _mm_shuffle_epi32(simdBBMax, _MM_SHUFFLE(1,1,1,1)));

		unsigned minXY = _mm_extract_epi32(simdBBMin, 0);
		unsigned maxXY = _mm_extract_epi32(simdBBMax, 0);
		
		int minX = minXY & 0xffff;
		int minY = minXY >> 16;
		int maxX = maxXY & 0xffff;
		int maxY = maxXY >> 16;

		static const int minLevel = tileSizeLog2;
		
#if _WIN32
		DWORD dwLevel = 0;
		_BitScanReverse(&dwLevel, max(maxX-minX, maxY-minY));
		int level = dwLevel;
		++level;
#else
		int level = 32 - __builtin_clz(max(maxX-minX, maxY-minY));
#endif
		
		if (level > maxLevel)
			level = maxLevel;
		else if (level < minLevel)
			level = minLevel;
		
		unsigned left = minX;
		unsigned top = minY;
		
		unsigned levelMask = ~((1 << level)-1);
		
		left &= levelMask;
		top &= levelMask;
		
		if ((int)left + (1 << level) <= maxX || (int)top + (1 << level) <= maxY) { // Crosses a boundary?
			++level;
		}
		else if (!importanceMap.isSet(level, left, top)) {
			continue;
		}

		simd_float halfWidth = 0.5f*(int)width;
		simd_float halfHeight = 0.5f*(int)height;
		
		edge0.z -= edge0.x*halfWidth + edge0.y*halfHeight;
		edge1.z -= edge1.x*halfWidth + edge1.y*halfHeight;
		edge2.z -= edge2.x*halfWidth + edge2.y*halfHeight;
		ez2 -= ez0*halfWidth + ez1*halfHeight;

		simd_float e0xp = edge0.x > simd_float::zero();
		simd_float e1xp = edge1.x > simd_float::zero();
		simd_float e2xp = edge2.x > simd_float::zero();
		simd_float e0yp = edge0.y > simd_float::zero();
		simd_float e1yp = edge1.y > simd_float::zero();
		simd_float e2yp = edge2.y > simd_float::zero();
		
		simd_float edgeIncr0, edgeIncr1, edgeIncr2;
		edgeIncr0 = (e0xp & edge0.x) + (e0yp & edge0.y);
		edgeIncr1 = (e1xp & edge1.x) + (e1yp & edge1.y);
		edgeIncr2 = (e2xp & edge2.x) + (e2yp & edge2.y);

		simd_float edgeDecr0, edgeDecr1, edgeDecr2;
		edgeDecr0 = (not_and(e0xp, edge0.x) + not_and(e0yp, edge0.y)) * (float)(1 << tileSizeLog2);
		edgeDecr1 = (not_and(e1xp, edge1.x) + not_and(e1yp, edge1.y)) * (float)(1 << tileSizeLog2);
		edgeDecr2 = (not_and(e2xp, edge2.x) + not_and(e2yp, edge2.y)) * (float)(1 << tileSizeLog2);

		// Z-min/max corner (dz/dx < 0, dz/dy < 0).
		simd_float dzdx = ZMode::less(ez0, simd_float::zero());
		simd_float dzdy = ZMode::less(ez1, simd_float::zero());
		
		simd_float ez2min = ez2 + ((dzdx & ez0) + (dzdy & ez1)) * (float)(1 << tileSizeLog2);
		simd_float ez2max = ez2 + (not_and(dzdx, ez0) + not_and(dzdy, ez1)) * (float)(1 << tileSizeLog2);
		
		struct {
			unsigned short laneMask;
			unsigned short level;
			unsigned short left;
			unsigned short top;
			void set(unsigned s, unsigned l, unsigned t, unsigned m) {
				level = (unsigned short)s;
				left = (unsigned short)l;
				top = (unsigned short)t;
				laneMask = (unsigned short)m;
			}
		} stack[32];
		unsigned stackHead = 0;
		
		for (;;) {
			float size = (float)(1 << level);
			float2 box((float)(int)left, (float)(int)top);

			simd_float tl0 = edge0.x*box.x + edge0.y*box.y + edge0.z;
			simd_float tl1 = edge1.x*box.x + edge1.y*box.y + edge1.z;
			simd_float tl2 = edge2.x*box.x + edge2.y*box.y + edge2.z;
			
			// Test conservative edges.
			laneMask &= ~mask((tl0 + edgeIncr0*size) |
							  (tl1 + edgeIncr1*size) |
							  (tl2 + edgeIncr2*size));
			
			if (laneMask) {
				if (level == minLevel) {
					unsigned* binZmaxPointer;
					BinList* bin = binListArray(left, top, thread, frameNumber, binZmaxPointer);
					BinList::Writer binWriter = bin->startWrite(drawCallIdx, localAllocator);

					unsigned binZmax = *binZmaxPointer;
					unsigned oldBinZmax = binZmax;

					simd_float tl = ez0*box.x + ez1*box.y;

					simd_float zmin = tl + ez2min;
					zmin = ZMode::max(zmin, zminVertex);
					zmin = ZMode::min(zmin, SRAST_FAR_Z);

					laneMask &= mask(ZMode::less(zmin, uint32_as_float(binZmax)));

					if (laneMask) {
						unsigned coverMask = laneMask & mask((tl0 + edgeDecr0 > 0.0f) & // Note: Cannot check sign bit here. Small triangles end up with the wrong result.
															 (tl1 + edgeDecr1 > 0.0f) &
															 (tl2 + edgeDecr2 > 0.0f) &
															 ZMode::less(SRAST_NEAR_Z, zmin));

						simd_float zmax = tl + ez2max;
						zmax = ZMode::min(zmax, zmaxVertex);

						SRAST_SIMD_ALIGNED float zmina[simd_float::width];
						SRAST_SIMD_ALIGNED float zmaxa[simd_float::width];
						zmin.store(zmina);
						zmax.store(zmaxa);
					
						for (unsigned l = 0; l < simd_float::width; ++l) {
							unsigned zmini = float_as_uint32(zmina[l]);
							unsigned zmaxi = float_as_uint32(zmaxa[l]);
							
							unsigned bit = (1 << l);
							
							if (laneMask & bit) {
								if (ZMode::less(binZmax, zmini))
									continue;

								if (Opaque) {
									if ((coverMask & bit) && ZMode::less(zmaxi, binZmax))
										binZmax = zmaxi;
								}

								binWriter.writeTriangle(triangleIndex[l], zmini);
							}
						}

						if (ZMode::less(binZmax, oldBinZmax)) {
							unsigned newValue = binZmax;
							unsigned oldValue = oldBinZmax;
							unsigned prevOldValue = oldValue;
							while ((oldValue = Atomics::compareAndSwap(reinterpret_cast<int*>(binZmaxPointer), newValue, oldValue)) != prevOldValue && ZMode::less(newValue, oldValue))
								prevOldValue = oldValue;
						}
					}
					
					bin->endWrite(binWriter, localAllocator);
				}
				else {
					unsigned stride = 1 << (--level);
					
					simd_float cx = (float)(int)(left + stride);
					simd_float cy = (float)(int)(top + stride);
					
					unsigned negX = mask(bbMin.x < cx) & laneMask;
					unsigned posX = left+stride < width ? mask(bbMax.x > cx) & laneMask : 0;
					unsigned negY = mask(bbMin.y < cy) & laneMask;
					unsigned posY = top+stride < height ? mask(bbMax.y > cy) & laneMask : 0;
					
					if ((posX & negY) && importanceMap.isSet(level, left+stride, top))
						stack[stackHead++].set(level, left+stride, top, posX & negY);
					
					if ((negX & posY) && importanceMap.isSet(level, left, top+stride))
						stack[stackHead++].set(level, left, top+stride, negX & posY);
					
					if ((posX & posY) && importanceMap.isSet(level, left+stride, top+stride))
						stack[stackHead++].set(level, left+stride, top+stride, posX & posY);
					
					laneMask = negX & negY;
					
					if (laneMask && importanceMap.isSet(level, left, top))
						continue;
				}
			}
			
			if (!stackHead)
				break;
			
			--stackHead;
			top = stack[stackHead].top;
			left = stack[stackHead].left;
			level = stack[stackHead].level;
			laneMask = stack[stackHead].laneMask;
		}
	}
}

void binDrawCall(Renderer& r, DrawCall& drawCall, unsigned start, unsigned end, int maxLevel, unsigned thread) {
	if (drawCall.fragmentRenderState.isOpaque() && drawCall.fragmentRenderState.getDepthWrite())
		binDrawCallInMode<ZLessMode, true>(r, drawCall, start, end, maxLevel, thread);
	else
		binDrawCallInMode<ZLessMode, false>(r, drawCall, start, end, maxLevel, thread);
}

}
