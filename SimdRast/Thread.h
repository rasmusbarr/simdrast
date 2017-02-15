//
//  Thread.h
//  SimdRast
//
//  Created by Rasmus Barringer on 2012-10-24.
//  Copyright (c) 2012 Rasmus Barringer. All rights reserved.
//

#ifndef SimdRast_Thread_h
#define SimdRast_Thread_h

#include "Config.h"
#include <stdexcept>

#ifdef _WIN32
#include <windows.h>
#else
#include <pthread.h>
#include <unistd.h>
#endif

namespace srast {

class Thread {
private:
#ifdef _WIN32
	HANDLE thread;
	void* result;
#else
	pthread_t thread;
#endif
	
public:
	Thread() {
		thread = 0;
	}
	
	void start() {
		if (thread)
			throw std::runtime_error("thread cannot start: already started");
		
#ifdef _WIN32
		DWORD id;
		thread = CreateThread(0, 0, entry, this, 0, &id);
#else
		pthread_create(&thread, 0, entry, this);
#endif
	}
	
	void sleep(unsigned millis) {
#ifdef _WIN32
		Sleep(millis);
#else
		usleep(millis * 1000ull);
#endif
	}
	
	void* join() {
		if (!thread)
			throw std::runtime_error("thread cannot be joined: not started");
		
#ifdef _WIN32
		WaitForSingleObject(thread, INFINITE);
		CloseHandle(thread);
#else
		void* result;
		pthread_join(thread, &result);
#endif
		thread = 0;
		return result;
	}
	
	virtual ~Thread() {
		if (thread)
			throw std::runtime_error("thread not joined before destruction");
	}
	
protected:
	virtual void* run() {
		return 0;
	}
	
private:
#ifdef _WIN32
	static DWORD WINAPI entry(LPVOID ptr) {
		static_cast<Thread*>(ptr)->result = static_cast<Thread*>(ptr)->run();
		return 0;
	}
#else
	static void* entry(void* ptr) {
		return static_cast<Thread*>(ptr)->run();
	}
#endif
};

class Mutex {
private:
#ifdef _WIN32
	CRITICAL_SECTION m;
	CONDITION_VARIABLE c;
#else
	pthread_mutex_t m;
	pthread_cond_t c;
#endif
	
public:
	Mutex() {
#ifdef _WIN32
		InitializeCriticalSection(&m);
		InitializeConditionVariable(&c);
#else
		pthread_mutex_init(&m, 0);
		pthread_cond_init(&c, 0);
#endif
	}
	
	void enter() {
#ifdef _WIN32
		EnterCriticalSection(&m);
#else
		pthread_mutex_lock(&m);
#endif
	}
	
	void wait() {
#ifdef _WIN32
		SleepConditionVariableCS(&c, &m, INFINITE);
#else
		pthread_cond_wait(&c, &m);
#endif
	}
	
	void notify() {
#ifdef _WIN32
		WakeConditionVariable(&c);
#else
		pthread_cond_signal(&c);
#endif
	}
	
	void notifyAll() {
#ifdef _WIN32
		WakeAllConditionVariable(&c);
#else
		pthread_cond_broadcast(&c);
#endif
	}
	
	void exit() {
#ifdef _WIN32
		LeaveCriticalSection(&m);
#else
		pthread_mutex_unlock(&m);
#endif
	}
	
	~Mutex() {
#ifdef _WIN32
		DeleteCriticalSection(&m);
#else
		pthread_mutex_destroy(&m);
		pthread_cond_destroy(&c);
#endif
	}
};

}

#endif
