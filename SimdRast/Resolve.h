//
//  Resolve.h
//  SimdRast
//
//  Created by Rasmus Barringer on 2012-11-02.
//  Copyright (c) 2012 Rasmus Barringer. All rights reserved.
//

#ifndef SimdRast_Resolve_h
#define SimdRast_Resolve_h

#include "Renderer.h"

namespace srast {

void resolveTile(Renderer& r, unsigned tx, unsigned ty, unsigned thread);

}

#endif
