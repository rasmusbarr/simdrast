//
//  LinearTexture.cpp
//  Framework
//
//  Created by Rasmus Barringer on 2013-05-28.
//  Copyright (c) 2013 Rasmus Barringer. All rights reserved.
//

#include "LinearTexture.h"
#include "External/lodepng/lodepng.h"

namespace fx {

static unsigned log2(unsigned x) {
	unsigned r = (x > 0xffff) << 4;
	x >>= r;
	
	unsigned shift = (x > 0xff) << 3;
	
	x >>= shift;
	r |= shift;
	
	shift = (x > 0xf) << 2;
	
	x >>= shift;
	r |= shift;
	
	shift = (x > 0x3) << 1;
	
	x >>= shift;
	r |= shift;
	
	r |= (x >> 1);
	return r;
}

LinearTexture::LinearTexture(const char* filename) {
	std::vector<unsigned char> image;
	
	if (LodePNG::decode(image, width, height, filename))
		throw std::runtime_error(std::string("unable to load ") + filename);
	
	widthLog2 = log2(width);
	heightLog2 = log2(height);
	
	if ((1 << widthLog2) != width || (1 << heightLog2) != height)
		throw std::runtime_error(std::string(filename) + " is not power of 2");
	
	alpha = false;
	
	for (size_t i = 0; i < image.size() >> 2; ++i) {
		unsigned a = image[(i << 2) + 3];
		if (a != 0xff) { // Premultiplied alpha.
			image[(i << 2) + 0] = (unsigned char)(image[(i << 2) + 0] * a / 255);
			image[(i << 2) + 1] = (unsigned char)(image[(i << 2) + 1] * a / 255);
			image[(i << 2) + 2] = (unsigned char)(image[(i << 2) + 2] * a / 255);
			alpha = true;
		}
	}
	
	unsigned* pixels = new unsigned[width*height];
	memcpy(pixels, &image[0], sizeof(unsigned) * width * height);
	
	mipLevels.push_back(pixels);
	
	unsigned mipWidth = width;
	unsigned mipHeight = height;
	
	while ((mipWidth >> 2) && (mipHeight >> 2)) {
		unsigned* parent = *mipLevels.rbegin();
		pixels = new unsigned[mipWidth*mipHeight >> 2];
		
		mipLevels.push_back(pixels);
		
		for (unsigned i = 0; i < mipHeight; i += 2) {
			for (unsigned j = 0; j < mipWidth; j += 2) {
				__m128i c00 = _mm_castps_si128(_mm_load_ss(reinterpret_cast<const float*>(&parent[(i+0)*mipWidth + (j+0)])));
				__m128i c10 = _mm_castps_si128(_mm_load_ss(reinterpret_cast<const float*>(&parent[(i+0)*mipWidth + (j+1)])));
				__m128i c01 = _mm_castps_si128(_mm_load_ss(reinterpret_cast<const float*>(&parent[(i+1)*mipWidth + (j+0)])));
				__m128i c11 = _mm_castps_si128(_mm_load_ss(reinterpret_cast<const float*>(&parent[(i+1)*mipWidth + (j+1)])));
				
				c00 = _mm_cvtepu8_epi16(c00);
				c10 = _mm_cvtepu8_epi16(c10);
				c01 = _mm_cvtepu8_epi16(c01);
				c11 = _mm_cvtepu8_epi16(c11);
				
				__m128i c = _mm_add_epi16(_mm_add_epi16(c00, c10),
										  _mm_add_epi16(c01, c11));
				
				c = _mm_srli_epi16(c, 2);
				c = _mm_packus_epi16(c, c);
				
				_mm_store_ss(reinterpret_cast<float*>(&pixels[(i*mipWidth >> 2) + (j >> 1)]), _mm_castsi128_ps(c));
			}
		}
		
		mipWidth >>= 1;
		mipHeight >>= 1;
	}
	
	mipCount = (unsigned)mipLevels.size();
}

LinearTexture::~LinearTexture() {
	for (size_t i = 0; i < mipLevels.size(); ++i)
		delete [] mipLevels[i];
}

}
