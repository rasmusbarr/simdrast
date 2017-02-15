//
//  TriangleSetup.cpp
//  SimdRast
//
//  Created by Rasmus Barringer on 2012-10-30.
//  Copyright (c) 2012 Rasmus Barringer. All rights reserved.
//

#include "TriangleSetup.h"
#include "IndexProvider.h"
#include "SimdDouble.h"
#include "Binning.h"
#include <new>

namespace srast {

#define GATHER_TRIANGLE(m0, m1, m2, idx) \
__m128 m0, m1, m2;\
if (idx == 0 || i + 3*idx < end) {\
int3 ind = indices(i + 3*idx);\
m0 = _mm_load_ps(&shadedPositions[ind.x].x);\
m1 = _mm_load_ps(&shadedPositions[ind.y].x);\
m2 = _mm_load_ps(&shadedPositions[ind.z].x);\
laneMask += laneMask + 1;\
}

#define GATHER_TRIANGLE_LO(m0, m1, m2, idx) \
__m256 m0, m1, m2;\
if (idx == 0 || i + 3*idx < end) {\
int3 ind = indices(i + 3*idx);\
m0 = _mm256_castps128_ps256(_mm_load_ps(&shadedPositions[ind.x].x));\
m1 = _mm256_castps128_ps256(_mm_load_ps(&shadedPositions[ind.y].x));\
m2 = _mm256_castps128_ps256(_mm_load_ps(&shadedPositions[ind.z].x));\
laneMask += laneMask + 1;\
}

#define GATHER_TRIANGLE_HI(m0, m1, m2, idx) \
if (idx == 0 || i + 3*idx < end) {\
int3 ind = indices(i + 3*idx);\
m0 = _mm256_insertf128_ps(m0, _mm_load_ps(&shadedPositions[ind.x].x), 1);\
m1 = _mm256_insertf128_ps(m1, _mm_load_ps(&shadedPositions[ind.y].x), 1);\
m2 = _mm256_insertf128_ps(m2, _mm_load_ps(&shadedPositions[ind.z].x), 1);\
laneMask += laneMask + 1;\
}

inline simd_float3 operator & (const simd_float3& lhs, const simd_float3& rhs) {
	return simd_float3(lhs.x & rhs.x, lhs.y & rhs.y, lhs.z & rhs.z);
}

inline simd_float3 operator * (const simd_float3& lhs, const simd_float3& rhs) {
	return simd_float3(lhs.x * rhs.x, lhs.y * rhs.y, lhs.z * rhs.z);
}

inline simd_float3 operator < (const simd_float3& lhs, const simd_float3& rhs) {
	return simd_float3(lhs.x < rhs.x, lhs.y < rhs.y, lhs.z < rhs.z);
}

inline simd_float3 operator != (const simd_float3& lhs, const simd_float3& rhs) {
	return simd_float3(lhs.x != rhs.x, lhs.y != rhs.y, lhs.z != rhs.z);
}

inline simd_float3 blend(const simd_float3& a, const simd_float3& b, const simd_float& s) {
	return simd_float3(blend(a.x, b.x, s),
					   blend(a.y, b.y, s),
					   blend(a.z, b.z, s));
}

inline simd_float2 operator & (const simd_float2& lhs, const simd_float2& rhs) {
	return simd_float2(lhs.x & rhs.x, lhs.y & rhs.y);
}

inline simd_float2 operator * (const simd_float2& lhs, const simd_float2& rhs) {
	return simd_float2(lhs.x * rhs.x, lhs.y * rhs.y);
}

inline simd_float2 operator < (const simd_float2& lhs, const simd_float2& rhs) {
	return simd_float2(lhs.x < rhs.x, lhs.y < rhs.y);
}

inline simd_float2 operator != (const simd_float2& lhs, const simd_float2& rhs) {
	return simd_float2(lhs.x != rhs.x, lhs.y != rhs.y);
}

inline simd_float2 recip(const simd_float2& v) {
	return simd_float2(recip(v.x), recip(v.y));
}

inline simd_float3 recip(const simd_float3& v) {
	return simd_float3(recip(v.x), recip(v.y), recip(v.z));
}

inline simd_float2 not_and(const simd_float2& a, const simd_float2& b) {
	return simd_float2(not_and(a.x, b.x), not_and(a.y, b.y));
}

inline simd_float3 not_and(const simd_float3& a, const simd_float3& b) {
	return simd_float3(not_and(a.x, b.x), not_and(a.y, b.y), not_and(a.z, b.z));
}

template<bool first>
inline simd_float4 computeClippedEdge(const simd_float3& v0, const simd_float2& a0, const simd_float3& b0,
									  const simd_float3& v1, const simd_float2& a1, const simd_float3& b1,
									  simd_double2& hpez, simd_float4& bb, const simd_float& halfWidth, const simd_float& halfHeight, simd_float3& firstVert) {
	// Frustum reject.
	simd_float ww0 = halfWidth * v0.z;
	simd_float hw0 = halfHeight * v0.z;
	simd_float ww1 = halfWidth * v1.z;
	simd_float hw1 = halfHeight * v1.z;

	simd_float trivialReject = (((ww0 + v0.x) & (ww1 + v1.x)) |
								((ww0 - v0.x) & (ww1 - v1.x)) |
								((hw0 + v0.y) & (hw1 + v1.y)) |
								((hw0 - v0.y) & (hw1 - v1.y)));

	simd_float cornerAccept = ((v0.x*v1.y - v1.x*v0.y) <= abs(v0.y*v1.z - v1.y*v0.z)*halfWidth + abs(v1.x*v0.z - v0.x*v1.z)*halfHeight);

	simd_float inside = not_and(trivialReject, cornerAccept);
	
	// Clip against frustum + guardband.
	simd_float2 ra = recip(a0 - a1) & (a0 != a1);
	simd_float3 rb = recip(b0 - b1) & (b0 != b1);
	
	simd_float2 sp0 = a0 * ra;
	simd_float3 sn0 = b0 * rb;
	simd_float2 sp1 = (-a1) * ra;
	simd_float3 sn1 = (-b1) * rb;
	
	simd_float2 na = a0 < a1;
	simd_float3 nb = b0 < b1;
	
	simd_float2 p0sp = na & sp0;
	simd_float3 p0sn = nb & sn0;
	simd_float2 p1sp = not_and(na, sp1);
	simd_float3 p1sn = not_and(nb, sn1);
	
	simd_float s0 = max(max(p0sp.x, max(p0sp.y, simd_float::zero())),
						max(p0sn.x, max(p0sn.y, p0sn.z)));
	simd_float s1 = max(max(p1sp.x, max(p1sp.y, simd_float::zero())),
						max(p1sn.x, max(p1sn.y, p1sn.z)));
	
	simd_float3 p0 = v0 + (v1 - v0)*s0;
	simd_float3 p1 = v1 + (v0 - v1)*s1;

	inside = inside & (s0+s1 < 1.0f); // Yes, this is really needed. Probably because the w corner is small.
	
	// Project and snap.
	simd_float rz0 = simd_float(16.0f) / p0.z;
	simd_float rz1 = simd_float(16.0f) / p1.z;
	
	simd_float w = (p0.z*p1.z) / (simd_float(1.0f)-s0-s1);
	
	p0.x = round(p0.x*rz0) * (1.0f / 16.0f);
	p0.y = round(p0.y*rz0) * (1.0f / 16.0f);
	p0.z = 1.0f;
	
	p1.x = round(p1.x*rz1) * (1.0f / 16.0f);
	p1.y = round(p1.y*rz1) * (1.0f / 16.0f);
	p1.z = 1.0f;
	
	// Default to homogeneous edge equation if outside.
	p0 = blend(v0, p0, inside);
	p1 = blend(v1, p1, inside);
	
	simd_float4 edge;
	
	edge.x = p0.y*p1.z - p1.y*p0.z;
	edge.y = p1.x*p0.z - p0.x*p1.z;
	edge.z = p0.x*p1.y - p1.x*p0.y;
	edge.w = blend(1.0f, w, inside);
	
	// Top-left fill convention.
	simd_float offset = blend(0.0f, -1.0f/(16.0f*32.0f), (edge.x | ((edge.x == simd_float::zero()) & edge.y)) & inside);

	// High-precision edge z.
	hpez.x = float2double_lo(p0.x)*float2double_lo(p1.y) - float2double_lo(p1.x)*float2double_lo(p0.y) + float2double_lo(offset);
	hpez.y = float2double_hi(p0.x)*float2double_hi(p1.y) - float2double_hi(p1.x)*float2double_hi(p0.y) + float2double_hi(offset);
	
	// Bounding box.
	simd_float signMask(-0.0f);
	
	simd_float xc0 = (edge.x & signMask) ^ (-1920.0f);
	simd_float yc0 = (edge.y & signMask) ^ (-1920.0f);
	
	simd_float xc1 = blend(xc0, -xc0, b0.z | b1.z); // Cover entire screen if behind view.
	simd_float yc1 = blend(yc0, -yc0, b0.z | b1.z);
	
	p0.x = blend(xc0, p0.x, inside);
	p1.x = blend(xc1, p1.x, inside);
	p0.y = blend(yc0, p0.y, inside);
	p1.y = blend(yc1, p1.y, inside);
	
	if (first) {
		bb.x = min(p0.x, p1.x);
		bb.y = min(p0.y, p1.y);
		bb.z = max(p0.x, p1.x);
		bb.w = max(p0.y, p1.y);
	}
	else {
		bb.x = min(bb.x, min(p0.x, p1.x));
		bb.y = min(bb.y, min(p0.y, p1.y));
		bb.z = max(bb.z, max(p0.x, p1.x));
		bb.w = max(bb.w, max(p0.y, p1.y));
	}

	firstVert = p0;
	return edge;
}

inline vec2<simd4_float> encodeBoundingBox(const vec4<simd4_float>& bb) {
	__m128i bbx32 = _mm_cvtps_epi32(floor(bb.x).mm);
	__m128i bby32 = _mm_cvtps_epi32(floor(bb.y).mm);
	__m128i bbz32 = _mm_cvtps_epi32(ceil(bb.z).mm);
	__m128i bbw32 = _mm_cvtps_epi32(ceil(bb.w).mm);
			
	__m128i bbxz16 = _mm_packs_epi32(bbx32, bbz32); // x x x x z z z z
	__m128i bbyw16 = _mm_packs_epi32(bby32, bbw32); // y y y y w w w w
			
	__m128i bbxy16 = _mm_unpacklo_epi16(bbxz16, bbyw16); // x y x y x y x y
	__m128i bbzw16 = _mm_unpackhi_epi16(bbxz16, bbyw16); // z w z w z w z w

	return vec2<simd4_float>(_mm_castsi128_ps(bbxy16), _mm_castsi128_ps(bbzw16));
}

template<class ZMode, class T>
static void setupDrawCallTriangles(Renderer& r, DrawCall& drawCall, T indices, unsigned start, unsigned end, unsigned thread) {
	float4* __restrict shadedPositions = drawCall.shadedPositions;
	unsigned char* __restrict flags = drawCall.flags + start/3;

	for (unsigned i = start; i < end; i += 3*simd_float::width) {
		unsigned laneMask = 0;

#ifdef SRAST_AVX
		GATHER_TRIANGLE_LO(m00, m01, m02, 0);
		GATHER_TRIANGLE_LO(m10, m11, m12, 1);
		GATHER_TRIANGLE_LO(m20, m21, m22, 2);
		GATHER_TRIANGLE_LO(m30, m31, m32, 3);
		
		GATHER_TRIANGLE_HI(m00, m01, m02, 4);
		GATHER_TRIANGLE_HI(m10, m11, m12, 5);
		GATHER_TRIANGLE_HI(m20, m21, m22, 6);
		GATHER_TRIANGLE_HI(m30, m31, m32, 7);
		
		SRAST_MM256_TRANSPOSE4_PS(m00, m10, m20, m30);
		SRAST_MM256_TRANSPOSE4_PS(m01, m11, m21, m31);
		SRAST_MM256_TRANSPOSE4_PS(m02, m12, m22, m32);
#else
		GATHER_TRIANGLE(m00, m01, m02, 0);
		GATHER_TRIANGLE(m10, m11, m12, 1);
		GATHER_TRIANGLE(m20, m21, m22, 2);
		GATHER_TRIANGLE(m30, m31, m32, 3);
		
		_MM_TRANSPOSE4_PS(m00, m10, m20, m30);
		_MM_TRANSPOSE4_PS(m01, m11, m21, m31);
		_MM_TRANSPOSE4_PS(m02, m12, m22, m32);
#endif
		
		simd_float3 v0(m00, m10, m30);
		simd_float3 v1(m01, m11, m31);
		simd_float3 v2(m02, m12, m32);
		
		simd_float z0(m20);
		simd_float z1(m21);
		simd_float z2(m22);

		// View frustum cull.
		if (mask(v0.z - abs(v0.x) | v0.z - abs(v0.y) | v0.z - abs(z0)) & laneMask) {
			laneMask &= ~mask((v0.z - v0.x & v1.z - v1.x & v2.z - v2.x) |
							  (v0.z + v0.x & v1.z + v1.x & v2.z + v2.x) |
							  (v0.z - v0.y & v1.z - v1.y & v2.z - v2.y) |
							  (v0.z + v0.y & v1.z + v1.y & v2.z + v2.y) |
							  (v0.z - z0   & v1.z - z1   & v2.z - z2  ) |
							  (v0.z + z0   & v1.z + z1   & v2.z + z2  ));
		}

		unsigned validFace = laneMask;

		// Facing before snapping. Used for silhouette detection.
		simd_float adj11 = v1.y*v2.z-v2.y*v1.z;
		simd_float adj12 = v1.x*v2.z-v1.z*v2.x;
		simd_float adj13 = v1.x*v2.y-v1.y*v2.x;
		
		unsigned backFacingBeforeSnap = mask(v0.x*adj11 - v0.y*adj12 + v0.z*adj13) & laneMask;
		
		if (laneMask) {
			simd_float halfWidth = 0.5f*r.getFrameBufferWidth();
			simd_float halfHeight = 0.5f*r.getFrameBufferHeight();
			
			v0.x = v0.x * halfWidth;
			v1.x = v1.x * halfWidth;
			v2.x = v2.x * halfWidth;
			
			v0.y = v0.y * halfHeight;
			v1.y = v1.y * halfHeight;
			v2.y = v2.y * halfHeight;

			/*
			 Edges within the frustum are projected, possibly clipped and snapped, in order to guarantee watertight rendering.
			 The _perfectly_ accurate range is p12+s4.
			 
			 The edge test is separated into per-pixel and per-sample computations.
			 The required number of bits for each component is (for tile size = p3):
			 
			 PX: (p12+s4) * (p2) x2 = 19 bits
			 SP: (p12+s4) * (s5) x2 = 22 bits
			 EZ: (p12+s4) * (p2+s5) x2 = 24 bits
			 LUT: SP+EZ
			 
			 Tile size p3 means p2 range since we offset from the middle.
			 The sign bit is thus utilized, effectively giving us 25 bits of precision.
			 
			 The range in EZ comes from the fact that only locations within a tile need to be accurate.
			 We accomplish this by storing 64-bit EZ and subtracting the tile center in high precision.
			 
			 Max res becomes 3584x3584 with a 128 pixel guardband.
			*/

			simd_float guardband = 1920.0f;
			
			simd_float gw0 = v0.z*guardband;
			simd_float2 a0 = simd_float2(gw0, gw0) - v0.xy();
			simd_float3 b0 = simd_float3(gw0, gw0, -1e-3f) + v0;
			unsigned m0 = mask((a0.x | a0.y) | (b0.x | b0.y | b0.z)) & laneMask;
			
			simd_float gw1 = v1.z*guardband;
			simd_float2 a1 = simd_float2(gw1, gw1) - v1.xy();
			simd_float3 b1 = simd_float3(gw1, gw1, -1e-3f) + v1;
			unsigned m1 = mask((a1.x | a1.y) | (b1.x | b1.y | b1.z)) & laneMask;
			
			simd_float gw2 = v2.z*guardband;
			simd_float2 a2 = simd_float2(gw2, gw2) - v2.xy();
			simd_float3 b2 = simd_float3(gw2, gw2, -1e-3f) + v2;
			unsigned m2 = mask((a2.x | a2.y) | (b2.x | b2.y | b2.z)) & laneMask;
			
			simd_float4 edge0, edge1, edge2;
			simd_double2 hpez0, hpez1, hpez2;
			simd_float4 bb;

			if ((m0 | m1 | m2) == 0) {
				// Project and snap.
				simd_float rz0 = simd_float(16.0f) / v0.z;
				simd_float rz1 = simd_float(16.0f) / v1.z;
				simd_float rz2 = simd_float(16.0f) / v2.z;
				
				simd_float2 p0, p1, p2;
				
				p0.x = round(v0.x*rz0) * (1.0f / 16.0f);
				p0.y = round(v0.y*rz0) * (1.0f / 16.0f);
				
				p1.x = round(v1.x*rz1) * (1.0f / 16.0f);
				p1.y = round(v1.y*rz1) * (1.0f / 16.0f);
				
				p2.x = round(v2.x*rz2) * (1.0f / 16.0f);
				p2.y = round(v2.y*rz2) * (1.0f / 16.0f);
				
				// Compute bounding-box.
				bb.x = min(p0.x, min(p1.x, p2.x));
				bb.y = min(p0.y, min(p1.y, p2.y));
				bb.z = max(p0.x, max(p1.x, p2.x));
				bb.w = max(p0.y, max(p1.y, p2.y));
				
				// Compute edges.
				edge0.x = p1.y - p2.y;
				edge0.y = p2.x - p1.x;
				edge0.z = p1.x*p2.y - p2.x*p1.y;
				edge0.w = v1.z*v2.z;
				
				edge1.x = p2.y - p0.y;
				edge1.y = p0.x - p2.x;
				edge1.z = p2.x*p0.y - p0.x*p2.y;
				edge1.w = v0.z*v2.z;
				
				edge2.x = p0.y - p1.y;
				edge2.y = p1.x - p0.x;
				edge2.z = p0.x*p1.y - p1.x*p0.y;
				edge2.w = v0.z*v1.z;
				
				// Discard back-facing.
				laneMask &= ~mask(p0.x*edge0.x + p0.y*edge0.y + edge0.z);
				
				// Top-left fill convention.
				simd_float offset0 = blend(0.0f, -1.0f/(16.0f*32.0f), (edge0.x | ((edge0.x == simd_float::zero()) & edge0.y)));
				simd_float offset1 = blend(0.0f, -1.0f/(16.0f*32.0f), (edge1.x | ((edge1.x == simd_float::zero()) & edge1.y)));
				simd_float offset2 = blend(0.0f, -1.0f/(16.0f*32.0f), (edge2.x | ((edge2.x == simd_float::zero()) & edge2.y)));
				
				// High-precision edge z.
				hpez0.x = float2double_lo(p1.x)*float2double_lo(p2.y) - float2double_lo(p2.x)*float2double_lo(p1.y) + float2double_lo(offset0);
				hpez0.y = float2double_hi(p1.x)*float2double_hi(p2.y) - float2double_hi(p2.x)*float2double_hi(p1.y) + float2double_hi(offset0);
				
				hpez1.x = float2double_lo(p2.x)*float2double_lo(p0.y) - float2double_lo(p0.x)*float2double_lo(p2.y) + float2double_lo(offset1);
				hpez1.y = float2double_hi(p2.x)*float2double_hi(p0.y) - float2double_hi(p0.x)*float2double_hi(p2.y) + float2double_hi(offset1);
				
				hpez2.x = float2double_lo(p0.x)*float2double_lo(p1.y) - float2double_lo(p1.x)*float2double_lo(p0.y) + float2double_lo(offset2);
				hpez2.y = float2double_hi(p0.x)*float2double_hi(p1.y) - float2double_hi(p1.x)*float2double_hi(p0.y) + float2double_hi(offset2);
			}
			else {
				simd_float3 ev0, ev1, ev2;
				edge0 = computeClippedEdge<true>(v1, a1, b1, v2, a2, b2, hpez0, bb, halfWidth, halfHeight, ev1);
				edge1 = computeClippedEdge<false>(v2, a2, b2, v0, a0, b0, hpez1, bb, halfWidth, halfHeight, ev2);
				edge2 = computeClippedEdge<false>(v0, a0, b0, v1, a1, b1, hpez2, bb, halfWidth, halfHeight, ev0);

				// Discard back-facing.
				laneMask &= ~mask((ev1.x*edge1.x + ev1.y*edge1.y + ev1.z*edge1.z) &
								  (ev2.x*edge2.x + ev2.y*edge2.y + ev2.z*edge2.z) &
								  (ev0.x*edge0.x + ev0.y*edge0.y + ev0.z*edge0.z));
			}

			// Discard degenerates.
			laneMask &= ~mask(((edge0.x == simd_float::zero()) & (edge0.y == simd_float::zero())) |
							  ((edge1.x == simd_float::zero()) & (edge1.y == simd_float::zero())) |
							  ((edge2.x == simd_float::zero()) & (edge2.y == simd_float::zero())));
			
			bb.x += halfWidth;
			bb.z += halfWidth;
			bb.y += halfHeight;
			bb.w += halfHeight;
			
			bb.x = max(bb.x, simd_float::zero());
			bb.y = max(bb.y, simd_float::zero());
			bb.z = min(bb.z, (float)(int)r.getFrameBufferWidth());
			bb.w = min(bb.w, (float)(int)r.getFrameBufferHeight());
			
			laneMask &= mask((bb.x < bb.z) & (bb.y < bb.w));
			
			// Encode each bounding-box as one double.
#ifdef SRAST_AVX
			vec2<simd4_float> bbfl = encodeBoundingBox(vec4<simd4_float>(bb.x.low(), bb.y.low(), bb.z.low(), bb.w.low()));
			vec2<simd4_float> bbfh = encodeBoundingBox(vec4<simd4_float>(bb.x.high(), bb.y.high(), bb.z.high(), bb.w.high()));

			simd_float2 bbf(simd_float_combine(bbfl.x.mm, bbfh.x.mm), simd_float_combine(bbfl.y.mm, bbfh.y.mm));
#else
			simd_float2 bbf = encodeBoundingBox(bb);
#endif

			if (laneMask) {
				unsigned idx = ((unsigned)i)/3;
				
				double* __restrict highpEdgeZ0 = drawCall.highpEdgeZ0 + idx;
				double* __restrict highpEdgeZ1 = drawCall.highpEdgeZ1 + idx;
				double* __restrict highpEdgeZ2 = drawCall.highpEdgeZ2 + idx;

				hpez0.x.stream_store(highpEdgeZ0);
				hpez0.y.stream_store(highpEdgeZ0 + simd_double::width);
				hpez1.x.stream_store(highpEdgeZ1);
				hpez1.y.stream_store(highpEdgeZ1 + simd_double::width);
				hpez2.x.stream_store(highpEdgeZ2);
				hpez2.y.stream_store(highpEdgeZ2 + simd_double::width);

				// Map z from [-1 1] to [1 0]. Note the reversed range for improved precision at the far plane.
				z0 = (v0.z-z0)*0.5f;
				z1 = (v1.z-z1)*0.5f;
				z2 = (v2.z-z2)*0.5f;
				
				simd_float zw0 = z0 * edge0.w;
				simd_float zw1 = z1 * edge1.w;
				simd_float zw2 = z2 * edge2.w;
				
				simd_float ez0 = (edge0.x*zw0 + edge1.x*zw1 + edge2.x*zw2);
				simd_float ez1 = (edge0.y*zw0 + edge1.y*zw1 + edge2.y*zw2);
				simd_float ez2 = (edge0.z*zw0 + edge1.z*zw1 + edge2.z*zw2);
				
				simd_float invDet = (z0 + z1 + z2) / (ez0*v0.x + ez1*v0.y + ez2*v0.z +
													  ez0*v1.x + ez1*v1.y + ez2*v1.z +
													  ez0*v2.x + ez1*v2.y + ez2*v2.z);
				ez0 = ez0 * invDet;
				ez1 = ez1 * invDet;
				ez2 = ez2 * invDet;

				// Z-min/max vertex.
				simd_float pz0 = ZMode::nearClip(z0/v0.z, v0.z > 0.0f);
				simd_float pz1 = ZMode::nearClip(z1/v1.z, v1.z > 0.0f);
				simd_float pz2 = ZMode::nearClip(z2/v2.z, v2.z > 0.0f);

				simd_float zminVertex =  ZMode::min(pz0, ZMode::min(pz1, pz2));
				simd_float zmaxVertex = ZMode::min(ZMode::max(pz0, ZMode::max(pz1, pz2)), SRAST_FAR_Z);

#ifdef SRAST_AVX
				__m256 m03, m13, m23, m33;
#else
				__m128 m03, m13, m23, m33;
#endif
				
				m00 = edge0.x.mm; m10 = edge0.y.mm; m20 = edge0.z.mm; m30 = ez0.mm;
				m01 = edge1.x.mm; m11 = edge1.y.mm; m21 = edge1.z.mm; m31 = ez1.mm;
				m02 = edge2.x.mm; m12 = edge2.y.mm; m22 = edge2.z.mm; m32 = ez2.mm;
				m03 = bbf.x.mm; m13 = bbf.y.mm; m23 = zminVertex.mm; m33 = zmaxVertex.mm;

#ifdef SRAST_AVX
				SRAST_MM256_TRANSPOSE4_PS(m00, m10, m20, m30);
				SRAST_MM256_TRANSPOSE4_PS(m01, m11, m21, m31);
				SRAST_MM256_TRANSPOSE4_PS(m02, m12, m22, m32);
				SRAST_MM256_TRANSPOSE4_PS(m03, m13, m23, m33);
				
				_mm_stream_ps(&drawCall.edges[idx*4+0+0].x, _mm256_castps256_ps128(m00));
				_mm_stream_ps(&drawCall.edges[idx*4+0+1].x, _mm256_castps256_ps128(m01));
				_mm_stream_ps(&drawCall.edges[idx*4+0+2].x, _mm256_castps256_ps128(m02));
				_mm_stream_ps(&drawCall.edges[idx*4+0+3].x, _mm256_castps256_ps128(m03));
				
				_mm_stream_ps(&drawCall.edges[idx*4+4+0].x, _mm256_castps256_ps128(m10));
				_mm_stream_ps(&drawCall.edges[idx*4+4+1].x, _mm256_castps256_ps128(m11));
				_mm_stream_ps(&drawCall.edges[idx*4+4+2].x, _mm256_castps256_ps128(m12));
				_mm_stream_ps(&drawCall.edges[idx*4+4+3].x, _mm256_castps256_ps128(m13));
				
				_mm_stream_ps(&drawCall.edges[idx*4+8+0].x, _mm256_castps256_ps128(m20));
				_mm_stream_ps(&drawCall.edges[idx*4+8+1].x, _mm256_castps256_ps128(m21));
				_mm_stream_ps(&drawCall.edges[idx*4+8+2].x, _mm256_castps256_ps128(m22));
				_mm_stream_ps(&drawCall.edges[idx*4+8+3].x, _mm256_castps256_ps128(m23));
				
				_mm_stream_ps(&drawCall.edges[idx*4+12+0].x, _mm256_castps256_ps128(m30));
				_mm_stream_ps(&drawCall.edges[idx*4+12+1].x, _mm256_castps256_ps128(m31));
				_mm_stream_ps(&drawCall.edges[idx*4+12+2].x, _mm256_castps256_ps128(m32));
				_mm_stream_ps(&drawCall.edges[idx*4+12+3].x, _mm256_castps256_ps128(m33));
				
				_mm_stream_ps(&drawCall.edges[idx*4+16+0].x, _mm256_extractf128_ps(m00, 1));
				_mm_stream_ps(&drawCall.edges[idx*4+16+1].x, _mm256_extractf128_ps(m01, 1));
				_mm_stream_ps(&drawCall.edges[idx*4+16+2].x, _mm256_extractf128_ps(m02, 1));
				_mm_stream_ps(&drawCall.edges[idx*4+16+3].x, _mm256_extractf128_ps(m03, 1));
				
				_mm_stream_ps(&drawCall.edges[idx*4+20+0].x, _mm256_extractf128_ps(m10, 1));
				_mm_stream_ps(&drawCall.edges[idx*4+20+1].x, _mm256_extractf128_ps(m11, 1));
				_mm_stream_ps(&drawCall.edges[idx*4+20+2].x, _mm256_extractf128_ps(m12, 1));
				_mm_stream_ps(&drawCall.edges[idx*4+20+3].x, _mm256_extractf128_ps(m13, 1));
				
				_mm_stream_ps(&drawCall.edges[idx*4+24+0].x, _mm256_extractf128_ps(m20, 1));
				_mm_stream_ps(&drawCall.edges[idx*4+24+1].x, _mm256_extractf128_ps(m21, 1));
				_mm_stream_ps(&drawCall.edges[idx*4+24+2].x, _mm256_extractf128_ps(m22, 1));
				_mm_stream_ps(&drawCall.edges[idx*4+24+3].x, _mm256_extractf128_ps(m23, 1));
				
				_mm_stream_ps(&drawCall.edges[idx*4+28+0].x, _mm256_extractf128_ps(m30, 1));
				_mm_stream_ps(&drawCall.edges[idx*4+28+1].x, _mm256_extractf128_ps(m31, 1));
				_mm_stream_ps(&drawCall.edges[idx*4+28+2].x, _mm256_extractf128_ps(m32, 1));
				_mm_stream_ps(&drawCall.edges[idx*4+28+3].x, _mm256_extractf128_ps(m33, 1));
#else
				_MM_TRANSPOSE4_PS(m00, m10, m20, m30);
				_MM_TRANSPOSE4_PS(m01, m11, m21, m31);
				_MM_TRANSPOSE4_PS(m02, m12, m22, m32);
				_MM_TRANSPOSE4_PS(m03, m13, m23, m33);
				
				_mm_stream_ps(&drawCall.edges[idx*4+0+0].x, m00);
				_mm_stream_ps(&drawCall.edges[idx*4+0+1].x, m01);
				_mm_stream_ps(&drawCall.edges[idx*4+0+2].x, m02);
				_mm_stream_ps(&drawCall.edges[idx*4+0+3].x, m03);
				
				_mm_stream_ps(&drawCall.edges[idx*4+4+0].x, m10);
				_mm_stream_ps(&drawCall.edges[idx*4+4+1].x, m11);
				_mm_stream_ps(&drawCall.edges[idx*4+4+2].x, m12);
				_mm_stream_ps(&drawCall.edges[idx*4+4+3].x, m13);
				
				_mm_stream_ps(&drawCall.edges[idx*4+8+0].x, m20);
				_mm_stream_ps(&drawCall.edges[idx*4+8+1].x, m21);
				_mm_stream_ps(&drawCall.edges[idx*4+8+2].x, m22);
				_mm_stream_ps(&drawCall.edges[idx*4+8+3].x, m23);
				
				_mm_stream_ps(&drawCall.edges[idx*4+12+0].x, m30);
				_mm_stream_ps(&drawCall.edges[idx*4+12+1].x, m31);
				_mm_stream_ps(&drawCall.edges[idx*4+12+2].x, m32);
				_mm_stream_ps(&drawCall.edges[idx*4+12+3].x, m33);
#endif
			}
		}
		
		for (unsigned j = 0; j < simd_float::width; ++j) {
			*(flags++) = (unsigned char)((1 - ((laneMask >> j) & 1)) | (((validFace >> j) & 1) << 1) | (((backFacingBeforeSnap >> j) & 1) << 2));
		}
	}
}

template<class T>
class TriangleSetupTask : public ThreadPoolTask {
private:
	Renderer& r;
	DrawCall& drawCall;
	T indices;
	
public:
	TriangleSetupTask(Renderer& r, DrawCall& drawCall, T indices) : r(r), drawCall(drawCall), indices(indices) {
	}
	
	virtual void run(unsigned start, unsigned end, unsigned thread) {
		setupDrawCallTriangles<ZLessMode>(r, drawCall, indices, start, end, thread);
	}
	
	virtual void finished() {
		if (r.rasterizeDrawCallToHim && !r.dense)
			r.rasterizeDrawCallToHim(r, drawCall);
	}
};

void setupDrawCallTriangles(Renderer& r, DrawCall& drawCall) {
	ThreadPool& threadPool = r.getThreadPool();
	
	if (drawCall.indexBuffer.stride == 1) {
		TriangleSetupTask<IndexProvider<unsigned char> >* t = new (drawCall.task) TriangleSetupTask<IndexProvider<unsigned char> >(r, drawCall, IndexProvider<unsigned char>(drawCall.indexBuffer.data));
		threadPool.startTask(t, drawCall.indexBuffer.count, 3*1024, true);
	}
	else if (drawCall.indexBuffer.stride == 2) {
		TriangleSetupTask<IndexProvider<unsigned short> >* t = new (drawCall.task) TriangleSetupTask<IndexProvider<unsigned short> >(r, drawCall, IndexProvider<unsigned short>(drawCall.indexBuffer.data));
		threadPool.startTask(t, drawCall.indexBuffer.count, 3*1024, true);
	}
	else if (drawCall.indexBuffer.stride == 4) {
		TriangleSetupTask<IndexProvider<unsigned int> >* t = new (drawCall.task) TriangleSetupTask<IndexProvider<unsigned int> >(r, drawCall, IndexProvider<unsigned int>(drawCall.indexBuffer.data));
		threadPool.startTask(t, drawCall.indexBuffer.count, 3*1024, true);
	}
	else {
		TriangleSetupTask<IndexProvider<> >* t = new (drawCall.task) TriangleSetupTask<IndexProvider<> >(r, drawCall, IndexProvider<>());
		threadPool.startTask(t, drawCall.vertexBuffer.count, 3*1024, true);
	}
}

}
