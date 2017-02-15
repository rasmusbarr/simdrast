//
//  SimdMath.cpp
//  SimdRast
//
//  Created by Rasmus Barringer on 2012-10-22.
//  Copyright (c) 2012 Rasmus Barringer. All rights reserved.
//

#include "SimdMath.h"

namespace srast {

void* simd_malloc(size_t size, size_t alignment) {
	return _mm_malloc(size, alignment);
}

void simd_free(void* ptr) {
	_mm_free(ptr);
}

}
