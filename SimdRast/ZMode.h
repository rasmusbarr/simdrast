//
//  ZMode.h
//  SimdRast
//
//  Created by Rasmus Barringer on 2013-01-09.
//  Copyright (c) 2013 Rasmus Barringer. All rights reserved.
//

#ifndef SimdRast_ZMode_h
#define SimdRast_ZMode_h

#include "SimdMath.h"
#include <functional>

namespace srast {

// Note that these are reversed. This increases the float precision at the far plane.
#define SRAST_NEAR_Z 1.0f
#define SRAST_FAR_Z 0.0f

struct ZLessMode {
	struct SortComparator {
		bool operator () (unsigned long long a, unsigned long long b) const {
			return a > b;
		}
	};

	static simd_float min(const simd_float& a, const simd_float& b) {
		return srast::max(a, b);
	}

	static simd_float max(const simd_float& a, const simd_float& b) {
		return srast::min(a, b);
	}
	
	static simd_float max_reduce(const simd_float& a) {
		return srast::min_reduce(a);
	}
	
	static simd_float less(const simd_float& a, const simd_float& b) {
		return a > b;
	}
	
	static bool less(float a, float b) {
		return a > b;
	}

	static bool less(unsigned a, unsigned b) {
		return a > b;
	}
	
	static simd_float nearClip(const simd_float& z, const simd_float& wpos) {
		return blend(SRAST_NEAR_Z, z, wpos);
	}
	
	static simd_float test(const simd_float& z, const simd_float& oldZ) {
		return (z < SRAST_NEAR_Z) & (z > oldZ);
	}
};

}

#endif
