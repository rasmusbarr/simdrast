//
//  Texture.h
//  DirectXA4
//
//  Created by Rasmus Barringer on 2013-06-07.
//  Copyright (c) 2013 Rasmus Barringer. All rights reserved.
//

#ifndef DirectXA4_Texture_h
#define DirectXA4_Texture_h

#include "DxContext.h"
#include "ReleasePointer.h"
#include "../Framework/LinearTexture.h"

class Texture {
private:
	ReleasePointer<ID3D11Texture2D> tex;
	ReleasePointer<ID3D11ShaderResourceView> textureView;

public:
	Texture(fx::LinearTexture* cpuTexture);

	void bind(unsigned slot);
};

#endif
