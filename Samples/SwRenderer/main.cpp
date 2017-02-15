//
//  main.cpp
//  SwRenderer
//
//  Created by Rasmus Barringer on 2012-10-19.
//  Copyright (c) 2012 Rasmus Barringer. All rights reserved.
//

#include "../../SimdRast/Renderer.h"
#include "../Framework/Mesh.h"
#include "LambertShader.h"
#include <iostream>
#include <sys/time.h>
#include <GLUT/GLUT.h>
#include <OpenGL/OpenGL.h>

using namespace srast;

static const unsigned windowWidth = 1024;
static const unsigned windowHeight = 768;

static const char* meshFile = "crytek-sponza/sponza.obj";

static const unsigned backBufferCount = 3;
static unsigned currentBackBuffer = 0;

static GLuint backBufferPbo[backBufferCount];
static GLuint backBufferTexture[backBufferCount];

static Renderer* renderer = 0;
static fx::Mesh* mesh = 0;
static float rot = 0.0f;

static void renderSw(unsigned nextBuffer, unsigned* image) {
	LambertVertexShader vs;
	LambertAttributeShader as;
	LambertFragmentShader fs;
	LambertVertexShader::Uniforms vsUniforms;
	
	rot += 0.02f;
	float pi = 3.14159265f;
	
	float4x4 proj = float4x4::perspectiveProjection(pi*0.2f, (int)windowWidth/(float)(int)windowHeight, 1.0f, 2500.0f);
	float4x4 modelView = float4x4::lookAt(float3(0.0f, 150.0f, 550.0f), float3(0.0f, 190.0f, 0.0f), float3(0.0f, 1.0f, 0.0f)) * float4x4::rotationY(rot);
	
	mesh->sortDrawCalls(modelView);
	
	vsUniforms.modelViewProj = proj * modelView;
	as.light = normalize(float3(1.0f, 1.0f, 1.0f));

	renderer->setClearColor(0xffffaaaa);
	renderer->bindFrameBuffer(FRAMEBUFFERFORMAT_RGBA8, image, windowWidth, windowHeight, windowWidth);
	
	renderer->forceDense();

	for (size_t i = 0; i < mesh->sortedDrawCalls.size(); ++i) {
		renderer->bindVertexBuffer(mesh->sortedDrawCalls[i]->vertices, 0, sizeof(fx::Mesh::Vertex), mesh->sortedDrawCalls[i]->vertexCount);
		renderer->bindAttributeBuffer(mesh->sortedDrawCalls[i]->attributes, 0, sizeof(fx::Mesh::VertexAttributes), mesh->sortedDrawCalls[i]->vertexCount);
		renderer->bindIndexBuffer(mesh->sortedDrawCalls[i]->indices, 0, sizeof(unsigned), mesh->sortedDrawCalls[i]->indexCount);
		
		LambertFragmentShader::Uniforms fsUniforms = { 0 };
		
		renderer->getFragmentRenderState().setBlendMode(BLENDMODE_REPLACE);
		renderer->getFragmentRenderState().setDepthWrite(true);
		
		if (mesh->sortedDrawCalls[i]->texture != fx::Mesh::noTexture) {
			fsUniforms.diffuseTexture = mesh->textures[mesh->sortedDrawCalls[i]->texture];
			if (fsUniforms.diffuseTexture->hasAlpha()) {
				renderer->getFragmentRenderState().setBlendMode(BLENDMODE_PREMUL_ALPHA);
				renderer->getFragmentRenderState().setDepthWrite(false);
			}
		}
		
		renderer->bindShader(SHADERKIND_VERT, &vs, &vsUniforms, sizeof(vsUniforms));
		renderer->bindShader(SHADERKIND_ATTR, &as, &vsUniforms, sizeof(vsUniforms));
		renderer->bindShader(SHADERKIND_FRAG, &fs, &fsUniforms, sizeof(fsUniforms));
		
		renderer->drawIndexed();
	}
	
	renderer->beginFrontEndShadeAndHimRast();
	renderer->beginFrontEndBin();
	renderer->beginBackEnd();
	renderer->finish();
}

static void render() {
	glClear(GL_COLOR_BUFFER_BIT);

	glBindTexture(GL_TEXTURE_2D, backBufferTexture[currentBackBuffer]);
	glBegin(GL_QUADS);
	glTexCoord2f(0.0f, 1.0f); glVertex2f(-1.0f, -1.0f);
	glTexCoord2f(1.0f, 1.0f); glVertex2f(1.0f, -1.0f);
	glTexCoord2f(1.0f, 0.0f); glVertex2f(1.0f, 1.0f);
	glTexCoord2f(0.0f, 0.0f); glVertex2f(-1.0f, 1.0f);
	glEnd();

	currentBackBuffer = (currentBackBuffer + 1) % backBufferCount;
	
	glBindTexture(GL_TEXTURE_2D, backBufferTexture[currentBackBuffer]);
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, backBufferPbo[currentBackBuffer]);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, windowWidth, windowHeight, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	
	unsigned nextBuffer = (currentBackBuffer + 1) % backBufferCount;
	
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, backBufferPbo[nextBuffer]);
	unsigned* image = static_cast<unsigned*>(glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY));

	renderSw(nextBuffer, image);
	
	glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
		
	glutSwapBuffers();
	glutPostRedisplay();
}

static bool changeDirectory(const char* directory) {
	return chdir(directory) == 0;
}

static bool findDataDirectory() {
	for (unsigned i = 0; i < 8; ++i) {
		bool found = changeDirectory("Data");
		
		if (found)
			return true;
		
		if (!changeDirectory(".."))
			break;
	}
	return false;
}

int main(int argc, const char* argv[]) {
	if (!findDataDirectory()) {
		std::cout << "unable to find data directory." << std::endl;
		return 1;
	}
	
	std::cout << "using " << ThreadPool::cpuCount() << " cpu cores." << std::endl;
	
#ifdef SRAST_AVX
	std::cout << "using AVX instructions." << std::endl;
#else
	std::cout << "using SSE instructions." << std::endl;
#endif

	renderer = new Renderer();
	mesh = new fx::Mesh(meshFile);
	
	glutInit(&argc, const_cast<char**>(argv));
	glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE);
	glutInitWindowSize(windowWidth, windowHeight);
	glutCreateWindow("SwRenderer");
	glutDisplayFunc(render);

	glEnable(GL_TEXTURE_2D);

	glGenBuffers(backBufferCount, backBufferPbo);
	glGenTextures(backBufferCount, backBufferTexture);
	
	for (unsigned i = 0; i < backBufferCount; ++i) {
		glBindTexture(GL_TEXTURE_2D, backBufferTexture[i]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, windowWidth, windowHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
		
		glBindBuffer(GL_PIXEL_UNPACK_BUFFER, backBufferPbo[i]);
		glBufferData(GL_PIXEL_UNPACK_BUFFER, windowWidth*windowHeight*4, 0, GL_STREAM_DRAW);
	}
	
	glutMainLoop();

	delete mesh;
	delete renderer;
    return 0;
}
