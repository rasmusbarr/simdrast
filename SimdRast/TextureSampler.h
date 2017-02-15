//
//  TextureSampler.h
//  SimdRast
//
//  Created by Rasmus Barringer on 2013-01-14.
//  Copyright (c) 2013 Rasmus Barringer. All rights reserved.
//

#ifndef SimdRast_TextureSampler_h
#define SimdRast_TextureSampler_h

#include "SimdMath.h"
#include "SimdTrans.h"

namespace srast {

struct NoTextureModifier {
	template<class T>
	T modify(const T& texel, unsigned lane) const {
		return texel;
	}
};

struct GreaterTextureModifier {
	SRAST_SIMD_ALIGNED float ref[simd_float::width];

	simd4_float modify(const simd4_float& texel, unsigned lane) const {
		return (texel > ref[lane]) & simd4_float(1.0f);
	}

	float modify(float texel, unsigned lane) const {
		return texel > ref[lane] ? 1.0f : 0.0f;
	}
};

struct LessTextureModifier {
	SRAST_SIMD_ALIGNED float ref[simd_float::width];

	simd4_float modify(const simd4_float& texel, unsigned lane) const {
		return (texel < ref[lane]) & simd4_float(1.0f);
	}

	float modify(float texel, unsigned lane) const {
		return texel < ref[lane] ? 1.0f : 0.0f;
	}
};

struct RepeatTextureAddressMode {
	static int boundCoord(int x, int extent) {
		return x & (extent-1);
	}
};

struct ClampTextureAddressMode {
	static int boundCoord(int x, int extent) {
		if (x < 0) x = 0;
		else if (x >= extent) x = extent-1;
		return x;
	}
};

template<class R, class T>
struct TexelGatherer {
};

template<class T, class F, class A = RepeatTextureAddressMode, class M = NoTextureModifier>
class TextureSampler {
private:
	typedef typename F::ReturnType ReturnType;
	typedef typename F::TexelType TexelType;
	typedef typename F::RawTexelType RawTexelType;

	const T& texture;
	
public:
	M modifier;

	TextureSampler(const T& texture) : texture(texture) {
	}

	SRAST_FORCEINLINE simd_float computeLod(const simd_float2& ddx, const simd_float2& ddy) const {
		float fwidth = (float)(int)(1 << texture.widthLog2);
		float fheight = (float)(int)(1 << texture.heightLog2);

		simd_float2 sddx(ddx.x*fwidth, ddx.y*fwidth);
		simd_float2 sddy(ddy.x*fheight, ddy.y*fheight);
		
		simd_float mip = log2(max(dot(sddx, sddx), dot(sddy, sddy)))*0.5f;
		mip = mip & (mip > 0.0f);
		return mip;
	}

	SRAST_FORCEINLINE ReturnType sample(const simd_float& x, const simd_float& y, const simd_float& lod, unsigned laneMask) const {
		SRAST_SIMD4_ALIGNED int x0[simd_float::width];
		SRAST_SIMD4_ALIGNED int y0[simd_float::width];
		SRAST_SIMD4_ALIGNED int mip0[simd_float::width];
		SRAST_SIMD_ALIGNED float mipt[simd_float::width];

		simd_float mip = min(lod, texture.mipCount-(1.0f-1e-6f));

		simd_float mipFloor = floor(mip);

		(mip-mipFloor).store(mipt);

		__m128i offset = _mm_set1_epi32(1 << (F::lerpPrecisionLog2-1));
		
#ifdef SRAST_AVX
		__m128i mipIndexL = _mm_cvttps_epi32(mipFloor.low().mm);
		__m128i mipIndexH = _mm_cvttps_epi32(mipFloor.high().mm);

		_mm_store_si128(reinterpret_cast<__m128i*>(mip0), mipIndexL);
		_mm_store_si128(reinterpret_cast<__m128i*>(mip0)+1, mipIndexH);

		__m128i mipWidthLog2L = _mm_sub_epi32(_mm_set1_epi32(texture.widthLog2 + F::lerpPrecisionLog2), mipIndexL);
		__m128i mipWidthLog2H = _mm_sub_epi32(_mm_set1_epi32(texture.widthLog2 + F::lerpPrecisionLog2), mipIndexH);

		__m128i mipHeightLog2L = _mm_sub_epi32(_mm_set1_epi32(texture.heightLog2 + F::lerpPrecisionLog2), mipIndexL);
		__m128i mipHeightLog2H = _mm_sub_epi32(_mm_set1_epi32(texture.heightLog2 + F::lerpPrecisionLog2), mipIndexH);

		_mm_store_si128(reinterpret_cast<__m128i*>(x0), _mm_sub_epi32(_mm_cvttps_epi32(simd_float_add_exp(x.low(), mipWidthLog2L).mm), offset));
		_mm_store_si128(reinterpret_cast<__m128i*>(x0)+1, _mm_sub_epi32(_mm_cvttps_epi32(simd_float_add_exp(x.high(), mipWidthLog2H).mm), offset));

		_mm_store_si128(reinterpret_cast<__m128i*>(y0), _mm_sub_epi32(_mm_cvttps_epi32(simd_float_add_exp(y.low(), mipHeightLog2L).mm), offset));
		_mm_store_si128(reinterpret_cast<__m128i*>(y0)+1, _mm_sub_epi32(_mm_cvttps_epi32(simd_float_add_exp(y.high(), mipHeightLog2H).mm), offset));
#else
		__m128i mipIndex = _mm_cvttps_epi32(mipFloor.mm);
		_mm_store_si128(reinterpret_cast<__m128i*>(mip0), mipIndex);
		
		__m128i mipWidthLog2 = _mm_sub_epi32(_mm_set1_epi32(texture.widthLog2 + F::lerpPrecisionLog2), mipIndex);
		__m128i mipHeightLog2 = _mm_sub_epi32(_mm_set1_epi32(texture.heightLog2 + F::lerpPrecisionLog2), mipIndex);
		
		_mm_store_si128(reinterpret_cast<__m128i*>(x0), _mm_sub_epi32(_mm_cvttps_epi32(simd_float_add_exp(x, mipWidthLog2).mm), offset));
		_mm_store_si128(reinterpret_cast<__m128i*>(y0), _mm_sub_epi32(_mm_cvttps_epi32(simd_float_add_exp(y, mipHeightLog2).mm), offset));
#endif

		return TexelGatherer<ReturnType, TexelType>::gatherTrilinear(*this, x0, y0, mip0, mipt, laneMask);
	}

	SRAST_FORCEINLINE ReturnType sampleMip(const simd_float& x, const simd_float& y, unsigned mip, unsigned laneMask) const {
		SRAST_SIMD4_ALIGNED int x0[simd_float::width];
		SRAST_SIMD4_ALIGNED int y0[simd_float::width];

		unsigned mipWidthLog2 = texture.widthLog2 + F::lerpPrecisionLog2 - mip;
		unsigned mipHeightLog2 = texture.heightLog2 + F::lerpPrecisionLog2 - mip;

		__m128i offset = _mm_set1_epi32(1 << (F::lerpPrecisionLog2-1));

#ifdef SRAST_AVX
		_mm_store_si128(reinterpret_cast<__m128i*>(x0), _mm_sub_epi32(_mm_cvttps_epi32(simd_float_add_exp(x.low(), mipWidthLog2).mm), offset));
		_mm_store_si128(reinterpret_cast<__m128i*>(x0)+1, _mm_sub_epi32(_mm_cvttps_epi32(simd_float_add_exp(x.high(), mipWidthLog2).mm), offset));

		_mm_store_si128(reinterpret_cast<__m128i*>(y0), _mm_sub_epi32(_mm_cvttps_epi32(simd_float_add_exp(y.low(), mipHeightLog2).mm), offset));
		_mm_store_si128(reinterpret_cast<__m128i*>(y0)+1, _mm_sub_epi32(_mm_cvttps_epi32(simd_float_add_exp(y.high(), mipHeightLog2).mm), offset));
#else
		_mm_store_si128(reinterpret_cast<__m128i*>(x0), _mm_sub_epi32(_mm_cvttps_epi32(simd_float_add_exp(x, mipWidthLog2).mm), offset));
		_mm_store_si128(reinterpret_cast<__m128i*>(y0), _mm_sub_epi32(_mm_cvttps_epi32(simd_float_add_exp(y, mipHeightLog2).mm), offset));
#endif

		return TexelGatherer<ReturnType, TexelType>::gatherBilinear(*this, x0, y0, mip, laneMask);
	}

	SRAST_FORCEINLINE TexelType operator () (int x0, int y0, int mip0, float mipt, unsigned lane) const {
		TexelType m0 = inlinedBilinearLookup(x0, y0, mip0, lane);

		x0 = (x0 >> 1) - (1 << (F::lerpPrecisionLog2 - 2));
		y0 = (y0 >> 1) - (1 << (F::lerpPrecisionLog2 - 2));

		TexelType m1 = inlinedBilinearLookup(x0, y0, mip0+1 < texture.mipCount ? mip0+1 : mip0, lane);
		TexelType m = m0*(1.0f-mipt) + m1*mipt;

		return F::scale(m);
	}

	SRAST_FORCEINLINE TexelType operator () (int x0, int y0, int mipIndex, unsigned lane) const {
		return F::scale(inlinedBilinearLookup(x0, y0, mipIndex, lane));
	}

private:
	SRAST_FORCEINLINE TexelType inlinedBilinearLookup(int x0, int y0, int mipIndex, unsigned lane) const {
		int xt = x0 & ((1 << F::lerpPrecisionLog2) - 1);
		int yt = y0 & ((1 << F::lerpPrecisionLog2) - 1);

		x0 >>= F::lerpPrecisionLog2;
		y0 >>= F::lerpPrecisionLog2;

		int mipWidthLog2 = texture.widthLog2 - mipIndex;
		int mipHeightLog2 = texture.heightLog2 - mipIndex;
		
		int mipWidth = 1 << mipWidthLog2;
		int mipHeight = 1 << mipHeightLog2;

		int x1 = x0+1;
		int y1 = y0+1;
		
		x0 = A::boundCoord(x0, mipWidth);
		x1 = A::boundCoord(x1, mipWidth);
		y0 = A::boundCoord(y0, mipHeight);
		y1 = A::boundCoord(y1, mipHeight);

		const float *p00, *p10, *p01, *p11;
		texture.getQuad(x0, y0, x1, y1, mipIndex, p00, p10, p01, p11);

		RawTexelType t00 = modifier.modify(F::texel(p00), lane);
		RawTexelType t10 = modifier.modify(F::texel(p10), lane);
		RawTexelType t01 = modifier.modify(F::texel(p01), lane);
		RawTexelType t11 = modifier.modify(F::texel(p11), lane);
		
		return F::interpolate(t00, t10, t01, t11, xt, yt);
	}
};

struct TextureFormatR8G8B8A8 {
	static const unsigned lerpPrecisionLog2 = 6;

	typedef simd_float4 ReturnType;
	typedef simd4_float TexelType;
	typedef __m128i RawTexelType;

	static RawTexelType texel(const float* tex) {
		return _mm_castps_si128(_mm_load_ss(tex));
	}

	static TexelType interpolate(const RawTexelType& t00, const RawTexelType& t10, const RawTexelType& t01, const RawTexelType& t11, int xt, int yt) {
		__m128i mxt = _mm_set1_epi16((unsigned short)(((1 << lerpPrecisionLog2) - xt) | (xt << 8)));
		__m128i myt = _mm_set1_epi32(((1 << lerpPrecisionLog2) - yt) | (yt << 16));

		__m128i texels01 = _mm_unpacklo_epi8(t00, t10);
		__m128i texels23 = _mm_unpacklo_epi8(t01, t11);

		__m128i texels = _mm_castps_si128(_mm_shuffle_ps(_mm_castsi128_ps(texels01), _mm_castsi128_ps(texels23), _MM_SHUFFLE(1,0,1,0)));
		texels = _mm_maddubs_epi16(texels, mxt);

		return _mm_cvtepi32_ps(_mm_madd_epi16(myt, _mm_unpacklo_epi16(texels, _mm_shuffle_epi32(texels, _MM_SHUFFLE(3,2,3,2)))));
	}

	static TexelType scale(const TexelType& x) {
		return x * (1.0f / (255.0f * (1 << (lerpPrecisionLog2*2))));
	}
};

struct TextureFormatR32F {
	static const unsigned lerpPrecisionLog2 = 8;

	typedef simd_float ReturnType;
	typedef float TexelType;
	typedef float RawTexelType;

	static RawTexelType texel(const float* tex) {
		return *tex;
	}

	static TexelType interpolate(const RawTexelType& t00, const RawTexelType& t10, const RawTexelType& t01, const RawTexelType& t11, int xt, int yt) {
		float bsx = (float)xt;
		float asx = (float)(int)(1 << lerpPrecisionLog2) - bsx;

		float bsy = (float)yt;
		float asy = (float)(int)(1 << lerpPrecisionLog2) - bsy;

		TexelType ty0 = t00*asx + t10*bsx;
		TexelType ty1 = t01*asx + t11*bsx;

		return ty0*asy + ty1*bsy;
	}

	static TexelType scale(const TexelType& x) {
		return x * (1.0f / (1 << (lerpPrecisionLog2*2)));
	}
};

template<>
struct TexelGatherer<simd_float, float> {
	typedef simd_float ReturnType;
	typedef float TexelType;

	template<class F>
	SRAST_FORCEINLINE static ReturnType gatherTrilinear(F& f, const int* x0, const int* y0, const int* mip0, const float* mipt, unsigned laneMask) {
		SRAST_SIMD_ALIGNED float texels[simd_float::width];

		for (unsigned i = 0; i < simd_float::width; ++i) {
			if (laneMask & (1 << i))
				texels[i] = f(x0[i], y0[i], mip0[i], mipt[i], i);
		}

		simd_float t;
		t.load(texels);
		return t;
	}

	template<class F>
	SRAST_FORCEINLINE static ReturnType gatherBilinear(F& f, const int* x0, const int* y0, int mip, unsigned laneMask) {
		SRAST_SIMD_ALIGNED float texels[simd_float::width];

		for (unsigned i = 0; i < simd_float::width; ++i) {
			if (laneMask & (1 << i))
				texels[i] = f(x0[i], y0[i], mip, i);
		}

		simd_float t;
		t.load(texels);
		return t;
	}
};

template<>
struct TexelGatherer<simd_float4, simd4_float> {
	typedef simd_float4 ReturnType;
	typedef simd4_float TexelType;

	template<class F>
	SRAST_FORCEINLINE static ReturnType gatherTrilinear(F& f, const int* x0, const int* y0, const int* mip0, const float* mipt, unsigned laneMask) {
		SRAST_SIMD_ALIGNED float texels[simd4_float::width*simd_float::width];

		for (unsigned i = 0; i < simd_float::width; ++i) {
			if (laneMask & (1 << i)) {
				unsigned idx = (i & (simd4_float::width-1)) * simd_float::width;
				if (i >= simd4_float::width)
					idx += simd4_float::width;
				f(x0[i], y0[i], mip0[i], mipt[i], i).store(texels + idx);
			}
		}

		simd_float t0, t1, t2, t3;
		t0.load(texels);
		t1.load(texels + simd_float::width);
		t2.load(texels + 2*simd_float::width);
		t3.load(texels + 3*simd_float::width);

#ifdef SRAST_AVX
		SRAST_MM256_TRANSPOSE4_PS(t0.mm, t1.mm, t2.mm, t3.mm);
#else
		_MM_TRANSPOSE4_PS(t0.mm, t1.mm, t2.mm, t3.mm);
#endif
		return simd_float4(t0, t1, t2, t3);
	}

	template<class F>
	SRAST_FORCEINLINE static ReturnType gatherBilinear(F& f, const int* x0, const int* y0, int mip, unsigned laneMask) {
		SRAST_SIMD_ALIGNED float texels[simd4_float::width*simd_float::width];

		for (unsigned i = 0; i < simd_float::width; ++i) {
			if (laneMask & (1 << i)) {
				unsigned idx = (i & (simd4_float::width-1)) * simd_float::width;
				if (i >= simd4_float::width)
					idx += simd4_float::width;
				f(x0[i], y0[i], mip, i).store(texels + idx);
			}
		}

		simd_float t0, t1, t2, t3;
		t0.load(texels);
		t1.load(texels + simd_float::width);
		t2.load(texels + 2*simd_float::width);
		t3.load(texels + 3*simd_float::width);

#ifdef SRAST_AVX
		SRAST_MM256_TRANSPOSE4_PS(t0.mm, t1.mm, t2.mm, t3.mm);
#else
		_MM_TRANSPOSE4_PS(t0.mm, t1.mm, t2.mm, t3.mm);
#endif
		return simd_float4(t0, t1, t2, t3);
	}
};

}

#endif
