//
//  VertexRenderState.h
//  SimdRast
//
//  Created by Rasmus Barringer on 2012-10-21.
//  Copyright (c) 2012 Rasmus Barringer. All rights reserved.
//

#ifndef SimdRast_VertexRenderState_h
#define SimdRast_VertexRenderState_h

#include "RenderState.h"

namespace srast {

enum CULLMODE {
	CULLMODE_BACK = 0,
};

class VertexRenderState : public RenderState {
private:
	struct CullSelector {
		typedef CULLMODE enumType;
		static const unsigned start = 0;
		static const unsigned end = start+2;
	};

public:
	void setCullMode(CULLMODE mode) {
		setMode<CullSelector>(mode);
	}
	
	CULLMODE getCullMode() const {
		return getMode<CullSelector>();
	}
};

}

#endif
