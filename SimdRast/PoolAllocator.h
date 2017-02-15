//
//  PoolAllocator.h
//  SimdRast
//
//  Created by Rasmus Barringer on 2012-10-20.
//  Copyright (c) 2012 Rasmus Barringer. All rights reserved.
//

#ifndef SimdRast_PoolAllocator_h
#define SimdRast_PoolAllocator_h

namespace srast {

class PoolAllocator {
private:
	void* start;
	int offset;
	unsigned size;
	
public:
	PoolAllocator(unsigned size);
	
	template<class T>
	T* basePointer() const {
		return static_cast<T*>(start);
	}
	
	void reset();
	
	void* allocate(unsigned size);
	
	void* clone(const void* source, unsigned size);
	
	~PoolAllocator();
};

}

#endif
