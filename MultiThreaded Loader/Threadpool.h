// Threadpool Header sourced & implemented from: https://www.youtube.com/watch?v=eWTGtp3HXiw
#pragma once
#include <iostream>
#include <condition_variable>
#include <functional>
#include <future>
#include <vector>
#include <thread>
#include <mutex>
#include <queue>

using std::cout;
using std::move;
using std::size_t;
using std::thread;
using std::vector;
using std::condition_variable;
using std::mutex;
using std::unique_lock;
using std::queue;

// Variables from main needed for threadpool
extern int xc;
extern int yc;
extern HWND _hwnd;

class ITask
{
public:

	ITask() {}
};

class Threadpool
{
public:
	explicit Threadpool(size_t numThreads) // take the desired num of threads in constructor
	{
		start(numThreads); // constructor calls a start method
	}

	~Threadpool() // the destructor
	{
		stop();
	}

	void enqueue(ImageTasks task) //PUSH
	{
		{ // added scopes are to restrict the locks of added mutexes
			unique_lock<mutex> lock{ threadpoolMutex };
			tasks.emplace(move(task)); // add new task to queue
		}

		threadpoolCVar.notify_one(); // notify a thread a new task has been added to queue
	}

private:

	vector<thread> threads; //vector for threads
	condition_variable threadpoolCVar; //Condition variable
	queue<ImageTasks> tasks; //create queue
	mutex threadpoolMutex; // mutex for condition variable
	bool stopping = false; // bool for stopping

	void start(size_t numThreads) // start method for constructor call
	{
		for (auto i = 0u; i < numThreads; i++)
		{
			threads.emplace_back([=] { //lambda function
				while (true)
				{
					//create task?
					ImageTasks task;

					{ // decrease scope of critical secion
						unique_lock<mutex> lock{ threadpoolMutex }; //acquire mutex

						threadpoolCVar.wait(lock, [=] {return stopping || !tasks.empty(); }); // condition variable wait for bool (stop flag) set or if another task available

						if (stopping && tasks.empty()) // if stopping, break and exit infinite loop
							break;

						//if not stopping...
						task = std::move(tasks.front()); // take first task from queue
						tasks.pop(); // remove from queue

					} // Also we do not want the mutex locked while the task is executing below

					// execute task
					task.loadPic(imageNo);
					task.PaintImageNow(_hwnd, xc, yc, imageNo)
				}
			});
		}
	}

	void stop()
	{
		{
			//acquire lock
			unique_lock<mutex> lock{ threadpoolMutex };
			//set stopping flag to true
			stopping = true;
		}

		threadpoolCVar.notify_all(); // notify variable that something has happened

		//Join up finished threads
		for (auto& thread : threads)
			thread.join();
	}
};