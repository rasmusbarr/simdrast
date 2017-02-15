//
//  SceneRenderer.cpp
//  DirectXA4
//
//  Created by Rasmus Barringer on 2013-06-07.
//  Copyright (c) 2013 Rasmus Barringer. All rights reserved.
//

#include "SceneRenderer.h"
#include "MsaaRenderTarget.h"
#include <iomanip>
#include <sstream>

static const char* renderModeString[] = {
	"no AA",
	"16x A4",
	"8x MSAA",
	"4x MSAA",
	"2x MSAA",
};

static double seconds() {
	return GetTickCount64()*(1.0/1000.0);
}

static const D3D_SHADER_MACRO* determineShaderMacros(Scene* scene) {
	static const D3D_SHADER_MACRO noTextureMacro[] = {
		{ "NO_DIFFUSE", "", },
		{ 0 },
	};

	for (size_t i = 0; i < scene->getMesh()->drawCalls.size(); ++i) {
		if (scene->getMesh()->drawCalls[i].texture == fx::Mesh::noTexture)
			return noTextureMacro;
	}

	return 0;
}

SceneRenderer::SceneRenderer(RENDERERTYPE rendererType, unsigned width, unsigned height, Scene* scene) : rendererType(rendererType), width(width), height(height), scene(scene), renderMesh(scene->getMesh()), meshShader("Scene.fx", renderMesh.layout(), determineShaderMacros(scene)), quadShader("QuadTexture.fx", quad.layout()), cpuRendererThread(Delegate<srast::Renderer*>::fromMember<SceneRenderer, &SceneRenderer::renderCPU>(*this), width, height) {
	renderMode = rendererType == RENDERERTYPE_WARP ? RENDERMODE_NOAA : RENDERMODE_16XA4;

	cpuRendererThread.start();

	if (rendererType != RENDERERTYPE_WARP)
		DraRenderTarget::activateExtension();

	ReleasePointer<ID3D11RasterizerState> rasterizerState;
	D3D11_RASTERIZER_DESC rasterizerStateDesc = { (D3D11_FILL_MODE)0 };
	rasterizerStateDesc.FillMode = D3D11_FILL_SOLID;
	rasterizerStateDesc.CullMode = D3D11_CULL_FRONT;
	rasterizerStateDesc.FrontCounterClockwise = FALSE;
	rasterizerStateDesc.DepthBias = D3D11_DEFAULT_DEPTH_BIAS;
	rasterizerStateDesc.DepthBiasClamp = D3D11_DEFAULT_DEPTH_BIAS_CLAMP;
	rasterizerStateDesc.SlopeScaledDepthBias = D3D11_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
	rasterizerStateDesc.DepthClipEnable = TRUE;
	rasterizerStateDesc.ScissorEnable = FALSE;
	rasterizerStateDesc.MultisampleEnable = FALSE;
	rasterizerStateDesc.AntialiasedLineEnable = FALSE;
	DxContext::getDevice()->CreateRasterizerState(&rasterizerStateDesc, &rasterizerState);
	DxContext::getContext()->RSSetState(rasterizerState);

	D3D11_SAMPLER_DESC sampDesc = { (D3D11_FILTER)0 };
	sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
	sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	sampDesc.MinLOD = 0;
	sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
	DX_THROW_ON_FAIL(DxContext::getDevice()->CreateSamplerState(&sampDesc, &quadSampler));

	sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	DX_THROW_ON_FAIL(DxContext::getDevice()->CreateSamplerState(&sampDesc, &meshSampler));

	D3D11_DEPTH_STENCIL_DESC dsDesc = { 0 };
	dsDesc.DepthEnable = TRUE;
	dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	dsDesc.DepthFunc = D3D11_COMPARISON_LESS;
	dsDesc.StencilEnable = FALSE;
	DX_THROW_ON_FAIL(DxContext::getDevice()->CreateDepthStencilState(&dsDesc, &transparencyDepthState));

	D3D11_BLEND_DESC blendStateDesc = { 0 };
	blendStateDesc.RenderTarget[0].BlendEnable = TRUE;
	blendStateDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
	blendStateDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	blendStateDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	blendStateDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	blendStateDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
	blendStateDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	blendStateDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
	DX_THROW_ON_FAIL(DxContext::getDevice()->CreateBlendState(&blendStateDesc, &transparencyBlendState));

	const std::vector<srast::int2>& shadowMapSizes = scene->getShadowMapSizes();
	
	for (size_t i = 0; i < shadowMapSizes.size(); ++i) {
		DraShadowMap* shadowMap = new DraShadowMap(shadowMapSizes[i].x, shadowMapSizes[i].y);
		DraTexture* shadowMapTexture = new DraTexture(shadowMapSizes[i].x, shadowMapSizes[i].y);
		
		ShadowMap entry = {
			shadowMap,
			shadowMapTexture,
		};
		shadowMaps.push_back(entry);
	}

	frameBufferTarget = 0;
	recreateFrameBuffer();
}

void SceneRenderer::nextMode() {
	if (rendererType == RENDERERTYPE_SW)
		return;

	renderMode = (RENDERMODE)((renderMode+1) % RENDERMODE_COUNT);

	if (renderMode == RENDERMODE_16XA4 && rendererType == RENDERERTYPE_WARP)
		renderMode = (RENDERMODE)((renderMode+1) % RENDERMODE_COUNT);

	recreateFrameBuffer();
}

void SceneRenderer::render(Window* window) {
	double time = seconds();
	float timeStep = (float)(time - lastTime);
	lastTime = time;

	scene->updateUniforms(window, timeStep, meshUniforms.data);

	for (size_t i = 0; i < shadowMaps.size(); ++i)
		shadowMaps[i].shadowMap->setWorldToShadowTransform(meshUniforms.data.shadowTransform[i]);

	meshUniforms.data.shadowCount = (int)shadowMaps.size();

	if (rendererType == RENDERERTYPE_SW || renderMode == RENDERMODE_16XA4)
		cpuRendererThread.begin();

	if (shadowMaps.size()) {
		renderRTs();

		for (size_t i = 0; i < shadowMaps.size(); ++i) {
			DraSurface surface = shadowMaps[i].shadowMap->map();
			shadowMaps[i].shadowMapTexture->setup(surface);
			shadowMaps[i].shadowMap->unmap(); // Note: We need to unmap for it to be used in GPU shaders. The pointer/surface is still valid after unmapping.
		}
	}

	if (rendererType == RENDERERTYPE_SW || renderMode == RENDERMODE_16XA4)
		cpuRendererThread.enableBackEnd();
		
	if (rendererType != RENDERERTYPE_SW)
		renderGPU();

	if (rendererType == RENDERERTYPE_SW || renderMode == RENDERMODE_16XA4) {
		DxContext::getContext()->Flush();
		cpuRendererThread.end();

		DraRenderTarget* dra = static_cast<DraRenderTarget*>(frameBufferTarget);
		cpuRendererThread.finish(dra);
	}

	window->bindRenderTarget();
	
	frameBufferTarget->resolve();
	frameBufferTarget->bindTexture(0);

	ID3D11SamplerState* samplers[] = { quadSampler };
	DxContext::getContext()->PSSetSamplers(0, 1, samplers);
	quadShader.bind();
	quad.render();

	const char helpText[] =
		"\n\n"
		"Scene:    1-2\n"
		"Renderer:   r\n"
		"AA mode:    m\n"
		"Animation:  s";

	font.draw(0.0f, 0.0f, helpText);

	sec += timeStep;

	if (++frameCount == printFreq) {
		sec *= 1.0 / printFreq;
		double fps = 1.0 / sec;

		std::ostringstream ss;
		ss << std::setw(3) << (int)(1000.0 * sec + 0.5) << " ms" << " (" << std::setw(3) << (int)(fps + 0.5) << " fps)";
		timing = ss.str();

		frameCount = 0;
		sec = 0.0;
	}

	font.draw(0.0f, 0.0f, status + " " + timing);
}

SceneRenderer::~SceneRenderer() {
	delete frameBufferTarget;
		
	cpuRendererThread.join();

	for (size_t i = 0; i < shadowMaps.size(); ++i) {
		delete shadowMaps[i].shadowMap;
		delete shadowMaps[i].shadowMapTexture;
	}
}

void SceneRenderer::renderRTs() {
	for (size_t i = 0; i < shadowMaps.size(); ++i) {
		shadowMaps[i].shadowMap->bind();
		renderMesh.renderOpaque();
	}
}

void SceneRenderer::renderCPU(srast::Renderer* renderer) {
	GenericVertexShader::Uniforms vsUniforms;
	SceneFragmentShader::Uniforms fsUniforms = { 0 };

	for (size_t i = 0; i < shadowMaps.size(); ++i) {
		fsUniforms.shadowTransform[i] = meshUniforms.data.shadowTransform[i];
		fsUniforms.light[i].x = meshUniforms.data.light[i].x;
		fsUniforms.light[i].y = meshUniforms.data.light[i].y;
		fsUniforms.light[i].z = meshUniforms.data.light[i].z;
	}

	vsUniforms.modelViewProj = meshUniforms.data.modelViewProj;
	fsUniforms.view = meshUniforms.data.view;

	renderer->setClearColor(0xffffcccc);
		
	if (rendererType == RENDERERTYPE_SW)
		renderer->forceDense();
	else
		renderer->setupHimRasterization(fx::rasterizeDrawCallSilhouettes);

	fx::Mesh& mesh = *scene->getMesh();

	for (size_t i = 0; i < mesh.sortedDrawCalls.size(); ++i) {
		renderer->bindVertexBuffer(mesh.sortedDrawCalls[i]->vertices, 0, sizeof(fx::Mesh::Vertex), mesh.sortedDrawCalls[i]->vertexCount);
		renderer->bindAttributeBuffer(mesh.sortedDrawCalls[i]->attributes, 0, sizeof(fx::Mesh::VertexAttributes), mesh.sortedDrawCalls[i]->vertexCount);
		renderer->bindIndexBuffer(mesh.sortedDrawCalls[i]->indices, 0, sizeof(unsigned), mesh.sortedDrawCalls[i]->indexCount);

		bool alphaBlend = false;

		if (mesh.sortedDrawCalls[i]->texture != fx::Mesh::noTexture) {
			fsUniforms.diffuseTexture = mesh.textures[mesh.sortedDrawCalls[i]->texture];
			alphaBlend = fsUniforms.diffuseTexture->hasAlpha();
		}

		fsUniforms.shadowCount = (unsigned)shadowMaps.size();

		for (unsigned i = 0; i < fsUniforms.shadowCount; ++i)
			fsUniforms.shadowMapTexture[i] = shadowMaps[i].shadowMapTexture;
		
		renderer->bindShader(srast::SHADERKIND_VERT, &vs, &vsUniforms, sizeof(vsUniforms));
		renderer->bindShader(srast::SHADERKIND_ATTR, &as, 0, 0);
		renderer->bindShader(srast::SHADERKIND_FRAG, &fs, &fsUniforms, sizeof(fsUniforms));

		if (alphaBlend) {
			renderer->getFragmentRenderState().setDepthWrite(false);
			renderer->getFragmentRenderState().setBlendMode(srast::BLENDMODE_PREMUL_ALPHA);
		}
		else {
			renderer->getFragmentRenderState().setDepthWrite(true);
			renderer->getFragmentRenderState().setBlendMode(srast::BLENDMODE_REPLACE);
		}
				
		renderer->drawIndexed();
	}
}

void SceneRenderer::renderGPU() {
	frameBufferTarget->clear(srast::float4(0.8f, 0.8f, 1.0f, 1.0f));
	frameBufferTarget->bind();

	for (size_t i = 0; i < shadowMaps.size(); ++i)
		shadowMaps[i].shadowMap->bindSamplerAndTexture((unsigned)i+1);

	meshUniforms.update();
	meshUniforms.bind();

	ID3D11SamplerState* samplers[] = { meshSampler };
	DxContext::getContext()->PSSetSamplers(0, 1, samplers);
	meshShader.bind();
	renderMesh.renderOpaque();

	DxContext::getContext()->OMSetBlendState(transparencyBlendState, NULL, 0xFFFFFF);
	DxContext::getContext()->OMSetDepthStencilState(transparencyDepthState, 1);
		
	renderMesh.renderTransparent();

	DxContext::getContext()->OMSetBlendState(0, NULL, 0xFFFFFF);
	DxContext::getContext()->OMSetDepthStencilState(0, 1);
}

void SceneRenderer::recreateFrameBuffer() {
	delete frameBufferTarget;
	frameBufferTarget = 0;

	if (renderMode == RENDERMODE_8XMSAA)
		frameBufferTarget = new MsaaRenderTarget(width, height, 8, 1, DXGI_FORMAT_R8G8B8A8_UNORM);
	else if (renderMode == RENDERMODE_4XMSAA)
		frameBufferTarget = new MsaaRenderTarget(width, height, 4, 1, DXGI_FORMAT_R8G8B8A8_UNORM);
	else if (renderMode == RENDERMODE_2XMSAA)
		frameBufferTarget = new MsaaRenderTarget(width, height, 2, 1, DXGI_FORMAT_R8G8B8A8_UNORM);
	else if (renderMode == RENDERMODE_NOAA)
		frameBufferTarget = new MsaaRenderTarget(width, height, 1, 1, DXGI_FORMAT_R8G8B8A8_UNORM);
	else
		frameBufferTarget = new DraRenderTarget(width, height, 1, DXGI_FORMAT_R8G8B8A8_UNORM);

	std::ostringstream ss;

	if (rendererType == RENDERERTYPE_WARP)
		ss << "WARP ";
	else if (rendererType == RENDERERTYPE_SW)
		ss << "SW (A4 100%) ";
	else
		ss << "HW ";

	ss << width << "x" << height << " ";
	ss << renderModeString[renderMode] << " ";
	status = ss.str();

	frameCount = 0;
	sec = 0.0;
	lastTime = seconds();
	timing = "...";
}
