//
//  TransferTask.h
//  DirectXA4
//
//  Created by Rasmus Barringer on 2013-06-07.
//  Copyright (c) 2013 Rasmus Barringer. All rights reserved.
//

#ifndef DirectXA4_TransferTask_h
#define DirectXA4_TransferTask_h

#include "../../SimdRast/Renderer.h"

template<class T>
class TransferTask : public srast::ThreadPoolTask {
private:
	const T& t;
	srast::Renderer& r;
	unsigned tileWidth;
	unsigned tileHeight;
	
public:
	TransferTask(const T& t, srast::Renderer& r, unsigned tileWidth, unsigned tileHeight) : t(t), r(r), tileWidth(tileWidth), tileHeight(tileHeight) {
	}
	
	virtual void run(unsigned start, unsigned end, unsigned thread) {
		unsigned width = r.getFrameBufferWidth();
		unsigned height = r.getFrameBufferHeight();
		
		unsigned* pixels = static_cast<unsigned*>(r.getFrameBuffer());
		unsigned pitch = r.getFrameBufferPitch();

		srast::ImportanceMap& importanceMap = r.getImportanceMap();
		
		for (unsigned i = start; i < end; ++i) {
			unsigned tx = (i % tileWidth) << srast::tileSizeLog2;
			unsigned ty = (i / tileWidth) << srast::tileSizeLog2;
			
			if (!importanceMap.isSet(srast::tileSizeLog2, tx, ty))
				continue;
			
			for (unsigned y = ty; y < height && y < ty + (1<<srast::tileSizeLog2); ++y) {
				for (unsigned x = tx; x < width && x < tx + (1<<srast::tileSizeLog2); ++x) {
					if (importanceMap.isSet(0, x, y))
						t(x, (height-y-1)) = pixels[(height-y-1)*pitch + x];
				}
			}
		}
	}
};

#endif
