//
//  VectorMath.h
//  SimdRast
//
//  Created by Rasmus Barringer on 2012-10-21.
//  Copyright (c) 2012 Rasmus Barringer. All rights reserved.
//

#ifndef SimdRast_VectorMath_h
#define SimdRast_VectorMath_h

#include "Config.h"
#include <cmath>
#include <immintrin.h>
#include <smmintrin.h>

namespace srast {

// Note: These x_as_y conversions rely on union aliasing and are technically not portable.
inline unsigned float_as_uint32(float x) {
	union { float a; unsigned b; } u;
	u.a = x;
	return u.b;
}

inline float uint32_as_float(unsigned x) {
	union { unsigned a; float b; } u;
	u.a = x;
	return u.b;
}

inline unsigned long long double_as_uint64(double x) {
	union { double a; unsigned long long b; } u;
	u.a = x;
	return u.b;
}

inline double uint64_as_double(unsigned long long x) {
	union { unsigned long long a; double b; } u;
	u.a = x;
	return u.b;
}

inline int min(int a, int b) {
	return a < b ? a : b;
}

inline int max(int a, int b) {
	return a > b ? a : b;
}

inline float min(float a, float b) {
	return _mm_cvtss_f32(_mm_min_ss(_mm_set_ss(a), _mm_set_ss(b)));
}

inline float max(float a, float b) {
	return _mm_cvtss_f32(_mm_max_ss(_mm_set_ss(a), _mm_set_ss(b)));
}

inline float rsqrt(float x) {
	return _mm_cvtss_f32(_mm_rsqrt_ss( _mm_set_ss(x)));
}

inline float recip(float x) {
	return _mm_cvtss_f32(_mm_rcp_ss( _mm_set_ss(x)));
}

inline float fast_floor(float x) {
	__m128 m = _mm_set_ss(x);
	return _mm_cvtss_f32(_mm_floor_ss(m, m));
}

template<class T>
struct vec2 {
	T x, y;
	
	vec2<T>() {}
	
	vec2<T>(const T& x) : x(x), y(x) {}
	
	vec2<T>(const T& x, const T& y) : x(x), y(y) {}
	
	T operator [] (unsigned i) const {
		return (&x)[i];
	}
	
	T& operator [] (unsigned i) {
		return (&x)[i];
	}
	
	vec2<T> operator + () const {
		return vec2<T>(x, y);
	}
	
	vec2<T> operator - () const {
		return vec2<T>(-x, -y);
	}
	
	vec2<T> operator + (const vec2<T>& rhs) const {
		return vec2<T>(x + rhs.x, y + rhs.y);
	}
	
	vec2<T> operator - (const vec2<T>& rhs) const {
		return vec2<T>(x - rhs.x, y - rhs.y);
	}
	
	vec2<T> operator * (const T& rhs) const {
		return vec2<T>(x * rhs, y * rhs);
	}
	
	T length2() const {
		return x*x + y*y;
	}
	
	T length() const {
		return sqrt(length2());
	}
	
	template<class E>
	vec2<E> to() const {
		return vec2<E>(x, y);
	}
};

template<class T>
struct vec3 {
	T x, y, z;
	
	vec3<T>() {}
	
	vec3<T>(const T& x) : x(x), y(x), z(x) {}
	
	vec3<T>(const T& x, const T& y, const T& z) : x(x), y(y), z(z) {}
	
	T operator [] (unsigned i) const {
		return (&x)[i];
	}
	
	T& operator [] (unsigned i) {
		return (&x)[i];
	}
	
	vec3<T> operator + () const {
		return vec3<T>(x, y, z);
	}
	
	vec3<T> operator - () const {
		return vec3<T>(-x, -y, -z);
	}
	
	vec3<T> operator + (const vec3<T>& rhs) const {
		return vec3<T>(x + rhs.x, y + rhs.y, z + rhs.z);
	}
	
	vec3<T> operator - (const vec3<T>& rhs) const {
		return vec3<T>(x - rhs.x, y - rhs.y, z - rhs.z);
	}
	
	vec3<T> operator * (const T& rhs) const {
		return vec3<T>(x * rhs, y * rhs, z * rhs);
	}
	
	T length2() const {
		return x*x + y*y + z*z;
	}
	
	T length() const {
		return sqrt(length2());
	}
	
	vec2<T> xy() const {
		return vec2<T>(x, y);
	}
	
	template<class E>
	vec3<E> to() const {
		return vec3<E>(x, y, z);
	}
};

template<class T>
struct vec4 {
	T x, y, z, w;
	
	vec4<T>() {}
	
	vec4<T>(const T& x) : x(x), y(x), z(x), w(x) {}
	
	vec4<T>(const T& x, const T& y, const T& z, const T& w) : x(x), y(y), z(z), w(w) {}
	
	T operator [] (unsigned i) const {
		return (&x)[i];
	}
	
	T& operator [] (unsigned i) {
		return (&x)[i];
	}
	
	vec4<T> operator + () const {
		return vec4<T>(x, y, z, w);
	}
	
	vec4<T> operator - () const {
		return vec4<T>(-x, -y, -z, -w);
	}
	
	vec4<T> operator + (const vec4<T>& rhs) const {
		return vec4<T>(x + rhs.x, y + rhs.y, z + rhs.z, w + rhs.w);
	}
	
	vec4<T> operator - (const vec4<T>& rhs) const {
		return vec4<T>(x - rhs.x, y - rhs.y, z - rhs.z, w - rhs.w);
	}
	
	vec4<T> operator * (const T& rhs) const {
		return vec4<T>(x * rhs, y * rhs, z * rhs, w * rhs);
	}
	
	vec2<T> xy() const {
		return vec2<T>(x, y);
	}
	
	vec3<T> xyz() const {
		return vec3<T>(x, y, z);
	}

	vec3<T> xyw() const {
		return vec3<T>(x, y, w);
	}
	
	template<class E>
	vec4<E> to() const {
		return vec4<E>(x, y, z, w);
	}
};

template<class T>
inline vec2<T> normalize(const vec2<T>& v) {
	return v * rsqrt(v.length2());
}

template<class T>
inline vec3<T> normalize(const vec3<T>& v) {
	return v * rsqrt(v.length2());
}

template<class T>
inline T dot(const vec2<T>& a, const vec2<T>& b) {
	return a.x*b.x + a.y*b.y;
}

template<class T>
inline T dot(const vec3<T>& a, const vec3<T>& b) {
	return a.x*b.x + a.y*b.y + a.z*b.z;
}

template<class T>
inline T dot(const vec4<T>& a, const vec4<T>& b) {
	return a.x*b.x + a.y*b.y + a.z*b.z + a.w*b.w;
}

template<class T>
inline vec3<T> cross(const vec3<T>& a, const vec3<T>& b) {
	return vec3<T>(a.y*b.z - a.z*b.y, a.z*b.x - a.x*b.z, a.x*b.y - a.y*b.x);
}

template<class T>
inline vec2<T> min(const vec2<T>& a, const vec2<T>& b) {
	return vec2<T>(min(a.x, b.x), min(a.y, b.y));
}

template<class T>
inline vec2<T> max(const vec2<T>& a, const vec2<T>& b) {
	return vec2<T>(max(a.x, b.x), max(a.y, b.y));
}

template<class T>
inline vec3<T> min(const vec3<T>& a, const vec3<T>& b) {
	return vec3<T>(min(a.x, b.x), min(a.y, b.y), min(a.z, b.z));
}

template<class T>
inline vec3<T> max(const vec3<T>& a, const vec3<T>& b) {
	return vec3<T>(max(a.x, b.x), max(a.y, b.y), max(a.z, b.z));
}

template<class T>
inline vec4<T> min(const vec4<T>& a, const vec4<T>& b) {
	return vec4<T>(min(a.x, b.x), min(a.y, b.y), min(a.z, b.z), min(a.w, b.w));
}

template<class T>
inline vec4<T> max(const vec4<T>& a, const vec4<T>& b) {
	return vec4<T>(max(a.x, b.x), max(a.y, b.y), max(a.z, b.z), max(a.w, b.w));
}

template<class T>
struct mat4 {
	vec4<T> c0, c1, c2, c3;
	
	mat4<T>() {
	}
	
	mat4<T>(const T* e) {
		c0 = vec4<T>(e[0], e[1], e[2], e[3]);
		c1 = vec4<T>(e[4], e[5], e[6], e[7]);
		c2 = vec4<T>(e[8], e[9], e[10], e[11]);
		c3 = vec4<T>(e[12], e[13], e[14], e[15]);
	}
	
	mat4<T>(const vec4<T>& c0, const vec4<T>& c1, const vec4<T>& c2, const vec4<T>& c3) : c0(c0), c1(c1), c2(c2), c3(c3) {
	}
	
	void loadIdentity() {
		c0 = vec4<T>(1.0f, 0.0f, 0.0f, 0.0f);
		c1 = vec4<T>(0.0f, 1.0f, 0.0f, 0.0f);
		c2 = vec4<T>(0.0f, 0.0f, 1.0f, 0.0f);
		c3 = vec4<T>(0.0f, 0.0f, 0.0f, 1.0f);
	}
	
	void loadZero() {
		c0 = vec4<T>(0.0f, 0.0f, 0.0f, 0.0f);
		c1 = vec4<T>(0.0f, 0.0f, 0.0f, 0.0f);
		c2 = vec4<T>(0.0f, 0.0f, 0.0f, 0.0f);
		c3 = vec4<T>(0.0f, 0.0f, 0.0f, 0.0f);
	}

	static mat4<T> frustumProjection(const T& left, const T& right, const T& bottom, const T& top, const T& znear, const T& zfar) {
		T temp = 2.0f * znear;
		T temp2 = right - left;
		T temp3 = top - bottom;
		T temp4 = zfar - znear;

		return mat4<T>(vec4<T>(temp / temp2, 0.0f, 0.0f, 0.0f),
					   vec4<T>(0.0f, temp / temp3, 0.0f, 0.0f),
					   vec4<T>((right + left) / temp2, (top + bottom) / temp3, (-zfar - znear) / temp4, -1.0f),
					   vec4<T>(0.0f, 0.0f, (-temp * zfar) / temp4, 0.0f));
	}
	
	static mat4<T> perspectiveProjection(const T& fovy, const T& aspectRatio, const T& znear, const T& zfar) {
		float ymax = znear * tanf(fovy);
		float xmax = ymax * aspectRatio;
		return frustumProjection(-xmax, xmax, -ymax, ymax, znear, zfar);
	}

	static mat4<T> orthoProjection(const T& left, const T& right, const T& bottom, const T& top, const T& znear, const T& zfar) {
		return mat4<T>(vec4<T>(2.0f/(right-left), 0.0f, 0.0f, 0.0f),
					   vec4<T>(0.0f, 2.0f/(top-bottom), 0.0f, 0.0f),
					   vec4<T>(0.0f, 0.0f, 2.0f/(znear-zfar), 0.0f),
					   vec4<T>((left+right)/(left-right), (bottom+top)/(bottom-top), (znear+zfar)/(znear-zfar), 1.0f));
	}

	static mat4<T> orthoProjection(const T& w, const T& h, const T& znear, const T& zfar) {
		return orthoProjection(-w, w, -h, h, znear, zfar);
	}
	
	static mat4<T> lookAt(const vec3<T>& origin, const vec3<T>& target, const vec3<T>& up) {
		vec3<T> zAxis = normalize(origin - target);
		vec3<T> xAxis = normalize(cross(up, zAxis));
		vec3<T> yAxis = cross(zAxis, xAxis);
		
		return mat4<T>(vec4<T>(xAxis.x, yAxis.x, zAxis.x, 0.0f),
					   vec4<T>(xAxis.y, yAxis.y, zAxis.y, 0.0f),
					   vec4<T>(xAxis.z, yAxis.z, zAxis.z, 0.0f),
					   vec4<T>(-dot(xAxis, origin), -dot(yAxis, origin), -dot(zAxis, origin), 1.0f));
	}
	
	static mat4<T> viewport(const T& w, const T& h) {
		return mat4<T>(vec4<T>(w*0.5f, 0.0f, 0.0f, 0.0f),
					   vec4<T>(0.0f, h*0.5f, 0.0f, 0.0f),
					   vec4<T>(0.0f, 0.0f, 1.0f, 0.0f),
					   vec4<T>(w*0.5f, h*0.5f, 0.0f, 1.0f));
	}

	static mat4<T> rotationX(const T& angle) {
		return mat4<T>(vec4<T>(1.0f, 0.0f, 0.0f, 0.0f),
					   vec4<T>(0.0f, cosf(angle), sinf(angle), 0.0f),
					   vec4<T>(0.0f, -sinf(angle), cosf(angle), 0.0f),
					   vec4<T>(0.0f, 0.0f, 0.0f, 1.0f));
	}
	
	static mat4<T> rotationY(const T& angle) {
		return mat4<T>(vec4<T>(cosf(angle), 0.0f, -sinf(angle), 0.0f),
					   vec4<T>(0.0f,        1.0f, 0.0f, 0.0f),
					   vec4<T>(sinf(angle), 0.0f, cosf(angle), 0.0f),
					   vec4<T>(0.0f, 0.0f, 0.0f, 1.0f));
	}
	
	static mat4<T> rotationZ(const T& angle) {
		return mat4<T>(vec4<T>(cosf(angle), sinf(angle), 0.0f, 0.0f),
					   vec4<T>(-sinf(angle), cosf(angle), 0.0f, 0.0f),
					   vec4<T>(0.0f, 0.0f, 1.0f, 0.0f),
					   vec4<T>(0.0f, 0.0f, 0.0f, 1.0f));
	}
	
	static mat4<T> translation(const T& x, const T& y, const T& z) {
		return mat4<T>(vec4<T>(1.0f, 0.0f, 0.0f, 0.0f),
					   vec4<T>(0.0f, 1.0f, 0.0f, 0.0f),
					   vec4<T>(0.0f, 0.0f, 1.0f, 0.0f),
					   vec4<T>(x, y, z, 1.0f));
	}
	
	operator T* () {
		return &c0.x;
	}
	
	operator const T* () const {
		return &c0.x;
	}
	
	vec4<T> operator * (const vec4<T>& rhs) const {
		return c0*rhs.x + c1*rhs.y + c2*rhs.z + c3*rhs.w;
	}
	
	vec4<T> operator * (const vec3<T>& rhs) const {
		return c0*rhs.x + c1*rhs.y + c2*rhs.z + c3;
	}
	
	mat4<T> operator * (const mat4<T>& rhs) const {
		return mat4<T>(c0*rhs.c0.x + c1*rhs.c0.y + c2*rhs.c0.z + c3*rhs.c0.w,
					   c0*rhs.c1.x + c1*rhs.c1.y + c2*rhs.c1.z + c3*rhs.c1.w,
					   c0*rhs.c2.x + c1*rhs.c2.y + c2*rhs.c2.z + c3*rhs.c2.w,
					   c0*rhs.c3.x + c1*rhs.c3.y + c2*rhs.c3.z + c3*rhs.c3.w);
	}
	
	mat4<T> transpose() const {
		return mat4<T>(vec4<T>(c0.x, c1.x, c2.x, c3.x),
					   vec4<T>(c0.y, c1.y, c2.y, c3.y),
					   vec4<T>(c0.z, c1.z, c2.z, c3.z),
					   vec4<T>(c0.w, c1.w, c2.w, c3.w));
	}
	
	mat4<T> inverse() const {
		const T* s = *this;
		T t[12];
		t[0] = s[10] * s[15];
		t[1] = s[14] * s[11];
		t[2] = s[6] * s[15];
		t[3] = s[14] * s[7];
		t[4] = s[6] * s[11];
		t[5] = s[10] * s[7];
		t[6] = s[2] * s[15];
		t[7] = s[14] * s[3];
		t[8] = s[2] * s[11];
		t[9] = s[10] * s[3];
		t[10] = s[2] * s[7];
		t[11] = s[6] * s[3];
		
		mat4<T> dst;
		T* d = dst;
		d[0] = t[0]*s[5] + t[3]*s[9] + t[4]*s[13];
		d[0] -= t[1]*s[5] + t[2]*s[9] + t[5]*s[13];
		d[1] = t[1]*s[1] + t[6]*s[9] + t[9]*s[13];
		d[1] -= t[0]*s[1] + t[7]*s[9] + t[8]*s[13];
		d[2] = t[2]*s[1] + t[7]*s[5] + t[10]*s[13];
		d[2] -= t[3]*s[1] + t[6]*s[5] + t[11]*s[13];
		d[3] = t[5]*s[1] + t[8]*s[5] + t[11]*s[9];
		d[3] -= t[4]*s[1] + t[9]*s[5] + t[10]*s[9];
		d[4] = t[1]*s[4] + t[2]*s[8] + t[5]*s[12];
		d[4] -= t[0]*s[4] + t[3]*s[8] + t[4]*s[12];
		d[5] = t[0]*s[0] + t[7]*s[8] + t[8]*s[12];
		d[5] -= t[1]*s[0] + t[6]*s[8] + t[9]*s[12];
		d[6] = t[3]*s[0] + t[6]*s[4] + t[11]*s[12];
		d[6] -= t[2]*s[0] + t[7]*s[4] + t[10]*s[12];
		d[7] = t[4]*s[0] + t[9]*s[4] + t[10]*s[8];
		d[7] -= t[5]*s[0] + t[8]*s[4] + t[11]*s[8];
		
		t[0] = s[8]*s[13];
		t[1] = s[12]*s[9];
		t[2] = s[4]*s[13];
		t[3] = s[12]*s[5];
		t[4] = s[4]*s[9];
		t[5] = s[8]*s[5];
		t[6] = s[0]*s[13];
		t[7] = s[12]*s[1];
		t[8] = s[0]*s[9];
		t[9] = s[8]*s[1];
		t[10] = s[0]*s[5];
		t[11] = s[4]*s[1];
		
		d[8] = t[0]*s[7] + t[3]*s[11] + t[4]*s[15];
		d[8]-= t[1]*s[7] + t[2]*s[11] + t[5]*s[15];
		d[9] = t[1]*s[3] + t[6]*s[11] + t[9]*s[15];
		d[9] -= t[0]*s[3] + t[7]*s[11] + t[8]*s[15];
		d[10] = t[2]*s[3] + t[7]*s[7] + t[10]*s[15];
		d[10]-= t[3]*s[3] + t[6]*s[7] + t[11]*s[15];
		d[11] = t[5]*s[3] + t[8]*s[7] + t[11]*s[11];
		d[11]-= t[4]*s[3] + t[9]*s[7] + t[10]*s[11];
		d[12] = t[2]*s[10] + t[5]*s[14] + t[1]*s[6];
		d[12]-= t[4]*s[14] + t[0]*s[6] + t[3]*s[10];
		d[13] = t[8]*s[14] + t[0]*s[2] + t[7]*s[10];
		d[13]-= t[6]*s[10] + t[9]*s[14] + t[1]*s[2];
		d[14] = t[6]*s[6] + t[11]*s[14] + t[3]*s[2];
		d[14]-= t[10]*s[14] + t[2]*s[2] + t[7]*s[6];
		d[15] = t[10]*s[10] + t[4]*s[2] + t[9]*s[6];
		d[15]-= t[8]*s[6] + t[11]*s[10] + t[5]*s[2];
		
		T det = s[0]*d[0] + s[4]*d[1] + s[8]*d[2] + s[12]*d[3];
		det = 1.0f/det;
		
		for (unsigned j = 0; j < 16; ++j)
			d[j] *= det;
		
		return dst;
	}
	
	template<class E>
	SRAST_FORCEINLINE mat4<E> to() const {
		return mat4<E>(c0.template to<E>(), c1.template to<E>(), c2.template to<E>(), c3.template to<E>());
	}
};

typedef vec2<float> float2;
typedef vec3<float> float3;
typedef vec4<float> float4;
typedef mat4<float> float4x4;

typedef vec2<int> int2;
typedef vec3<int> int3;
typedef vec4<int> int4;

}

#endif
