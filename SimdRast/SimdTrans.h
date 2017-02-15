//
//  SimdTrans.h
//  SimdRast
//
//  Created by Rasmus Barringer on 2012-12-28.
//  Copyright (c) 2012 Rasmus Barringer. All rights reserved.
//

#ifndef SimdRast_SimdTrans_h
#define SimdRast_SimdTrans_h

#include "SimdMath.h"

namespace srast {

#define SRAST_POLY1(x, c0, c1) (x*(c1) + c0)
#define SRAST_POLY2(x, c0, c1, c2) (x*SRAST_POLY1(x, c1, c2) + c0)
#define SRAST_POLY3(x, c0, c1, c2, c3) (x*SRAST_POLY2(x, c1, c2, c3) + c0)
#define SRAST_POLY4(x, c0, c1, c2, c3, c4) (x*SRAST_POLY3(x, c1, c2, c3, c4) + c0)

inline void exp2_extract(const __m128& x, simd4_float& fpart, simd4_float& expipart) {
	__m128i ipart = _mm_cvtps_epi32(_mm_sub_ps(x, _mm_set1_ps(0.5f)));
	fpart = _mm_sub_ps(x, _mm_cvtepi32_ps(ipart));
	expipart = _mm_castsi128_ps(_mm_slli_epi32(_mm_add_epi32(ipart, _mm_set1_epi32(127)), 23));
}

inline simd4_float exp2(const simd4_float& v) {
	simd4_float x = min(v, 129.00000f);
	x = max(x, -126.99999f);

	simd4_float fpart, expipart, expfpart;
	exp2_extract(x.mm, fpart, expipart);

	expfpart = SRAST_POLY3(fpart, 9.9992520e-1f, 6.9583356e-1f, 2.2606716e-1f, 7.8024521e-2f);
	return expipart * expfpart;
}

inline void log2_extract(const simd4_float& x, simd4_float& e, simd4_float& m) {
	__m128i exp = _mm_set1_epi32(0x7F800000);
	__m128i mant = _mm_set1_epi32(0x007FFFFF);
	
	__m128 one = _mm_set1_ps(1.0f);
	__m128i i = _mm_castps_si128(x.mm);
	
	e = _mm_cvtepi32_ps(_mm_sub_epi32(_mm_srli_epi32(_mm_and_si128(i, exp), 23), _mm_set1_epi32(127)));
	m = _mm_or_ps(_mm_castsi128_ps(_mm_and_si128(i, mant)), one);
}

inline simd4_float log2(const simd4_float& x) {
	simd4_float e, m, p;
	log2_extract(x, e, m);

	p = SRAST_POLY4(m, 2.8882704548164776201f, -2.52074962577807006663f, 1.48116647521213171641f, -0.465725644288844778798f, 0.0596515482674574969533f);
	p = p * (m - 1.0f);
	return p + e;
}

inline simd4_float pow(const simd4_float& x, const simd4_float& y) {
	return exp2(log2(x) * y);
}

#ifdef SRAST_AVX
inline simd8_float exp2(const simd8_float& v) {
	simd8_float x = min(v, 129.00000f);
	x = max(x, -126.99999f);

	simd8_float fpart, expipart, expfpart;

	simd4_float fpartlo, expipartlo;
	exp2_extract(x.low().mm, fpartlo, expipartlo);
	simd4_float fparthi, expiparthi;
	exp2_extract(x.high().mm, fparthi, expiparthi);

	fpart = simd_float_combine(fpartlo.mm, fparthi.mm);
	expipart = simd_float_combine(expipartlo.mm, expiparthi.mm);

	expfpart = SRAST_POLY3(fpart, 9.9992520e-1f, 6.9583356e-1f, 2.2606716e-1f, 7.8024521e-2f);
	return expipart * expfpart;
}

inline simd8_float log2(const simd8_float& x) {
	simd8_float e, m, p;

	simd4_float elo, mlo;
	log2_extract(x.low().mm, elo, mlo);
	simd4_float ehi, mhi;
	log2_extract(x.high().mm, ehi, mhi);

	e = simd_float_combine(elo.mm, ehi.mm);
	m = simd_float_combine(mlo.mm, mhi.mm);

	p = SRAST_POLY4(m, 2.8882704548164776201f, -2.52074962577807006663f, 1.48116647521213171641f, -0.465725644288844778798f, 0.0596515482674574969533f);
	p = p * (m - 1.0f);
	return p + e;
}

inline simd8_float pow(const simd8_float& x, const simd8_float& y) {
	return exp2(log2(x) * y);
}
#endif

}

#endif
