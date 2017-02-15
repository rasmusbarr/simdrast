//
//  SimdDouble.h
//  SimdRast
//
//  Created by Rasmus Barringer on 2013-02-28.
//  Copyright (c) 2013 Rasmus Barringer. All rights reserved.
//

#ifndef SimdRast_SimdDouble_h
#define SimdRast_SimdDouble_h

#include "SimdMath.h"

namespace srast {

struct simd2_double {
	static const int width = 2;
	
	__m128d mm;
	
	simd2_double() {
	}
	
	simd2_double(__m128d v) : mm(v) {
	}
	
	simd2_double(double v) {
		mm = _mm_set1_pd(v);
	}
	
	simd2_double(double x, double y) {
		mm = _mm_set_pd(y, x);
	}
	
	static simd2_double zero() {
		return _mm_setzero_pd();
	}
	
	static simd2_double broadcast_load(const double* f) {
		return *f;
	}
	
	simd2_double operator ~ () const {
		return _mm_andnot_pd(mm, _mm_set1_pd(uint64_as_double(0xffffffffffffffffull)));
	}
	
	simd2_double operator + () const {
		return *this;
	}
	
	simd2_double operator - () const {
		return _mm_sub_pd(_mm_setzero_pd(), mm);
	}
	
	simd2_double operator * (const simd2_double& v) const {
		return _mm_mul_pd(mm, v.mm);
	}
	
	simd2_double operator + (const simd2_double& v) const {
		return _mm_add_pd(mm, v.mm);
	}
	
	simd2_double operator - (const simd2_double& v) const {
		return _mm_sub_pd(mm, v.mm);
	}
	
	simd2_double operator / (const simd2_double& v) const {
		return _mm_div_pd(mm, v.mm);
	}
	
	void operator *= (const simd2_double& v) {
		mm = _mm_mul_pd(mm, v.mm);
	}
	
	void operator += (const simd2_double& v) {
		mm = _mm_add_pd(mm, v.mm);
	}
	
	void operator -= (const simd2_double& v) {
		mm = _mm_sub_pd(mm, v.mm);
	}
	
	void operator /= (const simd2_double& v) {
		mm = _mm_div_pd(mm, v.mm);
	}
	
	simd2_double operator | (const simd2_double& v) const {
		return _mm_or_pd(mm, v.mm);
	}
	
	simd2_double operator & (const simd2_double& v) const {
		return _mm_and_pd(mm, v.mm);
	}
	
	simd2_double operator ^ (const simd2_double& v) const {
		return _mm_xor_pd(mm, v.mm);
	}
	
	simd2_double operator > (const simd2_double& v) const {
		return _mm_cmpgt_pd(mm, v.mm);
	}
	
	simd2_double operator == (const simd2_double& v) const {
		return _mm_cmpeq_pd(mm, v.mm);
	}
	
	inline simd2_double operator != (const simd2_double& v) const {
		return _mm_cmpneq_pd(mm, v.mm);
	}
	
	inline simd2_double operator >= (const simd2_double& v) const {
		return _mm_cmpge_pd(mm, v.mm);
	}
	
	inline simd2_double operator < (const simd2_double& v) const {
		return _mm_cmplt_pd(mm, v.mm);
	}
	
	inline simd2_double operator <= (const simd2_double& v) const {
		return _mm_cmple_pd(mm, v.mm);
	}
	
	inline void load(const double* v) {
		mm = _mm_load_pd(v);
	}
	
	inline void store(double* v) const {
		_mm_store_pd(v, mm);
	}
	
	inline void stream_store(double* v) const {
		_mm_stream_pd(v, mm);
	}
	
	inline void masked_store(double* v, const simd2_double& mask) {
		_mm_store_pd(v, _mm_blendv_pd(_mm_load_pd(v), mm, mask.mm));
	}
	
	inline double first_double() const {
		return _mm_cvtsd_f64(mm);
	}
	
	inline int first_double_to_int() const {
		return _mm_cvttsd_si32(mm);
	}
	
	void print(const char* name, unsigned laneMask = (1 << width)-1) const {
		std::cout << name;
		
		SRAST_SIMD_ALIGNED double va[width];
		store(va);
		
		for (unsigned i = 0; i < width; ++i) {
			std::cout << " ";
			std::cout.precision(32);
			std::cout.width(32);
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

inline simd2_double mad(const simd2_double& op1, const simd2_double& op2, const simd2_double& addend) {
	return op1*op2 + addend;
}

inline simd2_double not_and(const simd2_double& a, const simd2_double& b) {
	return _mm_andnot_pd(a.mm, b.mm);
}

inline simd2_double blend(const simd2_double& a, const simd2_double& b, const simd2_double& mask) {
	return _mm_blendv_pd(a.mm, b.mm, mask.mm);
}

inline simd2_double floor(const simd2_double& v) {
	return _mm_round_pd(v.mm, _MM_FROUND_TO_NEG_INF);
}

inline simd2_double ceil(const simd2_double& v) {
	return _mm_round_pd(v.mm, _MM_FROUND_TO_POS_INF);
}

inline simd2_double round(const simd2_double& v) {
	return _mm_round_pd(v.mm, _MM_FROUND_TO_NEAREST_INT);
}

inline simd2_double trunc(const simd2_double& v) {
	return _mm_round_pd(v.mm, _MM_FROUND_TO_ZERO);
}

inline simd2_double min(const simd2_double& a, const simd2_double& b) {
	return _mm_min_pd(a.mm, b.mm);
}

inline simd2_double max(const simd2_double& a, const simd2_double& b) {
	return _mm_max_pd(a.mm, b.mm);
}

inline simd2_double sqrt(const simd2_double& v) {
	return _mm_sqrt_pd(v.mm);
}

inline simd2_double abs(const simd2_double& v) {
	return _mm_andnot_pd(_mm_set1_pd(-0.0f), v.mm);
}

inline int mask(const simd2_double& v) {
	return _mm_movemask_pd(v.mm);
}

inline bool all(const simd2_double& v) {
	return mask(v) == 0x3;
}

inline bool any(const simd2_double& v) {
	return mask(v) != 0;
}
	
inline simd2_double float2double_lo(const simd4_float& v) {
	return _mm_cvtps_pd(v.mm);
}

inline simd2_double float2double_hi(const simd4_float& v) {
	return _mm_cvtps_pd(_mm_shuffle_ps(v.mm, v.mm, _MM_SHUFFLE(3,2,3,2)));
}
	
inline simd4_float double2float(const simd2_double& v) {
	return _mm_cvtpd_ps(v.mm);
}

inline simd4_float double2float(const simd2_double& lo, const simd2_double& hi) {
	return _mm_shuffle_ps(_mm_cvtpd_ps(lo.mm), _mm_cvtpd_ps(hi.mm), _MM_SHUFFLE(1,0,1,0));
}

#define SRAST_MM_TRANSPOSE2_PD(row0, row1) \
do {\
__m128d tmp0 = _mm_unpacklo_pd((row0), (row1));\
__m128d tmp1 = _mm_unpackhi_pd((row0), (row1));\
(row0) = tmp0;\
(row1) = tmp1;\
} while (0)

#ifdef SRAST_AVX
struct simd4_double {
	static const int width = 4;
	
	__m256d mm;
	
	inline simd4_double() {
	}
	
	inline simd4_double(__m128d v) {
		mm = _mm256_castpd128_pd256(v);
		mm = _mm256_insertf128_pd(mm, v, 1);
	}
	
	inline simd4_double(__m256d v) : mm(v) {
	}
	
	inline simd4_double(double v) {
		mm = _mm256_set1_pd(v);
	}
	
	inline simd4_double(double x0, double y0, double x1, double y1) {
		mm = _mm256_set_pd(x0, y0, x1, y1);
	}
	
	inline static simd4_double zero() {
		return _mm256_setzero_pd();
	}
	
	inline static simd4_double broadcast_load(const double* f) {
		return _mm256_broadcast_sd(f);
	}

	inline simd4_double operator ~ () const {
		return _mm256_andnot_pd(mm, _mm256_set1_pd(uint64_as_double(0xffffffffffffffffull)));
	}
	
	inline simd4_double operator - () const {
		return _mm256_sub_pd(_mm256_setzero_pd(), mm);
	}
	
	inline simd4_double operator * (const simd4_double& v) const {
		return _mm256_mul_pd(mm, v.mm);
	}
	
	inline simd4_double operator + (const simd4_double& v) const {
		return _mm256_add_pd(mm, v.mm);
	}
	
	inline simd4_double operator - (const simd4_double& v) const {
		return _mm256_sub_pd(mm, v.mm);
	}
	
	inline simd4_double operator / (const simd4_double& v) const {
		return _mm256_div_pd(mm, v.mm);
	}
	
	inline void operator *= (const simd4_double& v) {
		mm = _mm256_mul_pd(mm, v.mm);
	}
	
	inline void operator += (const simd4_double& v) {
		mm = _mm256_add_pd(mm, v.mm);
	}
	
	inline void operator -= (const simd4_double& v) {
		mm = _mm256_sub_pd(mm, v.mm);
	}
	
	inline void operator /= (const simd4_double& v) {
		mm = _mm256_div_pd(mm, v.mm);
	}

	inline simd4_double operator | (const simd4_double& v) const {
		return _mm256_or_pd(mm, v.mm);
	}
	
	inline simd4_double operator & (const simd4_double& v) const {
		return _mm256_and_pd(mm, v.mm);
	}
	
	inline simd4_double operator ^ (const simd4_double& v) const {
		return _mm256_xor_pd(mm, v.mm);
	}

	inline simd4_double operator > (const simd4_double& v) const {
		return _mm256_cmp_pd(mm, v.mm, _CMP_NLE_US);
	}
	
	inline simd4_double operator == (const simd4_double& v) const {
		return _mm256_cmp_pd(mm, v.mm, _CMP_EQ_OQ);
	}
	
	inline simd4_double operator >= (const simd4_double& v) const {
		return _mm256_cmp_pd(mm, v.mm, _CMP_NLT_US);
	}
	
	inline simd4_double operator < (const simd4_double& v) const {
		return _mm256_cmp_pd(mm, v.mm, _CMP_LT_OS);
	}
	
	inline simd4_double operator <= (const simd4_double& v) const {
		return _mm256_cmp_pd(mm, v.mm, _CMP_LE_OS);
	}
	
	inline void load(const double* v) {
		mm = _mm256_load_pd(v);
	}
	
	inline void store(double* v) const {
		_mm256_store_pd(v, mm);
	}

	inline void stream_store(double* v) const {
		_mm256_stream_pd(v, mm);
	}
	
	inline void masked_store(double* v, const simd4_double& mask) {
		_mm256_store_pd(v, _mm256_blendv_pd(_mm256_load_pd(v), mm, mask.mm));
	}
	
	inline double first_double() const {
		return _mm_cvtsd_f64(_mm256_castpd256_pd128(mm));
	}
	
	inline int first_double_to_int() const {
		return _mm_cvttsd_si32(_mm256_castpd256_pd128(mm));
	}

	inline simd2_double low() const {
		return _mm256_castpd256_pd128(mm);
	}

	inline simd2_double high() const {
		return _mm256_extractf128_pd(mm, 1);
	}
};
	
inline simd4_double mad(const simd4_double& op1, const simd4_double& op2, const simd4_double& addend) {
	return op1*op2 + addend;
}

inline simd4_double not_and(const simd4_double& a, const simd4_double& b) {
	return _mm256_andnot_pd(a.mm, b.mm);
}

inline simd4_double blend(const simd4_double& a, const simd4_double& b, const simd4_double& mask) {
	return _mm256_blendv_pd(a.mm, b.mm, mask.mm);
}

inline simd4_double floor(const simd4_double& v) {
	return _mm256_round_pd(v.mm, _MM_FROUND_TO_NEG_INF);
}

inline simd4_double ceil(const simd4_double& v) {
	return _mm256_round_pd(v.mm, _MM_FROUND_TO_POS_INF);
}

inline simd4_double round(const simd4_double& v) {
	return _mm256_round_pd(v.mm, _MM_FROUND_TO_NEAREST_INT);
}

inline simd4_double trunc(const simd4_double& v) {
	return _mm256_round_pd(v.mm, _MM_FROUND_TO_ZERO);
}

inline simd4_double min(const simd4_double& a, const simd4_double& b) {
	return _mm256_min_pd(a.mm, b.mm);
}

inline simd4_double max(const simd4_double& a, const simd4_double& b) {
	return _mm256_max_pd(a.mm, b.mm);
}

inline simd4_double sqrt(const simd4_double& v) {
	return _mm256_sqrt_pd(v.mm);
}

inline simd4_double abs(const simd4_double& v) {
	return _mm256_andnot_pd(_mm256_set1_pd(-0.0f), v.mm);
}

inline int mask(const simd4_double& v) {
	return _mm256_movemask_pd(v.mm);
}

inline bool all(const simd4_double& v) {
	return mask(v) == 0xf;
}

inline bool any(const simd4_double& v) {
	return mask(v) != 0;
}

inline __m256d simd_double_combine(__m128d a, __m128d b) {
	__m256d c = _mm256_castpd128_pd256(a);
	c = _mm256_insertf128_pd(c, b, 1);
	return c;
}

inline simd4_double float2double_lo(const simd8_float& v) {
	return _mm256_cvtps_pd(v.low().mm);
}

inline simd4_double float2double_hi(const simd8_float& v) {
	return _mm256_cvtps_pd(v.high().mm);
}

inline simd8_float double2float(const simd4_double& v) {
	return _mm256_castps128_ps256(_mm256_cvtpd_ps(v.mm));
}

inline simd8_float double2float(const simd4_double& lo, const simd4_double& hi) {
	return simd_float_combine(_mm256_cvtpd_ps(lo.mm), _mm256_cvtpd_ps(hi.mm));
}

#define SRAST_MM256_TRANSPOSE2_PD(row0, row1) \
do {\
	__m256d tmp0 = _mm256_unpacklo_pd((row0), (row1));\
	__m256d tmp1 = _mm256_unpackhi_pd((row0), (row1));\
	(row0) = tmp0;\
	(row1) = tmp1;\
} while (0)

typedef simd4_double simd_double;
#else
typedef simd2_double simd_double;
#endif

typedef vec2<simd_double> simd_double2;
typedef vec3<simd_double> simd_double3;
typedef vec4<simd_double> simd_double4;
typedef mat4<simd_double> simd_double4x4;

}

#endif
