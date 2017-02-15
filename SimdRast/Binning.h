//
//  Binning.h
//  SimdRast
//
//  Created by Rasmus Barringer on 2013-05-30.
//  Copyright (c) 2013 Rasmus Barringer. All rights reserved.
//

#ifndef SimdRast_Binning_h
#define SimdRast_Binning_h

#include "Renderer.h"

namespace srast {

void binDrawCall(Renderer& r, DrawCall& drawCall, unsigned start, unsigned end, int maxLevel, unsigned thread);

}

#endif
