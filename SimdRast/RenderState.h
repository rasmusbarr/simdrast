//
//  RenderState.h
//  SimdRast
//
//  Created by Rasmus Barringer on 2012-10-21.
//  Copyright (c) 2012 Rasmus Barringer. All rights reserved.
//

#ifndef SimdRast_RenderState_h
#define SimdRast_RenderState_h

#include "Shader.h"

namespace srast {

class RenderState {
protected:
	unsigned mode;
	Shader* shader;
	const void* uniforms;
	
public:
	RenderState() {
		reset();
	}
	
	void reset() {
		mode = 0;
		shader = 0;
		uniforms = 0;
	}
	
	void setShader(Shader* shader, const void* uniforms) {
		this->shader = shader;
		this->uniforms = uniforms;
	}
	
	Shader* getShader() const {
		return shader;
	}
	
	void executeShader(const void* input, void* output, unsigned count) const {
		shader->execute(input, output, count, uniforms);
	}
	
	bool operator != (const RenderState& rhs) const {
		return mode != rhs.mode;
	}

protected:
	template<class T>
	void setMode(typename T::enumType x) {
		mode &= ~(((1 << (T::end-T::start))-1) << T::start);
		mode |= (unsigned)x << T::start;
	}
	
	template<class T>
	typename T::enumType getMode() const {
		unsigned x = (mode >> T::start) & ((1 << (T::end-T::start))-1);
		return (typename T::enumType)x;
	}
};

}

#endif
