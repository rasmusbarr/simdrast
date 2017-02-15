//
//  Config.h
//  SimdRast
//
//  Created by Rasmus Barringer on 2013-05-30.
//  Copyright (c) 2013 Rasmus Barringer. All rights reserved.
//

#ifndef SimdRast_Config_h
#define SimdRast_Config_h

#ifdef _WIN32
#define SRAST_ALIGNED(n) __declspec(align(n))
#else
#define SRAST_ALIGNED(n) __attribute__((aligned(n)))
#endif

#ifdef _WIN32
#define SRAST_FORCEINLINE __forceinline
#else
#define SRAST_FORCEINLINE __attribute__((always_inline))
#endif

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#endif

#ifdef __AVX__
#define SRAST_AVX
#endif

#endif
