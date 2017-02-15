//
//  SceneShader.h
//  DirectXA4
//
//  Created by Rasmus Barringer on 2013-06-07.
//  Copyright (c) 2013 Rasmus Barringer. All rights reserved.
//

#ifndef DirectXA4_SceneShader_h
#define DirectXA4_SceneShader_h

#include "DxContext.h"
#include "DraTexture.h"

class GenericVertexShader : public srast::Shader {
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

class SceneAttributeShader : public srast::Shader {
public:
	struct Input {
		srast::float3 p, n;
		srast::float2 uv;
	};
	
	struct Output {
		srast::float2 duvdx;
		srast::float2 duvdy;
		srast::float2 uv;
		srast::float3 n;
		srast::float3 p;
	};

	virtual unsigned outputStride() const {
		return sizeof(Output);
	}
	
	virtual void execute(const void* input, void* output, unsigned count, const void* uniforms) {
		shadeVertices(static_cast<const Input*>(input), static_cast<Output*>(output), count);
	}
	
private:
	void shadeVertices(const Input* __restrict input, Output* __restrict output, unsigned count) {
		using namespace srast;

		for (unsigned i = 0; i < count; i += simd_float::width) {
			simd_float3 p, n;
			simd_float2 uv;
			
			simd_float_load_aos8(p.x, p.y, p.z, n.x, n.y, n.z, uv.x, uv.y, reinterpret_cast<const float*>(input + i), 8);

			simd_float_store_aos4(reinterpret_cast<float*>(output + i), 12, uv.x, uv.y, uv.x, uv.y);
			simd_float_store_aos4(reinterpret_cast<float*>(output + i) + 4, 12, uv.x, uv.y, n.x, n.y);
			simd_float_store_aos4(reinterpret_cast<float*>(output + i) + 8, 12, n.z, p.x, p.y, p.z);
		}
	}
};

class SceneFragmentShader : public srast::Shader {
public:
	struct Input {
		srast::float2 duvdx;
		srast::float2 duvdy;
		srast::float2 uv;
		srast::float3 n;
		srast::float3 p;
	};

	struct Uniforms {
		fx::LinearTexture* diffuseTexture;
		fx::LinearTexture* specularTexture;
		DraTexture* shadowMapTexture[4];
		srast::float3 view;
		srast::float3 light[4];
		srast::float4x4 shadowTransform[4];
		unsigned shadowCount;
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

		float3 view = uniforms->view;

		for (unsigned i = 0; i < count; i += simd_float::width) {
			unsigned laneMask = (1 << simd_float::width) - 1;
			
			if (i + simd_float::width > count)
				laneMask = (1 << (count-i)) - 1;
			
			simd_float2 duvdx;
			simd_float2 duvdy;
			simd_float2 uv;
			simd_float3 p, n;
			simd_float_load_aos4(duvdx.x, duvdx.y, duvdy.x, duvdy.y, reinterpret_cast<const float*>(input + i), 12);
			simd_float_load_aos4(uv.x, uv.y, n.x, n.y, reinterpret_cast<const float*>(input + i) + 4, 12);
			simd_float_load_aos4(n.z, p.x, p.y, p.z, reinterpret_cast<const float*>(input + i) + 8, 12);
			
			simd_float3 viewDir = normalize(view.to<simd_float>() - p);
			simd_float radiance = 0.0f;
			n = normalize(n);

			for (unsigned j = 0; j < uniforms->shadowCount; ++j) {
				float3 light = uniforms->light[j];
				float4x4 shadowTransform = uniforms->shadowTransform[j];

				simd_float nDotL = max(dot(n, light.to<simd_float>()), simd_float::zero());

				simd_float3 h = normalize(viewDir + light.to<simd_float>());
				simd_float specLighting = pow(max(dot(h, n), simd_float::zero()), 32.0f)*0.5f;
				nDotL += specLighting;

				simd_float4 shadowPos = shadowTransform.to<simd_float>() * p;
				shadowPos.z = (shadowPos.z + shadowPos.w)*0.5f;

				TextureSampler<DraTexture, TextureFormatR32F, ClampTextureAddressMode, GreaterTextureModifier> sampler = uniforms->shadowMapTexture[j]->getModifiedSampler<TextureFormatR32F, ClampTextureAddressMode, GreaterTextureModifier>();

				simd_float2 coord = simd_float2(shadowPos.x, shadowPos.y)*0.5f + simd_float(0.5f);
				coord.y = simd_float(1.0f)-coord.y;

				simd_float ref = shadowPos.z - 1e-2f;
				ref.store(sampler.modifier.ref);

				simd_float r = sampler.sampleMip(coord.x, coord.y, 0, laneMask);
				nDotL *= (r + 1.0f)*0.5f;
				radiance += nDotL;
			}

			radiance *= 1.0f/uniforms->shadowCount;
			radiance += 0.25f;
			
			simd_float4 c(radiance, radiance, radiance, 1.0f);

			if (uniforms->diffuseTexture) {
				simd_float lod = uniforms->diffuseTexture->getSampler<RepeatTextureAddressMode>().computeLod(duvdx, duvdy);
				simd_float4 t = uniforms->diffuseTexture->getSampler<RepeatTextureAddressMode>().sample(uv.x, uv.y, lod, laneMask);
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
