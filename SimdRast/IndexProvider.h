//
//  IndexProvider.h
//  SimdRast
//
//  Created by Rasmus Barringer on 2013-05-30.
//  Copyright (c) 2013 Rasmus Barringer. All rights reserved.
//

#ifndef SimdRast_IndexProvider_h
#define SimdRast_IndexProvider_h

namespace srast {

template<class T = float>
struct IndexProvider {
	const void* indices;
	IndexProvider(const void* indices) : indices(indices) {}
	int3 operator () (unsigned index) const {
		return int3(static_cast<const T*>(indices)[index + 0],
					static_cast<const T*>(indices)[index + 1],
					static_cast<const T*>(indices)[index + 2]);
	}
};

template<>
struct IndexProvider<float> {
	int3 operator () (unsigned index) const {
		return int3(index + 0, index + 1, index + 2);
	}
};

}

#endif
