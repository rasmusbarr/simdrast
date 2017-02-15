//
//  Shader.h
//  SimdRast
//
//  Created by Rasmus Barringer on 2012-10-21.
//  Copyright (c) 2012 Rasmus Barringer. All rights reserved.
//

#ifndef SimdRast_Shader_h
#define SimdRast_Shader_h

#include "VectorMath.h"

namespace srast {

class Shader {
public:
	virtual unsigned outputStride() const = 0;
	
	virtual void execute(const void* input, void* output, unsigned count, const void* uniforms) = 0;
};

}

#endif
