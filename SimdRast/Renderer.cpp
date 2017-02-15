//
//  Renderer.cpp
//  SimdRast
//
//  Created by Rasmus Barringer on 2012-10-20.
//  Copyright (c) 2012 Rasmus Barringer. All rights reserved.
//

#include "Renderer.h"
#include "Binning.h"
#include "Resolve.h"
#include "TriangleSetup.h"
#include "IndexProvider.h"
#include <new>
#include <iostream>
#include <stdexcept>
#include <unordered_map>

namespace srast {

Renderer::Renderer() : poolAllocator(1024*1024*1024), binListArray(threadPool), compositeBinListArray(threadPool), localAllocators(poolAllocator, threadPool) {
	frameNumber = 0;
	reset();
}

VertexRenderState& Renderer::getVertexRenderState() {
	return currentDrawCall.vertexRenderState;
}

FragmentRenderState& Renderer::getFragmentRenderState() {
	return currentDrawCall.fragmentRenderState;
}

void Renderer::setClearColor(unsigned color) {
	clearColor = color;
}

void Renderer::forceDense() {
	dense = true;
}

void Renderer::forceTransparentImportance() {
	transparentImportance = true;
}

void Renderer::setupHimRasterization(void (*rasterizeDrawCallToHim)(Renderer& r, DrawCall& drawCall)) {
	this->rasterizeDrawCallToHim = rasterizeDrawCallToHim;
}

void Renderer::bindFrameBuffer(FRAMEBUFFERFORMAT format, void* frameBuffer, unsigned width, unsigned height, unsigned pitch) {
	this->frameBuffer = frameBuffer;
	frameBufferFormat = format;
	frameBufferWidth = width;
	frameBufferHeight = height;
	frameBufferPitch = pitch;
	importanceMap.resize(width, height);
	binListArray.resize(width, height);
}

void Renderer::bindIndexBuffer(void* indexBuffer, unsigned offset, unsigned size, unsigned count) {
	if (size != 1 && size != 2 && size != 4)
		throw std::runtime_error("invalid index buffer stride");
	
	currentDrawCall.indexBuffer.data = static_cast<char*>(indexBuffer) + offset;
	currentDrawCall.indexBuffer.stride = size;
	currentDrawCall.indexBuffer.count = count;
	
	if (dense) {
		currentDrawCall.adjacency = 0;
	}
	else {
		std::map<void*, unsigned*>::iterator i = adjacencyBuffers.find(indexBuffer);
		
		if (i != adjacencyBuffers.end()) {
			currentDrawCall.adjacency = i->second;
		}
		else {
			unsigned* adj = generateAdjacencyBuffer(currentDrawCall.indexBuffer.data, currentDrawCall.indexBuffer.stride, currentDrawCall.indexBuffer.count);
			adjacencyBuffers[indexBuffer] = adj;
			currentDrawCall.adjacency = adj;
		}
	}
}

void Renderer::bindVertexBuffer(void* vertexBuffer, unsigned offset, unsigned stride, unsigned count) {
	if (stride & 0xf || reinterpret_cast<unsigned long long>(static_cast<char*>(vertexBuffer) + offset) & 0xf)
		throw std::runtime_error("invalid vertex buffer alignment");
	
	currentDrawCall.vertexBuffer.data = static_cast<char*>(vertexBuffer) + offset;
	currentDrawCall.vertexBuffer.stride = stride;
	currentDrawCall.vertexBuffer.count = count;
}

void Renderer::bindAttributeBuffer(void* attributeBuffer, unsigned offset, unsigned stride, unsigned count) {
	if (stride & 0xf || reinterpret_cast<unsigned long long>(static_cast<char*>(attributeBuffer) + offset) & 0xf)
		throw std::runtime_error("invalid attribute buffer alignment");
	
	currentDrawCall.attributeBuffer.data = static_cast<char*>(attributeBuffer) + offset;
	currentDrawCall.attributeBuffer.stride = stride;
	currentDrawCall.attributeBuffer.count = count;
}

void Renderer::invalidateIndexBuffer(void* indexBuffer) {
	std::map<void*, unsigned*>::iterator i = adjacencyBuffers.find(indexBuffer);
	
	if (i == adjacencyBuffers.end())
		return;
	
	simd_free(i->second);
	adjacencyBuffers.erase(i);
}

void Renderer::bindShader(SHADERKIND shaderKind, Shader* shader, const void* uniforms, unsigned uniformSize) {
	if (shaderKind == SHADERKIND_ATTR && shader->outputStride() & 0xf)
		throw std::runtime_error("invalid alignment of attribute shader output");
	
	currentShader[shaderKind] = shader;
	currentUniforms[shaderKind] = poolAllocator.clone(uniforms, uniformSize);
}

void Renderer::drawList() {
	DrawCall d = currentDrawCall;

	d.indexBuffer.clear();
	setupShaders(d);
	drawCalls.push_back(d);
}

void Renderer::drawIndexed() {
	DrawCall d = currentDrawCall;

	if (!d.indexBuffer.data)
		throw std::runtime_error("no index buffer bound");
	
	setupShaders(d);
	drawCalls.push_back(d);
}

class VertexShadeTask : public ThreadPoolTask {
private:
	Renderer& r;
	DrawCall& d;
	
public:
	VertexShadeTask(Renderer& r, DrawCall& d) : r(r), d(d) {
	}
	
	virtual void run(unsigned start, unsigned end, unsigned thread) {
		d.vertexRenderState.executeShader(static_cast<char*>(d.vertexBuffer.data) + d.vertexBuffer.stride*start,
										  d.shadedPositions + start, end-start);
	}
	
	virtual void finished() {
		setupDrawCallTriangles(r, d);
	}
};

void Renderer::beginFrontEndShadeAndHimRast() {
	frameBufferSizeLog2 = 0;
	
	while (frameBufferWidth >> frameBufferSizeLog2 && frameBufferHeight >> frameBufferSizeLog2)
		frameBufferSizeLog2++;
	
	for (size_t i = 0; i < drawCalls.size(); ++i) {
		DrawCall& d = drawCalls[i];
		
		unsigned count = d.vertexBuffer.count;
		unsigned triangleCount = d.indexBuffer.stride ? d.indexBuffer.count/3 : d.vertexBuffer.count/3;

		d.shadedPositions = static_cast<float4*>(poolAllocator.allocate(sizeof(float4)*(count+32)));
		d.edges = static_cast<float4*>(poolAllocator.allocate(sizeof(float4)*(triangleCount*4+32)));
		d.highpEdgeZ0 = static_cast<double*>(poolAllocator.allocate(sizeof(double)*(triangleCount+32)));
		d.highpEdgeZ1 = static_cast<double*>(poolAllocator.allocate(sizeof(double)*(triangleCount+32)));
		d.highpEdgeZ2 = static_cast<double*>(poolAllocator.allocate(sizeof(double)*(triangleCount+32)));
		d.flags = static_cast<unsigned char*>(poolAllocator.allocate(triangleCount + 32));
		d.task = static_cast<ThreadPoolTask*>(poolAllocator.allocate(64));
		
		VertexShadeTask* t = new (d.task) VertexShadeTask(*this, d);
		threadPool.startTask(t, count, 1024, true);
	}

	if (frameNumber == 0xffffffff) {
		// Clear all bins. Expensive but happens less than once each month at a rate of 1000 fps.
		frameNumber = 0;
		binListArray.clear();
	}
	++frameNumber;
	binListArray.clearZ();
}

void Renderer::beginFrontEndBin() {
	threadPool.barrier();
	
	if (dense)
		importanceMap.fill();
	
	importanceMap.build(threadPool);

	for (size_t i = 0; i < drawCalls.size(); ++i)
		binDrawCall(drawCalls[i]);
}

void Renderer::beginBackEnd() {
	threadPool.barrier();
	resolveTiles();
}

void Renderer::finish() {
	threadPool.barrier();
	reset();
}

void Renderer::reset() {
	poolAllocator.reset();
	localAllocators.reset();
	
	dense = false;
	transparentImportance = false;
	clearColor = 0;
	frameBuffer = 0;
	rasterizeDrawCallToHim = 0;

	currentDrawCall = DrawCall();
	drawCalls.resize(0);
}

Renderer::~Renderer() {
	for (std::map<void*, unsigned*>::iterator i = adjacencyBuffers.begin(); i != adjacencyBuffers.end(); ++i)
		simd_free(i->second);
}

template<class T>
static unsigned* generateAdjacencyBuffer(T indices, unsigned count) {
	unsigned* adjacency = static_cast<unsigned*>(simd_malloc(sizeof(unsigned)*count, 64));
	std::unordered_map<unsigned long long, unsigned> edgeAdjacencyMap;
	
	std::cout << "calculating adjacency..." << std::flush;
	size_t progress = 0;
	
	for (unsigned i = 0; i < count; i += 3) {
		int3 ind = indices(i);
		
		for (unsigned j = 0; j < 3; ++j) {
			unsigned i0 = ind[j];
			unsigned i1 = ind[(j+1)%3];
			
			if (i0 > i1)
				std::swap(i0, i1);
			
			unsigned long long edge = (unsigned long long)i0 << 32 | i1;
			std::unordered_map<unsigned long long, unsigned>::iterator k = edgeAdjacencyMap.find(edge);
			
			if (k == edgeAdjacencyMap.end()) {
				edgeAdjacencyMap[edge] = i+j;
				adjacency[i+j] = 0xffffffff;
			}
			else {
				unsigned other = k->second;
				adjacency[other] = i+j;
				adjacency[i+j] = other;
			}
		}
		
		size_t np = 10*(i+3)/count;
		
		while (np != progress) {
			++progress;
			std::cout << " " << progress << std::flush;
		}
	}
	
	for (unsigned i = 0; i < count; ++i) {
		if (adjacency[i] != 0xffffffff)
			adjacency[i] /= 3;
	}
	
	std::cout << std::endl;
	return adjacency;
}

unsigned* Renderer::generateAdjacencyBuffer(const void* indices, unsigned stride, unsigned count) {
	if (stride == 1)
		return srast::generateAdjacencyBuffer(IndexProvider<unsigned char>(indices), count);
	else if (stride == 2)
		return srast::generateAdjacencyBuffer(IndexProvider<unsigned short>(indices), count);
	else if (stride == 4)
		return srast::generateAdjacencyBuffer(IndexProvider<unsigned int>(indices), count);
	else
		return 0;
}

class BinTask : public ThreadPoolTask {
private:
	Renderer& r;
	DrawCall& drawCall;

public:
	BinTask(Renderer& r, DrawCall& drawCall) : r(r), drawCall(drawCall) {
	}
	
	virtual void run(unsigned start, unsigned end, unsigned thread) {
		binDrawCall(r, drawCall, start, end, r.frameBufferSizeLog2, thread);
	}
};

void Renderer::binDrawCall(DrawCall& drawCall) {
	BinTask* t = new (drawCall.task) BinTask(*this, drawCall);
	unsigned triangleCount = drawCall.indexBuffer.stride ? drawCall.indexBuffer.count/3 : drawCall.vertexBuffer.count/3;
	
	threadPool.startTask(t, triangleCount, 1024);
}

class ResolveTask : public ThreadPoolTask {
private:
	Renderer& r;
	unsigned tileWidth;
	unsigned tileHeight;
	
public:
	ResolveTask(Renderer& r, unsigned tileWidth, unsigned tileHeight) : r(r), tileWidth(tileWidth), tileHeight(tileHeight) {
	}
	
	virtual void run(unsigned start, unsigned end, unsigned thread) {
		for (unsigned i = start; i < end; ++i) {
			unsigned x = (i % tileWidth) << tileSizeLog2;
			unsigned y = (i / tileWidth) << tileSizeLog2;
			resolveTile(r, x, y, thread);
		}
	}

	virtual void finish() {
		delete this;
	}
};

void Renderer::resolveTiles() {
	unsigned tileWidth = (frameBufferWidth + (1 << tileSizeLog2)-1) >> tileSizeLog2;
	unsigned tileHeight = (frameBufferHeight + (1 << tileSizeLog2)-1) >> tileSizeLog2;
	
	ResolveTask* t = new ResolveTask(*this, tileWidth, tileHeight);
	threadPool.startTask(t, tileWidth*tileHeight, 8, true);
}

void Renderer::setupShaders(DrawCall& drawCall) {
	for (unsigned i = 0; i < 3; ++i) {
		RenderState& state = i == SHADERKIND_ATTR ? drawCall.attributeRenderState :
		(i == SHADERKIND_VERT ? static_cast<RenderState&>(drawCall.vertexRenderState) : static_cast<RenderState&>(drawCall.fragmentRenderState));

		state.setShader(currentShader[i], currentUniforms[i]);
	}
}

}
