//
//  FragmentRenderState.h
//  SimdRast
//
//  Created by Rasmus Barringer on 2012-10-19.
//  Copyright (c) 2012 Rasmus Barringer. All rights reserved.
//

#ifndef SimdRast_FragmentRenderState_h
#define SimdRast_FragmentRenderState_h

#include "RenderState.h"

namespace srast {

enum BLENDMODE {
	BLENDMODE_REPLACE = 0,
	BLENDMODE_PREMUL_ALPHA,
};

enum DEPTHMODE {
	DEPTHMODE_LESS = 0,
};

class FragmentRenderState : public RenderState {
private:
	struct BlendSelector {
		typedef BLENDMODE enumType;
		static const unsigned start = 0;
		static const unsigned end = start+2;
	};

	struct DepthSelector {
		typedef DEPTHMODE enumType;
		static const unsigned start = BlendSelector::end;
		static const unsigned end = start+1;
	};
	
	struct DepthWriteSelector {
		typedef unsigned enumType;
		static const unsigned start = DepthSelector::end;
		static const unsigned end = start+1;
	};

public:
	FragmentRenderState() {
		setDepthWrite(true);
	}
	
	bool isOpaque() const {
		return getBlendMode() == BLENDMODE_REPLACE;
	}

	void setBlendMode(BLENDMODE mode) {
		setMode<BlendSelector>(mode);
	}
	
	BLENDMODE getBlendMode() const {
		return getMode<BlendSelector>();
	}
	
	void setDepthMode(DEPTHMODE mode) {
		setMode<DepthSelector>(mode);
	}
	
	DEPTHMODE getDepthMode() const {
		return getMode<DepthSelector>();
	}
	
	void setDepthWrite(bool write) {
		setMode<DepthWriteSelector>(write ? 1 : 0);
	}
	
	bool getDepthWrite() const {
		return getMode<DepthWriteSelector>() == 1;
	}
};

}

#endif
