//
//  ThreadPool.h
//  SimdRast
//
//  Created by Rasmus Barringer on 2012-10-24.
//  Copyright (c) 2012 Rasmus Barringer. All rights reserved.
//

#ifndef SimdRast_ThreadPool_h
#define SimdRast_ThreadPool_h

#include "Thread.h"
#include "Atomics.h"
#include "SimdMath.h"
#include <vector>

namespace srast {

class ThreadPoolTask {
public:
	virtual void run(unsigned start, unsigned end, unsigned thread) = 0;
	virtual void finished() {}
};

class ThreadPool {
private:
	struct TaskInfo {
		int workersLeft;
		unsigned workItemStart;
		unsigned workItemEnd;
		unsigned taskSize;
		unsigned workItemGranularity;
		ThreadPoolTask* task;
	};
	
	class Worker : public Thread {
	private:
		unsigned thread;
		ThreadPool& pool;
		unsigned pad0[16];
		int nextWorkItem;
		bool isReset;
		bool isPaused;
		unsigned pad1[16];

	public:
		Worker(ThreadPool& pool, unsigned thread) : pool(pool), thread(thread) {
			isReset = false;
			isPaused = false;
			nextWorkItem = 0;
		}
		
		bool hasWorkLeft() const {
			return !isPaused && (unsigned)nextWorkItem < pool.taskCount;
		}
		
		void reset() {
			isReset = true;
			nextWorkItem = 0;
		}
		
		void pause() {
			isPaused = true;
		}
		
		void resume() {
			isPaused = false;
		}
		
	protected:
		virtual void* run() {
			_MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON); // Disable denormal handling for performance.
			
			int* currentWorkItem = &pool.currentWorkItem;
			TaskInfo currentTask = { 0 };
			
			unsigned lockFreeTaskCount = 0;
			unsigned lockFreeNextWorkItem = 0;

			for (;;) {
				unsigned start = Atomics::increment(currentWorkItem)-1;

				if (start >= currentTask.workItemEnd) {
					if (lockFreeNextWorkItem < lockFreeTaskCount) {
						while (lockFreeNextWorkItem < lockFreeTaskCount && start >= pool.taskQueue[lockFreeNextWorkItem].workItemEnd) {
							if (Atomics::decrement(&pool.taskQueue[lockFreeNextWorkItem].workersLeft) == 0)
								pool.taskQueue[lockFreeNextWorkItem].task->finished();

							++lockFreeNextWorkItem;
						}
						
						if (lockFreeNextWorkItem < lockFreeTaskCount) {
							currentTask = pool.taskQueue[lockFreeNextWorkItem];
							goto skip_lock;
						}
					}

					pool.taskMutex.enter();
					
					nextWorkItem = lockFreeNextWorkItem;
					bool notify = true;
					
					for (;;) {
						if (pool.exit) {
							pool.taskMutex.exit();
							return 0;
						}
						
						if (!isPaused) {
							if (isReset) {
								isReset = false;
								start = Atomics::increment(currentWorkItem)-1;
							}
							
							if (notify || (unsigned)nextWorkItem < pool.taskCount) {
								while ((unsigned)nextWorkItem < pool.taskCount && start >= pool.taskQueue[nextWorkItem].workItemEnd) {
									if (Atomics::decrement(&pool.taskQueue[nextWorkItem].workersLeft) == 0) {
										pool.taskMutex.exit();
										pool.taskQueue[nextWorkItem].task->finished();
										pool.taskMutex.enter();
									}
									++nextWorkItem;
								}
								
								if ((unsigned)nextWorkItem < pool.taskCount) {
									currentTask = pool.taskQueue[nextWorkItem];
									break;
								}
								
								pool.taskMutex.notifyAll();
								notify = false;
							}
						}
						
						pool.taskMutex.wait();
					}
					
					lockFreeTaskCount = pool.taskCount;
					lockFreeNextWorkItem = nextWorkItem;
					
					pool.taskMutex.exit();
				}
			skip_lock:
				
				start = (start - currentTask.workItemStart) * currentTask.workItemGranularity;
				unsigned end = start + currentTask.workItemGranularity;
				
				if (end > currentTask.taskSize)
					end = currentTask.taskSize;

				currentTask.task->run(start, end, thread);
			}
			return 0;
		}
	};
	
	std::vector<Worker*> threads;
	
	bool exit;
	Mutex taskMutex;

	unsigned pad0[16];
	int currentWorkItem;
	unsigned pad1[16];
	
	static const unsigned maxTaskCount = 64*1024;
	
	unsigned addedWorkItems;
	TaskInfo* taskQueue;
	unsigned taskCount;

public:
	ThreadPool() {
		exit = false;
		currentWorkItem = 0;
		addedWorkItems = 0;
		
		taskQueue = static_cast<TaskInfo*>(simd_malloc(sizeof(TaskInfo)*maxTaskCount, 64));
		taskCount = 0;

		threads.resize(cpuCount(), 0);

		for (size_t i = 0; i < threads.size(); ++i) {
			threads[i] = new Worker(*this, (unsigned)i);
			threads[i]->start();
		}
	}
	
	static unsigned cpuCount() {
#ifdef _WIN32
		SYSTEM_INFO info;
		GetSystemInfo(&info);
		return info.dwNumberOfProcessors;
#else
		static unsigned cpuCount = 0;
		
		if (!cpuCount)
			cpuCount = (unsigned)sysconf(_SC_NPROCESSORS_ONLN);
		
		return cpuCount;
#endif
	}
	
	unsigned getThreadCount() const {
		return (unsigned)threads.size();
	}

	void startTask(ThreadPoolTask* t, unsigned count, unsigned granularity, bool finishedCallback = false) {
		unsigned workItemCount = (count + granularity - 1) / granularity;
		
		taskMutex.enter();
		
		if (taskCount == maxTaskCount)
			throw std::runtime_error("too many tasks");

		unsigned workItemStart = addedWorkItems;
		unsigned workItemEnd = workItemStart + workItemCount;
		addedWorkItems = workItemEnd;
		
		TaskInfo task = {
			finishedCallback ? (int)threads.size() : 0,
			workItemStart,
			workItemEnd,
			count,
			granularity,
			t,
		};
		
		taskQueue[taskCount++] = task;
		taskMutex.notifyAll();
		
		taskMutex.exit();
	}
	
	void barrier() {
		taskMutex.enter();
		waitAndReset();
		taskMutex.exit();
	}
	
	void singleThreaded() {
		taskMutex.enter();
		
		waitAndReset();
		
		for (size_t i = 1; i < threads.size(); ++i)
			threads[i]->pause();
		
		taskMutex.exit();
	}
	
	void multiThreaded() {
		taskMutex.enter();
		
		waitAndReset();
		
		for (size_t i = 1; i < threads.size(); ++i)
			threads[i]->resume();
		
		taskMutex.exit();
	}
	
	~ThreadPool() {
		taskMutex.enter();
		exit = true;
		taskMutex.notifyAll();
		taskMutex.exit();
		
		for (size_t i = 0; i < threads.size(); ++i)
			threads[i]->join();
		
		for (size_t i = 0; i < threads.size(); ++i)
			delete threads[i];
		
		simd_free(taskQueue);
	}
	
private:
	void waitAndReset() {
		while (hasWorkLeft())
			taskMutex.wait();
		
		addedWorkItems = 0;
		currentWorkItem = 0;
		taskCount = 0;
		
		for (size_t i = 0; i < threads.size(); ++i)
			threads[i]->reset();
	}
	
	bool hasWorkLeft() const {
		for (size_t i = 0; i < threads.size(); ++i) {
			if (threads[i]->hasWorkLeft())
				return true;
		}
		return false;
	}
};

}

#endif
