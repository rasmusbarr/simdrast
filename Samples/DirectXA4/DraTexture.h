//
//  DraTexture.h
//  DirectXA4
//
//  Created by Rasmus Barringer on 2013-06-07.
//  Copyright (c) 2013 Rasmus Barringer. All rights reserved.
//

#ifndef DirectXA4_DraTexture_h
#define DirectXA4_DraTexture_h

#include "DxContext.h"
#include "DraSurface.h"
#include "../../SimdRast/TextureSampler.h"

class DraTexture {
private:
	template<class T, class F, class A, class M>
	friend class srast::TextureSampler;

	DraSurface surface;
	unsigned widthLog2, heightLog2;

	static const int mipCount = 1;

	DraSurface localCopy;
	unsigned char* fetchedTiles;

public:
	DraTexture(unsigned width, unsigned height);

	void setup(DraSurface surface);
	
	template<class F, class A>
	srast::TextureSampler<DraTexture, F, A> getSampler() const {
		return srast::TextureSampler<DraTexture, F, A>(*this);
	}

	template<class F, class A, class M>
	srast::TextureSampler<DraTexture, F, A, M> getModifiedSampler() const {
		return srast::TextureSampler<DraTexture, F, A, M>(*this);
	}

	FORCEINLINE void getQuad(unsigned x0, unsigned y0, unsigned x1, unsigned y1, unsigned mipIndex, const float*& p00, const float*& p10, const float*& p01, const float*& p11) const {
		unsigned tileOffset00 = readTile(x0, y0);

		if ((x0 & (~0x3)) == (x1 & (~0x3)) && (y0 & (~0x1f)) == (y1 & (~0x1f))) {
			p00 = reinterpret_cast<float*>(&localCopy(x0, y0, tileOffset00));
			p10 = reinterpret_cast<float*>(&localCopy(x1, y0, tileOffset00));
			p01 = reinterpret_cast<float*>(&localCopy(x0, y1, tileOffset00));
			p11 = reinterpret_cast<float*>(&localCopy(x1, y1, tileOffset00));
		}
		else {
			unsigned tileOffset10 = readTile(x1, y0);
			unsigned tileOffset01 = readTile(x0, y1);
			unsigned tileOffset11 = readTile(x1, y1);

			p00 = reinterpret_cast<float*>(&localCopy(x0, y0, tileOffset00));
			p10 = reinterpret_cast<float*>(&localCopy(x1, y0, tileOffset10));
			p01 = reinterpret_cast<float*>(&localCopy(x0, y1, tileOffset01));
			p11 = reinterpret_cast<float*>(&localCopy(x1, y1, tileOffset11));
		}
	}

	~DraTexture();

private:
	FORCEINLINE unsigned readTile(unsigned x, unsigned y) const {
		// 32x4 tiles.
		unsigned tileOffset = ((x & ~0x3) << 7) + surface.pitch * ((y & ~0x1f) >> 5);
		unsigned tile = tileOffset >> 7;

		if (fetchedTiles[tile]) {
			_ReadBarrier();
			return tileOffset;
		}

		__m128i* src = reinterpret_cast<__m128i*>(static_cast<unsigned char*>(surface.base) + tileOffset);
		__m128i* dst = reinterpret_cast<__m128i*>(static_cast<unsigned char*>(localCopy.base) + tileOffset);

		__m128i* dstMax = dst + 8;

		do {
			_mm_store_si128(dst, _mm_stream_load_si128(src));
			_mm_store_si128(dst+8, _mm_stream_load_si128(src+8));
			_mm_store_si128(dst+16, _mm_stream_load_si128(src+16));
			_mm_store_si128(dst+24, _mm_stream_load_si128(src+24));
			++dst;
			++src;
		}
		while (dst < dstMax);

		_mm_sfence();
		fetchedTiles[tile] = 1;

		return tileOffset;
	}
};

#endif
