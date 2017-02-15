//
//  DxContext.cpp
//  DirectXA4
//
//  Created by Rasmus Barringer on 2013-06-07.
//  Copyright (c) 2013 Rasmus Barringer. All rights reserved.
//

#include "DxContext.h"
#include <stdexcept>
#include <string>
#include <cstdio>

ID3D11DeviceContext* DxContext::context = 0;
ID3D11Device* DxContext::device = 0;

ID3D11Device* DxContext::getDevice() {
	return device;
}

ID3D11DeviceContext* DxContext::getContext() {
	return context;
}

void DxContext::setCurrent(ID3D11Device* device, ID3D11DeviceContext* context) {
	DxContext::device = device;
	DxContext::context = context;
}

void DxContext::throwOnFail(HRESULT result, const char* call) {
	if (FAILED(result)) {
		char str[128];
		std::sprintf(str, "%X (%d) (%u)", result, (int)result, (unsigned)result);
		throw std::runtime_error(call + std::string(" failed with error ") + str + ".");
	}
}
