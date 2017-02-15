//
//  LambertShader.h
//  SwRenderer
//
//  Created by Rasmus Barringer on 2012-10-21.
//  Copyright (c) 2012 Rasmus Barringer. All rights reserved.
//

#ifndef SwRenderer_LambertShader_h
#define SwRenderer_LambertShader_h

#include "../../SimdRast/SimdMath.h"
#include "../../SimdRast/Shader.h"
#include "../Framework/LinearTexture.h"

class LambertVertexShader : public srast::Shader {
public:
	struct Input {
		srast::float3 p;
		float dummy;
	};

	struct Uniforms {
		srast::float4x4 modelViewProj;
	};
	
	virtual unsigned outputStride() const {
		return sizeof(srast::float4);
	}
	
	virtual void execute(const void* input, void* output, unsigned count, const void* uniforms) {
		shadeVertices(static_cast<const Input*>(input), static_cast<srast::float4*>(output), count, static_cast<const Uniforms*>(uniforms));
	}
	
private:
	void shadeVertices(const Input* __restrict input, srast::float4* __restrict position, unsigned count, const Uniforms* __restrict uniforms) {
		using namespace srast;
		float4x4 modelViewProj = uniforms->modelViewProj;
		
		for (unsigned i = 0; i < count; i += simd_float::width) {
			simd_float3 p;
			simd_float dummy;
			
			simd_float_load_aos4(p.x, p.y, p.z, dummy, reinterpret_cast<const float*>(input + i), sizeof(Input)/sizeof(float));
			
			simd_float4 outp = modelViewProj.to<simd_float>() * p;

			simd_float_stream_aos4(reinterpret_cast<float*>(position + i), 4, outp.x, outp.y, outp.z, outp.w);
		}
	}
};

class LambertAttributeShader : public srast::Shader {
public:
	struct Input {
		srast::float3 p, n;
		srast::float2 uv;
	};
	
	struct Output {
		srast::float2 duvdx;
		srast::float2 duvdy;
		float nDotL;
		srast::float3 uvw;
	};
	
	srast::float3 light;
	
	virtual unsigned outputStride() const {
		return sizeof(Output);
	}
	
	virtual void execute(const void* input, void* output, unsigned count, const void* uniforms) {
		shadeVertices(static_cast<const Input*>(input), static_cast<Output*>(output), count, static_cast<const LambertVertexShader::Uniforms*>(uniforms));
	}
	
private:
	void shadeVertices(const Input* __restrict input, Output* __restrict output, unsigned count, const LambertVertexShader::Uniforms* __restrict uniforms) {
		using namespace srast;

		for (unsigned i = 0; i < count; i += simd_float::width) {
			simd_float3 p, n;
			simd_float2 uv;
			
			simd_float_load_aos8(p.x, p.y, p.z, n.x, n.y, n.z, uv.x, uv.y, reinterpret_cast<const float*>(input + i), 8);

			simd_float nDotL = max(dot(n, light.to<simd_float>()), simd_float::zero()) + 0.25f;

			simd_float_store_aos4(reinterpret_cast<float*>(output + i), 8, uv.x, uv.y, uv.x, uv.y);
			simd_float_store_aos4(reinterpret_cast<float*>(output + i) + 4, 8, nDotL, uv.x, uv.y, 0.0f);
		}
	}
};

class LambertFragmentShader : public srast::Shader {
public:
	struct Input {
		srast::float2 duvdx;
		srast::float2 duvdy;
		float nDotL;
		srast::float3 uvw;
	};

	struct Uniforms {
		fx::LinearTexture* diffuseTexture;
	};
	
	virtual unsigned outputStride() const {
		return sizeof(unsigned);
	}

	virtual void execute(const void* input, void* output, unsigned count, const void* uniforms) {
		shadeFragments(static_cast<const Input*>(input), static_cast<unsigned*>(output), count, static_cast<const Uniforms*>(uniforms));
	}
	
private:
	void shadeFragments(const Input* __restrict input, unsigned* __restrict output, unsigned count, const Uniforms* __restrict uniforms) {
		using namespace srast;
		
		for (unsigned i = 0; i < count; i += simd_float::width) {
			unsigned laneMask = (1 << simd_float::width) - 1;
			
			if (i + simd_float::width > count)
				laneMask = (1 << (count-i)) - 1;
			
			simd_float2 duvdx;
			simd_float2 duvdy;
			simd_float nDotL;
			simd_float3 uvw;
			simd_float_load_aos4(duvdx.x, duvdx.y, duvdy.x, duvdy.y, reinterpret_cast<const float*>(input + i), 8);
			simd_float_load_aos4(nDotL, uvw.x, uvw.y, uvw.z, reinterpret_cast<const float*>(input + i) + 4, 8);
			
			simd_float4 c(nDotL, nDotL, nDotL, 1.0f);
			
			if (uniforms->diffuseTexture) {
				simd_float lod = uniforms->diffuseTexture->getSampler<RepeatTextureAddressMode>().computeLod(duvdx, duvdy);
				simd_float4 t = uniforms->diffuseTexture->getSampler<RepeatTextureAddressMode>().sample(uvw.x, uvw.y, lod, laneMask);
				c.x *= t.x;
				c.y *= t.y;
				c.z *= t.z;
				c.w = t.w;
			}
			
			simd_float_store_rgba(c.x, c.y, c.z, c.w, output + i);
		}
	}
};

#endif
