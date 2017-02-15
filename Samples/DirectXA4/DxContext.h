//
//  DxContext.h
//  DirectXA4
//
//  Created by Rasmus Barringer on 2013-06-07.
//  Copyright (c) 2013 Rasmus Barringer. All rights reserved.
//

#ifndef DirectXA4_DxContext_h
#define DirectXA4_DxContext_h

#define NOMINMAX
#include <D3D11.h>

#define DX_THROW_ON_FAIL(a) DxContext::throwOnFail(a, #a)

class DxContext {
private:
	static ID3D11Device* device;
	static ID3D11DeviceContext* context;

public:
	static ID3D11Device* getDevice();

	static ID3D11DeviceContext* getContext();

	static void setCurrent(ID3D11Device* device, ID3D11DeviceContext* context);

	static void throwOnFail(HRESULT result, const char* call);
};

#endif
