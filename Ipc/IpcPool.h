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

#include <vector>
#include <queue>
#include <functional>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>
#include <type_traits>

namespace X
{
	namespace IPC
	{
		class ThreadPool
		{
		public:
			ThreadPool(size_t threads);
			~ThreadPool();

			template<class F, class... Args>
			auto enqueue(F&& f, Args&&... args) -> std::future<typename std::invoke_result<F, Args...>::type>;

		private:
			std::vector<std::thread> workers;
			std::queue<std::function<void()>> tasks;

			std::mutex queue_mutex;
			std::condition_variable condition;
			bool stop;
		};

		inline ThreadPool::ThreadPool(size_t threads)
			: stop(false)
		{
			for (size_t i = 0; i < threads; ++i)
				workers.emplace_back(
					[this]
					{
						for (;;)
						{
							std::function<void()> task;

							{
								std::unique_lock<std::mutex> lock(this->queue_mutex);
								this->condition.wait(lock,
									[this] { return this->stop || !this->tasks.empty(); });
								if (this->stop && this->tasks.empty())
									return;
								task = std::move(this->tasks.front());
								this->tasks.pop();
							}
							task();
						}
					}
				);
		}

		inline ThreadPool::~ThreadPool()
		{
			{
				std::unique_lock<std::mutex> lock(queue_mutex);
				stop = true;
			}
			condition.notify_all();
			for (std::thread& worker : workers)
				worker.join();
		}

		template<class F, class... Args>
		auto ThreadPool::enqueue(F&& f, Args&&... args) -> std::future<typename std::invoke_result<F, Args...>::type>
		{
			using return_type = typename std::invoke_result<F, Args...>::type;

			auto task = std::make_shared<std::packaged_task<return_type()>>(
				std::bind(std::forward<F>(f), std::forward<Args>(args)...)
			);

			std::future<return_type> res = task->get_future();
			{
				std::unique_lock<std::mutex> lock(queue_mutex);

				// don't allow enqueueing after stopping the pool
				if (stop)
					throw std::runtime_error("enqueue on stopped ThreadPool");

				tasks.emplace([task]() { (*task)(); });
			}
			condition.notify_one();
			return res;
		}

	}
}
