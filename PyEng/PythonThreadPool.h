/*
Copyright (C) 2024 The XLang Foundation
Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#pragma once
#include "PyFunc.h"
#include <thread>
#include <mutex>
#include <queue>
#include <vector>
#include <functional>
#include <condition_variable>
#include <iostream>
#include <future>  // For std::promise and std::future

class PythonThreadPool {
    bool m_init = false;
	size_t m_numThreads =1;
public:
    PythonThreadPool(size_t numThreads) : m_numThreads(numThreads),stop(false) {

    }
    void Init()
    {
		if (m_init)
		{
			return;
		}
		m_init = true;
        // Initialize the Python interpreter (in the main thread)
        //Py_Initialize();
        //PyEval_InitThreads();  // Initialize threading support for Python

        // Save the main thread state
        //mainThreadState = PyEval_SaveThread();  // Release the GIL in the main thread

        // Create worker threads
        for (size_t i = 0; i < m_numThreads; ++i) {
            workers.emplace_back(&PythonThreadPool::workerThread, this);
        }
    }
    ~PythonThreadPool() {
        // Stop all threads
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            stop = true;
        }
        condition.notify_all();

        // Join all threads
        for (std::thread& worker : workers) {
            worker.join();
        }

        // Restore the main thread state and finalize the Python interpreter
        //PyEval_RestoreThread(mainThreadState);  // Re-acquire the GIL in the main thread
        //Py_Finalize();
    }

    // Submit a Python task to the thread pool and block until it completes
    void submit(const std::function<void()>& task) {
        Init();
        auto taskPromise = std::make_shared<std::promise<void>>();
        auto taskFuture = taskPromise->get_future();

        {
            std::unique_lock<std::mutex> lock(queueMutex);
            tasks.push([task, taskPromise]() {
                try {
                    // Execute the task
                    task();
                    // Signal that the task is complete
                    taskPromise->set_value();
                }
                catch (...) {
                    taskPromise->set_exception(std::current_exception());
                }
                });
        }
        condition.notify_one();

        // Wait for the task to complete
        taskFuture.get();  // Block until the task finishes
    }

private:
    // Worker thread function
    void workerThread() {
        while (true) {
            std::function<void()> task;

            // Wait for a task or for stop signal
            {
                std::unique_lock<std::mutex> lock(queueMutex);
                condition.wait(lock, [this] { return !tasks.empty() || stop; });

                if (stop && tasks.empty()) {
                    return;  // Exit if the thread pool is stopping
                }

                task = std::move(tasks.front());
                tasks.pop();
            }

            // Acquire the GIL manually and set the thread state
            //PyEval_AcquireLock();
            //PyThreadState* tstate = PyThreadState_New(mainThreadState->interp);
            //PyThreadState_Swap(tstate);
            PyGILState_STATE gstate = PyGILState_Ensure();
            // Execute the Python task
            try {
                task();
            }
            catch (const std::exception& ex) {
                std::cerr << "Task execution error: " << ex.what() << std::endl;
            }
            PyGILState_Release(gstate);
            // Restore the previous thread state and release the GIL
            //PyThreadState_Swap(nullptr);
            //PyEval_ReleaseLock();

            // Delete the thread state after releasing the GIL
            //PyThreadState_Delete(tstate);
        }
    }

    // Thread pool members
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;
    std::mutex queueMutex;
    std::condition_variable condition;
    bool stop;

    PyThreadState* mainThreadState;  // Save the main thread state
};
