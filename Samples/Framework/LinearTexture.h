//
//  LinearTexture.h
//  Framework
//
//  Created by Rasmus Barringer on 12/29/12.
//  Copyright (c) 2012 Rasmus Barringer. All rights reserved.
//

#ifndef Framework_LinearTexture_h
#define Framework_LinearTexture_h

#include "../../SimdRast/TextureSampler.h"
#include <vector>

namespace fx {

class LinearTexture {
private:
	template<class T, class F, class A, class M>
	friend class srast::TextureSampler;

	bool alpha;
	int mipCount;
	unsigned width, height, widthLog2, heightLog2;
	std::vector<unsigned*> mipLevels;
	
public:
	LinearTexture(const char* filename);

	bool hasAlpha() const {
		return alpha;
	}
	
	unsigned getWidth() const {
		return width;
	}
	
	unsigned getHeight() const {
		return height;
	}
	
	const std::vector<unsigned*>& getMipLevels() const {
		return mipLevels;
	}
	
	template<class A>
	srast::TextureSampler<LinearTexture, srast::TextureFormatR8G8B8A8, A> getSampler() const {
		return srast::TextureSampler<LinearTexture, srast::TextureFormatR8G8B8A8, A>(*this);
	}

	template<class A, class M>
	srast::TextureSampler<LinearTexture, srast::TextureFormatR8G8B8A8, A, M> getModifiedSampler() const {
		return srast::TextureSampler<LinearTexture, srast::TextureFormatR8G8B8A8, A, M>(*this);
	}

	void getQuad(unsigned x0, unsigned y0, unsigned x1, unsigned y1, unsigned mipIndex, const float*& p00, const float*& p10, const float*& p01, const float*& p11) const {
		unsigned mipWidthLog2 = widthLog2 - mipIndex;
		
		y0 <<= mipWidthLog2;
		y1 <<= mipWidthLog2;
		
		const float* tex = reinterpret_cast<const float*>(mipLevels[mipIndex]);
		
		p00 = tex + x0 + y0;
		p10 = tex + x1 + y0;
		p01 = tex + x0 + y1;
		p11 = tex + x1 + y1;
	}
	
	~LinearTexture();
};

}

#endif
