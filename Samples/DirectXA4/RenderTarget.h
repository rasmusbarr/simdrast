//
//  RenderTarget.h
//  DirectXA4
//
//  Created by Rasmus Barringer on 2013-06-07.
//  Copyright (c) 2013 Rasmus Barringer. All rights reserved.
//

#ifndef DirectXA4_RenderTarget_h
#define DirectXA4_RenderTarget_h

#include "DxContext.h"
#include "../../SimdRast/VectorMath.h"

class RenderTarget {
public:
	virtual void resolve() {}

	virtual void bind() = 0;

	virtual void bindTexture(unsigned slot) = 0;

	virtual void clear(const srast::float4& color) = 0;

	virtual ~RenderTarget() {}
};

#endif
