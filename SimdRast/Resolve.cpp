//
//  Resolve.cpp
//  SimdRast
//
//  Created by Rasmus Barringer on 2012-11-02.
//  Copyright (c) 2012 Rasmus Barringer. All rights reserved.
//

#include "Resolve.h"
#include "SimdMath.h"
#include "SimdDouble.h"
#include "ZMode.h"
#include "QuickSort.h"

namespace srast {

SRAST_ALIGNED(64) static const float sampleLocationsX[] = {
	0.031250f, 0.093750f, 0.156250f, 0.218750f,
	0.281250f, 0.343750f, 0.406250f, 0.468750f,
	0.531250f, 0.593750f, 0.656250f, 0.718750f,
	0.781250f, 0.843750f, 0.906250f, 0.968750f,
};
SRAST_ALIGNED(64) static const float sampleLocationsY[] = {
	0.343750f, 0.843750f, 0.531250f, 0.093750f,
	0.718750f, 0.468750f, 0.218750f, 0.968750f,
	0.593750f, 0.281250f, 0.781250f, 0.031250f,
	0.406250f, 0.906250f, 0.156250f, 0.656250f,
};

static const unsigned samplesPerPixelLog2 = 4;
static const unsigned samplesPerPixel = 1 << samplesPerPixelLog2;

#ifdef _WIN32
inline unsigned __builtin_ctz(unsigned x) {
   DWORD r = 0;
   _BitScanForward(&r, x);
   return r;
}
#endif

struct PixelSamples {
	unsigned fragmentCount;
	unsigned sampleMask;
	unsigned earlyOut;
	unsigned isImportant;
	float zmax;
	float padding[3];
	float z[samplesPerPixel];
	unsigned c[samplesPerPixel];
	unsigned long long fragments[samplesPerPixel];
};

struct SRAST_ALIGNED(8) BinnedTriangle {
	unsigned idx;
	unsigned z;
};

struct ResolveContext {
	static const unsigned maxTrianglesPerTile = 8*1024;
	static const unsigned maxFragments = samplesPerPixel << (tileSizeLog2 + tileSizeLog2);
	static const int maxAttributeSizeInSSE = 8;

	BinnedTriangle triangles[maxTrianglesPerTile];

	unsigned long long fragments[maxFragments + 16]; // Expanded for end marker.
	float inAttributes[maxAttributeSizeInSSE*maxFragments*4*3];
	float outAttributes[maxAttributeSizeInSSE*maxFragments*4*3];

	unsigned targetPixels[1 << (tileSizeLog2 + tileSizeLog2)];
	float targetPixelsX[1 << (tileSizeLog2 + tileSizeLog2)];
	float targetPixelsY[1 << (tileSizeLog2 + tileSizeLog2)];
	SRAST_SIMD_ALIGNED PixelSamples targetPixelSamples[1 << (tileSizeLog2 + tileSizeLog2)];
	unsigned targetPixelCount;
	float tileX;
	float tileY;
	float halfWidth;
	float halfHeight;
};

#define FRAGCMP_DRAWCALL 0xffff000000000000ull
#define FRAGCMP_TRIANGLE 0x0000ffffff000000ull
#define FRAGCMP_PIXEL    0x0000000000ff0000ull
#define FRAGCMP_SAMPLES  0x000000000000ffffull

inline unsigned long long fragCmp(unsigned long long a, unsigned long long b, unsigned long long flags) {
	return (a ^ b) & flags;
}

inline int3 triangleIndices(const DrawCall& drawCall, unsigned triangle) {
	int3 indices;
	
	switch (drawCall.indexBuffer.stride) {
		case 1:
		{
			const unsigned char* array = static_cast<const unsigned char*>(drawCall.indexBuffer.data) + triangle*3;
			indices = int3(array[0], array[1], array[2]);
		}
			break;
		case 2:
		{
			const unsigned short* array = static_cast<const unsigned short*>(drawCall.indexBuffer.data) + triangle*3;
			indices = int3(array[0], array[1], array[2]);
		}
			break;
		case 4:
		{
			const unsigned int* array = static_cast<const unsigned int*>(drawCall.indexBuffer.data) + triangle*3;
			indices = int3(array[0], array[1], array[2]);
		}
			break;
		default:
			indices = int3(triangle*3+0, triangle*3+1, triangle*3+2);
			break;
	}
	
	return indices;
}

static unsigned shadeDrawCallFragments(ResolveContext& context, const DrawCall* __restrict drawCalls, unsigned firstFragment, unsigned long long* __restrict fragments) {
	float tx = context.tileX;
	float ty = context.tileY;
	
	float* __restrict targetPixelsX = context.targetPixelsX;
	float* __restrict targetPixelsY = context.targetPixelsY;
	float* __restrict inAttributes = context.inAttributes;
	float* __restrict outAttributes = context.outAttributes;

	unsigned short drawCallIdx = fragments[firstFragment] >> 48;
	const DrawCall& drawCall = drawCalls[drawCallIdx];
	
	Shader* attributeShader = drawCall.attributeRenderState.getShader();
		
	unsigned inAttributeSizeInSSE = drawCall.attributeBuffer.stride / 16;
	unsigned outAttributeSizeInSSE = attributeShader->outputStride() / 16;

	// Gather per triangle input attributes.
	unsigned lastFragment = firstFragment;
	unsigned attributeCount = 0;
		
	do {
		unsigned long long triangleRef = fragments[lastFragment];
		unsigned triangle = (triangleRef >> 24) & 0xffffff;
			
		float* attributes0 = inAttributes + (attributeCount++) * inAttributeSizeInSSE * 4;
		float* attributes1 = inAttributes + (attributeCount++) * inAttributeSizeInSSE * 4;
		float* attributes2 = inAttributes + (attributeCount++) * inAttributeSizeInSSE * 4;

		int3 indices = triangleIndices(drawCall, triangle);
			
		const float* drawCallAttributes = static_cast<const float*>(drawCalls[drawCallIdx].attributeBuffer.data);
		const float* drawCallAttributes0 = drawCallAttributes + indices.x * inAttributeSizeInSSE * 4;
		const float* drawCallAttributes1 = drawCallAttributes + indices.y * inAttributeSizeInSSE * 4;
		const float* drawCallAttributes2 = drawCallAttributes + indices.z * inAttributeSizeInSSE * 4;
		
		for (unsigned i = 0; i < inAttributeSizeInSSE; ++i) {
			_mm_store_ps(attributes0 + i*4, _mm_load_ps(drawCallAttributes0 + i*4));
			_mm_store_ps(attributes1 + i*4, _mm_load_ps(drawCallAttributes1 + i*4));
			_mm_store_ps(attributes2 + i*4, _mm_load_ps(drawCallAttributes2 + i*4));
		}
			
		triangleRef &= FRAGCMP_DRAWCALL|FRAGCMP_TRIANGLE;

		do {
			++lastFragment;
		}
		while ((fragments[lastFragment] & (FRAGCMP_DRAWCALL|FRAGCMP_TRIANGLE)) == triangleRef);
	}
	while ((fragments[lastFragment] >> 48) == drawCallIdx);
	
	unsigned maxDrawCallFragment = lastFragment;

	// Run attribute shader.
	drawCall.attributeRenderState.executeShader(inAttributes, outAttributes, attributeCount);
		
	// Interpolate attributes to fragments.
	lastFragment = firstFragment;
	attributeCount = 0;

	float halfWidth = context.halfWidth;
	float halfHeight = context.halfHeight;
		
	do {
		unsigned long long triangleRef = fragments[lastFragment];
		unsigned triangle = (triangleRef >> 24) & 0xffffff;

		triangleRef &= FRAGCMP_DRAWCALL|FRAGCMP_TRIANGLE;

		int3 indices = triangleIndices(drawCall, triangle);
			
		__m128 m0 = _mm_load_ps(&drawCall.shadedPositions[indices.x].x);
		__m128 m1 = _mm_load_ps(&drawCall.shadedPositions[indices.y].x);
		__m128 m2 = _mm_load_ps(&drawCall.shadedPositions[indices.z].x);

		__m128 x1x2y1y2 = _mm_unpacklo_ps(m1, m2);
		__m128 x0ooy0oo = _mm_unpacklo_ps(m0, _mm_setzero_ps());
		__m128 z1z2w1w2 = _mm_unpackhi_ps(m1, m2);
		__m128 z0oow0oo = _mm_unpackhi_ps(m0, _mm_setzero_ps());

		vec3<simd4_float> v0;
		vec3<simd4_float> v1;

		v0.x = _mm_movelh_ps(x1x2y1y2, x0ooy0oo);
		v0.y = _mm_movehl_ps(x0ooy0oo, x1x2y1y2);
		v0.z = _mm_movehl_ps(z0oow0oo, z1z2w1w2);

		v1.x = _mm_shuffle_ps(v0.x.mm, v0.x.mm, _MM_SHUFFLE(3,0,2,1));
		v1.y = _mm_shuffle_ps(v0.y.mm, v0.y.mm, _MM_SHUFFLE(3,0,2,1));
		v1.z = _mm_shuffle_ps(v0.z.mm, v0.z.mm, _MM_SHUFFLE(3,0,2,1));

		vec3<simd4_float> edges;

		edges.x = (v0.y*v1.z - v1.y*v0.z)*halfHeight;
		edges.y = (v1.x*v0.z - v0.x*v1.z)*halfWidth;
		edges.z = (v0.x*v1.y - v1.x*v0.y)*(halfHeight*halfWidth);

		simd4_float ex = broadcast_first(sum_reduce(edges.x));
		simd4_float ey = broadcast_first(sum_reduce(edges.y));

		const float* triangleAttributes0 = outAttributes + (attributeCount++) * outAttributeSizeInSSE * 4;
		const float* triangleAttributes1 = outAttributes + (attributeCount++) * outAttributeSizeInSSE * 4;
		const float* triangleAttributes2 = outAttributes + (attributeCount++) * outAttributeSizeInSSE * 4;
			
		do {
			unsigned long long pixelRef = fragments[lastFragment];
			unsigned pixel = (pixelRef >> 16) & 0xff;
			
			float2 sampleLocation;
			sampleLocation.x = (tx + 0.5f) + targetPixelsX[pixel];
			sampleLocation.y = (ty + 0.5f) + targetPixelsY[pixel];

			simd4_float d = edges.x*sampleLocation.x + edges.y*sampleLocation.y + edges.z;

			simd4_float w = sum_reduce(d);
			w = _mm_div_ss(_mm_set_ss(1.0f), w.mm);
			w = broadcast_first(w);

			simd4_float uvw = d*w;

			simd4_float duvwdx = (edges.x - ex*uvw)*w;
			simd4_float duvwdy = (edges.y - ey*uvw)*w;
	
			__m128 mdu = _mm_shuffle_ps(duvwdx.mm, duvwdy.mm, _MM_SHUFFLE(0,0,0,0));
			__m128 mdv = _mm_shuffle_ps(duvwdx.mm, duvwdy.mm, _MM_SHUFFLE(1,1,1,1));
			__m128 mdw = _mm_shuffle_ps(duvwdx.mm, duvwdy.mm, _MM_SHUFFLE(2,2,2,2));

			__m128 mu = _mm_shuffle_ps(uvw.mm, uvw.mm, _MM_SHUFFLE(0,0,0,0));
			__m128 mv = _mm_shuffle_ps(uvw.mm, uvw.mm, _MM_SHUFFLE(1,1,1,1));
			__m128 mw = _mm_shuffle_ps(uvw.mm, uvw.mm, _MM_SHUFFLE(2,2,2,2));

			float* attributes = inAttributes + (lastFragment - firstFragment) * outAttributeSizeInSSE * 4;

			{
				// HACK: First SSE reg used for derivatives. Let the user specify this somehow.
				unsigned i = 0;
					
				__m128 m0 = _mm_load_ps(triangleAttributes0 + i*4);
				__m128 m1 = _mm_load_ps(triangleAttributes1 + i*4);
				__m128 m2 = _mm_load_ps(triangleAttributes2 + i*4);
					
				__m128 m = _mm_add_ps(_mm_add_ps(_mm_mul_ps(m0, mdu), _mm_mul_ps(m1, mdv)), _mm_mul_ps(m2, mdw));
					
				_mm_store_ps(attributes + i*4, m);
			}

			for (unsigned i = 1; i < outAttributeSizeInSSE; ++i) {
				__m128 m0 = _mm_load_ps(triangleAttributes0 + i*4);
				__m128 m1 = _mm_load_ps(triangleAttributes1 + i*4);
				__m128 m2 = _mm_load_ps(triangleAttributes2 + i*4);
					
				__m128 m = _mm_add_ps(_mm_add_ps(_mm_mul_ps(m0, mu), _mm_mul_ps(m1, mv)), _mm_mul_ps(m2, mw));

				_mm_store_ps(attributes + i*4, m);
			}
				
			++lastFragment;
		}
		while ((fragments[lastFragment] & (FRAGCMP_DRAWCALL|FRAGCMP_TRIANGLE)) == triangleRef);
	}
	while (lastFragment < maxDrawCallFragment);
		
	// Run fragment shader.
	drawCall.fragmentRenderState.executeShader(inAttributes, outAttributes, lastFragment - firstFragment);

	return maxDrawCallFragment;
}

static void shadeFragmentsAndBlend(ResolveContext& context, const DrawCall* __restrict drawCalls, unsigned long long* __restrict triangleFragment) {
	PixelSamples* __restrict targetPixelSamples = context.targetPixelSamples;
	unsigned targetPixelCount = context.targetPixelCount;

	unsigned long long* __restrict fragments = context.fragments;
	unsigned* __restrict outAttributes = reinterpret_cast<unsigned*>(context.outAttributes);

	// Gather fragments.
	static const unsigned maxFragmentsPerTriLog2 = tileSizeLog2 + tileSizeLog2;
	unsigned perTriFragmentCount[simd_float::width] = { 0 };

	for (unsigned i = 0; i < targetPixelCount; ++i) {
		PixelSamples& samples = targetPixelSamples[i];
		
		if (!samples.fragmentCount)
			continue;

		for (unsigned j = 0; j < samples.fragmentCount; ++j) {
			unsigned long long f = samples.fragments[j];
			unsigned tri = f & 0xffffff;

			fragments[(tri << maxFragmentsPerTriLog2) + (perTriFragmentCount[tri]++)] = (f >> 24) | triangleFragment[tri];
		}

		samples.fragmentCount = 0;
	}

	unsigned fragmentCount = perTriFragmentCount[0];

	for (unsigned i = 1; i < simd_float::width; ++i) {
		for (unsigned j = 0; j < perTriFragmentCount[i]; ++j) {
			fragments[fragmentCount++] = fragments[(i << maxFragmentsPerTriLog2) + j];
		}
	}

	if (!fragmentCount)
		return;

	// Mark end.
	fragments[fragmentCount] = 0xffffffffffffffffull;

	// Shade fragments.
	shadeDrawCallFragments(context, drawCalls, 0, fragments);

	// Blend fragments.
	for (unsigned i = 0; i < fragmentCount; ++i) {
		unsigned samples = (unsigned)fragments[i] & 0xffff;
		unsigned pixel = ((unsigned)fragments[i] >> 16) & 0xff;

		unsigned* dst = targetPixelSamples[pixel].c;
		unsigned sourcePixel = outAttributes[i];

		__m128i sourcePattern = _mm_set1_epi32(sourcePixel);
		__m128i sourceRgba = _mm_cvtepu8_epi16(sourcePattern);

		__m128i alpha = _mm_shufflelo_epi16(sourceRgba, _MM_SHUFFLE(3,3,3,3));
		__m128i invAlpha = _mm_sub_epi16(_mm_set1_epi16(0xff), alpha);

		__m128i op1 = _mm_unpacklo_epi16(_mm_set1_epi16(0xff), invAlpha);

		do {
			unsigned j = __builtin_ctz(samples);
			
			unsigned destPixel = dst[j];
			__m128i destPattern = _mm_set1_epi32(destPixel);

			unsigned mask = 0;

			for (unsigned s = 0; s < samplesPerPixel; s += 4) { // Note: Cannot do this with AVX (float) since some samples might be Inf or Nan.
				__m128i targetPattern = _mm_load_si128(reinterpret_cast<const __m128i*>(dst + s));
				__m128i match = _mm_cmpeq_epi32(destPattern, targetPattern);
				mask |= _mm_movemask_ps(_mm_castsi128_ps(match)) << s;
			}

			mask &= samples;
			samples &= ~mask;

			__m128i destRgba = _mm_cvtepu8_epi16(destPattern);
			__m128i op2 = _mm_unpacklo_epi16(sourceRgba, destRgba);
			__m128i result = _mm_madd_epi16(op1, op2);

			result = _mm_add_epi32(result, _mm_set1_epi32(255/2));

			// r/255 = (r + 1 + (r >> 8)) >> 8, r < 65535
			result = _mm_srli_epi32(_mm_add_epi32(_mm_add_epi32(result, _mm_set1_epi32(1)), _mm_srli_epi32(result, 8)), 8);

			result = _mm_packus_epi32(result, result);
			result = _mm_packus_epi16(result, result);
			unsigned resultPixel = _mm_cvtsi128_si32(result);

			do {
				dst[__builtin_ctz(mask)] = resultPixel;
				mask &= mask-1;
			}
			while (mask);
		}
		while (samples);
	}
}

static void shadeTile(ResolveContext& context, const DrawCall* __restrict drawCalls, bool earlyOut) {
	unsigned targetPixelCount = context.targetPixelCount;

	PixelSamples* __restrict targetPixelSamples = context.targetPixelSamples;
	unsigned long long* __restrict fragments = context.fragments;
	const unsigned* __restrict outAttributes = reinterpret_cast<const unsigned*>(context.outAttributes);
	
	// Gather fragments.
	unsigned fragmentCount = 0;
	
	for (unsigned i = 0; i < targetPixelCount; ++i) {
		PixelSamples& samples = targetPixelSamples[i];
		
		if (!samples.fragmentCount)
			continue;
		
		if (samples.earlyOut) {
			if (earlyOut && samples.sampleMask == ((1 << samplesPerPixel)-1)) {
				if (!samples.isImportant || samples.fragmentCount == 1) {
					// No important triangle or single fragment.
					continue;
				}
				else if (samples.fragmentCount == 2 &&
						 fragCmp(samples.fragments[0], samples.fragments[1], FRAGCMP_DRAWCALL) == 0) {
					// Two fragments sharing edge.
					unsigned drawCallIdx = samples.fragments[0] >> 48;
					unsigned a = (samples.fragments[0] >> 24) & 0xffffff;
					unsigned b = (samples.fragments[1] >> 24) & 0xffffff;
					
					const unsigned* adj = drawCalls[drawCallIdx].adjacency + a*3;
					
					if (adj[0] == b || adj[1] == b || adj[2] == b) {
						continue;
					}
				}
			}
			
			samples.earlyOut = 0;
		}

		for (unsigned j = 0; j < samples.fragmentCount; ++j)
			fragments[fragmentCount++] = samples.fragments[j];

		samples.fragmentCount = 0;
	}
	
	// Mark end.
	fragments[fragmentCount] = 0xffffffffffffffffull;
	
	// Sort them.
	quickSort(fragments, fragments + fragmentCount);
	
	// Process fragments for one draw call at a time.
	unsigned firstFragment = 0;
	
	while (firstFragment < fragmentCount) {
		unsigned maxDrawCallFragment = shadeDrawCallFragments(context, drawCalls, firstFragment, fragments);
		
		// Distribute shaded fragments to samples.
		unsigned lastFragment = firstFragment;

		do {
			unsigned color = outAttributes[lastFragment - firstFragment];

			unsigned long long fragment = fragments[lastFragment];

			unsigned pixel = (fragment >> 16) & 0xff;
			unsigned samples = fragment & 0xffff;

			unsigned* dst = targetPixelSamples[pixel].c;

			if (samples == (1 << samplesPerPixel)-1) {
				__m128 src = _mm_castsi128_ps(_mm_set1_epi32(color));

				_mm_store_ps(reinterpret_cast<float*>(dst) + 0, src);

				if (samplesPerPixel >= 8) {
					_mm_store_ps(reinterpret_cast<float*>(dst) + 4, src);
				}

				if (samplesPerPixel >= 16) {
					_mm_store_ps(reinterpret_cast<float*>(dst) + 8, src);
					_mm_store_ps(reinterpret_cast<float*>(dst) + 12, src);
				}
			}
			else {
				unsigned i = __builtin_ctz(samples) & ~0x3;
				samples >>= i;

				do {
					if (samples & 1) dst[i] = color;
					if (samples & 2) dst[i+1] = color;
					if (samples & 4) dst[i+2] = color;
					if (samples & 8) dst[i+3] = color;
					i += 4;
					samples >>= 4;
				}
				while (samples);
			}

			++lastFragment;
		}
		while (lastFragment < maxDrawCallFragment);

		// Done.
		firstFragment = lastFragment;
	}
}

#define TRIANGLE_LEFT(p)	p == 0 || tri + p < triangleCount
#define TRIANGLE_INDEX(p)	triangles[tri + p].idx & 0xffffff
#define TRIANGLE_Z(p)		uint32_as_float(triangles[tri + p].z)
#define TRIANGLE_DC(p)		drawCallMap[triangles[tri + p].idx >> 24]

#define GATHER_TRIANGLE(m0, m1, m2, p) \
__m128 m0, m1, m2;\
if (TRIANGLE_LEFT(p)) {\
unsigned ind = TRIANGLE_INDEX(p);\
unsigned dc = TRIANGLE_DC(p);\
float4* edges = drawCalls[dc].edges;\
importantMask += (drawCalls[dc].flags[ind] & FACEFLAG_IMPORTANT) << p;\
m0 = _mm_load_ps(&edges[ind*4+0].x);\
m1 = _mm_load_ps(&edges[ind*4+1].x);\
m2 = _mm_load_ps(&edges[ind*4+2].x);\
highpEdgeZ0[p] = drawCalls[dc].highpEdgeZ0[ind];\
highpEdgeZ1[p] = drawCalls[dc].highpEdgeZ1[ind];\
highpEdgeZ2[p] = drawCalls[dc].highpEdgeZ2[ind];\
triangleFragment[p] = (((unsigned long long)ind << 24) | ((unsigned long long)dc << 48));\
triangleZ[p] = TRIANGLE_Z(p);\
laneMask += laneMask + 1;\
}

#define GATHER_TRIANGLE_LO(m0, m1, m2, p) \
__m256 m0, m1, m2;\
if (TRIANGLE_LEFT(p)) {\
unsigned ind = TRIANGLE_INDEX(p);\
unsigned dc = TRIANGLE_DC(p);\
float4* edges = drawCalls[dc].edges;\
importantMask += (drawCalls[dc].flags[ind] & FACEFLAG_IMPORTANT) << p;\
m0 = _mm256_castps128_ps256(_mm_load_ps(&edges[ind*4+0].x));\
m1 = _mm256_castps128_ps256(_mm_load_ps(&edges[ind*4+1].x));\
m2 = _mm256_castps128_ps256(_mm_load_ps(&edges[ind*4+2].x));\
highpEdgeZ0[p] = drawCalls[dc].highpEdgeZ0[ind];\
highpEdgeZ1[p] = drawCalls[dc].highpEdgeZ1[ind];\
highpEdgeZ2[p] = drawCalls[dc].highpEdgeZ2[ind];\
triangleFragment[p] = (((unsigned long long)ind << 24) | ((unsigned long long)dc << 48));\
triangleZ[p] = TRIANGLE_Z(p);\
laneMask += laneMask + 1;\
}

#define GATHER_TRIANGLE_HI(m0, m1, m2, p) \
if (TRIANGLE_LEFT(p)) {\
unsigned ind = TRIANGLE_INDEX(p);\
unsigned dc = TRIANGLE_DC(p);\
float4* edges = drawCalls[dc].edges;\
importantMask += (drawCalls[dc].flags[ind] & FACEFLAG_IMPORTANT) << p;\
m0 = _mm256_insertf128_ps(m0, _mm_load_ps(&edges[ind*4+0].x), 1);\
m1 = _mm256_insertf128_ps(m1, _mm_load_ps(&edges[ind*4+1].x), 1);\
m2 = _mm256_insertf128_ps(m2, _mm_load_ps(&edges[ind*4+2].x), 1);\
highpEdgeZ0[p] = drawCalls[dc].highpEdgeZ0[ind];\
highpEdgeZ1[p] = drawCalls[dc].highpEdgeZ1[ind];\
highpEdgeZ2[p] = drawCalls[dc].highpEdgeZ2[ind];\
triangleFragment[p] = (((unsigned long long)ind << 24) | ((unsigned long long)dc << 48));\
triangleZ[p] = TRIANGLE_Z(p);\
laneMask += laneMask + 1;\
}

template<class ZMode, bool Opaque, bool ZWrite>
static void resolveDrawCall(unsigned tileZmax, ResolveContext& context, const DrawCall* __restrict drawCalls, unsigned startDrawCallIdx, CompositeBinList& bin, unsigned& idx, unsigned clearColor) {
	
	PixelSamples* __restrict targetPixelSamples = context.targetPixelSamples;
	float* __restrict targetPixelsX = context.targetPixelsX;
	float* __restrict targetPixelsY = context.targetPixelsY;
	unsigned targetPixelCount = context.targetPixelCount;
	float tx = context.tileX;
	float ty = context.tileY;

	// Read all triangles, then sort them. Allows us to do occlusion culling.
	unsigned drawCallMap[256];
	unsigned currentDCMapPos = 0;

	FragmentRenderState fragmentRenderState = drawCalls[startDrawCallIdx].fragmentRenderState;
	
	drawCallMap[currentDCMapPos] = startDrawCallIdx;

	BinnedTriangle* __restrict triangles = context.triangles;
	unsigned triangleCount = 0;
	
	for (;;) {
		for (;;) {
			idx = bin.next();
			
			if (idx & 0x80000000)
				break;
			
			BinnedTriangle tri = {
				idx | (currentDCMapPos << 24),
				bin.depth(),
			};
			
			if (ZMode::less(tri.z, tileZmax))
				triangles[triangleCount++] = tri;
		}
		
		if (!Opaque) // Can only handle one draw call at a time if blending is active.
			break;
		
		if (currentDCMapPos == 255)
			break;
		
		unsigned drawCallIndex = idx & (~0x80000000);
		
		if (drawCallIndex == 0x0fffffff)
			break;
		
		if (fragmentRenderState != drawCalls[drawCallIndex].fragmentRenderState)
			break;
		
		drawCallMap[++currentDCMapPos] = drawCallIndex;
	}
	
	if (Opaque && ZWrite) {
		typename ZMode::SortComparator cmp;
		quickSort(reinterpret_cast<unsigned long long*>(triangles), reinterpret_cast<unsigned long long*>(triangles) + triangleCount, cmp);
	}

	unsigned long long activePixels = (~0ull) >> (64-targetPixelCount);

	for (unsigned tri = 0; tri < triangleCount; tri += simd_float::width) {
		unsigned long long triangleFragment[simd_float::width];
		SRAST_SIMD_ALIGNED float triangleZ[simd_float::width];
		SRAST_SIMD_ALIGNED double highpEdgeZ0[simd_float::width];
		SRAST_SIMD_ALIGNED double highpEdgeZ1[simd_float::width];
		SRAST_SIMD_ALIGNED double highpEdgeZ2[simd_float::width];
		unsigned laneMask = 0;
		unsigned importantMask = 0;
		
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
		
		importantMask >>= 3; // Compensate for flag's bit-position.

		simd_float3 edge0(m00, m10, m20);
		simd_float3 edge1(m01, m11, m21);
		simd_float3 edge2(m02, m12, m22);
		
		simd_float z0(m30);
		simd_float z1(m31);
		simd_float z2(m32);

		z2 += z0*tx + z1*ty;
		
		simd_double hpe0zl, hpe1zl, hpe2zl;
		simd_double hpe0zh, hpe1zh, hpe2zh;
		simd_double txd(tx), tyd(ty);
		
		hpe0zl.load(highpEdgeZ0);
		hpe0zh.load(highpEdgeZ0 + simd_double::width);
		hpe1zl.load(highpEdgeZ1);
		hpe1zh.load(highpEdgeZ1 + simd_double::width);
		hpe2zl.load(highpEdgeZ2);
		hpe2zh.load(highpEdgeZ2 + simd_double::width);
		
		edge0.z = double2float(hpe0zl + txd*float2double_lo(edge0.x) + tyd*float2double_lo(edge0.y),
							   hpe0zh + txd*float2double_hi(edge0.x) + tyd*float2double_hi(edge0.y));
		edge1.z = double2float(hpe1zl + txd*float2double_lo(edge1.x) + tyd*float2double_lo(edge1.y),
							   hpe1zh + txd*float2double_hi(edge1.x) + tyd*float2double_hi(edge1.y));
		edge2.z = double2float(hpe2zl + txd*float2double_lo(edge2.x) + tyd*float2double_lo(edge2.y),
							   hpe2zh + txd*float2double_hi(edge2.x) + tyd*float2double_hi(edge2.y));

		SRAST_SIMD_ALIGNED float edge0xa[simd_float::width], edge0ya[simd_float::width], edge0za[simd_float::width];
		SRAST_SIMD_ALIGNED float edge1xa[simd_float::width], edge1ya[simd_float::width], edge1za[simd_float::width];
		SRAST_SIMD_ALIGNED float edge2xa[simd_float::width], edge2ya[simd_float::width], edge2za[simd_float::width];
		SRAST_SIMD_ALIGNED float z0a[simd_float::width], z1a[simd_float::width], z2a[simd_float::width];
		
		edge0.x.store(edge0xa);
		edge0.y.store(edge0ya);
		edge0.z.store(edge0za);
		edge1.x.store(edge1xa);
		edge1.y.store(edge1ya);
		edge1.z.store(edge1za);
		edge2.x.store(edge2xa);
		edge2.y.store(edge2ya);
		edge2.z.store(edge2za);
		z0.store(z0a);
		z1.store(z1a);
		z2.store(z2a);
		
		SRAST_SIMD_ALIGNED float LUT0[simd_float::width*samplesPerPixel];
		SRAST_SIMD_ALIGNED float LUT1[simd_float::width*samplesPerPixel];
		SRAST_SIMD_ALIGNED float LUT2[simd_float::width*samplesPerPixel];
		SRAST_SIMD_ALIGNED float LUTZ[simd_float::width*samplesPerPixel];
		
		for (unsigned i = 0; i < simd_float::width; ++i) {
			for (unsigned j = 0; j < samplesPerPixel; j += simd_float::width) {
				simd_float locX, locY;
				
				locX.load(&sampleLocationsX[j]);
				locY.load(&sampleLocationsY[j]);
				
				unsigned idx = i*samplesPerPixel + j;
				
				simd_float lut0 = mad(locX, simd_float::broadcast_load(edge0xa+i), mad(locY, simd_float::broadcast_load(edge0ya+i), simd_float::broadcast_load(edge0za+i)));
				simd_float lut1 = mad(locX, simd_float::broadcast_load(edge1xa+i), mad(locY, simd_float::broadcast_load(edge1ya+i), simd_float::broadcast_load(edge1za+i)));
				simd_float lut2 = mad(locX, simd_float::broadcast_load(edge2xa+i), mad(locY,simd_float::broadcast_load(edge2ya+i), simd_float::broadcast_load(edge2za+i)));
				simd_float lutz = mad(locX, simd_float::broadcast_load(z0a+i), mad(locY, simd_float::broadcast_load(z1a+i), simd_float::broadcast_load(z2a+i)));
				
				lut0.store(&LUT0[idx]);
				lut1.store(&LUT1[idx]);
				lut2.store(&LUT2[idx]);
				lutz.store(&LUTZ[idx]);
			}
		}
		
		// Make conservative.
		edge0.z += ((edge0.x > simd_float::zero()) & edge0.x) + ((edge0.y > simd_float::zero()) & edge0.y);
		edge1.z += ((edge1.x > simd_float::zero()) & edge1.x) + ((edge1.y > simd_float::zero()) & edge1.y);
		edge2.z += ((edge2.x > simd_float::zero()) & edge2.x) + ((edge2.y > simd_float::zero()) & edge2.y);

		for (unsigned p = 0; p < targetPixelCount; ++p) {
			unsigned long long pixelBit = 1ull << (unsigned long long)p;

			if (Opaque && ZWrite) {
				if ((activePixels & pixelBit) == 0)
					continue;
			}

			unsigned pixelSampleMask = targetPixelSamples[p].sampleMask;
			unsigned pixelMask = laneMask;

			float pixelZmax = targetPixelSamples[p].zmax;

			if (pixelSampleMask == (1 << samplesPerPixel)-1) {
				simd_float tz;
				tz.load(triangleZ);

				pixelMask &= mask(ZMode::less(tz, pixelZmax));

				if (!pixelMask) {
					if (Opaque && ZWrite)
						activePixels ^= pixelBit;
					continue;
				}
			}

			simd_float2 topLeft(simd_float::broadcast_load(targetPixelsX+p), simd_float::broadcast_load(targetPixelsY+p));

			simd_float tl0 = mad(edge0.x, topLeft.x, edge0.y*topLeft.y);
			simd_float tl1 = mad(edge1.x, topLeft.x, edge1.y*topLeft.y);
			simd_float tl2 = mad(edge2.x, topLeft.x, edge2.y*topLeft.y);
			
			simd_float cd0 = tl0 + edge0.z;
			simd_float cd1 = tl1 + edge1.z;
			simd_float cd2 = tl2 + edge2.z;

			pixelMask &= ~mask(cd0 | cd1 | cd2);
			
			if (!pixelMask)
				continue;
			
			SRAST_SIMD_ALIGNED float tl0a[simd_float::width];
			SRAST_SIMD_ALIGNED float tl1a[simd_float::width];
			SRAST_SIMD_ALIGNED float tl2a[simd_float::width];
			SRAST_SIMD_ALIGNED float tlza[simd_float::width];
			
			simd_float tlz = mad(z0, topLeft.x, z1*topLeft.y);

			tl0.store(tl0a);
			tl1.store(tl1a);
			tl2.store(tl2a);
			tlz.store(tlza);

			if (Opaque) {
				if (pixelMask & importantMask)
					targetPixelSamples[p].isImportant = 1;
			}

			do {
				unsigned tri = __builtin_ctz(pixelMask);
				pixelMask &= pixelMask-1;
				
				simd_float zmax = pixelZmax;
				
				if (pixelSampleMask == (1 << samplesPerPixel)-1 && ZMode::less(pixelZmax, triangleZ[tri]))
					continue;

				simd_float tl0 = simd_float::broadcast_load(tl0a+tri);
				simd_float tl1 = simd_float::broadcast_load(tl1a+tri);
				simd_float tl2 = simd_float::broadcast_load(tl2a+tri);
				simd_float tlz = simd_float::broadcast_load(tlza+tri);
				
				unsigned triangleSampleMask = 0;

				for (unsigned s = 0; s < samplesPerPixel; s += simd_float::width) {
					unsigned lutIdx = tri*samplesPerPixel + s;

					simd_float lut0, lut1, lut2, lutz;

					lut0.load(&LUT0[lutIdx]);
					lut1.load(&LUT1[lutIdx]);
					lut2.load(&LUT2[lutIdx]);
					lutz.load(&LUTZ[lutIdx]);

					simd_float d0 = lut0 + tl0;
					simd_float d1 = lut1 + tl1;
					simd_float d2 = lut2 + tl2;
					simd_float z = lutz + tlz;

					simd_float oldZ;
					oldZ.load(&targetPixelSamples[p].z[s]);

					simd_float sampleMask = not_and(d0 | d1 | d2, ZMode::test(z, oldZ));

					if (ZWrite) {
						blend(oldZ, z, sampleMask).store(&targetPixelSamples[p].z[s]);
						zmax = blend(zmax, ZMode::max(z, zmax), sampleMask);
					}

					triangleSampleMask |= mask(sampleMask) << s;
				}
				
				if (Opaque)
					pixelSampleMask |= triangleSampleMask;
				
				if (ZWrite)
					pixelZmax = ZMode::max_reduce(zmax).first_float();

				if (triangleSampleMask) {
					unsigned fragmentCount = targetPixelSamples[p].fragmentCount;
					unsigned long long* fragments = targetPixelSamples[p].fragments;

					if (Opaque) {
						unsigned long long f = (triangleSampleMask | (p << 16)) | triangleFragment[tri];
						unsigned long long sc = ~((unsigned long long)triangleSampleMask);

						unsigned i = 0;

						for (; i < fragmentCount; ++i) {
							if (((fragments[i] &= sc) & 0xffff) == 0) {
								fragments[i] = f;
								f = 0;
							}
						}

						if (f) {
							fragments[fragmentCount++] = f;
							targetPixelSamples[p].fragmentCount = fragmentCount;
						}
					}
					else {
						// Note: Custom fragment for shade and blend.
						unsigned long long f = tri | ((unsigned long long)(triangleSampleMask | (p << 16)) << 24);

						fragments[fragmentCount++] = f;
						targetPixelSamples[p].fragmentCount = fragmentCount;
					}
				}
			}
			while (pixelMask);

			if (Opaque)
				targetPixelSamples[p].sampleMask = pixelSampleMask;
			
			if (ZWrite)
				targetPixelSamples[p].zmax = pixelZmax;
		}

		if (!Opaque)
			shadeFragmentsAndBlend(context, drawCalls, triangleFragment);

		if (Opaque && ZWrite) {
			if (!activePixels)
				break;
		}
	}
}

void resolveTile(Renderer& r, unsigned tx, unsigned ty, unsigned thread) {
	if (!r.importanceMap.isSet(tileSizeLog2, tx, ty))
		return;

	unsigned width = r.frameBufferWidth;
	unsigned height = r.frameBufferHeight;
	
	static const int halfTile = 1 << (tileSizeLog2-1);

	ResolveContext* context = static_cast<ResolveContext*>(r.localAllocators[thread]->allocateTemporary(sizeof(ResolveContext)));
	
	PixelSamples* __restrict targetPixelSamples = context->targetPixelSamples;
	unsigned* __restrict targetPixels = context->targetPixels;
	float* __restrict targetPixelsX = context->targetPixelsX;
	float* __restrict targetPixelsY = context->targetPixelsY;
	unsigned targetPixelCount = 0;
	
	for (unsigned y = ty; y < height && y < ty + (1<<tileSizeLog2); ++y) {
		for (unsigned x = tx; x < width && x < tx + (1<<tileSizeLog2); ++x) {
			if (r.importanceMap.isSet(0, x, y)) {
				targetPixelsX[targetPixelCount] = (float)((int)x-((int)tx+halfTile));
				targetPixelsY[targetPixelCount] = (float)((int)y-((int)ty+halfTile));
				targetPixels[targetPixelCount++] = x | (y << 16);
			}
		}
	}

	context->targetPixelCount = targetPixelCount;
	context->tileX = (int)tx+halfTile - width*0.5f;
	context->tileY = (int)ty+halfTile - height*0.5f;
	context->halfWidth = 0.5f*width;
	context->halfHeight = 0.5f*height;
	
	simd_float cClear = _mm_castsi128_ps(_mm_set1_epi32(r.clearColor));
	simd_float zClear(SRAST_FAR_Z);

	unsigned earlyOut = r.dense ? 0 : 1;

	for (unsigned i = 0; i < targetPixelCount; ++i) {
		targetPixelSamples[i].zmax = SRAST_NEAR_Z;
		targetPixelSamples[i].fragmentCount = 0;
		targetPixelSamples[i].sampleMask = 0;
		targetPixelSamples[i].isImportant = 0;
		targetPixelSamples[i].earlyOut = earlyOut;

		for (unsigned s = 0; s < samplesPerPixel; s += simd_float::width) {
			cClear.store(reinterpret_cast<float*>(&targetPixelSamples[i].c[s]));
			zClear.store(&targetPixelSamples[i].z[s]);
		}
	}

	CompositeBinList& bin = r.compositeBinListArray[thread];
	unsigned tileZmax = bin.setup(r.poolAllocator, r.binListArray, tx, ty, r.frameNumber);
	
	unsigned idx = bin.next();
	bool isShaded = true;

	while (idx != 0x8fffffff) {
		unsigned i = idx & (~0x80000000);
		const DrawCall& drawCall = r.drawCalls[i];

		if (drawCall.fragmentRenderState.isOpaque()) {
			isShaded = false;
			
			if (drawCall.fragmentRenderState.getDepthWrite())
				resolveDrawCall<ZLessMode, true, true>(tileZmax, *context, &r.drawCalls[0], i, bin, idx, r.clearColor);
			else
				resolveDrawCall<ZLessMode, true, false>(tileZmax, *context, &r.drawCalls[0], i, bin, idx, r.clearColor);
		}
		else {
			if (!r.transparentImportance) {
				if (!isShaded) {
					shadeTile(*context, &r.drawCalls[0], true);
					isShaded = true;
					
					// Remove target pixels that early-out.
					unsigned removedPixels = 0;
					for (unsigned i = 0; i < targetPixelCount; ++i) {
						if (targetPixelSamples[i].earlyOut) {
							++removedPixels;
							unsigned x = targetPixels[i] & 0xffff;
							unsigned y = targetPixels[i] >> 16;
							r.importanceMap.setTo(x, y, tileSizeLog2);
						}
						else if (removedPixels) {
							targetPixels[i-removedPixels] = targetPixels[i];
							targetPixelSamples[i-removedPixels] = targetPixelSamples[i];
							targetPixelsX[i-removedPixels] = targetPixelsX[i];
							targetPixelsY[i-removedPixels] = targetPixelsY[i];
						}
					}
					
					targetPixelCount -= removedPixels;
					context->targetPixelCount = targetPixelCount;
					
					if (!targetPixelCount)
						return;
				}
			}
			else {
				if (!isShaded) {
					shadeTile(*context, &r.drawCalls[0], false);
					isShaded = true;
				}
			}

			if (drawCall.fragmentRenderState.getDepthWrite())
				resolveDrawCall<ZLessMode, false, true>(tileZmax, *context, &r.drawCalls[0], i, bin, idx, r.clearColor);
			else
				resolveDrawCall<ZLessMode, false, false>(tileZmax, *context, &r.drawCalls[0], i, bin, idx, r.clearColor);
		}
	}
	
	if (!isShaded)
		shadeTile(*context, &r.drawCalls[0], true);
	
	unsigned* pixels = static_cast<unsigned*>(r.frameBuffer);
	unsigned pitch = r.frameBufferPitch;

	for (unsigned i = 0; i < targetPixelCount; ++i) {
		if (targetPixelSamples[i].earlyOut) {
			unsigned x = targetPixels[i] & 0xffff;
			unsigned y = targetPixels[i] >> 16;
			r.importanceMap.setTo(x, y, tileSizeLog2);
			continue;
		}
		
		__m128i pixelColor = _mm_setzero_si128();
		
		for (unsigned s = 0; s < samplesPerPixel; s += 4) {
			__m128i samples = _mm_load_si128(reinterpret_cast<const __m128i*>(&targetPixelSamples[i].c[s]));
			
			__m128i sample0 = _mm_cvtepu8_epi32(samples);
			__m128i sample1 = _mm_cvtepu8_epi32(_mm_srli_si128(samples, 4));
			__m128i sample2 = _mm_cvtepu8_epi32(_mm_srli_si128(samples, 8));
			__m128i sample3 = _mm_cvtepu8_epi32(_mm_srli_si128(samples, 12));
			
			sample0 = _mm_add_epi32(sample0, sample1);
			sample2 = _mm_add_epi32(sample2, sample3);
			
			pixelColor = _mm_add_epi32(pixelColor, sample0);
			pixelColor = _mm_add_epi32(pixelColor, sample2);
		}
		
		pixelColor = _mm_srli_epi32(pixelColor, samplesPerPixelLog2);
		pixelColor = _mm_packus_epi32(pixelColor, pixelColor);
		pixelColor = _mm_packus_epi16(pixelColor, pixelColor);

		unsigned x = targetPixels[i] & 0xffff;
		unsigned y = targetPixels[i] >> 16;

		pixels[(height-y-1)*pitch + x] = _mm_cvtsi128_si32(pixelColor);
	}
}

}
