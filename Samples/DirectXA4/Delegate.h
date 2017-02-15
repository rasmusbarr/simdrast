//
//  Delegate.h
//  DirectXA4
//
//  Created by Rasmus Barringer on 2013-06-07.
//  Copyright (c) 2013 Rasmus Barringer. All rights reserved.
//

#ifndef DirectXA4_Delegate_h
#define DirectXA4_Delegate_h

struct Delegate_NoArg {};

template<class A = Delegate_NoArg>
class Delegate {
private:
	void* ptr;
	void (*callback)(void*, A);

public:
	Delegate() {
		callback = 0;
	}

	template<class T>
	static Delegate fromFunctor(T& t) {
		Delegate d;
		d.ptr = &t;
		d.callback = functorStub<T>;
		return d;
	}

	template<class T, void (T::*m)(A)>
	static Delegate fromMember(T& t) {
		Delegate d;
		d.ptr = &t;
		d.callback = memberStub<T, m>;
		return d;
	}

	void operator () (A a) const {
		if (callback)
			callback(ptr, a);
	}

private:
	template<class T>
	static void functorStub(void* ptr, A a) {
		(*reinterpret_cast<T*>(ptr))(a);
	}

	template<class T, void (T::*m)(A)>
	static void memberStub(void* ptr, A a) {
		(reinterpret_cast<T*>(ptr)->*m)(a);
	}
};

template<>
class Delegate<Delegate_NoArg> {
private:
	void* ptr;
	void (*callback)(void*);

public:
	Delegate() {
		callback = 0;
	}

	template<class T>
	static Delegate fromFunctor(T& t) {
		Delegate d;
		d.ptr = &t;
		d.callback = functorStub<T>;
		return d;
	}

	template<class T, void (T::*m)()>
	static Delegate fromMember(T& t) {
		Delegate d;
		d.ptr = &t;
		d.callback = memberStub<T, m>;
		return d;
	}

	void operator () () const {
		if (callback)
			callback(ptr);
	}

private:
	template<class T>
	static void functorStub(void* ptr) {
		(*reinterpret_cast<T*>(ptr))();
	}

	template<class T, void (T::*m)()>
	static void memberStub(void* ptr) {
		(reinterpret_cast<T*>(ptr)->*m)();
	}
};

#endif
