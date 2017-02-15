//
//  SceneRenderer.h
//  DirectXA4
//
//  Created by Rasmus Barringer on 2013-06-07.
//  Copyright (c) 2013 Rasmus Barringer. All rights reserved.
//

#ifndef DirectXA4_SceneRenderer_h
#define DirectXA4_SceneRenderer_h

#include "../../SimdRast/Renderer.h"
#include "../Framework/SilhouetteRast.h"
#include "Quad.h"
#include "Font.h"
#include "Scene.h"
#include "Window.h"
#include "Shader.h"
#include "RenderMesh.h"
#include "DraTexture.h"
#include "SceneShader.h"
#include "DraShadowMap.h"
#include "ConstantBuffer.h"
#include "CpuRendererThread.h"

enum RENDERERTYPE {
	RENDERERTYPE_HW = 0,
	RENDERERTYPE_SW,
	RENDERERTYPE_WARP,
	RENDERERTYPE_COUNT,
};

enum RENDERMODE {
	RENDERMODE_NOAA = 0,
	RENDERMODE_16XA4,
	RENDERMODE_8XMSAA,
	RENDERMODE_4XMSAA,
	RENDERMODE_2XMSAA,
	RENDERMODE_COUNT,
};

class SceneRenderer {
private:
	struct ShadowMap {
		DraShadowMap* shadowMap;
		DraTexture* shadowMapTexture;
	};

	RENDERERTYPE rendererType;
	RENDERMODE renderMode;

	unsigned width;
	unsigned height;

	std::string status;
	std::string timing;

	Scene* scene;
	RenderTarget* frameBufferTarget;

	RenderMesh renderMesh;
	Shader meshShader;
	ConstantBuffer<SceneUniforms, 0> meshUniforms;
	ReleasePointer<ID3D11SamplerState> meshSampler;

	ReleasePointer<ID3D11DepthStencilState> transparencyDepthState;
	ReleasePointer<ID3D11BlendState> transparencyBlendState;
	
	Font font;
	Quad quad;
	Shader quadShader;
	ReleasePointer<ID3D11SamplerState> quadSampler;

	std::vector<ShadowMap> shadowMaps;

	static const unsigned printFreq = 20;

	double sec;
	double lastTime;
	unsigned frameCount;

	GenericVertexShader vs;
	SceneAttributeShader as;
	SceneFragmentShader fs;

	CpuRendererThread cpuRendererThread;

public:
	SceneRenderer(RENDERERTYPE rendererType, unsigned width, unsigned height, Scene* scene);

	void nextMode();

	void render(Window* window);

	~SceneRenderer();

private:
	void renderRTs();

	void renderCPU(srast::Renderer* renderer);

	void renderGPU();
	
	void recreateFrameBuffer();
};

#endif
