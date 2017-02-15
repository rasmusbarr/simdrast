//
//  Texture.cpp
//  DirectXA4
//
//  Created by Rasmus Barringer on 2013-06-07.
//  Copyright (c) 2013 Rasmus Barringer. All rights reserved.
//

#include "Texture.h"

Texture::Texture(fx::LinearTexture* cpuTexture) {
	D3D11_TEXTURE2D_DESC desc = { 0 };
    desc.Width = cpuTexture->getWidth();
    desc.Height = cpuTexture->getHeight();
	desc.MipLevels = (unsigned)cpuTexture->getMipLevels().size();
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    desc.CPUAccessFlags = 0;
    desc.MiscFlags = 0;

	std::vector<D3D11_SUBRESOURCE_DATA> initData;

	for (size_t i = 0; i < cpuTexture->getMipLevels().size(); ++i) {
		D3D11_SUBRESOURCE_DATA data = { 0 };

		data.pSysMem = cpuTexture->getMipLevels()[i];
		data.SysMemPitch = (desc.Width >> i)*4;

		initData.push_back(data);
	}

    DX_THROW_ON_FAIL(DxContext::getDevice()->CreateTexture2D(&desc, &initData[0], &tex));

	D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc = { (DXGI_FORMAT)0 };
    SRVDesc.Format = desc.Format;
    SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    SRVDesc.Texture2D.MipLevels = desc.MipLevels;

    DX_THROW_ON_FAIL(DxContext::getDevice()->CreateShaderResourceView(tex, &SRVDesc, &textureView));
}

void Texture::bind(unsigned slot) {
	ID3D11ShaderResourceView* views[] = { textureView };
	DxContext::getContext()->PSSetShaderResources(slot, 1, views);
}
