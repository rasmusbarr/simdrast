//
//  ConstantBuffer.h
//  DirectXA4
//
//  Created by Rasmus Barringer on 2013-06-07.
//  Copyright (c) 2013 Rasmus Barringer. All rights reserved.
//

#ifndef DirectXA4_ConstantBuffer_h
#define DirectXA4_ConstantBuffer_h

#include "DxContext.h"

template<class T, int Reg>
class ConstantBuffer {
private:
	ReleasePointer<ID3D11Buffer> buffer;

public:
	T data;

	ConstantBuffer() {
		D3D11_BUFFER_DESC bd = { 0 };
		bd.Usage = D3D11_USAGE_DEFAULT;
		bd.ByteWidth = sizeof(T);
		bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		bd.CPUAccessFlags = 0;
		DX_THROW_ON_FAIL(DxContext::getDevice()->CreateBuffer(&bd, NULL, &buffer));
	}

	void update() {
		DxContext::getContext()->UpdateSubresource(buffer, 0, NULL, &data, 0, 0);
	}

	void bind() {
		ID3D11Buffer* buffers[] = { buffer };
		DxContext::getContext()->VSSetConstantBuffers(Reg, 1, buffers);
		DxContext::getContext()->PSSetConstantBuffers(Reg, 1, buffers);
	}
};

#endif
