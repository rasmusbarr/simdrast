//
//  SimdMath.h
//  SimdRast
//
//  Created by Rasmus Barringer on 2012-10-21.
//  Copyright (c) 2012 Rasmus Barringer. All rights reserved.
//

#ifndef SimdRast_SimdMath_h
#define SimdRast_SimdMath_h

#include "VectorMath.h"
#include <iostream>

namespace srast {

#define SRAST_SIMD4_ALIGNED SRAST_ALIGNED(16)
#define SRAST_SIMD8_ALIGNED SRAST_ALIGNED(32)

#ifdef SRAST_AVX
#define SRAST_SIMD_ALIGNED SRAST_SIMD8_ALIGNED
#else
#define SRAST_SIMD_ALIGNED SRAST_SIMD4_ALIGNED
#endif

struct simd4_float {
	static const int width = 4;
	
	__m128 mm;
	
	simd4_float() {
	}
	
	simd4_float(__m128 v) : mm(v) {
	}
	
	simd4_float(float v) {
		mm = _mm_set1_ps(v);
	}
	
	simd4_float(float x, float y, float z, float w) {
		mm = _mm_set_ps(w, z, y, x);
	}
	
	static simd4_float zero() {
		return _mm_setzero_ps();
	}
	
	static simd4_float broadcast_load(const float* f) {
#ifdef SRAST_AVX
		return _mm_broadcast_ss(f);
#else
		return *f;
#endif
	}
	
	simd4_float operator ~ () const {
		return _mm_andnot_ps(mm, _mm_set1_ps(uint32_as_float(0xffffffff)));
	}
	
	simd4_float operator + () const {
		return *this;
	}
	
	simd4_float operator - () const {
		return _mm_sub_ps(_mm_setzero_ps(), mm);
	}
	
	simd4_float operator * (const simd4_float& v) const {
		return _mm_mul_ps(mm, v.mm);
	}
	
	simd4_float operator + (const simd4_float& v) const {
		return _mm_add_ps(mm, v.mm);
	}
	
	simd4_float operator - (const simd4_float& v) const {
		return _mm_sub_ps(mm, v.mm);
	}
	
	simd4_float operator / (const simd4_float& v) const {
		return _mm_div_ps(mm, v.mm);
	}
	
	void operator *= (const simd4_float& v) {
		mm = _mm_mul_ps(mm, v.mm);
	}
	
	void operator += (const simd4_float& v) {
		mm = _mm_add_ps(mm, v.mm);
	}
	
	void operator -= (const simd4_float& v) {
		mm = _mm_sub_ps(mm, v.mm);
	}
	
	void operator /= (const simd4_float& v) {
		mm = _mm_div_ps(mm, v.mm);
	}
	
	simd4_float operator | (const simd4_float& v) const {
		return _mm_or_ps(mm, v.mm);
	}
	
	simd4_float operator & (const simd4_float& v) const {
		return _mm_and_ps(mm, v.mm);
	}
	
	simd4_float operator ^ (const simd4_float& v) const {
		return _mm_xor_ps(mm, v.mm);
	}
	
	simd4_float operator > (const simd4_float& v) const {
		return _mm_cmpgt_ps(mm, v.mm);
	}
	
	simd4_float operator == (const simd4_float& v) const {
		return _mm_cmpeq_ps(mm, v.mm);
	}
	
	simd4_float operator != (const simd4_float& v) const {
		return _mm_cmpneq_ps(mm, v.mm);
	}
	
	simd4_float operator >= (const simd4_float& v) const {
		return _mm_cmpge_ps(mm, v.mm);
	}
	
	simd4_float operator < (const simd4_float& v) const {
		return _mm_cmplt_ps(mm, v.mm);
	}
	
	simd4_float operator <= (const simd4_float& v) const {
		return _mm_cmple_ps(mm, v.mm);
	}
	
	void load(const float* v) {
		mm = _mm_load_ps(v);
	}
	
	void store(float* v) const {
		_mm_store_ps(v, mm);
	}
	
	void stream_store(float* v) const {
		_mm_stream_ps(v, mm);
	}
	
	void masked_store(float* v, const simd4_float& mask) {
		_mm_store_ps(v, _mm_blendv_ps(_mm_load_ps(v), mm, mask.mm));
	}
	
	void shuffle_right() {
		mm = _mm_shuffle_ps(mm, mm, _MM_SHUFFLE(0, 3, 2, 1));
	}
	
	float first_float() const {
		return _mm_cvtss_f32(mm);
	}
	
	int first_float_to_int() const {
		return _mm_cvttss_si32(mm);
	}
	
	void print(const char* name, unsigned laneMask = (1 << width)-1) const {
		std::cout << name;
		
		SRAST_SIMD_ALIGNED float va[width];
		store(va);
		
		for (unsigned i = 0; i < width; ++i) {
			std::cout << " ";
			std::cout.precision(16);
			std::cout.width(16);
			if (laneMask & (1 << i)) {
				std::cout << va[i];
			}
			else {
				std::cout << "-";
			}
		}
		
		std::cout << std::endl;
	}
};

inline simd4_float mad(const simd4_float& op1, const simd4_float& op2, const simd4_float& addend) {
	return op1*op2 + addend;
}

inline simd4_float not_and(const simd4_float& a, const simd4_float& b) {
	return _mm_andnot_ps(a.mm, b.mm);
}

inline simd4_float blend(const simd4_float& a, const simd4_float& b, const simd4_float& mask) {
	return _mm_blendv_ps(a.mm, b.mm, mask.mm);
}

inline simd4_float floor(const simd4_float& v) {
	return _mm_round_ps(v.mm, _MM_FROUND_TO_NEG_INF);
}

inline simd4_float ceil(const simd4_float& v) {
	return _mm_round_ps(v.mm, _MM_FROUND_TO_POS_INF);
}

inline simd4_float round(const simd4_float& v) {
	return _mm_round_ps(v.mm, _MM_FROUND_TO_NEAREST_INT);
}

inline simd4_float trunc(const simd4_float& v) {
	return _mm_round_ps(v.mm, _MM_FROUND_TO_ZERO);
}

inline simd4_float min(const simd4_float& a, const simd4_float& b) {
	return _mm_min_ps(a.mm, b.mm);
}

inline simd4_float max(const simd4_float& a, const simd4_float& b) {
	return _mm_max_ps(a.mm, b.mm);
}

inline simd4_float sqrt(const simd4_float& v) {
	return _mm_sqrt_ps(v.mm);
}
	
inline simd4_float rsqrt(const simd4_float& v) {
	return _mm_rsqrt_ps(v.mm);
}

inline simd4_float recip(const simd4_float& v) {
	return _mm_rcp_ps(v.mm);
}

inline simd4_float abs(const simd4_float& v) {
	return _mm_andnot_ps(_mm_set1_ps(-0.0f), v.mm);
}

inline int mask(const simd4_float& v) {
	return _mm_movemask_ps(v.mm);
}

inline bool all(const simd4_float& v) {
	return mask(v) == 0xf;
}

inline bool any(const simd4_float& v) {
	return mask(v) != 0;
}

inline simd4_float broadcast_first(const simd4_float& v) {
	return _mm_shuffle_ps(v.mm, v.mm, _MM_SHUFFLE(0, 0, 0, 0));
}

inline simd4_float sum_reduce(const simd4_float& v) {
	__m128 a = v.mm;
    a = _mm_add_ps(a, _mm_movehl_ps(a, a));
    a = _mm_add_ss(a, _mm_shuffle_ps(a, a, _MM_SHUFFLE(1, 1, 1, 1)));
	return a;
}

inline simd4_float min_reduce(const simd4_float& v) {
	__m128 a = v.mm;
    a = _mm_min_ps(a, _mm_movehl_ps(a, a)); // min(a0, a2), min(a1, a3), min(a2, a2), min(a3, a3)
    a = _mm_min_ss(a, _mm_shuffle_ps(a, a, _MM_SHUFFLE(1, 1, 1, 1))); // min(a0, a1), a1, a2, a3
	return a;
}

inline simd4_float max_reduce(const simd4_float& v) {
	__m128 a = v.mm;
    a = _mm_max_ps(a, _mm_movehl_ps(a, a)); // min(a0, a2), min(a1, a3), min(a2, a2), min(a3, a3)
    a = _mm_max_ss(a, _mm_shuffle_ps(a, a, _MM_SHUFFLE(1, 1, 1, 1))); // min(a0, a1), a1, a2, a3
	return a;
}

inline simd4_float simd_float_add_exp(const simd4_float& x, const __m128i& y) {
	__m128i bits = _mm_castps_si128(x.mm);
	__m128i exp = _mm_slli_epi32(y, 23);
	bits = _mm_add_epi32(bits, exp);
	return _mm_castsi128_ps(bits);
}

inline simd4_float simd_float_add_exp(const simd4_float& x, unsigned y) {
	__m128i bits = _mm_castps_si128(x.mm);
	bits = _mm_add_epi32(bits, _mm_set1_epi32(y << 23));
	return _mm_castsi128_ps(bits);
}

inline simd4_float next_after(const simd4_float& x) { // The next representable float.
	// Note: Does handle fraction overflow automatically.
	// Note: Does NOT handle NaN/Inf.
	__m128i bits = _mm_castps_si128(x.mm);
	bits = _mm_add_epi32(bits, _mm_set1_epi32(1));
	return _mm_castsi128_ps(bits);
}

inline void simd_float_load_aos4(simd4_float& f0, simd4_float& f1, simd4_float& f2, simd4_float& f3, const float* v, unsigned strideInFloats) {
	__m128 m0 = _mm_load_ps(v + 0*strideInFloats);
	__m128 m1 = _mm_load_ps(v + 1*strideInFloats);
	__m128 m2 = _mm_load_ps(v + 2*strideInFloats);
	__m128 m3 = _mm_load_ps(v + 3*strideInFloats);
	
	_MM_TRANSPOSE4_PS(m0, m1, m2, m3);
	
	f0.mm = m0;
	f1.mm = m1;
	f2.mm = m2;
	f3.mm = m3;
}

inline void simd_float_store_aos4(float* v, unsigned strideInFloats, const simd4_float& f0, const simd4_float& f1, const simd4_float& f2, const simd4_float& f3) {
	__m128 m0 = f0.mm;
	__m128 m1 = f1.mm;
	__m128 m2 = f2.mm;
	__m128 m3 = f3.mm;
	
	_MM_TRANSPOSE4_PS(m0, m1, m2, m3);
	
	_mm_store_ps(v + 0*strideInFloats, m0);
	_mm_store_ps(v + 1*strideInFloats, m1);
	_mm_store_ps(v + 2*strideInFloats, m2);
	_mm_store_ps(v + 3*strideInFloats, m3);
}

inline void simd_float_stream_aos4(float* v, unsigned strideInFloats, const simd4_float& f0, const simd4_float& f1, const simd4_float& f2, const simd4_float& f3) {
	__m128 m0 = f0.mm;
	__m128 m1 = f1.mm;
	__m128 m2 = f2.mm;
	__m128 m3 = f3.mm;
	
	_MM_TRANSPOSE4_PS(m0, m1, m2, m3);
	
	_mm_stream_ps(v + 0*strideInFloats, m0);
	_mm_stream_ps(v + 1*strideInFloats, m1);
	_mm_stream_ps(v + 2*strideInFloats, m2);
	_mm_stream_ps(v + 3*strideInFloats, m3);
}

inline void simd_float_load_aos8(simd4_float& f0, simd4_float& f1, simd4_float& f2, simd4_float& f3, simd4_float& f4, simd4_float& f5, simd4_float& f6, simd4_float& f7, const float* v, unsigned strideInFloats) {
	simd_float_load_aos4(f0, f1, f2, f3, v, strideInFloats);
	simd_float_load_aos4(f4, f5, f6, f7, v + 4, strideInFloats);
}

inline void simd_float_store_aos8(float* v, unsigned strideInFloats, const simd4_float& f0, const simd4_float& f1, const simd4_float& f2, const simd4_float& f3, const simd4_float& f4, const simd4_float& f5, const simd4_float& f6, const simd4_float& f7) {
	simd_float_store_aos4(v, strideInFloats, f0, f1, f2, f3);
	simd_float_store_aos4(v + 4, strideInFloats, f4, f5, f6, f7);
}

inline void simd_float_stream_aos8(float* v, unsigned strideInFloats, const simd4_float& f0, const simd4_float& f1, const simd4_float& f2, const simd4_float& f3, const simd4_float& f4, const simd4_float& f5, const simd4_float& f6, const simd4_float& f7) {
	simd_float_stream_aos4(v, strideInFloats, f0, f1, f2, f3);
	simd_float_stream_aos4(v + 4, strideInFloats, f4, f5, f6, f7);
}

inline void simd_float_store_rgba(const simd4_float& r, const simd4_float& g, const simd4_float& b, const simd4_float& a, unsigned* output) {
	__m128 m0 = r.mm;
	__m128 m1 = g.mm;
	__m128 m2 = b.mm;
	__m128 m3 = a.mm;
	
	_MM_TRANSPOSE4_PS(m0, m1, m2, m3);
	
	__m128 mf = _mm_set1_ps(255.0f);
	__m128 ma = _mm_set1_ps(0.5f);
	
	m0 = _mm_add_ps(_mm_mul_ps(m0, mf), ma);
	m1 = _mm_add_ps(_mm_mul_ps(m1, mf), ma);
	m2 = _mm_add_ps(_mm_mul_ps(m2, mf), ma);
	m3 = _mm_add_ps(_mm_mul_ps(m3, mf), ma);

	__m128i mi0 = _mm_cvttps_epi32(m0);
	__m128i mi1 = _mm_cvttps_epi32(m1);
	__m128i mi2 = _mm_cvttps_epi32(m2);
	__m128i mi3 = _mm_cvttps_epi32(m3);
	
	mi0 = _mm_packus_epi32(mi0, mi1);
	mi2 = _mm_packus_epi32(mi2, mi3);
	mi0 = _mm_packus_epi16(mi0, mi2);
	
	_mm_store_si128(reinterpret_cast<__m128i*>(output), mi0);
}

#ifdef SRAST_AVX
struct simd8_float {
	static const int width = 8;
	
	__m256 mm;
	
	simd8_float() {
	}
	
	simd8_float(__m128 v) {
		mm = _mm256_castps128_ps256(v);
		mm = _mm256_insertf128_ps(mm, v, 1);
	}
	
	simd8_float(__m256 v) : mm(v) {
	}
	
	simd8_float(float v) {
		mm = _mm256_set1_ps(v);
	}
	
	simd8_float(float x0, float y0, float z0, float w0, float x1, float y1, float z1, float w1) {
		mm = _mm256_set_ps(x0, y0, z0, w0, x1, y1, z1, w1);
	}
	
	static simd8_float zero() {
		return _mm256_setzero_ps();
	}
	
	static simd8_float broadcast_load(const float* f) {
		return _mm256_broadcast_ss(f);
	}

	simd8_float operator ~ () const {
		return _mm256_andnot_ps(mm, _mm256_set1_ps(uint32_as_float(0xffffffff)));
	}
	
	simd8_float operator - () const {
		return _mm256_sub_ps(_mm256_setzero_ps(), mm);
	}
	
	simd8_float operator * (const simd8_float& v) const {
		return _mm256_mul_ps(mm, v.mm);
	}
	
	simd8_float operator + (const simd8_float& v) const {
		return _mm256_add_ps(mm, v.mm);
	}
	
	simd8_float operator - (const simd8_float& v) const {
		return _mm256_sub_ps(mm, v.mm);
	}
	
	simd8_float operator / (const simd8_float& v) const {
		return _mm256_div_ps(mm, v.mm);
	}
	
	void operator *= (const simd8_float& v) {
		mm = _mm256_mul_ps(mm, v.mm);
	}
	
	void operator += (const simd8_float& v) {
		mm = _mm256_add_ps(mm, v.mm);
	}
	
	void operator -= (const simd8_float& v) {
		mm = _mm256_sub_ps(mm, v.mm);
	}
	
	void operator /= (const simd8_float& v) {
		mm = _mm256_div_ps(mm, v.mm);
	}

	simd8_float operator | (const simd8_float& v) const {
		return _mm256_or_ps(mm, v.mm);
	}
	
	simd8_float operator & (const simd8_float& v) const {
		return _mm256_and_ps(mm, v.mm);
	}
	
	simd8_float operator ^ (const simd8_float& v) const {
		return _mm256_xor_ps(mm, v.mm);
	}

	simd8_float operator > (const simd8_float& v) const {
		return _mm256_cmp_ps(mm, v.mm, _CMP_NLE_US);
	}
	
	simd8_float operator == (const simd8_float& v) const {
		return _mm256_cmp_ps(mm, v.mm, _CMP_EQ_OQ);
	}

	simd8_float operator != (const simd8_float& v) const {
		return _mm256_cmp_ps(mm, v.mm, _CMP_NEQ_OQ);
	}
	
	simd8_float operator >= (const simd8_float& v) const {
		return _mm256_cmp_ps(mm, v.mm, _CMP_NLT_US);
	}
	
	simd8_float operator < (const simd8_float& v) const {
		return _mm256_cmp_ps(mm, v.mm, _CMP_LT_OS);
	}
	
	simd8_float operator <= (const simd8_float& v) const {
		return _mm256_cmp_ps(mm, v.mm, _CMP_LE_OS);
	}
	
	void load(const float* v) {
		mm = _mm256_load_ps(v);
	}
	
	void store(float* v) const {
		_mm256_store_ps(v, mm);
	}

	void stream_store(float* v) const {
		_mm256_stream_ps(v, mm);
	}
	
	void masked_store(float* v, const simd8_float& mask) {
		_mm256_store_ps(v, _mm256_blendv_ps(_mm256_load_ps(v), mm, mask.mm));
	}
	
	void shuffle_right() {
		__m256 tmp =  (__m256)_mm256_permute_ps(mm, _MM_SHUFFLE(0, 3, 2, 1));
		mm = _mm256_blend_ps(tmp, _mm256_permute2f128_ps(tmp, tmp, 1), 136);
	}
	
	float first_float() const {
		return _mm_cvtss_f32(_mm256_castps256_ps128(mm));
	}
	
	int first_float_to_int() const {
		return _mm_cvttss_si32(_mm256_castps256_ps128(mm));
	}

	simd4_float low() const {
		return _mm256_castps256_ps128(mm);
	}

	simd4_float high() const {
		return _mm256_extractf128_ps(mm, 1);
	}
};
	
inline simd8_float mad(const simd8_float& op1, const simd8_float& op2, const simd8_float& addend) {
	return op1*op2 + addend;
}

inline simd8_float not_and(const simd8_float& a, const simd8_float& b) {
	return _mm256_andnot_ps(a.mm, b.mm);
}

inline simd8_float blend(const simd8_float& a, const simd8_float& b, const simd8_float& mask) {
	return _mm256_blendv_ps(a.mm, b.mm, mask.mm);
}

inline simd8_float floor(const simd8_float& v) {
	return _mm256_round_ps(v.mm, _MM_FROUND_TO_NEG_INF);
}

inline simd8_float ceil(const simd8_float& v) {
	return _mm256_round_ps(v.mm, _MM_FROUND_TO_POS_INF);
}

inline simd8_float round(const simd8_float& v) {
	return _mm256_round_ps(v.mm, _MM_FROUND_TO_NEAREST_INT);
}

inline simd8_float trunc(const simd8_float& v) {
	return _mm256_round_ps(v.mm, _MM_FROUND_TO_ZERO);
}

inline simd8_float min(const simd8_float& a, const simd8_float& b) {
	return _mm256_min_ps(a.mm, b.mm);
}

inline simd8_float max(const simd8_float& a, const simd8_float& b) {
	return _mm256_max_ps(a.mm, b.mm);
}

inline simd8_float sqrt(const simd8_float& v) {
	return _mm256_sqrt_ps(v.mm);
}
	
inline simd8_float rsqrt(const simd8_float& v) {
	return _mm256_rsqrt_ps(v.mm);
}

inline simd8_float recip(const simd8_float& v) {
	return _mm256_rcp_ps(v.mm);
}

inline simd8_float abs(const simd8_float& v) {
	return _mm256_andnot_ps(_mm256_set1_ps(-0.0f), v.mm);
}

inline int mask(const simd8_float& v) {
	return _mm256_movemask_ps(v.mm);
}

inline bool all(const simd8_float& v) {
	return mask(v) == 0xff;
}

inline bool any(const simd8_float& v) {
	return mask(v) != 0;
}

inline __m256 simd_float_combine(__m128 a, __m128 b) {
	__m256 c = _mm256_castps128_ps256(a);
	c = _mm256_insertf128_ps(c, b, 1);
	return c;
}

inline simd8_float broadcast_first(const simd8_float& v) {
	__m128 mm = _mm256_castps256_ps128(v.mm);
	mm = _mm_shuffle_ps(mm, mm, _MM_SHUFFLE(0, 0, 0, 0));
	return simd_float_combine(mm, mm);
}

inline simd8_float min_reduce(const simd8_float& v) {
	__m128 a = _mm_min_ps(_mm256_castps256_ps128(v.mm), _mm256_extractf128_ps(v.mm, 1));
    a = _mm_min_ps(a, _mm_movehl_ps(a, a)); // min(a0, a2), min(a1, a3), min(a2, a2), min(a3, a3)
    a = _mm_min_ss(a, _mm_shuffle_ps(a, a, _MM_SHUFFLE(1, 1, 1, 1))); // min(a0, a1), a1, a2, a3
	return _mm256_castps128_ps256(a);
}

inline simd8_float max_reduce(const simd8_float& v) {
	__m128 a = _mm_max_ps(_mm256_castps256_ps128(v.mm), _mm256_extractf128_ps(v.mm, 1));
    a = _mm_max_ps(a, _mm_movehl_ps(a, a)); // min(a0, a2), min(a1, a3), min(a2, a2), min(a3, a3)
    a = _mm_max_ss(a, _mm_shuffle_ps(a, a, _MM_SHUFFLE(1, 1, 1, 1))); // min(a0, a1), a1, a2, a3
	return _mm256_castps128_ps256(a);
}

inline simd8_float next_after(const simd8_float& x) {
	return simd_float_combine(next_after(x.low()).mm, next_after(x.high()).mm);
}

inline __m256 _mm256_movelh_ps(__m256 a, __m256 b) {
	return _mm256_shuffle_ps(a, b, _MM_SHUFFLE(1, 0, 1, 0));
}

inline __m256 _mm256_movehl_ps(__m256 a, __m256 b) {
	return _mm256_shuffle_ps(b, a, _MM_SHUFFLE(3, 2, 3, 2));
}

#define SRAST_MM256_TRANSPOSE4_PS(row0, row1, row2, row3) \
do {\
	__m256 tmp3, tmp2, tmp1, tmp0;\
	tmp0 = _mm256_unpacklo_ps((row0), (row1));\
	tmp2 = _mm256_unpacklo_ps((row2), (row3));\
	tmp1 = _mm256_unpackhi_ps((row0), (row1));\
	tmp3 = _mm256_unpackhi_ps((row2), (row3));\
	(row0) = _mm256_movelh_ps(tmp0, tmp2);\
	(row1) = _mm256_movehl_ps(tmp2, tmp0);\
	(row2) = _mm256_movelh_ps(tmp1, tmp3);\
	(row3) = _mm256_movehl_ps(tmp3, tmp1);\
} while (0)

#define SRAST_MM256_TRANSPOSE8_PS(row0, row1, row2, row3, row4, row5, row6, row7) \
do {\
	__m256 t0, t1, t2, t3, t4, t5, t6, t7;\
	__m256 tt0, tt1, tt2, tt3, tt4, tt5, tt6, tt7;\
	\
	t0 = _mm256_unpacklo_ps(row0, row1);\
	t1 = _mm256_unpackhi_ps(row0, row1);\
	t2 = _mm256_unpacklo_ps(row2, row3);\
	t3 = _mm256_unpackhi_ps(row2, row3);\
	t4 = _mm256_unpacklo_ps(row4, row5);\
	t5 = _mm256_unpackhi_ps(row4, row5);\
	t6 = _mm256_unpacklo_ps(row6, row7);\
	t7 = _mm256_unpackhi_ps(row6, row7);\
	\
	tt0 = _mm256_shuffle_ps(t0, t2, _MM_SHUFFLE(1,0,1,0));\
	tt1 = _mm256_shuffle_ps(t0, t2, _MM_SHUFFLE(3,2,3,2));\
	tt2 = _mm256_shuffle_ps(t1, t3, _MM_SHUFFLE(1,0,1,0));\
	tt3 = _mm256_shuffle_ps(t1, t3, _MM_SHUFFLE(3,2,3,2));\
	tt4 = _mm256_shuffle_ps(t4, t6, _MM_SHUFFLE(1,0,1,0));\
	tt5 = _mm256_shuffle_ps(t4, t6, _MM_SHUFFLE(3,2,3,2));\
	tt6 = _mm256_shuffle_ps(t5, t7, _MM_SHUFFLE(1,0,1,0));\
	tt7 = _mm256_shuffle_ps(t5, t7, _MM_SHUFFLE(3,2,3,2));\
	\
	row0 = _mm256_permute2f128_ps(tt0, tt4, 0x20);\
	row1 = _mm256_permute2f128_ps(tt1, tt5, 0x20);\
	row2 = _mm256_permute2f128_ps(tt2, tt6, 0x20);\
	row3 = _mm256_permute2f128_ps(tt3, tt7, 0x20);\
	row4 = _mm256_permute2f128_ps(tt0, tt4, 0x31);\
	row5 = _mm256_permute2f128_ps(tt1, tt5, 0x31);\
	row6 = _mm256_permute2f128_ps(tt2, tt6, 0x31);\
	row7 = _mm256_permute2f128_ps(tt3, tt7, 0x31);\
} while (0)

inline void simd_float_load_aos4_indexed(simd8_float& f0, simd8_float& f1, simd8_float& f2, simd8_float& f3, const float* v, unsigned strideInFloats, const unsigned* indices, unsigned indexStride) {
	__m256 m0 = _mm256_castps128_ps256(_mm_load_ps(v + indices[0*indexStride]*strideInFloats));
	__m256 m1 = _mm256_castps128_ps256(_mm_load_ps(v + indices[1*indexStride]*strideInFloats));
	__m256 m2 = _mm256_castps128_ps256(_mm_load_ps(v + indices[2*indexStride]*strideInFloats));
	__m256 m3 = _mm256_castps128_ps256(_mm_load_ps(v + indices[3*indexStride]*strideInFloats));
	
	m0 = _mm256_insertf128_ps(m0, _mm_load_ps(v + indices[4*indexStride]*strideInFloats), 1);
	m1 = _mm256_insertf128_ps(m1, _mm_load_ps(v + indices[5*indexStride]*strideInFloats), 1);
	m2 = _mm256_insertf128_ps(m2, _mm_load_ps(v + indices[6*indexStride]*strideInFloats), 1);
	m3 = _mm256_insertf128_ps(m3, _mm_load_ps(v + indices[7*indexStride]*strideInFloats), 1);

	SRAST_MM256_TRANSPOSE4_PS(m0, m1, m2, m3);
	
	f0.mm = m0;
	f1.mm = m1;
	f2.mm = m2;
	f3.mm = m3;
}

inline void simd_float_load_aos4(simd8_float& f0, simd8_float& f1, simd8_float& f2, simd8_float& f3, const float* v, unsigned strideInFloats) {
	__m256 m0 = _mm256_castps128_ps256(_mm_load_ps(v + 0*strideInFloats));
	__m256 m1 = _mm256_castps128_ps256(_mm_load_ps(v + 1*strideInFloats));
	__m256 m2 = _mm256_castps128_ps256(_mm_load_ps(v + 2*strideInFloats));
	__m256 m3 = _mm256_castps128_ps256(_mm_load_ps(v + 3*strideInFloats));
	
	m0 = _mm256_insertf128_ps(m0, _mm_load_ps(v + 4*strideInFloats), 1);
	m1 = _mm256_insertf128_ps(m1, _mm_load_ps(v + 5*strideInFloats), 1);
	m2 = _mm256_insertf128_ps(m2, _mm_load_ps(v + 6*strideInFloats), 1);
	m3 = _mm256_insertf128_ps(m3, _mm_load_ps(v + 7*strideInFloats), 1);

	SRAST_MM256_TRANSPOSE4_PS(m0, m1, m2, m3);
	
	f0.mm = m0;
	f1.mm = m1;
	f2.mm = m2;
	f3.mm = m3;
}

inline void simd_float_store_aos4(float* v, unsigned strideInFloats, const simd8_float& f0, const simd8_float& f1, const simd8_float& f2, const simd8_float& f3) {
	__m256 m0 = f0.mm;
	__m256 m1 = f1.mm;
	__m256 m2 = f2.mm;
	__m256 m3 = f3.mm;
	
	SRAST_MM256_TRANSPOSE4_PS(m0, m1, m2, m3);

	_mm_store_ps(v + 0*strideInFloats, _mm256_castps256_ps128(m0));
	_mm_store_ps(v + 1*strideInFloats, _mm256_castps256_ps128(m1));
	_mm_store_ps(v + 2*strideInFloats, _mm256_castps256_ps128(m2));
	_mm_store_ps(v + 3*strideInFloats, _mm256_castps256_ps128(m3));
	
	_mm_store_ps(v + 4*strideInFloats, _mm256_extractf128_ps(m0, 1));
	_mm_store_ps(v + 5*strideInFloats, _mm256_extractf128_ps(m1, 1));
	_mm_store_ps(v + 6*strideInFloats, _mm256_extractf128_ps(m2, 1));
	_mm_store_ps(v + 7*strideInFloats, _mm256_extractf128_ps(m3, 1));
}

inline void simd_float_stream_aos4(float* v, unsigned strideInFloats, const simd8_float& f0, const simd8_float& f1, const simd8_float& f2, const simd8_float& f3) {
	__m256 m0 = f0.mm;
	__m256 m1 = f1.mm;
	__m256 m2 = f2.mm;
	__m256 m3 = f3.mm;
	
	SRAST_MM256_TRANSPOSE4_PS(m0, m1, m2, m3);

	_mm_stream_ps(v + 0*strideInFloats, _mm256_castps256_ps128(m0));
	_mm_stream_ps(v + 1*strideInFloats, _mm256_castps256_ps128(m1));
	_mm_stream_ps(v + 2*strideInFloats, _mm256_castps256_ps128(m2));
	_mm_stream_ps(v + 3*strideInFloats, _mm256_castps256_ps128(m3));
	
	_mm_stream_ps(v + 4*strideInFloats, _mm256_extractf128_ps(m0, 1));
	_mm_stream_ps(v + 5*strideInFloats, _mm256_extractf128_ps(m1, 1));
	_mm_stream_ps(v + 6*strideInFloats, _mm256_extractf128_ps(m2, 1));
	_mm_stream_ps(v + 7*strideInFloats, _mm256_extractf128_ps(m3, 1));
}

inline void simd_float_load_aos8(simd8_float& f0, simd8_float& f1, simd8_float& f2, simd8_float& f3, simd8_float& f4, simd8_float& f5, simd8_float& f6, simd8_float& f7, const float* v, unsigned strideInFloats) {
	__m256 m0 = _mm256_load_ps(v + 0*strideInFloats);
	__m256 m1 = _mm256_load_ps(v + 1*strideInFloats);
	__m256 m2 = _mm256_load_ps(v + 2*strideInFloats);
	__m256 m3 = _mm256_load_ps(v + 3*strideInFloats);
	__m256 m4 = _mm256_load_ps(v + 4*strideInFloats);
	__m256 m5 = _mm256_load_ps(v + 5*strideInFloats);
	__m256 m6 = _mm256_load_ps(v + 6*strideInFloats);
	__m256 m7 = _mm256_load_ps(v + 7*strideInFloats);
	
	SRAST_MM256_TRANSPOSE8_PS(m0, m1, m2, m3, m4, m5, m6, m7);
	
	f0.mm = m0;
	f1.mm = m1;
	f2.mm = m2;
	f3.mm = m3;
	f4.mm = m4;
	f5.mm = m5;
	f6.mm = m6;
	f7.mm = m7;
}

inline void simd_float_store_aos8(float* v, unsigned strideInFloats, const simd8_float& f0, const simd8_float& f1, const simd8_float& f2, const simd8_float& f3, const simd8_float& f4, const simd8_float& f5, const simd8_float& f6, const simd8_float& f7) {
	__m256 m0 = f0.mm;
	__m256 m1 = f1.mm;
	__m256 m2 = f2.mm;
	__m256 m3 = f3.mm;
	__m256 m4 = f4.mm;
	__m256 m5 = f5.mm;
	__m256 m6 = f6.mm;
	__m256 m7 = f7.mm;
	
	SRAST_MM256_TRANSPOSE8_PS(m0, m1, m2, m3, m4, m5, m6, m7);
	
	_mm256_store_ps(v + 0*strideInFloats, m0);
	_mm256_store_ps(v + 1*strideInFloats, m1);
	_mm256_store_ps(v + 2*strideInFloats, m2);
	_mm256_store_ps(v + 3*strideInFloats, m3);
	_mm256_store_ps(v + 4*strideInFloats, m4);
	_mm256_store_ps(v + 5*strideInFloats, m5);
	_mm256_store_ps(v + 6*strideInFloats, m6);
	_mm256_store_ps(v + 7*strideInFloats, m7);
}

inline void simd_float_stream_aos8(float* v, unsigned strideInFloats, const simd8_float& f0, const simd8_float& f1, const simd8_float& f2, const simd8_float& f3, const simd8_float& f4, const simd8_float& f5, const simd8_float& f6, const simd8_float& f7) {
	__m256 m0 = f0.mm;
	__m256 m1 = f1.mm;
	__m256 m2 = f2.mm;
	__m256 m3 = f3.mm;
	__m256 m4 = f4.mm;
	__m256 m5 = f5.mm;
	__m256 m6 = f6.mm;
	__m256 m7 = f7.mm;
	
	SRAST_MM256_TRANSPOSE8_PS(m0, m1, m2, m3, m4, m5, m6, m7);
	
	_mm256_stream_ps(v + 0*strideInFloats, m0);
	_mm256_stream_ps(v + 1*strideInFloats, m1);
	_mm256_stream_ps(v + 2*strideInFloats, m2);
	_mm256_stream_ps(v + 3*strideInFloats, m3);
	_mm256_stream_ps(v + 4*strideInFloats, m4);
	_mm256_stream_ps(v + 5*strideInFloats, m5);
	_mm256_stream_ps(v + 6*strideInFloats, m6);
	_mm256_stream_ps(v + 7*strideInFloats, m7);
}

inline void simd_float_store_rgba(const simd8_float& r, const simd8_float& g, const simd8_float& b, const simd8_float& a, unsigned* output) {
	__m256 m0 = r.mm;
	__m256 m1 = g.mm;
	__m256 m2 = b.mm;
	__m256 m3 = a.mm;
	
	SRAST_MM256_TRANSPOSE4_PS(m0, m1, m2, m3);
	
	__m256 mf = _mm256_set1_ps(255.0f);
	__m256 ma = _mm256_set1_ps(0.5f);
	
	m0 = _mm256_add_ps(_mm256_mul_ps(m0, mf), ma);
	m1 = _mm256_add_ps(_mm256_mul_ps(m1, mf), ma);
	m2 = _mm256_add_ps(_mm256_mul_ps(m2, mf), ma);
	m3 = _mm256_add_ps(_mm256_mul_ps(m3, mf), ma);
	
	__m128i mi0, mi1, mi2, mi3;
	
	mi0 = _mm_cvttps_epi32(_mm256_castps256_ps128(m0));
	mi1 = _mm_cvttps_epi32(_mm256_castps256_ps128(m1));
	mi2 = _mm_cvttps_epi32(_mm256_castps256_ps128(m2));
	mi3 = _mm_cvttps_epi32(_mm256_castps256_ps128(m3));
	
	mi0 = _mm_packus_epi32(mi0, mi1);
	mi2 = _mm_packus_epi32(mi2, mi3);
	mi0 = _mm_packus_epi16(mi0, mi2);
	
	_mm_store_si128(reinterpret_cast<__m128i*>(output), mi0);
	
	mi0 = _mm_cvttps_epi32(_mm256_extractf128_ps(m0, 1));
	mi1 = _mm_cvttps_epi32(_mm256_extractf128_ps(m1, 1));
	mi2 = _mm_cvttps_epi32(_mm256_extractf128_ps(m2, 1));
	mi3 = _mm_cvttps_epi32(_mm256_extractf128_ps(m3, 1));
	
	mi0 = _mm_packus_epi32(mi0, mi1);
	mi2 = _mm_packus_epi32(mi2, mi3);
	mi0 = _mm_packus_epi16(mi0, mi2);
	
	_mm_store_si128(reinterpret_cast<__m128i*>(output) + 1, mi0);
}

typedef simd8_float simd_float;
#else
typedef simd4_float simd_float;
#endif

typedef vec2<simd_float> simd_float2;
typedef vec3<simd_float> simd_float3;
typedef vec4<simd_float> simd_float4;
typedef mat4<simd_float> simd_float4x4;

void* simd_malloc(size_t size, size_t alignment);
void simd_free(void* ptr);

}

#endif
