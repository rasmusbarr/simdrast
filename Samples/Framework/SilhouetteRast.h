//
//  SilhouetteRast.h
//  Framework
//
//  Created by Rasmus Barringer on 2012-10-26.
//  Copyright (c) 2012 Rasmus Barringer. All rights reserved.
//

#ifndef Framework_SilhouetteRast_h
#define Framework_SilhouetteRast_h

#include "../../SimdRast/Renderer.h"

namespace fx {

void rasterizeDrawCallSilhouettes(srast::Renderer& r, srast::DrawCall& drawCall);

}

#endif
