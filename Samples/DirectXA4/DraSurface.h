//
//  DraSurface.h
//  DirectXA4
//
//  Created by Rasmus Barringer on 2013-06-07.
//  Copyright (c) 2013 Rasmus Barringer. All rights reserved.
//

#ifndef DirectXA4_DraSurface_h
#define DirectXA4_DraSurface_h

struct DraSurface {
	void* base;
	unsigned pitch;

	inline unsigned& operator () (unsigned x, unsigned y) const {
		x <<= 2;
		x = (x & 0xF) | ((x & 0xFFFFFFF0) << 5);
		y = ((y & 0x1f) << 4) + pitch * (y >> 5);

		unsigned offset = x + y;
		offset = offset ^ ((offset & (1 << 9)) >> 3);

		return *reinterpret_cast<unsigned*>(static_cast<char*>(base) + offset);
	}

	inline unsigned& operator () (unsigned x, unsigned y, unsigned tileOffset) const {
		x = (x & 0x3) << 2;
		y = (y & 0x1f) << 4;

		unsigned swizzleBit = (tileOffset & (1 << 9)) >> 3;
		unsigned offset = (x + y) ^ swizzleBit;

		return *reinterpret_cast<unsigned*>(static_cast<char*>(base) + tileOffset + offset);
	}
};

#endif
