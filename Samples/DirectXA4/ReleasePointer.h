//
//  ReleasePointer.h
//  DirectXA4
//
//  Created by Rasmus Barringer on 2013-06-07.
//  Copyright (c) 2013 Rasmus Barringer. All rights reserved.
//

#ifndef DirectXA4_ReleasePointer_h
#define DirectXA4_ReleasePointer_h

#include <stdexcept>

template<class T>
class ReleasePointer {
private:
	T* p;
	ReleasePointer(const ReleasePointer&);
	void operator = (const ReleasePointer&);

public:
	ReleasePointer() : p(0) {
	}

	operator T* () const {
		return p;
	}

	T* operator -> () const {
		return p;
	}

	T** operator & () {
		if (p)
			throw std::runtime_error("Unsafe address of valid release-ptr. Release it first.");
		return &p;
	}

	void release() {
		if (p) {
			p->Release();
			p = 0;
		}
	}

	~ReleasePointer() {
		release();
	}
};

#endif
