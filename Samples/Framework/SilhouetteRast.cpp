//
//  SilhouetteRast.cpp
//  Framework
//
//  Created by Rasmus Barringer on 2012-10-26.
//  Copyright (c) 2012 Rasmus Barringer. All rights reserved.
//

#include "SilhouetteRast.h"
#include "../../SimdRast/SimdMath.h"
#include "../../SimdRast/IndexProvider.h"
#include <new>

using namespace srast;

namespace fx {

SRAST_FORCEINLINE void rasterizeEdge(int2 start, int2 end, ImportanceMap& importanceMap) {
	if (start.x > end.x)
		std::swap(start, end);
	
	int x = start.x >> 4;
	int y = start.y >> 4;

	int2 v = end - start;

	int stepy = v.y < 0 ? -1 : 1;
	
	int tmaxx = 16 - (start.x & 0xf);
	int tmaxy = (stepy > 0 ? 16 : 0) - (start.y & 0xf);

	int fx = v.y;
	int fy = v.x;

	if (v.y < 0) {
		fx = -fx;
		fy = -fy;
	}

	long long tdeltax = fx << 4;
	long long tdeltay = std::abs(fy) << 4;
	
	long long error = (long long)tmaxx*(long long)fx - (long long)tmaxy*(long long)fy;

	unsigned maxLevel = importanceMap.getMaxLevel();
	stepy <<= maxLevel;
	
	int lastx = end.x >> 4;
	int lasty = end.y >> 4;
	unsigned steps = (lastx-x) + std::abs(lasty-y);
	
	unsigned idx = (y << maxLevel) + x;
	importanceMap.set(idx);

	while (steps--) {
		if (error <= 0) {
			error += tdeltax;
			++idx;
		}
		else {
			error -= tdeltay;
			idx += stepy;
		}

		importanceMap.set(idx);
	}
}

template<class T>
static void rasterizeDrawCallSilhouettes(Renderer& r, DrawCall& drawCall, T indices, unsigned start, unsigned end, unsigned thread) {
	float4* __restrict shadedPositions = drawCall.shadedPositions;
	unsigned char* __restrict flags = drawCall.flags;
	const unsigned* __restrict adjacency = drawCall.adjacency;
	
	ImportanceMap& importanceMap = r.getImportanceMap();
	
	unsigned width = r.getFrameBufferWidth();
	unsigned height = r.getFrameBufferHeight();

	simd4_float halfViewport = simd4_float((float)(int)width, (float)(int)height, (float)(int)width, (float)(int)height)*0.5f;
	
	unsigned face = start/3;

	bool generateSilhouettes = drawCall.fragmentRenderState.isOpaque() || r.hasTransparentImportance();
	
	for (unsigned i = start; i < end; i += 3) {
		unsigned faceFlags = flags[face++];

		if ((faceFlags & FACEFLAG_BACK) == 0) { // Visible AFTER snapping. Need to set important flag based on neighbours.
			bool important = false;

			if (generateSilhouettes) {
				for (unsigned j = 0; j < 3; ++j) {
					unsigned adj = adjacency[i + j];
			
					if (adj != 0xffffffff) {
						if ((flags[adj] & (FACEFLAG_BACK | FACEFLAG_VALID)) != (FACEFLAG_BACK | FACEFLAG_VALID)) // Invalid or visible AFTER snapping.
							continue;
					}

					important = true;
					break;
				}
			}

			if (important)
				flags[face-1] |= FACEFLAG_IMPORTANT;
		}

		if (!generateSilhouettes || (faceFlags & (FACEFLAG_VALID | FACEFLAG_BACK_BEFORE_SNAP)) != FACEFLAG_VALID) // Invalid or before-snap back-facing.
			continue;
		
		int3 ind = indices(i);

		__m128 m0 = _mm_load_ps(&shadedPositions[ind.x].x);
		__m128 m1 = _mm_load_ps(&shadedPositions[ind.y].x);
		__m128 m2 = _mm_load_ps(&shadedPositions[ind.z].x);

		for (unsigned j = 0; j < 3; ++j) {
			unsigned adj = adjacency[i + j];
			
			if (adj != 0xffffffff) {
				if ((flags[adj] & (FACEFLAG_VALID | FACEFLAG_BACK_BEFORE_SNAP)) != (FACEFLAG_VALID | FACEFLAG_BACK_BEFORE_SNAP)) // Invalid or not before-snap back-facing.
					continue;
			}
			
			// This is a silhouette.
			simd4_float p0, p1;
			
			if (j == 0) {
				p0 = m0;
				p1 = m1;
			}
			else if (j == 1) {
				p0 = m1;
				p1 = m2;
			}
			else {
				p0 = m2;
				p1 = m0;
			}

			simd4_float w0 = _mm_shuffle_ps(p0.mm, p0.mm, _MM_SHUFFLE(3, 3, 3, 3));
			simd4_float w1 = _mm_shuffle_ps(p1.mm, p1.mm, _MM_SHUFFLE(3, 3, 3, 3));
			
			// Trivial frustum reject.
			if (mask((w0 <= p0 & w1 <= p1) | (w0 <= -p0 & w1 <= -p1)) & 0x7)
				continue;
			
			// Clipping against all planes.
			if (mask(w0 < abs(p0) | w1 < abs(p1)) & 0x7) {
				simd4_float a0 = w0 - p0;
				simd4_float a1 = w1 - p1;
				simd4_float b0 = w0 + p0;
				simd4_float b1 = w1 + p1;
				
				simd4_float sp = a0 / (a0 - a1); // Positive planes.
				simd4_float sn = b0 / (b0 - b1); // Negative planes.
				
				simd4_float p0sp = sp & (a0 < a1);
				simd4_float p0sn = sn & (b0 < b1);
				p0sp = p0sp & (p0sp > simd4_float::zero());
				p0sn = p0sn & (p0sn > simd4_float::zero());
				
				simd4_float p1sp = (simd4_float(1.0f) - sp) & (a0 > a1);
				simd4_float p1sn = (simd4_float(1.0f) - sn) & (b0 > b1);
				p1sp = p1sp & (p1sp > simd4_float::zero());
				p1sn = p1sn & (p1sn > simd4_float::zero());
				
				simd4_float p0s = max(p0sp, p0sn);
				simd4_float p0st = _mm_max_ss(p0s.mm, _mm_shuffle_ps(p0s.mm, p0s.mm, _MM_SHUFFLE(2, 2, 2, 2)));
				p0s = _mm_max_ss(p0st.mm, _mm_shuffle_ps(p0s.mm, p0s.mm, _MM_SHUFFLE(1, 1, 1, 1)));
				
				simd4_float p1s = max(p1sp, p1sn);
				simd4_float p1st = _mm_max_ss(p1s.mm, _mm_shuffle_ps(p1s.mm, p1s.mm, _MM_SHUFFLE(2, 2, 2, 2)));
				p1s = _mm_max_ss(p1st.mm, _mm_shuffle_ps(p1s.mm, p1s.mm, _MM_SHUFFLE(1, 1, 1, 1)));
				
				float s0 = _mm_cvtss_f32(p0s.mm);
				float s1 = _mm_cvtss_f32(p1s.mm);
				
				if (s0 >= 1.0f || s1 >= 1.0f || s0 + s1 >= 1.0f)
					continue;
				
				simd4_float op0 = p0;

				if (s0 > 0.0f && s0 < 1.0f)
					p0 += (p1 - p0) * s0;
				
				if (s1 > 0.0f && s1 < 1.0f)
					p1 += (op0 - p1) * s1;
			}
			
			// Project to 2D.
			simd4_float w01 = _mm_shuffle_ps(p0.mm, p1.mm, _MM_SHUFFLE(3, 3, 3, 3));
			simd4_float p01 = _mm_shuffle_ps(p0.mm, p1.mm, _MM_SHUFFLE(1, 0, 1, 0));
			
			p01 *= halfViewport;
			p01 *= simd4_float(16.0f) / w01;
			p01 = round(p01);

			int2 start, end;
			__m128i p01i = _mm_cvttps_epi32(p01.mm);
			
			start.x = _mm_extract_epi32(p01i, 0);
			start.y = _mm_extract_epi32(p01i, 1);
			end.x = _mm_extract_epi32(p01i, 2);
			end.y = _mm_extract_epi32(p01i, 3);

			start.x += width << 3;
			start.y += height << 3;
			end.x += width << 3;
			end.y += height << 3;

			start.x = min(max(start.x, 1), ((int)width << 4)-1);
			end.x = min(max(end.x, 1), ((int)width << 4)-1);
			start.y = min(max(start.y, 1), ((int)height << 4)-1);
			end.y = min(max(end.y, 1), ((int)height << 4)-1);

			rasterizeEdge(start, end, importanceMap);
		}
	}
}

template<class T>
class SilhouetteTask : public ThreadPoolTask {
private:
	Renderer& r;
	DrawCall& drawCall;
	T indices;
	
public:
	SilhouetteTask(Renderer& r, DrawCall& drawCall, T indices) : r(r), drawCall(drawCall), indices(indices) {
	}
	
	virtual void run(unsigned start, unsigned end, unsigned thread) {
		rasterizeDrawCallSilhouettes(r, drawCall, indices, start, end, thread);
	}
};

void rasterizeDrawCallSilhouettes(Renderer& r, DrawCall& drawCall) {
	ThreadPool& threadPool = r.getThreadPool();
	
	if (drawCall.indexBuffer.stride == 1) {
		SilhouetteTask<IndexProvider<unsigned char> >* t = new (drawCall.task) SilhouetteTask<IndexProvider<unsigned char> >(r, drawCall, IndexProvider<unsigned char>(drawCall.indexBuffer.data));
		threadPool.startTask(t, drawCall.indexBuffer.count, 3*1024);
	}
	else if (drawCall.indexBuffer.stride == 2) {
		SilhouetteTask<IndexProvider<unsigned short> >* t = new (drawCall.task) SilhouetteTask<IndexProvider<unsigned short> >(r, drawCall, IndexProvider<unsigned short>(drawCall.indexBuffer.data));
		threadPool.startTask(t, drawCall.indexBuffer.count, 3*1024);
	}
	else if (drawCall.indexBuffer.stride == 4) {
		SilhouetteTask<IndexProvider<unsigned int> >* t = new (drawCall.task) SilhouetteTask<IndexProvider<unsigned int> >(r, drawCall, IndexProvider<unsigned int>(drawCall.indexBuffer.data));
		threadPool.startTask(t, drawCall.indexBuffer.count, 3*1024);
	}
	else {
		SilhouetteTask<IndexProvider<> >* t = new (drawCall.task) SilhouetteTask<IndexProvider<> >(r, drawCall, IndexProvider<>());
		threadPool.startTask(t, drawCall.vertexBuffer.count, 3*1024);
	}
}
	
}
