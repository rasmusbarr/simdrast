//
//  Atomics.h
//  SimdRast
//
//  Created by Rasmus Barringer on 2012-10-20.
//  Copyright (c) 2012 Rasmus Barringer. All rights reserved.
//

#ifndef SimdRast_Atomics_h
#define SimdRast_Atomics_h

#include "Config.h"
#include <emmintrin.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <libkern/OSAtomic.h>
#endif

namespace srast {

class Atomics {
public:
	// Returns the new value.
	static int add(int* i, int x) {
#ifdef _WIN32
		return InterlockedExchangeAdd(reinterpret_cast<volatile LONG*>(i), x) + x;
#else
		return OSAtomicAdd32(x, reinterpret_cast<volatile int*>(i));
#endif
	}
	
	static int increment(int* i) {
#ifdef _WIN32
		return InterlockedIncrement(reinterpret_cast<volatile LONG*>(i));
#else
		return OSAtomicIncrement32(reinterpret_cast<volatile int*>(i));
#endif
	}
	
	static int decrement(int* i) {
#ifdef _WIN32
		return InterlockedDecrement(reinterpret_cast<volatile LONG*>(i));
#else
		return OSAtomicDecrement32(reinterpret_cast<volatile int*>(i));
#endif
	}

	static int compareAndSwap(int* i, int newValue, int oldValue) {
#ifdef _WIN32
		return InterlockedCompareExchange(reinterpret_cast<volatile LONG*>(i), newValue, oldValue);
#else
		return __sync_val_compare_and_swap(i, oldValue, newValue);
#endif
	}
};
	
}

#endif
