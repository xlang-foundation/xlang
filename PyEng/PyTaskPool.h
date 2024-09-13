#pragma once
#include "PyFunc.h"
#include <thread>
#include <queue>
#include <functional>
#include <future>  // For std::promise and std::future
#include <mutex>
#include <condition_variable>

// PyTaskPool class
class PyTaskPool {
public:
    // Submit a task and wait for its completion inside the function
    void submit(const std::function<void()>& task) {
        // Create a promise to signal when the task is done
        std::promise<void> taskPromise;
        std::future<void> taskFuture = taskPromise.get_future();

        // Submit the task to the main thread
        submit_to_main_thread([task, &taskPromise]() {
            try {
                task();  // Run the task
                taskPromise.set_value();  // Set the promise once the task is done
            }
            catch (...) {
                taskPromise.set_exception(std::current_exception());  // Set any exception
            }
            });

        // Wait for the task to complete
        taskFuture.get();  // Block until the task completes
    }

private:
    // Task queue and synchronization primitives
    std::queue<std::function<void()>> taskQueue;
    std::mutex queueMutex;

    // Function executed by the main thread to process tasks
    static int execute_task(void* pool) {
        auto* taskPool = static_cast<PyTaskPool*>(pool);
        std::function<void()> task;

        {
            std::unique_lock<std::mutex> lock(taskPool->queueMutex);
            if (taskPool->taskQueue.empty()) {
                return 0;  // No task to process
            }
            task = std::move(taskPool->taskQueue.front());
            taskPool->taskQueue.pop();
        }

        // Execute the task
        task();

        return 0;  // Task processed successfully
    }

    // Function to submit tasks to the main thread using Py_AddPendingCall
    void submit_to_main_thread(const std::function<void()>& task) {
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            taskQueue.push(task);
        }

        // Add a pending call to ensure the main thread processes the task
        Py_AddPendingCall(execute_task, this);
    }
};
