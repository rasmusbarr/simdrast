//
//  Mesh.cpp
//  Framework
//
//  Created by Rasmus Barringer on 2012-10-22.
//  Copyright (c) 2012 Rasmus Barringer. All rights reserved.
//

#include "Mesh.h"
#include <unordered_map>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <string>
#include <map>

using namespace srast;

namespace fx {

static unsigned align(unsigned x, unsigned boundary) {
	unsigned alignMask = boundary-1;
	return (x+alignMask) & (~alignMask);
}

static std::string folderFromPath(const std::string& path) {
	std::string folder;
	size_t sep = path.rfind('/');
	
	if (sep != std::string::npos)
		folder = path.substr(0, sep+1);
	
	return folder;
}

static std::string toUnixPath(std::string path) {
	std::replace(path.begin(), path.end(), '\\', '/');
	return path;
}

inline bool isSpace(char c) {
	return c == ' ' || c == '\t';
}

inline void skipSpace(const char*& l) {
	while (isSpace(*l))
		++l;
}

inline bool matchPattern(const char*& l, const char* pattern) {
	while (*pattern) {
		if (*(pattern++) != *(l++))
			return false;
	}
	return true;
}

static bool parseString(const std::string& line, const char* pattern, std::string& str) {
	const char* l = line.c_str();
	skipSpace(l);

	if (!matchPattern(l, pattern) || !isSpace(*l))
		return false;
	
	skipSpace(++l);
	str = l;
	return true;
}

static std::map<std::string, LinearTexture*> parseMaterialTextures(const std::string& folder, std::istream& file) {
	using namespace std;
	
	map<std::string, LinearTexture*> textureMap;

	string line;
	string material;
	
	while (getline(file, line)) {
		string name;

		if (parseString(line, "newmtl", name)) {
			material = name;
		}
		else if (parseString(line, "map_Kd", name)) {
			if (!material.empty() && !textureMap.count(material)) {
				string file = folder + toUnixPath(name);

				try {
					LinearTexture* texture = new LinearTexture(file.c_str());
					textureMap[material] = texture;
					std::cout << "loaded texture " << file << std::endl;
				}
				catch (...) {
					std::cout << "FAILED to load texture " << file << std::endl;
				}
			}
		}
	}
	
	return textureMap;
}

Mesh::Mesh(const char* filename) {
	using namespace std;

	ifstream file(filename);
	
	if (!file)
		throw runtime_error((string("cannot find ") + filename).c_str());
	
	string line;

	vector<float3> filePositions;
	vector<float3> fileNormals;
	vector<float2> fileTextures;
	
	std::map<std::string, LinearTexture*> textureMap;
	
	std::vector<std::vector<int3> > objectTriangles;
	std::vector<std::string> objectMaterials;
	
	objectTriangles.push_back(std::vector<int3>());
	objectMaterials.push_back("");

	vector<int3>* triangles = &(*objectTriangles.rbegin());
	
	cout << "loading obj " << filename << endl;

	while (getline(file, line)) {
		float x, y, z;
		int a[3], b[3], c[3], d[3];
		string name;
		
		if (parseString(line, "mtllib", name)) {
			string folder = folderFromPath(filename);
			
			ifstream lib(folder + toUnixPath(name));
			
			if (lib)
				textureMap = parseMaterialTextures(folder, lib);
		}
		else if (sscanf(line.c_str(), "v %f %f %f", &x, &y, &z) == 3) {
			filePositions.push_back(float3(x, y, z));
		}
		else if (sscanf(line.c_str(), "vn %f %f %f", &x, &y, &z) == 3) {
			fileNormals.push_back(normalize(float3(x, y, z)));
		}
		else if (sscanf(line.c_str(), "vt %f %f", &x, &y) == 2) {
			fileTextures.push_back(float2(x, 1.0f-y));
		}
		else if (parseString(line, "g", name)) {
			if (!objectTriangles.rbegin()->empty()) {
				objectTriangles.push_back(std::vector<int3>());
				objectMaterials.push_back("");
				
				triangles = &(*objectTriangles.rbegin());
			}
		}
		else if (parseString(line, "usemtl", name)) {
			if (!objectTriangles.rbegin()->empty()) {
				objectTriangles.push_back(std::vector<int3>());
				objectMaterials.push_back("");
				
				triangles = &(*objectTriangles.rbegin());
			}

			*objectMaterials.rbegin() = name;
		}
		else if (sscanf(line.c_str(), "f %d %d %d", &a[0], &b[0], &c[0]) == 3) {
			triangles->push_back(int3(c[0]-1, b[0]-1, a[0]-1));
			triangles->push_back(int3(-1, -1, -1));
			triangles->push_back(int3(-1, -1, -1));
		}
		else if (sscanf(line.c_str(), "f %d/%d/%d %d/%d/%d %d/%d/%d %d/%d/%d", &a[0], &a[1], &a[2], &b[0], &b[1], &b[2], &c[0], &c[1], &c[2], &d[0], &d[1], &d[2]) == 12) {
			triangles->push_back(int3(a[0]-1, b[0]-1, c[0]-1));
			triangles->push_back(int3(a[2]-1, b[2]-1, c[2]-1));
			triangles->push_back(int3(a[1]-1, b[1]-1, c[1]-1));
			
			triangles->push_back(int3(a[0]-1, c[0]-1, d[0]-1));
			triangles->push_back(int3(a[2]-1, c[2]-1, d[2]-1));
			triangles->push_back(int3(a[1]-1, c[1]-1, d[1]-1));
		}
		else if (sscanf(line.c_str(), "f %d//%d %d//%d %d//%d %d//%d", &a[0], &a[2], &b[0], &b[2], &c[0], &c[2], &d[0], &d[2]) == 8) {
			triangles->push_back(int3(a[0]-1, b[0]-1, c[0]-1));
			triangles->push_back(int3(a[2]-1, b[2]-1, c[2]-1));
			triangles->push_back(int3(-1, -1, -1));
			
			triangles->push_back(int3(a[0]-1, c[0]-1, d[0]-1));
			triangles->push_back(int3(a[2]-1, c[2]-1, d[2]-1));
			triangles->push_back(int3(-1, -1, -1));
		}
		else if (sscanf(line.c_str(), "f %d/%d/%d %d/%d/%d %d/%d/%d", &a[0], &a[1], &a[2], &b[0], &b[1], &b[2], &c[0], &c[1], &c[2]) == 9) {
			triangles->push_back(int3(a[0]-1, b[0]-1, c[0]-1));
			triangles->push_back(int3(a[2]-1, b[2]-1, c[2]-1));
			triangles->push_back(int3(a[1]-1, b[1]-1, c[1]-1));
		}
		else if (sscanf(line.c_str(), "f %d/%d %d/%d %d/%d", &a[0], &a[1], &b[0], &b[1], &c[0], &c[1]) == 6) {
			triangles->push_back(int3(a[0]-1, b[0]-1, c[0]-1));
			triangles->push_back(int3(-1, -1, -1));
			triangles->push_back(int3(a[1]-1, b[1]-1, c[1]-1));
		}
		else if (sscanf(line.c_str(), "f %d//%d %d//%d %d//%d", &a[0], &a[2], &b[0], &b[2], &c[0], &c[2]) == 6) {
			triangles->push_back(int3(a[0]-1, b[0]-1, c[0]-1));
			triangles->push_back(int3(a[2]-1, b[2]-1, c[2]-1));
			triangles->push_back(int3(-1, -1, -1));
		}
	}
	
	size_t triangleCount = 0;
	
	int t0 = -1;

	for (size_t m = 0; m < objectTriangles.size(); ++m) {
		std::vector<int3>& fileTriangles = objectTriangles[m];
		
		for (size_t i = 0; i < fileTriangles.size(); i += 3) {
			int3 p = fileTriangles[i];
			int3 n = fileTriangles[i+1];
			int3 t = fileTriangles[i+2];
			
			if (n.x == -1) {
				float3 a = filePositions[p.y] - filePositions[p.x];
				float3 b = filePositions[p.z] - filePositions[p.x];
				
				float3 n = normalize(cross(a, b));
				
				fileTriangles[i+1] = int3((int)fileNormals.size());
				fileNormals.push_back(n);
			}
			if (t.x == -1) {
				if (t0 < 0) {
					t0 = (int)fileTextures.size();
					fileTextures.push_back(0.0f);
				}
				fileTriangles[i+2] = int3(t0);
			}
		}
		
		triangleCount += fileTriangles.size()/3;
	}
	
	cout << "merging obj vertices..." << flush;
	size_t progress = 0;
	size_t currentTriangle = 0;
	
	std::map<LinearTexture*, unsigned> textureToIndexMap;

	for (size_t m = 0; m < objectTriangles.size(); ++m) {
		std::vector<int3>& fileTriangles = objectTriangles[m];
		
		if (fileTriangles.empty())
			continue;
		
		unordered_map<unsigned long long, unsigned> filePairIndexMap;
		
		vector<Vertex> vertices;
		vector<VertexAttributes> attributes;
		vector<unsigned> indices;
		
		float3 bbMin(1e8f);
		float3 bbMax(-1e8f);
		
		for (size_t i = 0; i < fileTriangles.size(); i += 3) {
			int3 p = fileTriangles[i];
			int3 n = fileTriangles[i+1];
			int3 t = fileTriangles[i+2];
			int3 o;
			
			for (size_t j = 0; j < 3; ++j) {
				unsigned long long pair = (unsigned long long)(&p.x)[j] << 32 | (&n.x)[j];
				unordered_map<unsigned long long, unsigned>::iterator k = filePairIndexMap.find(pair);
				
				if (k == filePairIndexMap.end()) {
					(&o.x)[j] = (unsigned)vertices.size();
					filePairIndexMap[pair] = (unsigned)vertices.size();
					
					size_t pi = (&p.x)[j];
					size_t ni = (&n.x)[j];
					size_t ti = (&t.x)[j];
					
					if (pi >= filePositions.size() || ni >= fileNormals.size() || ti >= fileTextures.size())
						throw runtime_error((string("corrupt obj-file ") + filename).c_str());
					
					Vertex v;
					VertexAttributes va;
					
					v.p = filePositions[pi];
					v.dummy = 1.0f;
					
					bbMin = min(bbMin, v.p);
					bbMax = max(bbMax, v.p);
					
					va.p = v.p;
					va.n = fileNormals[ni];
					va.uv = fileTextures[ti];
					
					vertices.push_back(v);
					attributes.push_back(va);
				}
				else {
					(&o.x)[j] = k->second;
				}
			}
			
			indices.push_back(o.x);
			indices.push_back(o.y);
			indices.push_back(o.z);
			
			size_t np = 10*(++currentTriangle)/triangleCount;
			
			while (np != progress) {
				++progress;
				cout << " " << progress << flush;
			}
		}
		
		DrawCall d;
		
		d.bbMin = bbMin;
		d.bbMax = bbMax;

		d.material = objectMaterials[m];
		
		if (textureMap.count(objectMaterials[m])) {
			LinearTexture* t = textureMap[objectMaterials[m]];
			
			if (textureToIndexMap.count(t)) {
				d.texture = textureToIndexMap[t];
			}
			else {
				d.texture = (unsigned)textures.size();
				textureToIndexMap[t] = d.texture;
				textures.push_back(t);
			}
		}
		else {
			d.texture = noTexture;
		}
		
		d.vertexCount = (unsigned)vertices.size();
		d.indexCount = (unsigned)indices.size();
		
		d.vertices = static_cast<Vertex*>(simd_malloc(align(sizeof(Vertex)*d.vertexCount, 64), 64));
		d.attributes = static_cast<VertexAttributes*>(simd_malloc(align(sizeof(VertexAttributes)*d.vertexCount, 64), 64));
		d.indices = static_cast<unsigned*>(simd_malloc(align(sizeof(unsigned)*d.indexCount, 64), 64));
		
		copy(vertices.begin(), vertices.end(), d.vertices);
		copy(attributes.begin(), attributes.end(), d.attributes);
		copy(indices.begin(), indices.end(), d.indices);
		
		drawCalls.push_back(d);
	}
	
	for (map<std::string, LinearTexture*>::iterator i = textureMap.begin(); i != textureMap.end(); ++i) {
		if (!textureToIndexMap.count(i->second))
			delete i->second;
	}
	
	for (size_t i = 0; i < drawCalls.size(); ++i)
		sortedDrawCalls.push_back(&drawCalls[i]);
	
	cout << " ...done" << endl;
	cout << triangleCount << " triangles in " << drawCalls.size() << " draw calls" << endl;
}

struct DrawCallSorter {
	LinearTexture** textures;
	float3 viewDir;
	
	bool operator () (Mesh::DrawCall* a, Mesh::DrawCall* b) const {
		unsigned alphaA = a->texture != Mesh::noTexture && textures[a->texture]->hasAlpha() ? 1 : 0;
		unsigned alphaB = b->texture != Mesh::noTexture && textures[b->texture]->hasAlpha() ? 1 : 0;

		if (alphaA < alphaB)
			return true;
		else if (alphaA > alphaB)
			return false;

		return dot(point(a) - point(b), viewDir) < 0.0f;
	}
	
	float3 point(Mesh::DrawCall* d) const {
		return float3(viewDir.x < 0.0f ? d->bbMin.x : d->bbMax.x,
					  viewDir.y < 0.0f ? d->bbMin.y : d->bbMax.y,
					  viewDir.z < 0.0f ? d->bbMin.z : d->bbMax.z);
	}
};

void Mesh::sortDrawCalls(const float4x4& modelViewMatrix) {
	float3 viewDir(modelViewMatrix.c0.z, modelViewMatrix.c1.z, modelViewMatrix.c2.z);
	viewDir = normalize(viewDir);
	
	DrawCallSorter sorter = { textures.size() ? &textures[0] : 0, viewDir };
	std::sort(sortedDrawCalls.begin(), sortedDrawCalls.end(), sorter);
}

void Mesh::eraseDrawCall(unsigned index) {
	simd_free(drawCalls[index].vertices);
	simd_free(drawCalls[index].attributes);
	simd_free(drawCalls[index].indices);
	drawCalls.erase(drawCalls.begin() + index);

	sortedDrawCalls.resize(0);
	for (size_t i = 0; i < drawCalls.size(); ++i)
		sortedDrawCalls.push_back(&drawCalls[i]);
}

Mesh::~Mesh() {
	for (size_t i = 0; i < drawCalls.size(); ++i) {
		simd_free(drawCalls[i].vertices);
		simd_free(drawCalls[i].attributes);
		simd_free(drawCalls[i].indices);
	}
	
	for (size_t i = 0; i < textures.size(); ++i)
		delete textures[i];
}

}
